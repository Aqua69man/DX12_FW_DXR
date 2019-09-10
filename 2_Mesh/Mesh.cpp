#include "Mesh.h"
#include "..\DX12FrameWork\Utils\Utils.h"

// ==============================================================================
//									Init 
// ==============================================================================
Mesh::Mesh(HINSTANCE hInstance, const wchar_t * wndTitle, int width, int height, bool vSync) :
	Application(hInstance, wndTitle, width, height, vSync),
	m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
	m_Viewprt(CD3DX12_VIEWPORT(0.0f, 0.0f, (float)width, (float)height)),
	m_FOV(45.0f)
{
	// The first back buffer index will very likely be 0, but it depends
	m_CurrentBackbufferIndex = Application::GetCurrentBackbufferIndex(); 
}

Mesh::~Mesh()
{

}

void Mesh::LoadContent()
{
	auto device = Application::GetDevice();

	// Create the descriptor heap for the depth-stencil view.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DsvHeap)));

	m_ContentLoaded = true;

	ResizeDepthBuffer(Application::GetClientWidth(), Application::GetClientHeight());
}

void Mesh::UnloadContent()
{
	m_ContentLoaded = false;
}

// ==============================================================================
//									Resize 
// ==============================================================================
void Mesh::ResizeDepthBuffer(UINT32 width, UINT32 height)
{
	if (m_ContentLoaded) {
		// Flush any GPU commands that might be 
		// referencing the depth buffer.
		Application::Flush();

		width = std::max((UINT32)1, width);
		height = std::max((UINT32)1, height);

		auto device = Application::GetDevice();

		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		optimizedClearValue.DepthStencil = { 1.0f, 0 };

		ThrowIfFailed(
			device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&optimizedClearValue,
				IID_PPV_ARGS(&m_DepthBuffer)
			)
		);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsv;
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv, m_DsvHeap->GetCPUDescriptorHandleForHeapStart());
	}
}

void Mesh::Resize(UINT32 width, UINT32 height)
{
	if (Application::GetClientWidth() != width && Application::GetClientHeight() != height) 
	{
		Application::Resize(width, height);
		m_CurrentBackbufferIndex = Application::GetCurrentBackbufferIndex();

		m_Viewprt = CD3DX12_VIEWPORT(0.0f, 0.0f, (float)width, (float)height);
		
		// Resize DepthBuffer
		ResizeDepthBuffer(width, height);
	}
}

// ==============================================================================
//								Update & Render
// ==============================================================================
void Mesh::Update() 
{
	Application::Update();
	auto updTotalTime = Application::GetUpdateTotalTime();

	// TODO - UPDs here 
}

void Mesh::Render()
{
	Application::Render();
	auto rndrTotalTime = Application::GetRenderTotalTime();

	auto cmdQueue = Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto cmdList = cmdQueue->GetCommandList();

	m_CurrentBackbufferIndex = Application::GetCurrentBackbufferIndex();
	auto backbuff = Application::GetBackbuffer(m_CurrentBackbufferIndex);

	auto rtv = Application::GetCurrentBackbufferRTV();
	auto dsv = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();

	// Mesh RT
	{
		TransitionResource(cmdList, backbuff, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.4f, 0.9f, 0.6f, 1.0f };
		ClearRTV(cmdList, rtv, clearColor);
		ClearDepth(cmdList, dsv);
	}

	// PRESENT image
	{
		TransitionResource(cmdList, backbuff, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		// Execute
		m_FenceValues[m_CurrentBackbufferIndex] = cmdQueue->ExecuteCommandList(cmdList);

		m_CurrentBackbufferIndex = Application::Present();
		cmdQueue->WaitForFenceValue(m_FenceValues[m_CurrentBackbufferIndex]);
	}
}

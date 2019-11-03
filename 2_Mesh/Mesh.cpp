#include <d3dcompiler.h> // D3DReadFileToBlob

#include "..\DX12FrameWork\Utils\Utils.h"

#include "Mesh.h"
#include "Scene.h"

using namespace DirectX;

// ==============================================================================
//								Global Vars 
// ==============================================================================

// Vertex data for a colored cube.
struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static uint16_t g_Indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

// ==============================================================================
//									Init 
// ==============================================================================
Mesh::Mesh(HINSTANCE hInstance, const wchar_t * wndTitle, int width, int height, bool vSync) :
	Application(hInstance, wndTitle, width, height, vSync),
	m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
	m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, (float)width, (float)height)),
	m_FOV(45.0f)
{
	// The first back buffer index will very likely be 0, but it depends
	m_CurrentBackbufferIndex = Application::GetCurrentBackbufferIndex(); 
}
Mesh::~Mesh()
{

}

// =====================================================================================
//						      LoadContent & UnloadContent
// =====================================================================================

void Mesh::UpdateBufferResource(
	ComPtr<ID3D12GraphicsCommandList4> commandList,
	ID3D12Resource** pDestinationResource,
	ID3D12Resource** pIntermediateResource,
	size_t numElements, size_t elementSize, const void* bufferData,
	D3D12_RESOURCE_FLAGS flags)
{
	auto device = Application::GetDevice();

	size_t bufferSize = numElements * elementSize;

	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(pDestinationResource)));

	// Create a committed resource for the upload.
	if (bufferData)
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(commandList.Get(),
			*pDestinationResource, *pIntermediateResource,
			0, 0, 1, &subresourceData);
	}
}


bool Mesh::LoadContent(std::wstring shaderBlobPath)
{
	auto device = Application::GetDevice();
	auto commandQueue = Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	// Vertex buffer
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	{
		// Upload vertex buffer data.
		UpdateBufferResource(commandList,
			&m_VertexBuffer, &intermediateVertexBuffer,
			_countof(g_Vertices), sizeof(VertexPosColor), g_Vertices);

		// Create the vertex buffer view.
		m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
		m_VertexBufferView.SizeInBytes = sizeof(g_Vertices);
		m_VertexBufferView.StrideInBytes = sizeof(VertexPosColor);
	}

	// Index buffer
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	{
		// Upload index buffer data.
		UpdateBufferResource(commandList,
			&m_IndexBuffer, &intermediateIndexBuffer,
			_countof(g_Indicies), sizeof(uint16_t), g_Indicies);

		// Create index buffer view.
		m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
		m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
		m_IndexBufferView.SizeInBytes = sizeof(g_Indicies);
	}

	// Create the descriptor heap for the depth-stencil view.
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DsvHeap)));
	}

	// PSO
	{
		// Load the vertex shader.
		ComPtr<ID3DBlob> vertexShaderBlob;
		std::wstring blobPath = std::wstring(shaderBlobPath + L"VertexShader.cso");
		ThrowIfFailed(D3DReadFileToBlob(blobPath.c_str(), &vertexShaderBlob));

		// Load the pixel shader.
		ComPtr<ID3DBlob> pixelShaderBlob;
		blobPath = std::wstring(shaderBlobPath + L"PixelShader.cso");
		ThrowIfFailed(D3DReadFileToBlob(blobPath.c_str(), &pixelShaderBlob));

		// Create the vertex input layout
		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// Create a root signature.
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		// A single 32-bit constant root parameter that is used by the vertex shader.
		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

		// Serialize the root signature.
		ComPtr<ID3DBlob> rootSignatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
			featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
		// Create the root signature.
		ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
			rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
			CD3DX12_PIPELINE_STATE_STREAM_VS VS;
			CD3DX12_PIPELINE_STATE_STREAM_PS PS;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		} pipelineStateStream;

		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

		pipelineStateStream.pRootSignature = m_RootSignature.Get();
		pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
		pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		pipelineStateStream.RTVFormats = rtvFormats;

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
			sizeof(PipelineStateStream), &pipelineStateStream
		};
		ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));
	}

	// Execute and Sync queue
	{
		auto fenceValue = commandQueue->ExecuteCommandList(commandList);
		commandQueue->WaitForFenceValue(fenceValue);
	}

	m_ContentLoaded = true;

	// Resize/Create the depth buffer.
	ResizeDepthBuffer(Application::GetClientWidth(), Application::GetClientHeight());

	return true;
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
	if (Application::GetClientWidth() != width || Application::GetClientHeight() != height)
	{
		Application::Resize(width, height);
		m_CurrentBackbufferIndex = Application::GetCurrentBackbufferIndex();

		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, (float)width, (float)height);
		
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
	double totalUpdateTime = Application::GetUpdateTotalTime();

	// Update the model matrix.
	float angle = static_cast<float>(totalUpdateTime * 90.0);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
	m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

	// Update the view matrix.
	const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Update the projection matrix.
	float aspectRatio = Application::GetClientWidth() / static_cast<float>(Application::GetClientHeight());
	m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FOV), aspectRatio, 0.1f, 100.0f);
}

void Mesh::Render()
{
	Application::Render();
	double totalRenderTime = Application::GetRenderTotalTime();

	auto commandQueue = Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();

	m_CurrentBackbufferIndex = Application::GetCurrentBackbufferIndex();
	auto backBuffer = Application::GetBackbuffer(m_CurrentBackbufferIndex);

	auto rtv = Application::GetCurrentBackbufferRTV();
	auto dsv = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();

	// Clear RT
	{
		TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		ClearRTV(commandList, rtv, clearColor);
		ClearDepth(commandList, dsv);
	}

	// Set Graphics state
	commandList->SetPipelineState(m_PipelineState.Get());
	commandList->SetGraphicsRootSignature(m_RootSignature.Get());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	commandList->IASetIndexBuffer(&m_IndexBufferView);

	commandList->RSSetViewports(1, &m_Viewport);
	commandList->RSSetScissorRects(1, &m_ScissorRect);

	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	// Update the MVP matrix
	XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

	// Draw
	commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);

	// PRESENT image
	{
		// After rendering the scene, the current back buffer is PRESENTed 
		//     to the screen.
		// !!! Before presenting, the back buffer resource must be 
		//     transitioned to the PRESENT state.
		TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		// Execute
		m_FenceValues[m_CurrentBackbufferIndex] = commandQueue->ExecuteCommandList(commandList);

		m_CurrentBackbufferIndex = Application::Present();
		commandQueue->WaitForFenceValue(m_FenceValues[m_CurrentBackbufferIndex]);
	}
}

// ==============================================================================
//										Main
// ==============================================================================



int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
//int main(int argc, char** argv)
{
	std::wstring exePath = GetExePath();
	assert(exePath.size() != 0);

	const wchar_t* windowTitle = L"Learning DirectX 12";

	//Mesh game(hInstance, windowTitle, 3500, 1800, false);
	//game.LoadContent(exePath);
	//game.Run();
	//game.UnloadContent();

	// Change the following filename to a suitable filename value.
	FbxLoader fbxLoader;
	fbxLoader.Test("T:\\Playpan\\DX12_FW_DXR\\2_Mesh\\Data\\PepeMocap.fbx");

	return 0;
}
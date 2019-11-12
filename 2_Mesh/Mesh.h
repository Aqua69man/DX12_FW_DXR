#pragma once
#include <DirectXMath.h>

#include "..\DX12FrameWork\Framework\Application.h"
#include "..\DX12FrameWork\Framework\CommandQueue.h"

class Mesh : public Application
{
public:
	Mesh(HINSTANCE hInstance, const wchar_t * wndTitle, int width, int height, bool vSync);
	~Mesh();

	void Resize(UINT32 width, UINT32 height);
	void Update();
	void Render();

	void ResizeDepthBuffer(UINT32 width, UINT32 height);
	
	// Create a GPU buffer.
	void UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList4> commandList,
		ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

public:
	bool LoadContent(std::wstring shaderBlobPath, std::string fbxFilePath);
	void UnloadContent();

private:
	bool m_ContentLoaded = false;

	// Vertex buffer for the cube.
	ComPtr<ID3D12Resource> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	// Index buffer for the cube.
	ComPtr<ID3D12Resource> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	// Depth buffer and DescriptorHeap for it 
	ComPtr<ID3D12Resource> m_DepthBuffer;
	ComPtr<ID3D12DescriptorHeap> m_DsvHeap;

	// Root signature
	ComPtr<ID3D12RootSignature> m_RootSignature;

	// Pipeline state object.
	ComPtr<ID3D12PipelineState> m_PipelineState;

private:
	// View params
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
	float m_FOV;
	// Matrices
	DirectX::XMMATRIX m_ModelMatrix;
	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ProjectionMatrix;

private:
	// Frame Sync 
	UINT m_CurrentBackbufferIndex;
	UINT64 m_FenceValues[NUM_FRAMES_IN_FLIGHT] = {};
};


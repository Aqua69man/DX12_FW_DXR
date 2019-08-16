#pragma once

#include "Framework/Application.h"

#include <DirectXMath.h>

class DxrGame : public Application
{
// ------------------------------------------------------------------------------------------
//										Structs
// ------------------------------------------------------------------------------------------
public:
	struct AccelerationStructureBuffers
	{
		ComPtr<ID3D12Resource> pScratch;
		ComPtr<ID3D12Resource> pResult;
		ComPtr<ID3D12Resource> pInstanceDesc;    // Used only for top-level AS
	};

// ------------------------------------------------------------------------------------------
//									Function members
// ------------------------------------------------------------------------------------------
public:
	DxrGame(HINSTANCE hInstance, const wchar_t * windowTitle, int width, int height, bool vSync);
	virtual ~DxrGame();

	virtual void Update();
	virtual void Render();
	virtual void Resize(UINT32 width, UINT32 height);

	// Sample
	bool LoadContent(std::wstring shaderBlobPath);
	void UnloadContent();

	// DXR
	void InitDXR();
	void createAccelerationStructures();
	void createRtPipelineState();
	void createShaderResources();
	void createConstantBuffers();
	void createShaderTable();

protected:			
	/* --- Samle Funcs --- */
	
	// Create a GPU buffer.
	void UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList4> commandList,
		ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	// Resize the depth buffer to match the size of the client area.
	void ResizeDepthBuffer(int width, int height);

	// Helpers
	void TransitionResource(ComPtr<ID3D12GraphicsCommandList4> commandList, ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);
	void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

// ------------------------------------------------------------------------------------------
//										Data members
// ------------------------------------------------------------------------------------------
private:
	// createAccelerationStructures()
	ComPtr <ID3D12Resource> m_VertexBuffers[2];
	ComPtr <ID3D12Resource> m_BottomLevelAS[2];
	AccelerationStructureBuffers m_TopLevelBuffers;
	uint64_t c_TlasSize = 0;

	// createRtPipelineState()
	ComPtr<ID3D12StateObject> m_PipelineStateRtx;
	ComPtr<ID3D12RootSignature> m_EmptyRootSig;
	
	// createShaderResources()
	ComPtr<ID3D12Resource> m_OutputResource;
	ComPtr<ID3D12DescriptorHeap> m_SrvUavHeap;
	static const uint32_t c_SrvUavHeapSize = 2;

	// createConstantBuffers()
	ComPtr<ID3D12Resource> m_ConstantBuffer[3];

	// createShaderTable()
	ComPtr<ID3D12Resource> m_ShaderTable;
	uint32_t c_ShaderTableEntrySize = 0;

private:
	bool m_ContentLoaded;

	// Vertex buffer for the cube.
	ComPtr<ID3D12Resource> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	// Index buffer for the cube.
	ComPtr<ID3D12Resource> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	// Depth buffer.
	ComPtr<ID3D12Resource> m_DepthBuffer;
	// Descriptor heap for depth buffer.
	ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

	// Root signature
	ComPtr<ID3D12RootSignature> m_RootSignature;

	// Pipeline state object.
	ComPtr<ID3D12PipelineState> m_PipelineStateMain;
private:
	// View Settings
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
	float m_FoV;

	// Camera
	DirectX::XMMATRIX m_ModelMatrix;
	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ProjectionMatrix;

private:
	// Frame Sync 
	UINT m_CurrentBackBufferIndex;
	UINT64 m_FenceValues[NUM_FRAMES_IN_FLIGHT] = {};
};
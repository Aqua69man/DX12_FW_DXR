#pragma once
#include "..\DX12FrameWork\Framework\Application.h"
#include "..\DX12FrameWork\Framework\CommandQueue.h"
#include <DirectXMath.h>


class RasterRT : public Application
{
public:
	RasterRT(HINSTANCE hInstance, const wchar_t * wndTitle, int width, int height, bool vSync);
	~RasterRT();

	void Resize(UINT32 width, UINT32 height);
	void Update();
	void Render();


	void ResizeDepthBuffer(UINT32 width, UINT32 height);

private:
	// View params
	D3D12_VIEWPORT m_Viewprt;
	D3D12_RECT m_ScissorRect;
	float m_FOV;
	// Matrices
	DirectX::XMMATRIX m_ModelMatrix;
	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ProjectionMatrix;

private:
	bool m_ContentLoaded = false;

	ComPtr<ID3D12Resource> m_DepthBuffer;
	ComPtr<ID3D12DescriptorHeap> m_DsvHeap;

private:
	// Frame Sync 
	UINT m_CurrentBackbufferIndex;
	UINT64 m_FenceValues[NUM_FRAMES_IN_FLIGHT] = {};
};


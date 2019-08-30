#pragma once

#include <wrl.h>
#include "..\Helpers\d3dx12.h"

using Microsoft::WRL::ComPtr;

inline void TransitionResource(ComPtr<ID3D12GraphicsCommandList4> commandList, ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition
	(
		resource.Get(), before, after
	);

	commandList->ResourceBarrier(1, &barrier);
}

inline void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor)
{
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

inline void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
{
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}
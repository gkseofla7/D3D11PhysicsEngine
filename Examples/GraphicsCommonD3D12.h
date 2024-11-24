#pragma once

#include "D3D12Utils.h"
namespace hlab {

namespace Graphics{

extern ComPtr<ID3D12DescriptorHeap> srvCbvHeap;
extern ComPtr<ID3D12DescriptorHeap> rtvHeap;
extern ComPtr<ID3D12DescriptorHeap> samplerHeap;

//CD3DX12_CPU_DESCRIPTOR_HANDLE
extern CD3DX12_CPU_DESCRIPTOR_HANDLE curSrvCbvHandle;
extern CD3DX12_CPU_DESCRIPTOR_HANDLE curRtvHandle;

extern UINT srvCbvDescriptorSize;
extern UINT rtvDescriptorSize;

extern ComPtr<ID3DBlob> basicVS;
extern ComPtr<ID3DBlob> skinnedVS;
extern ComPtr<ID3DBlob> basicPS;

// Rasterizer States
D3D12_RASTERIZER_DESC solidRSDesc; // front only

extern ComPtr<ID3D12RootSignature> defaultRootSignature;

extern ComPtr<ID3D12PipelineState> defaultSolidPSO;

extern vector< D3D12_SAMPLER_DESC> sampDescs;
extern D3D12_BLEND_DESC mirrorBSDesc;
extern D3D12_BLEND_DESC accumulateBSDesc;
extern D3D12_BLEND_DESC alphaBSDesc;

void InitCommonStates(ComPtr<ID3D12Device>& device);
void InitShaders(ComPtr<ID3D12Device>& device);
void InitSamplerDescs();
void InitRasterizerDesc();
void InitDescriptorHeap(ComPtr<ID3D12Device>& device);
void InitBlendStates();
void InitRootSignature(ComPtr<ID3D12Device>& device);
void InitPipelineStates(ComPtr<ID3D12Device>& device);

void RegisterSrvCbvHeap(ComPtr<ID3D12Device>& device, const ComPtr<ID3D12Resource>& resource
	, const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE& srvHandle);
void RegisterRtvHeap(ComPtr<ID3D12Device>& device, const ComPtr<ID3D12Resource>& resource
	, const D3D12_RENDER_TARGET_VIEW_DESC* rtvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle);

}


}

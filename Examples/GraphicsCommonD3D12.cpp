#include "GraphicsCommonD3D12.h"
namespace hlab {

namespace Graphics {

ComPtr<ID3D12DescriptorHeap> rtvCbvHeap;
ComPtr<ID3D12DescriptorHeap> samplerHeap;

ComPtr<ID3DBlob> basicVS;
ComPtr<ID3DBlob> skinnedVS;
ComPtr<ID3DBlob> basicPS;

ComPtr<ID3D12RootSignature> defaultrootSignature;
ComPtr<ID3D12PipelineState> defaultSolidPSO;

vector< D3D12_SAMPLER_DESC> sampDescs;
vector< D3D12_BLEND_DESC> blendDescs;
}

void Graphics::InitSamplerDescs()
{
    D3D12_SAMPLER_DESC sampDesc = {};
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    // s0, linearWarpSS
    sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D12_FLOAT32_MAX;
    sampDescs.push_back(sampDesc);
    // s1, pointWarpSS
    sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampDescs.push_back(sampDesc);
    // s2, linearClampSS
    sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampDescs.push_back(sampDesc);
    // s3, pointClampSS
    sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampDescs.push_back(sampDesc);
    // TODO. Shadow 다시 확인 필요
    // s4, shadowPointSS
    sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampDesc.BorderColor[0] = 1.0f; // 큰 Z값
    sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampDescs.push_back(sampDesc);
    // s5, shadowCompareSS
    sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampDesc.BorderColor[0] = 100.0f; // 큰 Z값
    sampDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    sampDescs.push_back(sampDesc);
    // s6, linearMirrorSS
    sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D12_FLOAT32_MAX;
    sampDescs.push_back(sampDesc);

}

void Graphics::InitDescriptorHeap(ComPtr<ID3D12Device>& device)
{
    D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
    HeapDesc.NumDescriptors = 6 + 1 + 3; //* FrameCount; // TODO. 각 Frame마다 따로 저장 예정
    HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&rtvCbvHeap)));


    // 디스크립터 힙 생성
    D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
    samplerHeapDesc.NumDescriptors = static_cast<UINT>(sampDescs.size());
    samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;  // 셰이더에서 참조할 수 있도록 설정
    ThrowIfFailed(device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&samplerHeap)));

    // 샘플러 디스크립터를 디스크립터 힙에 추가
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(samplerHeap->GetCPUDescriptorHandleForHeapStart());
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    for (size_t i = 0; i < sampDescs.size(); ++i) {
        device->CreateSampler(&sampDescs[i], cpuHandle);
        cpuHandle.Offset(descriptorSize);  // 다음 샘플러의 주소로 이동
    }
}

void Graphics::InitShaders(ComPtr<ID3D12Device>& device)
{
    // Input Desc은 RootSignature에서 정의한다

    D3D12Utils::CreateVertexShader(device, L"BasicVS.hlsl",basicVS);
    D3D12Utils::CreateVertexShader(device, L"BasicVS.hlsl", skinnedVS,
        vector<D3D_SHADER_MACRO>{ {"SKINNED", "1"}, { NULL, NULL }});
    D3D12Utils::CreatePixelShader(device, L"BasicPS.hlsl", basicPS);
}

void Graphics::InitRasterizerDesc()
{
    // Rasterizer States
    ZeroMemory(&solidRSDesc, sizeof(D3D12_RASTERIZER_DESC));
    solidRSDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    solidRSDesc.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
    solidRSDesc.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
    solidRSDesc.FrontCounterClockwise = false;
    solidRSDesc.DepthClipEnable = true;
    solidRSDesc.MultisampleEnable = true;
}

void Graphics::InitBlendStates(ComPtr<ID3D12Device>& device)
{
    // "이미 그려져있는 화면"과 어떻게 섞을지를 결정
// Dest: 이미 그려져 있는 값들을 의미
// Src: 픽셀 쉐이더가 계산한 값들을 의미 (여기서는 마지막 거울)

    ZeroMemory(&mirrorBSDesc, sizeof(mirrorBSDesc));
    mirrorBSDesc.AlphaToCoverageEnable = true; // MSAA
    mirrorBSDesc.IndependentBlendEnable = false;
    // 개별 RenderTarget에 대해서 설정 (최대 8개)
    mirrorBSDesc.RenderTarget[0].BlendEnable = true;
    mirrorBSDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_BLEND_FACTOR;
    mirrorBSDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_BLEND_FACTOR;
    mirrorBSDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    mirrorBSDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    mirrorBSDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
    mirrorBSDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    // 필요하면 RGBA 각각에 대해서도 조절 가능
    mirrorBSDesc.RenderTarget[0].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;

    ZeroMemory(&accumulateBSDesc, sizeof(accumulateBSDesc));
    accumulateBSDesc.AlphaToCoverageEnable = true; // MSAA
    accumulateBSDesc.IndependentBlendEnable = false;
    accumulateBSDesc.RenderTarget[0].BlendEnable = true;
    accumulateBSDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_BLEND_FACTOR;
    accumulateBSDesc.RenderTarget[0].DestBlend = D3D12_BLEND_BLEND_FACTOR; // INV 아님
    accumulateBSDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    accumulateBSDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    accumulateBSDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
    accumulateBSDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    accumulateBSDesc.RenderTarget[0].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;
    

    // Dst: 현재 백버퍼, Src: 새로 픽셀 쉐이더에서 출력,
    ZeroMemory(&alphaBSDesc, sizeof(alphaBSDesc));
    alphaBSDesc.AlphaToCoverageEnable = false; // <- 주의: FALSE
    alphaBSDesc.IndependentBlendEnable = false;
    alphaBSDesc.RenderTarget[0].BlendEnable = true;
    alphaBSDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    alphaBSDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    alphaBSDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    alphaBSDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    alphaBSDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
    alphaBSDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    alphaBSDesc.RenderTarget[0].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;
}

void Graphics::InitRootSignature(ComPtr<ID3D12Device>& device)
{
    // 그동안 defaultSolidPSO쓸때 VIew 어떻게 넣었는지 확인
    CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 10, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);//Texture
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);//Texture
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 3, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
    rootParameters.resize(1);
    rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);

    vector<CD3DX12_DESCRIPTOR_RANGE1> sampleRanges(sampDescs.size());
    for (size_t i = 0; i < sampDescs.size(); ++i) {
        sampleRanges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, i);  // 각 샘플러를 하나씩 할당
    }
    vector<CD3DX12_ROOT_PARAMETER1> rootParams(1);
    rootParams[3].InitAsDescriptorTable(static_cast<UINT>(sampleRanges.size()), sampleRanges.data(), D3D12_SHADER_VISIBILITY_ALL);

    D3D12Utils::CreateRootSignature(device, defaultrootSignature, rootParameters);
}

void Graphics::InitPipelineStates(ComPtr<ID3D12Device>& device)
{
    D3D12_INPUT_ELEMENT_DESC basicIEs[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { basicIEs, _countof(basicIEs)};
    psoDesc.pRootSignature = defaultrootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(basicVS.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(basicPS.Get());
    psoDesc.RasterizerState = solidRSDesc;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    //TODO RTV Format 다시 확인 필요
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&defaultSolidPSO)));
}
}
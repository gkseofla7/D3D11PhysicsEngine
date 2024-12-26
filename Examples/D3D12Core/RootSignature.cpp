#include "RootSignature.h"
#include "Engine.h"
#include "Device.h"
#include "Samplers2.h"
namespace dengine {
void RootSignature::Init()
{
	//CreateGraphicsRootSignature();
	CreateDefaultRootSignature();
	CreateSkyboxRootSignature();
	CreateSamplingRootSignature();
}

void RootSignature::CreateDefaultRootSignature()
{
	vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
	rootParameters.resize(6);
	// 공용 데이터
	// b0, GlobalConstants
	{
		rootParameters[0].InitAsConstantBufferView(static_cast<uint32>(CBV_REGISTER::b0)); // b0
	}
	// t10, t11, t12, t13
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 11);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 12);
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 13);
		rootParameters[1].InitAsDescriptorTable(4, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	}
	{
		size_t sampleSize = GEngine->GetSamples()->GetSampleDesc().size();
		CD3DX12_DESCRIPTOR_RANGE1 sampleRanges[7];
		for (size_t i = 0; i < sampleSize; ++i)
		{
			sampleRanges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, static_cast<UINT>(i));  // 각 샘플러를 하나씩 할당
		}
		rootParameters[2].InitAsDescriptorTable(static_cast<UINT>(sampleSize), &sampleRanges[0], D3D12_SHADER_VISIBILITY_ALL);
	}
	// 로컬 데이터
	// b1 : MeshConstants, b2 : MaterialConstants
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
		//ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3);
		rootParameters[3].InitAsDescriptorTable(2, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	}
	{//t0~t5
		CD3DX12_DESCRIPTOR_RANGE1 ranges[6];
		// t0 : height,	 t1 : albedo,	t2 : normal,	t3: ao
		// t4 : metallicRoughness,	 t5 : emissive
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
		ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
		ranges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);
		rootParameters[4].InitAsDescriptorTable(6, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	}


	// TODO 임시로, 제거 필요
	const int MAX_LIGHTS = 3;
	CD3DX12_DESCRIPTOR_RANGE1 shadowMapRange;
	shadowMapRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_LIGHTS, 15); // Start from t15
	rootParameters[5].InitAsDescriptorTable(1, &shadowMapRange, D3D12_SHADER_VISIBILITY_ALL);

	CreateRootSignature(m_defaultRootSignature, rootParameters);
}

void RootSignature::CreateSkyboxRootSignature()
{
	vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
	rootParameters.resize(3);
	// 공용 데이터
	// b0, GlobalConstants
	{
		rootParameters[0].InitAsConstantBufferView(static_cast<uint32>(CBV_REGISTER::b0)); // b0
	}
	// t10, t11, t12, t13
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 11);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 12);
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 13);
		rootParameters[1].InitAsDescriptorTable(4, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	}

	size_t sampleSize = GEngine->GetSamples()->GetSampleDesc().size();
	CD3DX12_DESCRIPTOR_RANGE1 sampleRanges[7];
	for (size_t i = 0; i < sampleSize; ++i) {
		sampleRanges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, static_cast<UINT>(i));  // 각 샘플러를 하나씩 할당
	}
	rootParameters[2].InitAsDescriptorTable(static_cast<UINT>(sampleSize), &sampleRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	CreateRootSignature(m_skyboxRootSignature, rootParameters);
}

void RootSignature::CreateSamplingRootSignature()
{
	vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
	rootParameters.resize(3);
	// b0, GlobalConstants
	{
		rootParameters[0].InitAsConstantBufferView(static_cast<uint32>(CBV_REGISTER::b0)); // b0
	}
	// t0
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	}
	// s0
	{
		CD3DX12_DESCRIPTOR_RANGE1 samplerRange[1];
		samplerRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
		rootParameters[2].InitAsDescriptorTable(1, &samplerRange[0], D3D12_SHADER_VISIBILITY_ALL);
	}
	CreateRootSignature(m_samplinigRootSignature, rootParameters);
}

void RootSignature::CreateRootSignature(ComPtr<ID3D12RootSignature>& rootSignature, vector<CD3DX12_ROOT_PARAMETER1>& rootParams)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(DEVICE->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny uneccessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(static_cast<UINT>(rootParams.size()), rootParams.data(), 0, nullptr, rootSignatureFlags);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	//ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
	ThrowIfFailed(DEVICE->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}
}



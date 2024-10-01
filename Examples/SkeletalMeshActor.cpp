#include"SkeletalMeshActor.h"
#include "DSkinnedMeshModel.h"
#include "AnimHelper.h"

namespace hlab {
    SkeletalMeshActor::SkeletalMeshActor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
        shared_ptr<DModel> InModel)
        :Actor(device, context, InModel)
    {
    }
    void SkeletalMeshActor::Initialize(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
        shared_ptr<DModel>  InModel)
    {
        Actor::Initialize(device, context, InModel);
    }
    void SkeletalMeshActor::Update(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, float dt)
    {
        Actor::Update(device, context, dt);
    }

    void SkeletalMeshActor::Render(ComPtr<ID3D11DeviceContext>& context)
    {
        // ConstBuffer ��� StructuredBuffer ���
        // context->VSSetConstantBuffers(3, 1, m_skinnedConsts.GetAddressOf());

        //context->VSSetShaderResources(
        //    9, 1, m_boneTransforms.GetAddressOfSRV()); // �׻� slot index ����

        // Skinned VS/PS�� GetPSO()�� ���ؼ� �����Ǳ� ������
        // Model::Render(.)�� ���� ��� ����
        Actor::Render(context);
    };

    shared_ptr<DSkinnedMeshModel> SkeletalMeshActor::GetSkinnedMeshModel()
    {
        if (std::shared_ptr<DSkinnedMeshModel> derivedPtr = std::dynamic_pointer_cast<DSkinnedMeshModel>(GetModel()))
        {
            return derivedPtr;
        }
        return nullptr;
    }
}
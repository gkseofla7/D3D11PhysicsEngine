#pragma once

#include "Actor.h"

namespace hlab {
    enum CommonAnimState {
        Idle = 0,
        IdleToWalk = 1,
        Walking = 2,
        WalkingBackward = 3,
        WalkingToIdle = 4,
        Specail = 5,
        AnimNum,
    };

class SkeletalMeshActor : public Actor {
public:
    // �Է� ���� ������ ���� �����ϵ��� ����, State�� Animation �и��� �ʿ��ϴٰ� ����.
    void UpdateAnimation(ComPtr<ID3D11DeviceContext> m_context, float dt, bool* keyPressed);
    void InitAnimationData(ComPtr<ID3D11Device>& device,
        const AnimationData& aniData);
    //void UpdateState();

public:
    SkeletalMeshActor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
        const string& basePath, const string& filename)
    {
        Initialize(device, context, basePath, filename);
    }
    virtual void Initialize(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
        const string& basePath, const string& filename)
    {
        //InitAnimationData(device, aniData);
        Actor::Initialize(device, context, basePath, filename);
    }

    void InitAnimationData(ComPtr<ID3D11Device>& device,
        const AnimationData& aniData) {
        if (!aniData.clips.empty()) {
            m_aniData = aniData;

            // ���⼭�� AnimationClip�� SkinnedMesh��� �����ϰڽ��ϴ�.
            // �Ϲ������δ� ��� Animation�� SkinnedMesh Animation�� �ƴմϴ�.
            m_boneTransforms.m_cpu.resize(aniData.clips.front().keys.size());

            // ����: ��� keys() ������ �������� ���� ���� �ֽ��ϴ�.
            for (int i = 0; i < aniData.clips.front().keys.size(); i++)
                m_boneTransforms.m_cpu[i] = Matrix();
            m_boneTransforms.Initialize(device);
        }
    }

    void UpdateAnimation(ComPtr<ID3D11DeviceContext>& context, int clipId,
        int frame, int type = 0) override {

        m_aniData.Update(clipId, frame, type);

        for (int i = 0; i < m_boneTransforms.m_cpu.size(); i++) {
            m_boneTransforms.m_cpu[i] =
                m_aniData.Get(clipId, i, frame).Transpose();
        }

        m_boneTransforms.Upload(context);
    }

    void Render(ComPtr<ID3D11DeviceContext>& context) override {

        // ConstBuffer ��� StructuredBuffer ���
        // context->VSSetConstantBuffers(3, 1, m_skinnedConsts.GetAddressOf());

        context->VSSetShaderResources(
            9, 1, m_boneTransforms.GetAddressOfSRV()); // �׻� slot index ����

        // Skinned VS/PS�� GetPSO()�� ���ؼ� �����Ǳ� ������
        // Model::Render(.)�� ���� ��� ����

        Model::Render(context);
    };

    // SkinnedMesh�� BoundingBox�� �׸� �� Root�� Transform�� �ݿ��ؾ� �մϴ�.
    // virtual void RenderWireBoundingBox(ComPtr<ID3D11DeviceContext> &context);
    // virtual void RenderWireBoundingSphere(ComPtr<ID3D11DeviceContext>
    // &context);
    void UpdateVelocity(float dt) {
        Vector3 prevPos = m_prevRootTransform.Translation();
        Vector3 curPos = m_aniData.accumulatedRootTransform.Translation();

        m_velocity = (curPos - prevPos).Length() / dt;
        m_prevRootTransform = m_aniData.accumulatedRootTransform;
    }
public:
    // ConstantBuffer<SkinnedConsts> m_skinnedConsts;
    StructuredBuffer<Matrix> m_boneTransforms;

    AnimationData m_aniData;
    float m_velocity = 0.0f;
    Matrix m_prevRootTransform;
private:
    int m_state = 0;
    bool m_isAnimationInitialized = false;
};
} // namespace hlab
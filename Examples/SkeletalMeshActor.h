#pragma once

#include "Actor.h"

namespace hlab {
    class ActorState;
    class DSkinnedMeshModel;
class SkeletalMeshActor : public Actor {
public:
    // �Է� ���� ������ ���� �����ϵ��� ����, State�� Animation �и��� �ʿ��ϴٰ� ����.
    //void UpdateAnimation(ComPtr<ID3D11DeviceContext> m_context, float dt, bool* keyPressed);
    //void InitAnimationData(ComPtr<ID3D11Device>& device,
    //    const AnimationData& aniData);
    //void UpdateState();

public:
    SkeletalMeshActor() {}
    SkeletalMeshActor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
        shared_ptr<DModel>  InModel);
    void Initialize(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
        shared_ptr<DModel>  InModel);
    virtual void Update(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, float dt) override;
    virtual void InitializeAnimation() {}
    //TODO. UpdateAnimation���� ���� �񵿱� �ε��� �ϱ� ������(defered..) 
    // �ʱ�ȭ ���� UpdateAnimation���ο��� �ϰԵȴ�.
    // device, context�� �ȹް� ����������..���� ����� ����غ���
    void UpdateAnimation(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, int clipId,
        int frame, int type = 0);

    void Render(ComPtr<ID3D11DeviceContext>& context) override;

    // SkinnedMesh�� BoundingBox�� �׸� �� Root�� Transform�� �ݿ��ؾ� �մϴ�.
    // virtual void RenderWireBoundingBox(ComPtr<ID3D11DeviceContext> &context);
    // virtual void RenderWireBoundingSphere(ComPtr<ID3D11DeviceContext>
    // &context);
    void UpdateVelocity(float dt) {
        //Vector3 prevPos = m_prevRootTransform.Translation();
        //Vector3 curPos = m_aniData.accumulatedRootTransform.Translation();

        //m_velocity = (curPos - prevPos).Length() / dt;
        //m_prevRootTransform = m_aniData.accumulatedRootTransform;
    }

    virtual shared_ptr<DSkinnedMeshModel> GetSkinnedMeshModel() override;
public:
    // ConstantBuffer<SkinnedConsts> m_skinnedConsts;
    StructuredBuffer<Matrix> m_boneTransforms;

    Matrix accumulatedRootTransform = Matrix();
    float m_velocity = 0.0f;
    Matrix m_prevRootTransform;

    //Anim ����
    int m_curFrame = 0;
private:
    int m_state = 0;

};
} // namespace hlab
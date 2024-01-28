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
private:
    int state = 0;
public:
    shared_ptr< class SkinnedMeshModel> m_skinnedMeshModel;
    StructuredBuffer<Matrix> m_boneTransforms;
};
} // namespace hlab
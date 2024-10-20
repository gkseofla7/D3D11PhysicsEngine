#include "Projectile.h"
#include "BillboardModel.h"
#include "DModel.h"

#include "DaerimsEngineBase.h"
#include "bullet/BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "bullet/btBulletDynamicsCommon.h"

namespace hlab {
    bool isObjectInContact(btCollisionObject* obj, btCollisionWorld* world) {
        class SimpleContactCallback : public btCollisionWorld::ContactResultCallback {
        public:
            bool hasCollided = false;
            btCollisionObjectWrapper* collisionObjWrapper = nullptr;
            btScalar addSingleResult(btManifoldPoint& cp,
                const btCollisionObjectWrapper* colObj0Wrap,
                int partId0,
                int index0,
                const btCollisionObjectWrapper* colObj1Wrap,
                int partId1,
                int index1) override {
                hasCollided = true;  // �浹�� �߻������� ���
                collisionObjWrapper = const_cast<btCollisionObjectWrapper*>(colObj1Wrap);
                return 0;  // �⺻ ��ȯ��
            }
        };

        SimpleContactCallback callback;
        world->contactTest(obj, callback);  // �浹 �׽�Ʈ ����

        return callback.hasCollided;  // �浹 ���� ��ȯ
    }
    void Projectile::Initialize(ComPtr<ID3D11Device>& InDevice, ComPtr<ID3D11DeviceContext>& InContext,
        const Vector3& InPos, const Vector3& InVelocity, float InRadius)
    {
        m_model =  std::make_shared<BillboardModel>();

        Vector3 dir(float(InVelocity.x), float(InVelocity.y), float(InVelocity.z));
        dir.Normalize();
        shared_ptr<BillboardModel> billboardModel =std::dynamic_pointer_cast<BillboardModel>(m_model);
        billboardModel->m_billboardConsts.m_cpu.directionWorld = dir;
        billboardModel->m_castShadow = false;
        billboardModel->Initialize(InDevice, InContext, { {0.0f, 0.0f, 0.0f, 1.0f} },
            0.2f, Graphics::volumetricFirePS);
        billboardModel->SetWorldPosition(InPos);
        m_usePhysicsSimulation = true;
    }

    void Projectile::Tick(float dt)
    {
        Object::Tick(dt);
        // TODO. �� ����� ���ؼ��� ����ó���ϴ°� �� ���� ������..
        m_elasedSeconds += dt;
        static const float notCollapsedTime = 0.3f;
        if (m_elasedSeconds > notCollapsedTime)
        {
            if (m_physicsBody != nullptr)
            {
                // TODO. �̰͵� �̵� �ʿ�
                if (m_startPendingKill == false)
                {
                    btCollisionObject* collisionObj = (btCollisionObject*)(m_physicsBody);
                    if (isObjectInContact(collisionObj, DaerimsEngineBase::GetInstance().GetDynamicWorld()))
                    {
                        m_startPendingKill = true;
                    }
                }
                else
                {
                    m_pendingKillElapsedSeconds += dt;
                    static const float killTime = 0.3f;
                    if (m_pendingKillElapsedSeconds > killTime)
                    {
                        SetPendingKill(true);
                    }
                }
            }
        }
    }
}
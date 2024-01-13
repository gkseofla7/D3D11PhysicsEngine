#pragma once
#include "AppBase.h"
#include "BillboardModel.h"
#include "SkinnedMeshModel.h"
namespace hlab
{
class DaerimGTA : public AppBase
{
public:
	DaerimGTA();
       ~DaerimGTA()
	{
    }
	bool InitScene() override;
	void InitPhysics(bool interactive);
	void UpdateLights(float dt) override;
	void UpdateGUI() override;
	void Update(float dt) override;
	void Render() override;
	void createEmptyDynamicsWorld();
	btBoxShape* createBoxShape(const btVector3& halfExtents);
	btRigidBody* createRigidBody(float mass, const btTransform& startTransform, btCollisionShape* shape, const btScalar m_angularDamping = 0, const btVector4& color = btVector4(1, 0, 0, 1));
	void CreateStack(const btTransform& t, int numStacks, int numSlices,
		btScalar halfExtent);
	btRigidBody* CreateDynamic(const btTransform& t,
		btCollisionShape* shape,
		const btVector3& velocity);
public:
	vector<shared_ptr<Model>>
		m_objects; // ���� ������ ����ȭ ������ �� ��� TODO: actor list�� ����
	shared_ptr<BillboardModel> m_fireball;
	shared_ptr<SkinnedMeshModel> m_character;

	btAlignedObjectArray<btCollisionShape*> m_collisionShapes;
	btDefaultCollisionConfiguration* m_collisionConfiguration;
	btBroadphaseInterface* m_broadphase;
	btCollisionDispatcher* m_dispatcher;
	btConstraintSolver* m_solver;
	btDiscreteDynamicsWorld* m_dynamicsWorld;
	btScalar stackZ = 10.0f;

	float m_simToRenderScale = 0.01f; // �ùķ��̼� ��ü�� �ʹ� ������ �Ҿ���
	};
}
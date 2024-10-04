#pragma once
#include "GeometryGenerator.h"
#include "DModel.h"
#include "GameDef.h"
#include <map>
namespace hlab {
	using std::function;
	class DModel;
	class ActorState;
	class DSkinnedMeshModel;
	//using std::map;
class Actor : public std::enable_shared_from_this<Actor> {
public:
	Actor();
	Actor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
		shared_ptr<DModel> InModel);
	virtual void Initialize(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
		shared_ptr<DModel> InModel);
	//TODO. device�� context�� �Ȱǳ��� ����� ã�ƺ���..
	virtual void Update(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, float dt);
	
	// ��ġ �̵� ����
	void UpdatePosition(const Vector3& InDelta);
	void UpdateVelocity(float InDelta);
	void UpdateRotationY(float InDelta);
	void SetVelocity(float InVelocity) { 
		if (InVelocity < 0.0)
		{
			m_velocity = 0.0;
			return;
		}
		m_velocity = InVelocity; 
	}
	float GetVelocity() { return m_velocity; }
	void SetState(ActorStateType InType);
	ActorStateType GetPrevState() { return m_prevStateType; }
	// ī�޶� ����
	void ActiveCaemera();
	void UpdateCemeraCorrection(Vector3 deltaPos);

	bool MsgProc(WPARAM wParam, bool bPress);
	
	virtual void Render(ComPtr<ID3D11DeviceContext>& context);

	shared_ptr<DModel> GetModel() { return m_model; }
	virtual shared_ptr<DSkinnedMeshModel> GetSkinnedMeshModel() { return nullptr; };
	shared_ptr<ActorState> GetState() { return m_actorState; }
private:
	void UpdateState();
public:
	//ActorState GetActorState() { return m_actorState; }
protected:
	virtual void InitBoundingKey() {};
protected:
	shared_ptr<class Camera> m_camera;
	Matrix m_cameraCorrection;

	std::map<WPARAM, function<void()>> m_keyBindingPress;
	std::map<WPARAM, function<void()>> m_keyBindingRelease;
	// Actor���� State�� � ������ �ϴ���
	// � �������� �𸣰� ���ư��°� ����Ʈ �ƴѰ��ʹ�.
	shared_ptr<ActorState> m_actorState;
	ActorStateType m_actorStateType;
	ActorStateType m_prevStateType;
	shared_ptr<DModel> m_model;

	float m_velocity = 0.0f;
};

} // namespace hlab

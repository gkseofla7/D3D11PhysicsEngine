#pragma once
#include "GameDef.h"
#include <memory>
namespace hlab {

class Actor;
class ActorState
{

	//C++���� �߻� Ŭ������ �����ڿ��� �Լ��� ȣ���� ���� ������,
	//�����ؾ� �� �߿��� ������ �ֽ��ϴ�. �߻� Ŭ������ �ϳ� �̻���
	// ���� ���� �Լ�(pure virtual function)�� �����ϴ� Ŭ�����Դϴ�.
	// �߻� Ŭ������ �����ڿ��� ���� �Լ��� ȣ���ϴ� ���,
	// �ش� �Լ��� �Ļ� Ŭ�������� �����ǵ� �Լ��� �ִ���,
	//ȣ��Ǵ� �Լ��� �߻� Ŭ���� ��ü���� ���ǵ� �Լ��Դϴ�.
	//�̴� �����ڰ� ȣ��� �� �Ļ� Ŭ������ �����ڰ� ���� ȣ����� �ʾұ� �����Դϴ�.
public:
	ActorState(){}
	ActorState(std::weak_ptr<Actor> InActor);

	virtual void Initialize() = 0;
	virtual void Tick(float dt);
	virtual void Finish();
	// ��ǲ �޾� ��õ
	virtual void Transition();
	virtual void UpdateAnimation();

	ActorStateType GetStateType() { return m_state; }
	int GetFrame() { return m_frame; }
protected:
	ActorStateType m_state;
	bool m_loopState = false;
	int m_frame = 0;

	std::weak_ptr<Actor> m_actor;
private:
	bool m_finished = false;
	
};
}

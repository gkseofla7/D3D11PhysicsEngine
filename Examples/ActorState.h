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
	virtual void Tick();
	virtual void Finish() = 0;
	// ��ǲ �޾� ��õ
	virtual ActorStateType Transition() = 0;

	ActorStateType GetStateType() { return m_state; }
	int GetFrame() { return m_frame; }
protected:
	ActorStateType m_state;
	bool m_loopState = false;
	ActorStateType m_afterState;
private:

	bool m_finished = false;
	std::weak_ptr<Actor> m_actor;
	int m_frame = 0;
};
}

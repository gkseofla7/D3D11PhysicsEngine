#pragma once
#include "GameDef.h"
#include "ActorState.h"
namespace hlab {
class MoveState : public ActorState
{
public:
	MoveState() {}
	MoveState(std::weak_ptr<Actor> InModel);

	virtual void Initialize();
	virtual void Tick();
	virtual void Finish();
	// ��ǲ �޾� ��õ
	virtual void Transition();

private:
	MoveStateType m_moveState;
};
}

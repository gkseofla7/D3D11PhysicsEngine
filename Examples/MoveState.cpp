#include "MoveState.h"
#include "Actor.h"
namespace hlab {

	MoveState::MoveState(std::weak_ptr<Actor> InActor)
		:ActorState(InActor)
	{
		m_state = ActorStateType::Move;
		m_loopState = false;
	}
	void MoveState::Initialize()
	{

	}
	void MoveState::Tick()
	{
		ActorState::Tick();
	}
	void MoveState::Finish()
	{
		ActorState::Finish();
	}
	// ��ǲ �޾� ��õ
	void MoveState::Transition()
	{
		if (m_moveState != MoveStateType::EndOfEnum)
		{
			m_moveState = MoveStateType((int)m_moveState + 1);
		}
		
		if (m_moveState == MoveStateType::EndOfEnum)
		{
			std::shared_ptr<Actor> actorLock = m_actor.lock();
			actorLock->SetState(ActorStateType::Idle);
		}
	}
}
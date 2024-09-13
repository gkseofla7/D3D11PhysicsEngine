#include "ActorState.h"
#include "DModel.h"
#include "AnimHelper.h"
#include "DSkinnedMeshModel.h"
#include "Actor.h"
namespace hlab {

ActorState::ActorState(std::weak_ptr<Actor> InActor)
{	m_actor = InActor;
}
void ActorState::Tick()
{
	std::shared_ptr<Actor> actorLock = m_actor.lock();
	if (actorLock.get() == nullptr)
	{
		return;
	}
	std::shared_ptr<ActorState> myLock = actorLock->GetState();
	if (std::shared_ptr<DSkinnedMeshModel> derivedPtr = std::dynamic_pointer_cast<DSkinnedMeshModel>(actorLock->GetModel()))
	{
		AnimHelper::GetInstance().UpdateAnimation(derivedPtr.get(), (int)m_state, m_frame);

		//TODO. Loop �ý������� Ȯ��. �ƴ϶�� Transition �ʿ�
		if (m_frame == derivedPtr->m_maxFrame)
		{
			m_frame = 0;
			if (false == m_loopState)
			{
				// ����! ���� shared_ptr�� �����ϰ� �ִ� ������ ���� �����ش�..
				actorLock->SetState(m_afterState);
			}
		}
		else
		{
			m_frame++;
		}
	}
}
}
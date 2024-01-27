#include <future>
#include <thread>

#include "AnimStatus.h"
#include "Actor.h"
namespace hlab {
void AnimStatus::AddAnimPath(int InActorId, string InPathName)
{
	m_pathMap[InActorId] = InPathName;
}
void AnimStatus::AddAnimStateToAnim(int InActorId,int InState, string InAnimName)
{
	// �� ������ �� �ѹ� ���ִ°�
	m_animStateToAnim[InActorId].insert({ InState,InAnimName});
}
void AnimStatus::PlayAnimation(string InAnimName)
{

}
AnimationData ReadAnimationFromFile(string path,string name)
{
	auto [_, ani] =
		GeometryGenerator::ReadAnimationFromFile(path, name);
	return ani;
}
void AnimStatus::UpdateAnimation(ComPtr<ID3D11DeviceContext>& context, Actor* InActor,int InState,
	int frame, int type = 0)
{
	//�񵿱� �ε��ϵ��� �Ѵ�.
	static std::future<AnimationData> AnimDataFuture;
	int ActorId = InActor->getActorId();
	if (m_aniData.find(ActorId) == m_aniData.end())
	{
		const string& path = m_pathMap[ActorId];
		const string& name = m_animStateToAnim[ActorId][InState];
		auto [_, ani] =
			GeometryGenerator::ReadAnimationFromFile(path, name);
		AnimDataFuture =
			std::async(std::launch::async, ReadAnimationFromFile, path, name);
		//�ϴ� �� ����
		m_aniData[ActorId] = AnimationData();
		AnimDataFuture._Is_ready();
		if (aniData.clips.empty()) {
			aniData = ani;
		}
		else {
			aniData.clips.push_back(ani.clips.front());
		}
		return;
	}
	if (m_aniData[ActorId].clipMaps.find(InState) == m_aniData[ActorId].clipMaps.end())
	{

		return;
	}
	m_aniData[ActorId].Update(InState, frame, type);
	// �Լ��� ������Ʈ�ϴ°�..��
	for (int i = 0; i < InActor->m_boneTransforms.m_cpu.size(); i++) {
		m_boneTransforms.m_cpu[i] =
			m_aniData.Get(InState, i, frame).Transpose();
	}

	m_boneTransforms.Upload(context);
}

}
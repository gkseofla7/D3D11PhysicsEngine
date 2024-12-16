#pragma once
#include <map>
#include <string>
#include <future>
#include <unordered_set>
#include "GeometryGenerator2.h"
#include "EnginePch.h"

namespace dengine {
using namespace std;
class DSkinnedMeshModel2;
class Actor;
struct AnimationBlock
{
	//AnimationBlock() = default;
	//AnimationBlock(AnimationBlock&&) noexcept = default; // �̵� ������
	//AnimationBlock& operator=(AnimationBlock&&) noexcept = default; // �̵� ���� ������

	//// ���� ����
	//AnimationBlock(const AnimationBlock&) = delete;
	//AnimationBlock& operator=(const AnimationBlock&) = delete;

	//map<int, string > AnimStateToAnimName;
	string PathName;
	AnimationData AniData;
	std::unordered_map<string,std::future<AnimationData>> Loaders;
	bool IsFirstSetting = true;
	std::mutex mtx;
};
class AnimHelper
{
private:
	AnimHelper() {};
public:
	static AnimHelper& GetInstance();
	void Initialize();
	void AddAnimPath(int InActorId, string InPathName);
	void AddAnimStateToAnim(int InActorId, string inState, string inAnimName);
	bool LoadAnimation(DSkinnedMeshModel2* InActor, string inState);
	bool UpdateAnimation(Actor* InActor, string inState,
		int frame, int type = 0);

private:
	bool LoadAnimation(DSkinnedMeshModel2* InActor, string inState, bool& bInit);
private:

	map<int ,map<string, string>> m_animStateToAnim;
	map<int, string> m_pathMap;
	// modelId -> AnimationBlock
	map<int, AnimationBlock> m_animDatas;

	// ������ ���� �ִϸ��̼�
	map<int, string> m_actorAnimState;
	
	std::mutex mtx;
};

}
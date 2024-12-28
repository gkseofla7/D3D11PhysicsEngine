#pragma once
#include <map>
#include <string>
#include <future>
#include <unordered_set>
#include "GeometryGenerator2.h"
#include "../D3D12Core/EnginePch.h"
#include <shared_mutex>

namespace dengine {
using namespace std;
class DSkinnedMeshModel;
class Actor;
struct AnimationBlock
{
	AnimationBlock() = default;
	AnimationBlock(const AnimationBlock& other) = delete;

	AnimationBlock(AnimationBlock&& other)
	{
		PathName = other.PathName;
		AniData = other.AniData;
		Loaders = std::move(other.Loaders);
		IsFirstSetting = other.IsFirstSetting;
	}

	AnimationBlock& operator=(const AnimationBlock& other) = delete;

	AnimationBlock& operator=(AnimationBlock&& other) noexcept {
		if (this != &other) {
			PathName = other.PathName;
			AniData = other.AniData;
			Loaders = std::move(other.Loaders);
			IsFirstSetting = other.IsFirstSetting;
		}
		return *this;
	}

	string PathName;
	AnimationData AniData;
	std::unordered_map<string,std::future<AnimationData>> Loaders;
	bool IsFirstSetting = true;
	std::shared_mutex mtx;
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
	bool LoadAnimation(DSkinnedMeshModel* InActor, string inState);
	bool UpdateAnimation(Actor* InActor, string inState,
		int frame, int type = 0);

private:
	bool LoadAnimation(DSkinnedMeshModel* InActor, string inState, bool& bInit);
private:

	map<int ,map<string, string>> m_animStateToAnim;
	map<int, string> m_pathMap;
	// modelId -> AnimationBlock
	// ���� �����尡 ���ÿ� �����ϸ� ���� �߻��ϱ� ������
	// ����/�б� �� �и�
	map<int, AnimationBlock> m_animDatas;

	// ������ ���� �ִϸ��̼�
	map<int, string> m_actorAnimState;
	
	std::shared_mutex mtx;
};

}
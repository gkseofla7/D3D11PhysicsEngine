#pragma once
#include "SkeletalMeshActor.h"

namespace hlab {
class Wizard : public SkeletalMeshActor
{
public:
	Wizard(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
		shared_ptr<DModel> InModel);
	void Initialize(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
		shared_ptr<DModel> InModel) override;
	void Tick(float dt);
private:
	// TODO. Actor�� �ű� ����
	// �ʱ�ȭ
	void InitAnimPath();
	void LoadAnimAsync(string InState);
	virtual void InitBoundingKey() override;

	// ��Ǭ ���� �Լ�
	void Attack();
	void WalkStart();
	void WalkEnd();
	void RotateLeft(bool InOn);
	void RotateRight(bool InOn);
	void Jump();

	void ShotFireBall();

	bool bLeft = false;
	bool bRight = false;
};
}



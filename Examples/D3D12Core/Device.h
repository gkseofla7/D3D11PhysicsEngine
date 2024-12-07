#pragma once
#include "EnginePch.h"

namespace hlab {
class Device
{
public:
	void Init();

	ComPtr<IDXGIFactory> GetDXGI() { return _dxgi; }
	ComPtr<ID3D12Device> GetDevice() { return m_device; }

private:
	// COM(Component Object Model)
	// - DX�� ���α׷��� ��� �������� ���� ȣȯ���� �����ϰ� �ϴ� ���
	// - COM ��ü(COM �������̽�)�� ���. ���λ����� �츮���� ������
	// - ComPtr ������ ����Ʈ ������
	ComPtr<ID3D12Debug>			_debugController;
	ComPtr<IDXGIFactory>		_dxgi; // ȭ�� ���� ��ɵ�
	ComPtr<ID3D12Device>		m_device; // ���� ��ü ����
};


}

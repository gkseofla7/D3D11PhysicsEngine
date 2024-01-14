#include "Actor.h"
#include "Camera.h"
namespace hlab {

    //Camera ����
	void Actor::ActiveCaemera()
	{ 
        m_camera->UpdatePosDir(m_cameraCorrection* m_actorConsts.GetCpu().world);
	}
    void Actor::UpdateCemeraCorrection(Vector3 deltaPos)
    {
        m_cameraCorrection = Matrix::CreateTranslation(deltaPos);
    }

    //Actor ��ġ ����
	void Actor::UpdateWorldRow(const Matrix& worldRow)
	{
        this->m_worldRow = worldRow;
        this->m_worldITRow = worldRow;
        m_worldITRow.Translation(Vector3(0.0f));
        m_worldITRow = m_worldITRow.Invert().Transpose();

        // �ٿ�����Ǿ� ��ġ ������Ʈ
        // �����ϱ��� ����ϰ� �ʹٸ� x, y, z ������ �� ���� ū ������ ������
        // ��(sphere)�� ȸ���� ����� �ʿ� ����
        m_boundingSphere.Center = this->m_worldRow.Translation();

        m_actorConsts.GetCpu().world = worldRow.Transpose();
        m_actorConsts.GetCpu().worldIT = m_worldITRow.Transpose();
        m_actorConsts.GetCpu().worldInv = m_actorConsts.GetCpu().world.Invert();
	}

    void Actor::UpdateConstantBuffers(ComPtr<ID3D11Device>& device,
        ComPtr<ID3D11DeviceContext>& context) {
        if (m_isVisible) {
            m_model->UpdateConstantBuffers(device, context);
            m_actorConsts.Upload(context);
        }
    }
}

#include "DModel.h"
#include "GeometryGenerator.h"
#include "MeshLoadHelper.h"

#include <filesystem>

namespace hlab {

    using namespace std;
    using namespace DirectX;

    DModel::DModel(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
        const std::string& basePath, const std::string& filename) {
        Initialize(device, context, basePath, filename);
    }

    void DModel::Initialize(ComPtr<ID3D11Device>& device,
        ComPtr<ID3D11DeviceContext>& context) {
        std::cout << "Model::Initialize(ComPtr<ID3D11Device> &device, "
            "ComPtr<ID3D11DeviceContext> &context) was not implemented."
            << std::endl;
        exit(-1);
    }

    void DModel::InitMeshBuffers(ComPtr<ID3D11Device>& device,
        const MeshData& meshData,
        shared_ptr<Mesh>& newMesh) {

        D3D11Utils::CreateVertexBuffer(device, meshData.vertices,
            newMesh->vertexBuffer);
        newMesh->indexCount = UINT(meshData.indices.size());
        newMesh->vertexCount = UINT(meshData.vertices.size());
        newMesh->stride = UINT(sizeof(Vertex));
        D3D11Utils::CreateIndexBuffer(device, meshData.indices,
            newMesh->indexBuffer);
    }

    void DModel::Initialize(ComPtr<ID3D11Device>& device,
        ComPtr<ID3D11DeviceContext>& context,
        const std::string& basePath,
        const std::string& filename) {
        // �Ϲ������δ� Mesh���� m_mesh/materialConsts�� ���� ���� ����
        // ���⼭�� �� Model ���� ���� Mesh���� Consts�� ��� ����
        m_basePath = basePath;
        m_filename = filename;

        m_meshConsts.GetCpu().world = Matrix();
        m_meshConsts.Initialize(device);
        m_materialConsts.Initialize(device);

        if (m_initializeMesh = MeshLoadHelper::LoadModelData(device, context, basePath, filename, m_meshes))
        {
            MeshLoadHelper::SetMaterial(m_basePath, m_filename, m_materialConsts.GetCpu());
        }
    }
    


    void DModel::UpdateConstantBuffers(ComPtr<ID3D11Device>& device,
        ComPtr<ID3D11DeviceContext>& context) {
        if (m_isVisible) {
            m_meshConsts.Upload(context);
            m_materialConsts.Upload(context);
        }
    }

    GraphicsPSO& DModel::GetPSO(const bool wired) {
        return wired ? Graphics::defaultWirePSO : Graphics::defaultSolidPSO;
    }

    GraphicsPSO& DModel::GetDepthOnlyPSO() { return Graphics::depthOnlyPSO; }

    GraphicsPSO& DModel::GetReflectPSO(const bool wired) {
        return wired ? Graphics::reflectWirePSO : Graphics::reflectSolidPSO;
    }

    void DModel::Render(ComPtr<ID3D11DeviceContext>& context) {
        if (m_initializeMesh == false)
        {
            if (m_initializeMesh = MeshLoadHelper::GetMesh(m_basePath, m_filename, m_meshes))
            {
                MeshLoadHelper::SetMaterial(m_basePath, m_filename, m_materialConsts.GetCpu());
            }
            else
            {
                return;
            }

        } 
        if (m_isVisible) { 
            for (auto& mesh : *m_meshes) {
                //TODO mesh Consts�� ������, Model �������� ���
                ID3D11Buffer* constBuffers[2] = { m_meshConsts.Get(),
                                                 m_materialConsts.Get() };
                context->VSSetConstantBuffers(1, 2, constBuffers);

                context->VSSetShaderResources(0, 1, mesh.heightSRV.GetAddressOf());

                // ��ü �������� �� �������� �ؽ��� ��� (t0 ���ͽ���)
                vector<ID3D11ShaderResourceView*> resViews = {
                    mesh.albedoSRV.Get(), mesh.normalSRV.Get(), mesh.aoSRV.Get(),
                    mesh.metallicRoughnessSRV.Get(), mesh.emissiveSRV.Get() };
                context->PSSetShaderResources(0, // register(t0)
                    UINT(resViews.size()),
                    resViews.data());
                context->PSSetConstantBuffers(1, 2, constBuffers);

                // Volume Rendering
                if (mesh.densityTex.GetSRV())
                    context->PSSetShaderResources(
                        5, 1, mesh.densityTex.GetAddressOfSRV());
                if (mesh.lightingTex.GetSRV())
                    context->PSSetShaderResources(
                        6, 1, mesh.lightingTex.GetAddressOfSRV());

                context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(),
                    &mesh.stride, &mesh.offset);
                context->IASetIndexBuffer(mesh.indexBuffer.Get(),
                    DXGI_FORMAT_R32_UINT, 0);
                context->DrawIndexed(mesh.indexCount, 0, 0);

                // Release resources
                ID3D11ShaderResourceView* nulls[3] = { NULL, NULL, NULL };
                context->PSSetShaderResources(5, 3, nulls);
            }
        }
    }

    void DModel::UpdateAnimation(ComPtr<ID3D11DeviceContext>& context, string clipId,
        int frame, int type = 0) {
        // class SkinnedMeshModel���� override
        cout << "Model::UpdateAnimation(ComPtr<ID3D11DeviceContext> &context, "
            "int clipId, int frame) was not implemented."
            << endl;
        exit(-1);
    }

    void DModel::RenderWireBoundingBox(ComPtr<ID3D11DeviceContext>& context) {
        ID3D11Buffer* constBuffers[2] = {
            m_boundingBoxMesh->meshConstsGPU.Get(),
            m_boundingBoxMesh->materialConstsGPU.Get() };
        context->VSSetConstantBuffers(1, 2, constBuffers);
        context->IASetVertexBuffers(
            0, 1, m_boundingBoxMesh->vertexBuffer.GetAddressOf(),
            &m_boundingBoxMesh->stride, &m_boundingBoxMesh->offset);
        context->IASetIndexBuffer(m_boundingBoxMesh->indexBuffer.Get(),
            DXGI_FORMAT_R32_UINT, 0);
        context->DrawIndexed(m_boundingBoxMesh->indexCount, 0, 0);
    }

    void DModel::RenderWireBoundingSphere(ComPtr<ID3D11DeviceContext>& context) {
        ID3D11Buffer* constBuffers[2] = {
            m_boundingBoxMesh->meshConstsGPU.Get(),
            m_boundingBoxMesh->materialConstsGPU.Get() };
        context->VSSetConstantBuffers(1, 2, constBuffers);
        context->IASetVertexBuffers(
            0, 1, m_boundingSphereMesh->vertexBuffer.GetAddressOf(),
            &m_boundingSphereMesh->stride, &m_boundingSphereMesh->offset);
        context->IASetIndexBuffer(m_boundingSphereMesh->indexBuffer.Get(),
            DXGI_FORMAT_R32_UINT, 0);
        context->DrawIndexed(m_boundingSphereMesh->indexCount, 0, 0);
    }

    void DModel::UpdateWorldRow(const Matrix& worldRow) {
        this->m_worldRow = worldRow;
        this->m_worldITRow = worldRow;
        m_worldITRow.Translation(Vector3(0.0f));
        m_worldITRow = m_worldITRow.Invert().Transpose();

        // �ٿ�����Ǿ� ��ġ ������Ʈ
        // �����ϱ��� ����ϰ� �ʹٸ� x, y, z ������ �� ���� ū ������ ������
        // ��(sphere)�� ȸ���� ����� �ʿ� ����
        m_boundingSphere.Center = this->m_worldRow.Translation();

        m_meshConsts.GetCpu().world = worldRow.Transpose();
        m_meshConsts.GetCpu().worldIT = m_worldITRow.Transpose();
        m_meshConsts.GetCpu().worldInv = m_meshConsts.GetCpu().world.Invert();
    }

} // namespace hlab
#pragma once
#include "ConstantBuffers.h"
#include "D3D11Utils.h"
#include "GraphicsCommon.h"
#include "Mesh.h"
#include "MeshData.h"
#include "StructuredBuffer.h"

#include <directxtk/SimpleMath.h>
namespace hlab {

    using std::cout;
    using std::endl;
    using std::string;
    using std::vector;

    class DModel {
    public:
        DModel() {}
        DModel(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
            const string& basePath, const string& filename);
        DModel(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
            const vector<MeshData>& meshes);

        virtual void Initialize(ComPtr<ID3D11Device>& device,
            ComPtr<ID3D11DeviceContext>& context);

        virtual void InitMeshBuffers(ComPtr<ID3D11Device>& device,
            const MeshData& meshData,
            shared_ptr<Mesh>& newMesh);

        void Initialize(ComPtr<ID3D11Device>& device,
            ComPtr<ID3D11DeviceContext>& context,
            const string& basePath, const string& filename);

        void Initialize(ComPtr<ID3D11Device>& device,
            ComPtr<ID3D11DeviceContext>& context,
            const vector<MeshData>& meshes);

        void UpdateConstantBuffers(ComPtr<ID3D11Device>& device,
            ComPtr<ID3D11DeviceContext>& context);

        virtual GraphicsPSO& GetPSO(const bool wired);
        virtual GraphicsPSO& GetDepthOnlyPSO();
        virtual GraphicsPSO& GetReflectPSO(const bool wired);

        virtual void Render(ComPtr<ID3D11DeviceContext>& context);
        virtual void UpdateAnimation(ComPtr<ID3D11DeviceContext>& context,
            int clipId, int frame, int type);
        virtual void RenderNormals(ComPtr<ID3D11DeviceContext>& context);
        virtual void RenderWireBoundingBox(ComPtr<ID3D11DeviceContext>& context);
        virtual void RenderWireBoundingSphere(ComPtr<ID3D11DeviceContext>& context);
        void UpdateWorldRow(const Matrix& worldRow);

    public:
        Matrix m_worldRow = Matrix();   // Model(Object) To World ���
        Matrix m_worldITRow = Matrix(); // InverseTranspose

        bool m_drawNormals = false;
        bool m_isVisible = true;
        bool m_castShadow = true;
        bool m_isPickable = false; // ���콺�� ����/���� ���� ����

        vector<shared_ptr<Mesh>> m_meshes;

        ConstantBuffer<MeshConstants> m_meshConsts;
        ConstantBuffer<MaterialConstants> m_materialConsts;

        DirectX::BoundingBox m_boundingBox;
        DirectX::BoundingSphere m_boundingSphere;

        string m_name = "NoName";
    private:
        shared_ptr<Mesh> m_boundingBoxMesh;
        shared_ptr<Mesh> m_boundingSphereMesh;
    };

} // namespace hlab

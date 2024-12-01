#pragma once
#pragma once

#include "ConstantBuffers.h"
#include "D3D12Utils.h"
#include "GraphicsCommonD3D12.h"
#include "Mesh.h"
#include "MeshData.h"
#include "StructuredBuffer.h"

#include <directxtk/SimpleMath.h>

// ����: DirectX-Graphics-Sampels
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Model/Model.h

namespace hlab {

    using std::cout;
    using std::endl;
    using std::string;
    using std::vector;

    class DModel2 {
    public:
        DModel2() {}
        DModel2(ComPtr<ID3D12Device>& device, const string& basePath, const string& filename);
        DModel2(ComPtr<ID3D12Device>& device, const string& meshKey);
        virtual void Initialize(ComPtr<ID3D12Device>& device,
            const string& basePath,
            const string& filename);
        void Initialize(ComPtr<ID3D12Device>& device,
            const string& meshKey);
        virtual void InitMeshBuffers(ComPtr<ID3D12Device>& device,
            const MeshData& meshData,
            shared_ptr<Mesh>& newMesh);
        void Tick(float dt);
        void UpdateConstantBuffers(ComPtr<ID3D11DeviceContext>& context);
        virtual void UpdateAnimation(ComPtr<ID3D11Device>& device,
            ComPtr<ID3D11DeviceContext>& context,
            string clipId, int frame, int type = 0) {}
        virtual void UpdatePosition(const Vector3& inDelta);
        void SetWorldPosition(const Vector3& InPos);
        virtual void UpdateRotation(const Matrix& inDelta);
        virtual void UpdateVelocity(float dt) {}
        void SetDirection(const Vector3& inDirection);
        Vector3 GetWorldPosition() { return m_worldRow.Translation(); }
        Vector3 GetDirection() { return m_direction; }
        virtual GraphicsPSO& GetPSO(const bool wired);
        virtual GraphicsPSO& GetDepthOnlyPSO();
        virtual GraphicsPSO& GetReflectPSO(const bool wired);

        virtual void Render(ComPtr<ID3D11DeviceContext>& context);
        virtual void UpdateAnimation(ComPtr<ID3D11DeviceContext>& context,
            string clipId, int frame, int type);
        virtual void RenderWireBoundingBox(ComPtr<ID3D11DeviceContext>& context);
        virtual void RenderWireBoundingSphere(ComPtr<ID3D11DeviceContext>& context);
        void UpdateWorldRow(const Matrix& worldRow);


        void SetScale(float InScale) { m_scale = InScale; }

        bool IsMeshInitialized() { return m_initializeMesh; }
    private:
        bool LoadMesh();
    public:
        Matrix m_worldRow = Matrix();   // Model(Object) To World ���
        Matrix m_worldITRow = Matrix(); // InverseTranspose

        bool m_drawNormals = false;
        bool m_isVisible = true;
        bool m_castShadow = true;
        bool m_isPickable = false; // ���콺�� ����/���� ���� ����

        vector<Mesh>* m_meshes;

        DConstantBuffer<MeshConstants> m_meshConsts;
        DConstantBuffer<MaterialConstants> m_materialConsts;

        DirectX::BoundingBox m_boundingBox;
        DirectX::BoundingSphere m_boundingSphere;

        string m_name = "NoName";
        int m_modelId = -1;

        int m_maxFrame = 0;
    protected:
        // Node : �� ���� true���� gpu �޸𸮿� �ö󰣰��� ������� �ʴ´�.
        bool m_initializeMesh = false;
    private:
        string m_basePath;
        string m_filename;

        string m_meshKey;

        shared_ptr<Mesh> m_boundingBoxMesh;
        shared_ptr<Mesh> m_boundingSphereMesh;

        float m_scale = 1.0f;
        bool m_initializeBoundingVolume = false;

        Vector3 m_direction;
    };

} // namespace hlab

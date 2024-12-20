#pragma once
#include "EnginePch.h"
#include "Mesh2.h"
#include "MeshData.h"
#include "ConstantBuffer.h"
#include <directxtk/SimpleMath.h>

namespace dengine {

    using std::cout;
    using std::endl;
    using std::string;
    using std::vector;
//class GraphicsPSO2;
class DModel2 
{
public:
    DModel2() {}
    DModel2(const string& basePath, const string& filename);
    DModel2(const string& meshKey);
    virtual void Initialize(const string& basePath,
        const string& filename);
    void Initialize(const string& meshKey);
    virtual void InitMeshBuffers(const MeshData& meshData,
        shared_ptr<DMesh>& newMesh);
    void Tick(float dt);
    void UpdateConstantBuffers();
    virtual void UpdatePosition(const Vector3& inDelta);
    void SetWorldPosition(const Vector3& InPos);
    virtual void UpdateRotation(const Matrix& inDelta);
    virtual void UpdateVelocity(float dt) {}
    void SetDirection(const Vector3& inDirection);
    Vector3 GetWorldPosition() { return m_worldRow.Translation(); }
    Vector3 GetDirection() { return m_direction; }
    //virtual GraphicsPSO2& GetPSO();
    //virtual GraphicsPSO2& GetDepthOnlyPSO();
    //virtual GraphicsPSO2& GetReflectPSO(const bool wired);

    virtual void Render();
    virtual void UpdateAnimation(string clipId, int frame, int type = 0);
    void UpdateWorldRow(const Matrix& worldRow);


    void SetScale(float InScale) { m_scale = InScale; }

    bool IsMeshInitialized() { return m_initializeMesh; }

private:
    bool LoadMesh();
public:
    Matrix m_worldRow = Matrix();   // Model(Object) To World 행렬
    Matrix m_worldITRow = Matrix(); // InverseTranspose

    bool m_drawNormals = false;
    bool m_isVisible = true;
    bool m_castShadow = true;
    bool m_isPickable = false; // 마우스로 선택/조작 가능 여부

    vector<DMesh>* m_meshes;

    ConstantBuffer2<MeshConstants2> m_meshConsts;
    ConstantBuffer2<MaterialConstants2> m_materialConsts;

    DirectX::BoundingBox m_boundingBox;
    DirectX::BoundingSphere m_boundingSphere;

    string m_name = "NoName";
    int m_modelId = -1;

    int m_maxFrame = 0;
protected:
    // Node : 이 값이 true여도 gpu 메모리에 올라간것은 보장되지 않는다.
    bool m_initializeMesh = false;
private:
    string m_basePath;
    string m_filename;

    string m_meshKey;

    shared_ptr<DMesh> m_boundingBoxMesh;
    shared_ptr<DMesh> m_boundingSphereMesh;

    float m_scale = 1.0f;
    Vector3 m_direction;
};

} // namespace dengine

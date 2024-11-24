#pragma once

#include <directxtk/SimpleMath.h>
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include <vector>
#include <unordered_map>

#include "Camera.h"
#include "ComputePSO.h"
#include "ConstantBuffers.h"
#include "D3D12Utils.h"
#include "GraphicsPSO.h"
#include "Model.h"
#include "Actor.h"
#include "PostProcess.h"
#include "Timer.h"
#include "bullet/btBulletDynamicsCommon.h"
//#include "Delegate.h"

namespace hlab {

    using DirectX::BoundingSphere;
    using DirectX::SimpleMath::Quaternion;
    using DirectX::SimpleMath::Ray;
    using DirectX::SimpleMath::Vector3;
    using Microsoft::WRL::ComPtr;
    using std::shared_ptr;
    using std::vector;
    using std::wstring;

    class Actor;
    class Object;
    class AppBase {
    public:
        AppBase();
        virtual ~AppBase();

        int Run();
        float GetAspectRatio() const;

        virtual bool Initialize();
        virtual bool InitScene();
        virtual void UpdateGUI() {}
        virtual void Update(float dt);
        virtual void UpdateLights(float dt);
        virtual void RenderDepthOnly();
        virtual void RenderShadowMaps();
        virtual void RenderOpaqueObjects();
        virtual void RenderMirror();
        virtual void Render();

        virtual void OnMouseMove(int mouseX, int mouseY);
        virtual void OnMouseClick(int mouseX, int mouseY);
        virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        void PostRender();

        void InitCubemaps(wstring basePath, wstring envFilename,
            wstring specularFilename, wstring irradianceFilename,
            wstring brdfFilename);
        void UpdateGlobalConstants(const float& dt, const Vector3& eyeWorld,
            const Matrix& viewRow, const Matrix& projRow,
            const Matrix& refl = Matrix());
        void SetGlobalConsts(ComPtr<ID3D11Buffer>& globalConstsGPU);

        void CreateDepthBuffers();
        void SetPipelineState(const GraphicsPSO& pso);
        void SetPipelineState(const ComputePSO& pso);
        shared_ptr<Model> PickClosest(const Ray& pickingRay, float& minDist);
        void SelectClosestActor(const Ray& pickingRay, float& minDist);
        void ProcessMouseControl();

    protected: // ��� ���� Ŭ���������� ���� ����
        bool InitMainWindow();
        bool InitDirect3D();
        bool InitGUI();
        void CreateBuffers();
        void SetMainViewport();
        void SetShadowViewport();
        void ComputeShaderBarrier();

        void KillObjects();
    public:
        // ���� �̸� ���̴� ��Ģ�� VS DX11/12 �⺻ ���ø��� �����ϴ�.
        // ���� �̸��� ���̱� ���� d3d�� �����߽��ϴ�.
        // ��: m_d3dDevice -> m_device

        int m_screenWidth;
        int m_screenHeight;
        HWND m_mainWindow;
        bool m_useMSAA = true;
        UINT m_numQualityLevels = 0;
        bool m_drawAsWire = false;
        bool m_drawOBB = false; // Draw Object Oriented Bounding Box
        bool m_drawBS = false;  // Draw Bounding Sphere

        DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D11RenderTargetView> m_backBufferRTV;
        ComPtr<ID3D12Resource> m_renderTargets[2];
        // �ﰢ�� ������ȭ -> float(MSAA) -> resolved(No MSAA)
        // -> ��ó��(���, �����) -> backBuffer(���� SwapChain Present)
        CD3DX12_CPU_DESCRIPTOR_HANDLE m_renderTargetViews[2];

        ComPtr<ID3D12Resource> m_floatBuffer;
        ComPtr<ID3D11Texture2D> m_resolvedBuffer;
        ComPtr<ID3D11Texture2D> m_postEffectsBuffer;
        ComPtr<ID3D11Texture2D> m_prevBuffer; // ������ ��� �� ȿ��
        CD3DX12_CPU_DESCRIPTOR_HANDLE m_floatRTV;
        ComPtr<ID3D11RenderTargetView> m_resolvedRTV;
        ComPtr<ID3D11RenderTargetView> m_postEffectsRTV;
        ComPtr<ID3D11RenderTargetView> m_prevRTV;
        ComPtr<ID3D11ShaderResourceView> m_resolvedSRV;
        ComPtr<ID3D11ShaderResourceView> m_postEffectsSRV;
        ComPtr<ID3D11ShaderResourceView> m_prevSRV;

        // Depth buffer ����
        ComPtr<ID3D11Texture2D> m_depthOnlyBuffer; // No MSAA
        ComPtr<ID3D11DepthStencilView> m_depthOnlyDSV;
        CD3DX12_CPU_DESCRIPTOR_HANDLE m_defaultDSV;
        ComPtr<ID3D11ShaderResourceView> m_depthOnlySRV;

        // Shadow maps
        int m_shadowWidth = 1280;
        int m_shadowHeight = 1280;
        ComPtr<ID3D11Texture2D> m_shadowBuffers[MAX_LIGHTS]; // No MSAA
        ComPtr<ID3D11DepthStencilView> m_shadowDSVs[MAX_LIGHTS];
        ComPtr<ID3D11ShaderResourceView> m_shadowSRVs[MAX_LIGHTS];

        D3D12_VIEWPORT m_screenViewport;
        D3D12_RECT m_scissorRect;
        // ������ �����ϴ� ī�޶� Ŭ���� �߰�
        Camera m_camera;
        bool m_keyPressed[256] = {
            false,
        };

        bool m_leftButton = false;
        bool m_rightButton = false;
        bool m_dragStartFlag = false;

        // ���콺 Ŀ�� ��ġ ���� (Picking�� ���)
        float m_mouseNdcX = 0.0f;
        float m_mouseNdcY = 0.0f;
        float m_wheelDelta = 0.0f;
        int m_mouseX = -1;
        int m_mouseY = -1;

        // ������ -> PostEffects -> PostProcess
        PostEffectsConstants m_postEffectsConstsCPU;
        ComPtr<ID3D11Buffer> m_postEffectsConstsGPU;

        PostProcess m_postProcess;

        // �پ��� Pass���� �� ������ �����ϱ� ���� ConstBuffer�� �и�
        GlobalConstants m_globalConstsCPU;
        GlobalConstants m_reflectGlobalConstsCPU;
        GlobalConstants m_shadowGlobalConstsCPU[MAX_LIGHTS];
        ComPtr<ID3D11Buffer> m_globalConstsGPU;
        ComPtr<ID3D11Buffer> m_reflectGlobalConstsGPU;
        ComPtr<ID3D11Buffer> m_shadowGlobalConstsGPU[MAX_LIGHTS];

        // �������� ����ϴ� �ؽ����
        ComPtr<ID3D11ShaderResourceView> m_envSRV;
        ComPtr<ID3D11ShaderResourceView> m_irradianceSRV;
        ComPtr<ID3D11ShaderResourceView> m_specularSRV;
        ComPtr<ID3D11ShaderResourceView> m_brdfSRV;

        bool m_lightRotate = false;
        bool m_pauseAnimation = false;

        // ���� ������ ����
        shared_ptr<Model> m_screenSquare; // PostEffect�� ���
        shared_ptr<Model> m_skybox;
        shared_ptr<Model> m_pickedModel;
        shared_ptr<Model> m_lightSphere[MAX_LIGHTS];
        shared_ptr<Model> m_cursorSphere;
        shared_ptr<Model> m_mirror; // �ſ��� ������ �׸�
        DirectX::SimpleMath::Plane m_mirrorPlane;
        float m_mirrorAlpha = 1.0f; // Opacity

        // �ſ��� �ƴ� ��ü���� ����Ʈ (for������ �׸��� ����)
        vector<shared_ptr<Model>> m_basicList;

        vector<shared_ptr<Model>> m_objects; // ���� ������ ����ȭ ������ �� ��� TODO: actor list�� ����

        vector<shared_ptr<Object>> m_objectList;

        //���� ���� ��� �ش� ���Ϳ� Ű�� ���ε� ���ִٸ� ���Ͱ� ����
        shared_ptr<Actor> m_activateActor;

        // Physics Engine
        btAlignedObjectArray<btCollisionShape*> m_collisionShapes;

    };

} // namespace hlab
#include "AppBaseD3D12.h"

#include <algorithm>
#include <directxtk/SimpleMath.h>

#include "GraphicsCommonD3D12.h"
#include "Actor.h"
#include "DaerimsEngineBase.h"

// imgui_impl_win32.cpp�� ���ǵ� �޽��� ó�� �Լ��� ���� ���� ����
// Vcpkg�� ���� IMGUI�� ����� ��� �����ٷ� ��� �� �� ����
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

namespace hlab {

    using namespace std;
    using namespace DirectX;
    using namespace DirectX::SimpleMath;
    using DirectX::BoundingSphere;
    using DirectX::SimpleMath::Vector3;

    AppBase* g_appBase = nullptr;
    void GetHardwareAdapter(
        IDXGIFactory1* pFactory,
        IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter);

    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        return g_appBase->MsgProc(hWnd, msg, wParam, lParam);
    }

    AppBase::AppBase()
        : m_screenWidth(1280), m_screenHeight(720), m_mainWindow(0),
        m_screenViewport(D3D11_VIEWPORT()) {

        g_appBase = this;

        m_camera.SetAspectRatio(this->GetAspectRatio());
    }

    AppBase::~AppBase() {
        g_appBase = nullptr;

        // Cleanup
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        DestroyWindow(m_mainWindow);
        // UnregisterClass(wc.lpszClassName, wc.hInstance);//����
    }

    float AppBase::GetAspectRatio() const {
        return float(m_screenWidth) / m_screenHeight;
    }

    int AppBase::Run() {

        // Main message loop
        MSG msg = { 0 };
        while (WM_QUIT != msg.message) {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                ImGui_ImplDX11_NewFrame();
                ImGui_ImplWin32_NewFrame();

                ImGui::NewFrame();
                ImGui::Begin("Scene Control");

                // ImGui�� �������ִ� Framerate ���
                ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);

                UpdateGUI(); // �߰������� ����� GUI

                ImGui::End();
                ImGui::Render();
                Update(ImGui::GetIO().DeltaTime);

                ThreadPool& threadPool = ThreadPool::getInstance();

                // ���� ������ �۾� �ϷḦ ��ٸ�
                {
                    std::unique_lock<std::mutex> lock(threadPool.m_render_job_q_);
                    threadPool.cv_render_job_q_.wait(lock, [&]() { return threadPool.IsRenderThreadDone(); });
                    threadPool.SetUsingMainThreadUsingRendering(true);
                }
                Render();


                // Example�� Render()���� RT ������ ������ �ʾ��� ��쿡��
                // �� ���ۿ� GUI�� �׸������� RT ����
                // ��) Render()���� ComputeShader�� ���
                AppBase::SetMainViewport();
                m_context->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(),
                    NULL);

                // GUI ������
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

                // GUI ������ �Ŀ� Present() ȣ��
                m_swapChain->Present(1, 0);

                threadPool.SetUsingMainThreadUsingRendering(false);
                threadPool.cv_render_job_q_.notify_one();
            }
        }

        return 0;
    }

    bool AppBase::Initialize() {

        if (!InitMainWindow())
            return false;
        ThreadPool& tPool = ThreadPool::getInstance();
        {
            // TODO. Atomic �������� ����
            std::unique_lock<std::mutex> lock(tPool.m_render_job_q_);
            tPool.SetUsingMainThreadUsingRendering(true);
        }
        bool bInitDirect = InitDirect3D();
        tPool.SetUsingMainThreadUsingRendering(false);
        tPool.cv_render_job_q_.notify_one();

        if (!bInitDirect)
            return false;

        if (!InitGUI())
            return false;

        if (!InitScene())
            return false;

        // PostEffect�� ���
        m_screenSquare = make_shared<Model>(
            m_device, m_context, vector{ GeometryGenerator::MakeSquare() });

        // ȯ�� �ڽ� �ʱ�ȭ
        MeshData skyboxMesh = GeometryGenerator::MakeBox(40.0f);
        std::reverse(skyboxMesh.indices.begin(), skyboxMesh.indices.end());
        m_skybox = make_shared<Model>(m_device, m_context, vector{ skyboxMesh });
        m_skybox->m_name = "SkyBox";

        // �ܼ�â�� ������ â�� ���� ���� ����
        SetForegroundWindow(m_mainWindow);

        return true;
    }

    // ���� �������� ���������� ����ϱ� ���� ��� ����
    bool AppBase::InitScene() {

        // ���� ����
        {
            // ���� 0�� ����
            m_globalConstsCPU.lights[0].radiance = Vector3(5.0f);
            m_globalConstsCPU.lights[0].position = Vector3(0.0f, 1.5f, 1.1f);
            m_globalConstsCPU.lights[0].direction = Vector3(0.0f, -1.0f, 0.0f);
            m_globalConstsCPU.lights[0].spotPower = 3.0f;
            m_globalConstsCPU.lights[0].radius = 0.04f;
            m_globalConstsCPU.lights[0].type =
                LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow

            // ���� 1�� ��ġ�� ������ Update()���� ����
            m_globalConstsCPU.lights[1].radiance = Vector3(5.0f);
            m_globalConstsCPU.lights[1].spotPower = 3.0f;
            m_globalConstsCPU.lights[1].fallOffEnd = 20.0f;
            m_globalConstsCPU.lights[1].radius = 0.02f;
            m_globalConstsCPU.lights[1].type =
                LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow

            // ���� 2�� ������
            m_globalConstsCPU.lights[2].type = LIGHT_OFF;
        }

        // TODO. ��� Object�� ��ȯ �ʿ�
        return true;
        // ���� ��ġ ǥ��
        {
            for (int i = 0; i < MAX_LIGHTS; i++) {
                MeshData sphere = GeometryGenerator::MakeSphere(1.0f, 20, 20);
                m_lightSphere[i] =
                    make_shared<Model>(m_device, m_context, vector{ sphere });
                m_lightSphere[i]->UpdateWorldRow(Matrix::CreateTranslation(
                    m_globalConstsCPU.lights[i].position));
                m_lightSphere[i]->m_materialConsts.GetCpu().albedoFactor =
                    Vector3(0.0f);
                m_lightSphere[i]->m_materialConsts.GetCpu().emissionFactor =
                    Vector3(1.0f, 1.0f, 0.0f);
                m_lightSphere[i]->m_castShadow =
                    false; // ���� ǥ�� ��ü���� �׸��� X

                // if (m_globalConstsCPU.lights[i].type == 0)
                m_lightSphere[i]->m_isVisible = false;
                m_lightSphere[i]->m_name = "LightSphere" + std::to_string(i);
                m_lightSphere[i]->m_isPickable = false;

                m_basicList.push_back(m_lightSphere[i]); // ����Ʈ�� ���
            }
        }

        // Ŀ�� ǥ�� (Main sphere���� �浹�� �����Ǹ� ���� ������ �۰� �׷����� ��)
        {
            MeshData sphere = GeometryGenerator::MakeSphere(0.01f, 10, 10);
            m_cursorSphere =
                make_shared<Model>(m_device, m_context, vector{ sphere });
            m_cursorSphere->m_isVisible = false; // ���콺�� ������ ���� ����
            m_cursorSphere->m_castShadow = false; // �׸��� X
            m_cursorSphere->m_materialConsts.GetCpu().albedoFactor = Vector3(0.0f);
            m_cursorSphere->m_materialConsts.GetCpu().emissionFactor =
                Vector3(0.0f, 1.0f, 0.0f);

            m_basicList.push_back(m_cursorSphere); // ����Ʈ�� ���
        }

        return true;
    }

    void AppBase::Update(float dt) {

        KillObjects();
        // ī�޶��� �̵�
        m_camera.UpdateKeyboard(dt, m_keyPressed);

        // �ݻ� ��� �߰�
        const Vector3 eyeWorld = m_camera.GetEyePos();
        const Matrix reflectRow = Matrix::CreateReflection(m_mirrorPlane);
        const Matrix viewRow = m_camera.GetViewRow();
        const Matrix projRow = m_camera.GetProjRow();

        UpdateLights(dt);

        // ���� ConstantBuffer ������Ʈ
        AppBase::UpdateGlobalConstants(dt, eyeWorld, viewRow, projRow, reflectRow);

        // �ſ��� ���� ó��
        if (m_mirror)
            m_mirror->UpdateConstantBuffers(m_device, m_context);

        // ������ ��ġ �ݿ�
        //for (int i = 0; i < MAX_LIGHTS; i++)
        //    m_lightSphere[i]->UpdateWorldRow(
        //        Matrix::CreateScale(
        //            std::max(0.01f, m_globalConstsCPU.lights[i].radius)) *
        //        Matrix::CreateTranslation(m_globalConstsCPU.lights[i].position));

        ProcessMouseControl();
        for (auto& i : m_basicList) {
            i->UpdateConstantBuffers(m_device, m_context);
        }
        int objectCount = m_objectList.size();
        for (int i = 0; i < m_objectList.size(); i++)
        {
            m_objectList[i]->Tick(dt);
            // TODO. Tick���� ���°� ������ ������,,
            m_objectList[i]->GetModel()->UpdateConstantBuffers(m_context);
        }
    }

    void AppBase::UpdateLights(float dt) {

        // ȸ���ϴ� lights[1] ������Ʈ
        static Vector3 lightDev = Vector3(1.0f, 0.0f, 0.0f);
        if (m_lightRotate) {
            lightDev = Vector3::Transform(
                lightDev, Matrix::CreateRotationY(dt * 3.141592f * 0.5f));
        }
        m_globalConstsCPU.lights[1].position = Vector3(0.0f, 1.1f, 2.0f) + lightDev;
        Vector3 focusPosition = Vector3(0.0f, -0.5f, 1.7f);
        m_globalConstsCPU.lights[1].direction =
            focusPosition - m_globalConstsCPU.lights[1].position;
        m_globalConstsCPU.lights[1].direction.Normalize();

        // �׸��ڸ��� ����� ���� ����
        for (int i = 0; i < MAX_LIGHTS; i++) {
            const auto& light = m_globalConstsCPU.lights[i];
            if (light.type & LIGHT_SHADOW) {

                Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
                if (abs(up.Dot(light.direction) + 1.0f) < 1e-5)
                    up = Vector3(1.0f, 0.0f, 0.0f);

                // �׸��ڸ��� ���� �� �ʿ�
                Matrix lightViewRow = XMMatrixLookAtLH(
                    light.position, light.position + light.direction, up);

                Matrix lightProjRow = XMMatrixPerspectiveFovLH(
                    XMConvertToRadians(120.0f), 1.0f, 0.1f, 10.0f);

                m_shadowGlobalConstsCPU[i].eyeWorld = light.position;
                m_shadowGlobalConstsCPU[i].view = lightViewRow.Transpose();
                m_shadowGlobalConstsCPU[i].proj = lightProjRow.Transpose();
                m_shadowGlobalConstsCPU[i].invProj =
                    lightProjRow.Invert().Transpose();
                m_shadowGlobalConstsCPU[i].viewProj =
                    (lightViewRow * lightProjRow).Transpose();

                // LIGHT_FRUSTUM_WIDTH Ȯ��
                // Vector4 eye(0.0f, 0.0f, 0.0f, 1.0f);
                // Vector4 xLeft(-1.0f, -1.0f, 0.0f, 1.0f);
                // Vector4 xRight(1.0f, 1.0f, 0.0f, 1.0f);
                // eye = Vector4::Transform(eye, lightProjRow);
                // xLeft = Vector4::Transform(xLeft, lightProjRow.Invert());
                // xRight = Vector4::Transform(xRight, lightProjRow.Invert());
                // xLeft /= xLeft.w;
                // xRight /= xRight.w;
                // cout << "LIGHT_FRUSTUM_WIDTH = " << xRight.x - xLeft.x <<
                // endl;

                D3D11Utils::UpdateBuffer(m_context, m_shadowGlobalConstsCPU[i],
                    m_shadowGlobalConstsGPU[i]);

                // �׸��ڸ� ������ �������� �� �ʿ�
                m_globalConstsCPU.lights[i].viewProj =
                    m_shadowGlobalConstsCPU[i].viewProj;
                m_globalConstsCPU.lights[i].invProj =
                    m_shadowGlobalConstsCPU[i].invProj;

                // �ݻ�� ��鿡���� �׸��ڸ� �׸��� �ʹٸ� ���� �ݻ���Ѽ�
                // �־��ָ� �˴ϴ�.
            }
        }
    }

    void AppBase::RenderDepthOnly() {
        m_context->OMSetRenderTargets(0, NULL, // DepthOnly�� RTV ���ʿ�
            m_depthOnlyDSV.Get());
        m_context->ClearDepthStencilView(m_depthOnlyDSV.Get(), D3D11_CLEAR_DEPTH,
            1.0f, 0);
        AppBase::SetGlobalConsts(m_globalConstsGPU);
        for (const auto& model : m_basicList) {
            if (model->IsPostProcess() == false)
            {
                continue;
            }
            AppBase::SetPipelineState(model->GetDepthOnlyPSO());
            model->Render(m_context);
        }
        for (const auto& object : m_objectList) {
            if (object->GetBillboardModel() != nullptr)
            {
                continue;
            }
            AppBase::SetPipelineState(object->GetModel()->GetDepthOnlyPSO());
            object->Render(m_context);
        }
        for (const auto& object : m_objectList) {
            if (object->GetBillboardModel() == nullptr)
            {
                continue;
            }
            AppBase::SetPipelineState(object->GetModel()->GetDepthOnlyPSO());
            object->Render(m_context);
        }

        AppBase::SetPipelineState(Graphics::depthOnlyPSO);
        if (m_skybox)
            m_skybox->Render(m_context);
        if (m_mirror)
            m_mirror->Render(m_context);
    }

    void AppBase::RenderShadowMaps() {

        // ������ ���� �ٸ� ���̴����� SRV ����
        ID3D11ShaderResourceView* nulls[2] = { 0, 0 };
        m_context->PSSetShaderResources(15, 2, nulls);

        AppBase::SetShadowViewport(); // �׸��ڸ� �ػ�
        for (int i = 0; i < MAX_LIGHTS; i++) {
            if (m_globalConstsCPU.lights[i].type & LIGHT_SHADOW) {
                m_context->OMSetRenderTargets(0, NULL, // DepthOnly�� RTV ���ʿ�
                    m_shadowDSVs[i].Get());
                m_context->ClearDepthStencilView(m_shadowDSVs[i].Get(),
                    D3D11_CLEAR_DEPTH, 1.0f, 0);
                AppBase::SetGlobalConsts(m_shadowGlobalConstsGPU[i]);

                for (const auto& model : m_basicList) {
                    if (model->m_castShadow && model->m_isVisible) {
                        AppBase::SetPipelineState(model->GetDepthOnlyPSO());
                        model->Render(m_context);
                    }
                }
                for (const auto& object : m_objectList) {
                    if (object->GetBillboardModel() != nullptr)
                    {
                        continue;
                    }
                    shared_ptr<DModel> model = object->GetModel();
                    if (model->m_castShadow && model->m_isVisible) {
                        AppBase::SetPipelineState(model->GetDepthOnlyPSO());
                        model->Render(m_context);
                    }
                }
                for (const auto& object : m_objectList) {
                    if (object->GetBillboardModel() == nullptr)
                    {
                        continue;
                    }
                    shared_ptr<DModel> model = object->GetModel();
                    if (model->m_castShadow && model->m_isVisible) {
                        AppBase::SetPipelineState(model->GetDepthOnlyPSO());
                        model->Render(m_context);
                    }
                }

                if (m_mirror && m_mirror->m_castShadow)
                    m_mirror->Render(m_context);
            }
        }
    }

    void AppBase::RenderOpaqueObjects() {
        // �ٽ� ������ �ػ󵵷� �ǵ�����
        AppBase::SetMainViewport();

        // �ſ� 1. �ſ��� ���� ���� ��� �׸���
        const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_context->ClearRenderTargetView(m_floatRTV.Get(), clearColor);
        m_context->OMSetRenderTargets(1, m_floatRTV.GetAddressOf(),
            m_defaultDSV.Get());

        // �׸��ڸʵ鵵 ���� �ؽ���� ���Ŀ� �߰�
        // ����: ������ shadowDSV�� RenderTarget���� ������ �� ����
        vector<ID3D11ShaderResourceView*> shadowSRVs;
        for (int i = 0; i < MAX_LIGHTS; i++) {
            shadowSRVs.push_back(m_shadowSRVs[i].Get());
        }
        m_context->PSSetShaderResources(15, UINT(shadowSRVs.size()),
            shadowSRVs.data());
        m_context->ClearDepthStencilView(
            m_defaultDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        AppBase::SetGlobalConsts(m_globalConstsGPU);

        // ��ī�̹ڽ� �׸���
        // ������ ��ü�� �־ ���ǻ� �ٸ� ��ü�麸�� ���� �׷Ƚ��ϴ�.
        // ����ȭ�� �ϰ� �ʹٸ� ������ ��ü�鸸 ���� �������� �׸��� �˴ϴ�.
        AppBase::SetPipelineState(m_drawAsWire ? Graphics::skyboxWirePSO
            : Graphics::skyboxSolidPSO);
        m_skybox->Render(m_context);
        for (const auto& model : m_basicList) {

            AppBase::SetPipelineState(model->GetPSO(m_drawAsWire));
            model->Render(m_context);
        }
        for (const auto& object : m_objectList) {
            if (object->GetBillboardModel() != nullptr)
            {
                continue;
            }
            shared_ptr<DModel> model = object->GetModel();
            AppBase::SetPipelineState(model->GetPSO(m_drawAsWire));
            model->Render(m_context);
        }

        for (const auto& object : m_objectList) {
            if (object->GetBillboardModel() == nullptr)
            {
                continue;
            }
            shared_ptr<DModel> model = object->GetModel();
            AppBase::SetPipelineState(model->GetPSO(m_drawAsWire));
            model->Render(m_context);
        }
        // �ſ� �ݻ縦 �׸� �ʿ䰡 ������ ������ �ſ︸ �׸���
        if (m_mirrorAlpha == 1.0f && m_mirror) {
            AppBase::SetPipelineState(m_drawAsWire ? Graphics::defaultWirePSO
                : Graphics::defaultSolidPSO);
            m_mirror->Render(m_context);
        }

        // ��� ���� �׸���
        AppBase::SetPipelineState(Graphics::normalsPSO);
        for (auto& model : m_basicList) {
            if (model->m_drawNormals)
                model->RenderNormals(m_context);
        }

        AppBase::SetPipelineState(Graphics::boundingBoxPSO);
        if (AppBase::m_drawOBB) {
            for (auto& model : m_basicList) {
                model->RenderWireBoundingBox(m_context);
            }
            for (const auto& object : m_objectList) {
                shared_ptr<DModel> model = object->GetModel();
                model->RenderWireBoundingBox(m_context);
            }
        }
        if (AppBase::m_drawBS) {
            for (auto& model : m_basicList) {
                model->RenderWireBoundingSphere(m_context);
            }
            for (const auto& object : m_objectList) {
                shared_ptr<DModel> model = object->GetModel();
                model->RenderWireBoundingSphere(m_context);
            }
        }
    }

    void AppBase::RenderMirror() {

        if (m_mirrorAlpha < 1.0f && m_mirror) { // �ſ� �ݻ縦 �׷��� �ϴ� ��Ȳ

            // �ſ� 2. �ſ� ��ġ�� StencilBuffer�� 1�� ǥ��
            AppBase::SetPipelineState(Graphics::stencilMaskPSO);
            m_mirror->Render(m_context);

            // �ſ� 3. �ſ� ��ġ�� �ݻ�� ��ü���� ������
            AppBase::SetGlobalConsts(m_reflectGlobalConstsGPU);
            m_context->ClearDepthStencilView(m_defaultDSV.Get(), D3D11_CLEAR_DEPTH,
                1.0f, 0);

            for (auto& model : m_basicList) {
                AppBase::SetPipelineState(model->GetReflectPSO(m_drawAsWire));
                model->Render(m_context);
            }
            for (const auto& object : m_objectList) {
                if (object->GetBillboardModel() != nullptr)
                {
                    continue;
                }
                AppBase::SetPipelineState(object->GetModel()->GetReflectPSO(m_drawAsWire));
                object->Render(m_context);
            }
            for (const auto& object : m_objectList) {
                if (object->GetBillboardModel() == nullptr)
                {
                    continue;
                }
                AppBase::SetPipelineState(object->GetModel()->GetReflectPSO(m_drawAsWire));
                object->Render(m_context);
            }
            AppBase::SetPipelineState(m_drawAsWire
                ? Graphics::reflectSkyboxWirePSO
                : Graphics::reflectSkyboxSolidPSO);
            m_skybox->Render(m_context);

            // �ſ� 4. �ſ� ��ü�� ������ "Blend"�� �׸�
            AppBase::SetPipelineState(m_drawAsWire ? Graphics::mirrorBlendWirePSO
                : Graphics::mirrorBlendSolidPSO);
            AppBase::SetGlobalConsts(m_globalConstsGPU);
            m_mirror->Render(m_context);

        } // end of if (m_mirrorAlpha < 1.0f)
    }

    void AppBase::Render() {
        // Note : ��� �������� �������������� ��û�Ѵٸ�
        // ���� ��ٸ��鼭 �� ���� ������ �׳� ���� �����忡�� ���������Ѵ�.

        AppBase::SetMainViewport();

        // �������� ����ϴ� ���÷��� ����
        m_context->VSSetSamplers(0, UINT(Graphics::sampleStates.size()),
            Graphics::sampleStates.data());
        m_context->PSSetSamplers(0, UINT(Graphics::sampleStates.size()),
            Graphics::sampleStates.data());

        // �������� ����� �ؽ����: "Common.hlsli"���� register(t10)���� ����
        vector<ID3D11ShaderResourceView*> commonSRVs = {
            m_envSRV.Get(), m_specularSRV.Get(), m_irradianceSRV.Get(),
            m_brdfSRV.Get() };
        m_context->PSSetShaderResources(10, UINT(commonSRVs.size()),
            commonSRVs.data());

        RenderDepthOnly();

        RenderShadowMaps();

        RenderOpaqueObjects();

        RenderMirror();
    }


    void AppBase::OnMouseMove(int mouseX, int mouseY) {

        m_mouseX = mouseX;
        m_mouseY = mouseY;

        // ���콺 Ŀ���� ��ġ�� NDC�� ��ȯ
        // ���콺 Ŀ���� ���� ��� (0, 0), ���� �ϴ�(width-1, height-1)
        // NDC�� ���� �ϴ��� (-1, -1), ���� ���(1, 1)
        m_mouseNdcX = mouseX * 2.0f / m_screenWidth - 1.0f;
        m_mouseNdcY = -mouseY * 2.0f / m_screenHeight + 1.0f;

        // Ŀ���� ȭ�� ������ ������ ��� ���� ����
        // ���ӿ����� Ŭ������ ���� ���� �ֽ��ϴ�.
        m_mouseNdcX = std::clamp(m_mouseNdcX, -1.0f, 1.0f);
        m_mouseNdcY = std::clamp(m_mouseNdcY, -1.0f, 1.0f);

        // ī�޶� ���� ȸ��
        m_camera.UpdateMouse(m_mouseNdcX, m_mouseNdcY);
    }

    void AppBase::OnMouseClick(int mouseX, int mouseY) {

        m_mouseX = mouseX;
        m_mouseY = mouseY;

        m_mouseNdcX = mouseX * 2.0f / m_screenWidth - 1.0f;
        m_mouseNdcY = -mouseY * 2.0f / m_screenHeight + 1.0f;
    }

    LRESULT AppBase::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return true;

        switch (msg) {
        case WM_SIZE:
            // ȭ�� �ػ󵵰� �ٲ�� SwapChain�� �ٽ� ����
            if (m_swapChain) {

                m_screenWidth = int(LOWORD(lParam));
                m_screenHeight = int(HIWORD(lParam));

                // �����찡 Minimize ��忡���� screenWidth/Height�� 0
                if (m_screenWidth && m_screenHeight) {

                    cout << "Resize SwapChain to " << m_screenWidth << " "
                        << m_screenHeight << endl;

                    m_backBufferRTV.Reset();
                    m_swapChain->ResizeBuffers(
                        0,                    // ���� ���� ����
                        (UINT)LOWORD(lParam), // �ػ� ����
                        (UINT)HIWORD(lParam),
                        DXGI_FORMAT_UNKNOWN, // ���� ���� ����
                        0);
                    CreateBuffers();
                    SetMainViewport();
                    m_camera.SetAspectRatio(this->GetAspectRatio());

                    m_postProcess.Initialize(
                        m_device, m_context, { m_postEffectsSRV, m_prevSRV },
                        { m_backBufferRTV }, m_screenWidth, m_screenHeight, 4);
                }
            }
            break;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        case WM_MOUSEMOVE:
            OnMouseMove(LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_LBUTTONDOWN:
            if (!m_leftButton) {
                m_dragStartFlag = true; // �巡�׸� ���� �����ϴ��� Ȯ��
            }
            m_leftButton = true;
            OnMouseClick(LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_LBUTTONUP:
            m_leftButton = false;
            break;
        case WM_RBUTTONDOWN:
            if (!m_rightButton) {
                m_dragStartFlag = true; // �巡�׸� ���� �����ϴ��� Ȯ��
            }
            m_rightButton = true;
            break;
        case WM_RBUTTONUP:
            m_rightButton = false;
            break;
        case WM_KEYDOWN:
            if (m_activateActor != nullptr)
            {
                if (m_activateActor->MsgProc(wParam, true))
                {
                    return true;
                }
            }
            m_keyPressed[wParam] = true;
            if (wParam == VK_ESCAPE) { // ESCŰ ����
                DestroyWindow(hwnd);
            }
            if (wParam == VK_SPACE) {
                m_lightRotate = !m_lightRotate;
            }
            break;
        case WM_KEYUP:
            if (m_activateActor != nullptr)
            {
                if (m_activateActor->MsgProc(wParam, false))
                {
                    return true;
                }
            }
            if (wParam == 'F') { // fŰ ����Ī ����
                m_camera.m_useFirstPersonView = !m_camera.m_useFirstPersonView;
            }
            if (wParam == 'C') { // cŰ ȭ�� ĸ��
                ComPtr<ID3D11Texture2D> backBuffer;
                m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
                D3D11Utils::WriteToPngFile(m_device, m_context, backBuffer,
                    "captured.png");
            }
            if (wParam == 'P') { // �ִϸ��̼� �Ͻ������� �� ���
                m_pauseAnimation = !m_pauseAnimation;
            }
            if (wParam == 'Z') { // ī�޶� ���� ȭ�鿡 ���
                m_camera.PrintView();
            }

            m_keyPressed[wParam] = false;
            break;
        case WM_MOUSEWHEEL:
            m_wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        }

        return ::DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void AppBase::PostRender() {

        // Resolve MSAA texture
        m_context->ResolveSubresource(m_resolvedBuffer.Get(), 0,
            m_floatBuffer.Get(), 0,
            DXGI_FORMAT_R16G16B16A16_FLOAT);

        // PostEffects (m_globalConstsGPU ���)
        AppBase::SetMainViewport();
        AppBase::SetPipelineState(Graphics::postEffectsPSO);
        AppBase::SetGlobalConsts(m_globalConstsGPU);
        vector<ID3D11ShaderResourceView*> postEffectsSRVs = { m_resolvedSRV.Get(),
                                                              m_depthOnlySRV.Get() };
        m_context->PSSetShaderResources(20, // ����: Startslop 20
            UINT(postEffectsSRVs.size()),
            postEffectsSRVs.data());
        m_context->OMSetRenderTargets(1, m_postEffectsRTV.GetAddressOf(), NULL);
        m_context->PSSetConstantBuffers(5, // register(b5)
            1, m_postEffectsConstsGPU.GetAddressOf());
        m_screenSquare->Render(m_context);

        ID3D11ShaderResourceView* nulls[2] = { 0, 0 };
        m_context->PSSetShaderResources(20, 2, nulls);

        // ��ó�� (��� ���� ���� �̹��� ó��)
        AppBase::SetPipelineState(Graphics::postProcessingPSO);
        m_postProcess.Render(m_context);

        ComPtr<ID3D11Texture2D> backBuffer;
        ThrowIfFailed(
            m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));
        m_context->CopyResource(
            m_prevBuffer.Get(),
            backBuffer.Get()); // ��� �� ȿ���� ���� ������ ��� ����
    }

    void AppBase::InitCubemaps(wstring basePath, wstring envFilename,
        wstring specularFilename, wstring irradianceFilename,
        wstring brdfFilename) {

        // BRDF LookUp Table�� CubeMap�� �ƴ϶� 2D �ؽ��� �Դϴ�.
        D3D11Utils::CreateDDSTexture(m_device, (basePath + envFilename).c_str(),
            true, m_envSRV);
        D3D11Utils::CreateDDSTexture(
            m_device, (basePath + specularFilename).c_str(), true, m_specularSRV);
        D3D11Utils::CreateDDSTexture(m_device,
            (basePath + irradianceFilename).c_str(), true,
            m_irradianceSRV);
        D3D11Utils::CreateDDSTexture(m_device, (basePath + brdfFilename).c_str(),
            false, m_brdfSRV);
    }

    // ���� ��ü���� ���������� ����ϴ� Const ������Ʈ
    void AppBase::UpdateGlobalConstants(const float& dt, const Vector3& eyeWorld,
        const Matrix& viewRow,
        const Matrix& projRow, const Matrix& refl) {

        m_globalConstsCPU.globalTime += dt;
        m_globalConstsCPU.eyeWorld = eyeWorld;
        m_globalConstsCPU.view = viewRow.Transpose();
        m_globalConstsCPU.proj = projRow.Transpose();
        m_globalConstsCPU.invProj = projRow.Invert().Transpose();
        m_globalConstsCPU.viewProj = (viewRow * projRow).Transpose();
        m_globalConstsCPU.invView = viewRow.Invert().Transpose();

        // �׸��� �������� ���
        m_globalConstsCPU.invViewProj = m_globalConstsCPU.viewProj.Invert();

        m_reflectGlobalConstsCPU = m_globalConstsCPU;
        memcpy(&m_reflectGlobalConstsCPU, &m_globalConstsCPU,
            sizeof(m_globalConstsCPU));
        m_reflectGlobalConstsCPU.view = (refl * viewRow).Transpose();
        m_reflectGlobalConstsCPU.viewProj = (refl * viewRow * projRow).Transpose();
        // �׸��� �������� ��� (TODO: ������ ��ġ�� �ݻ��Ų �Ŀ� ����ؾ� ��)
        m_reflectGlobalConstsCPU.invViewProj =
            m_reflectGlobalConstsCPU.viewProj.Invert();

        D3D11Utils::UpdateBuffer(m_context, m_globalConstsCPU, m_globalConstsGPU);
        D3D11Utils::UpdateBuffer(m_context, m_reflectGlobalConstsCPU,
            m_reflectGlobalConstsGPU);
    }

    void AppBase::SetGlobalConsts(ComPtr<ID3D11Buffer>& globalConstsGPU) {
        // ���̴��� �ϰ��� ���� cbuffer GlobalConstants : register(b0)
        m_context->VSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
        m_context->PSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
        m_context->GSSetConstantBuffers(0, 1, globalConstsGPU.GetAddressOf());
    }

    void AppBase::CreateDepthBuffers() {

        D3D12_RESOURCE_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = m_screenWidth;
        desc.Height = m_screenHeight;
        desc.MipLevels = 1;
        desc.DepthOrArraySize = 1;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;


        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (m_useMSAA && m_numQualityLevels) {
            desc.SampleDesc.Count = 4;
            desc.SampleDesc.Quality = m_numQualityLevels - 1;
        }
        else {
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
        }
        ComPtr<ID3D12Resource> depthStencilBuffer;
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&depthStencilBuffer)));

        //ThrowIfFailed(m_device->CreateDepthStencilView(
          //  depthStencilBuffer.Get(), NULL, m_defaultDSV.GetAddressOf()));

        //TODO. �ٽ� �۾� Ȯ��
    }

    void AppBase::SetPipelineState(const GraphicsPSO& pso) {

        m_context->VSSetShader(pso.m_vertexShader.Get(), 0, 0);
        m_context->PSSetShader(pso.m_pixelShader.Get(), 0, 0);
        m_context->HSSetShader(pso.m_hullShader.Get(), 0, 0);
        m_context->DSSetShader(pso.m_domainShader.Get(), 0, 0);
        m_context->GSSetShader(pso.m_geometryShader.Get(), 0, 0);
        m_context->CSSetShader(NULL, 0, 0);
        m_context->IASetInputLayout(pso.m_inputLayout.Get());
        m_context->RSSetState(pso.m_rasterizerState.Get());
        m_context->OMSetBlendState(pso.m_blendState.Get(), pso.m_blendFactor,
            0xffffffff);
        m_context->OMSetDepthStencilState(pso.m_depthStencilState.Get(),
            pso.m_stencilRef);
        m_context->IASetPrimitiveTopology(pso.m_primitiveTopology);
    }

    void AppBase::SetPipelineState(const ComputePSO& pso) {
        m_context->VSSetShader(NULL, 0, 0);
        m_context->PSSetShader(NULL, 0, 0);
        m_context->HSSetShader(NULL, 0, 0);
        m_context->DSSetShader(NULL, 0, 0);
        m_context->GSSetShader(NULL, 0, 0);
        m_context->CSSetShader(pso.m_computeShader.Get(), 0, 0);
    }

    shared_ptr<Model> AppBase::PickClosest(const Ray& pickingRay, float& minDist) {
        minDist = 1e5f;
        shared_ptr<Model> minModel = nullptr;
        for (auto& model : m_basicList) {
            float dist = 0.0f;
            if (model->m_isPickable &&
                pickingRay.Intersects(model->m_boundingSphere, dist) &&
                dist < minDist) {
                minModel = model;
                minDist = dist;
            }
        }
        return minModel;
    }

    void AppBase::SelectClosestActor(const Ray& pickingRay, float& minDist) {
        minDist = 1e5f;
        for (auto& object : m_objectList) {
            std::shared_ptr<Actor> actor = std::dynamic_pointer_cast<Actor>(object);
            if (actor == nullptr)
            {
                continue;
            }
            float dist = 0.0f;
            std::shared_ptr<DModel> model = object->GetModel();

            if (object->IsPickable() &&
                pickingRay.Intersects(model->m_boundingSphere, dist) &&
                dist < minDist) {
                m_activateActor = actor;
                minDist = dist;
            }
        }
    }

    void AppBase::ProcessMouseControl() {

        static shared_ptr<Model> activeModel = nullptr;
        static float prevRatio = 0.0f;
        static Vector3 prevPos(0.0f);
        static Vector3 prevVector(0.0f);

        // ������ ȸ���� �̵� �ʱ�ȭ
        Quaternion q =
            Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), 0.0f);
        Vector3 dragTranslation(0.0f);
        Vector3 pickPoint(0.0f);
        float dist = 0.0f;

        // ����ڰ� �� ��ư �� �ϳ��� �����ٰ� �����մϴ�.
        if (m_leftButton || m_rightButton) {
            const Matrix viewRow = m_camera.GetViewRow();
            const Matrix projRow = m_camera.GetProjRow();
            const Vector3 ndcNear = Vector3(m_mouseNdcX, m_mouseNdcY, 0.0f);
            const Vector3 ndcFar = Vector3(m_mouseNdcX, m_mouseNdcY, 1.0f);
            const Matrix invProjView = (viewRow * projRow).Invert();
            const Vector3 worldNear = Vector3::Transform(ndcNear, invProjView);
            const Vector3 worldFar = Vector3::Transform(ndcFar, invProjView);
            Vector3 dir = worldFar - worldNear;
            dir.Normalize();
            const Ray curRay = SimpleMath::Ray(worldNear, dir);

            SelectClosestActor(curRay, dist);

            // ���� �����ӿ��� �ƹ� ��ü�� ���õ��� �ʾ��� ��쿡�� ���� ����
            if (!activeModel) {
                auto newModel = AppBase::PickClosest(curRay, dist);
                if (newModel) {
                    cout << "Newly selected model: " << newModel->m_name << endl;
                    activeModel = newModel;
                    m_pickedModel = newModel; // GUI ���ۿ� ������
                    pickPoint = curRay.position + dist * curRay.direction;
                    if (m_leftButton) { // ���� ��ư ȸ�� �غ�
                        prevVector =
                            pickPoint - activeModel->m_boundingSphere.Center;
                        prevVector.Normalize();
                    }
                    else { // ������ ��ư �̵� �غ�
                        m_dragStartFlag = false;
                        prevRatio = dist / (worldFar - worldNear).Length();
                        prevPos = pickPoint;
                    }
                }
            }
            else {                // �̹� ���õ� ��ü�� �־��� ���
                if (m_leftButton) { // ���� ��ư���� ��� ȸ��
                    if (curRay.Intersects(activeModel->m_boundingSphere, dist)) {
                        pickPoint = curRay.position + dist * curRay.direction;
                    }
                    else {
                        // �ٿ�� ���Ǿ ���� ����� ���� ã��
                        Vector3 c =
                            activeModel->m_boundingSphere.Center - worldNear;
                        Vector3 centerToRay = dir.Dot(c) * dir - c;
                        pickPoint =
                            c +
                            centerToRay *
                            std::clamp(activeModel->m_boundingSphere.Radius /
                                centerToRay.Length(),
                                0.0f, 1.0f);
                        pickPoint += worldNear;
                    }

                    Vector3 currentVector =
                        pickPoint - activeModel->m_boundingSphere.Center;
                    currentVector.Normalize();
                    float theta = acos(prevVector.Dot(currentVector));
                    if (theta > 3.141592f / 180.0f * 3.0f) {
                        Vector3 axis = prevVector.Cross(currentVector);
                        axis.Normalize();
                        q = SimpleMath::Quaternion::CreateFromAxisAngle(axis,
                            theta);
                        prevVector = currentVector;
                    }

                }
                else { // ������ ��ư���� ��� �̵�
                    Vector3 newPos = worldNear + prevRatio * (worldFar - worldNear);
                    if ((newPos - prevPos).Length() > 1e-3) {
                        dragTranslation = newPos - prevPos;
                        prevPos = newPos;
                    }
                    pickPoint = newPos; // Cursor sphere �׷��� ��ġ
                }
            }
        }
        else {
            // ��ư���� ���� ���� ��쿡�� ������ ���� nullptr�� ����
            activeModel = nullptr;

            // m_pickedModel�� GUI ������ ���� ���콺���� ���� ���� nullptr��
            // �������� ����
        }

        // Cursor sphere �׸���
        if (activeModel) {
            Vector3 translation = activeModel->m_worldRow.Translation();
            activeModel->m_worldRow.Translation(Vector3(0.0f));
            activeModel->UpdateWorldRow(
                activeModel->m_worldRow * Matrix::CreateFromQuaternion(q) *
                Matrix::CreateTranslation(dragTranslation + translation));
            activeModel->m_boundingSphere.Center =
                activeModel->m_worldRow.Translation();

            // �浹 ������ ���� �� �׸���
            //m_cursorSphere->m_isVisible = true;
            //m_cursorSphere->UpdateWorldRow(Matrix::CreateTranslation(pickPoint));
        }
        else {
            //m_cursorSphere->m_isVisible = false;
        }
    }

    bool AppBase::InitMainWindow() {

        WNDCLASSEX wc = { sizeof(WNDCLASSEX),
                         CS_CLASSDC,
                         WndProc,
                         0L,
                         0L,
                         GetModuleHandle(NULL),
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         L"Daerim'sGTA", // lpszClassName, L-string
                         NULL };

        if (!RegisterClassEx(&wc)) {
            cout << "RegisterClassEx() failed." << endl;
            return false;
        }

        RECT wr = { 0, 0, m_screenWidth, m_screenHeight };
        AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);
        m_mainWindow = CreateWindow(wc.lpszClassName, L"Daerim'sGTA",
            WS_OVERLAPPEDWINDOW,
            100, // ������ ���� ����� x ��ǥ
            100, // ������ ���� ����� y ��ǥ
            wr.right - wr.left, // ������ ���� ���� �ػ�
            wr.bottom - wr.top, // ������ ���� ���� �ػ�
            NULL, NULL, wc.hInstance, NULL);

        if (!m_mainWindow) {
            cout << "CreateWindow() failed." << endl;
            return false;
        }

        ShowWindow(m_mainWindow, SW_SHOWDEFAULT);
        UpdateWindow(m_mainWindow);

        return true;
    }

    bool AppBase::InitDirect3D() {
        UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();

                // Enable additional debug layers.
                dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
#endif
        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter,false);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)));

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

        DXGI_SWAP_CHAIN_DESC1 sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.Width = m_screenWidth;
        sd.Height = m_screenHeight;
        sd.Format = m_backBufferFormat;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT
                | DXGI_USAGE_UNORDERED_ACCESS;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sd.SampleDesc.Count = 1; // _FLIP_�� MSAA ������
        sd.SampleDesc.Quality = 0;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenSwapchainDesc;
        ZeroMemory(&fullScreenSwapchainDesc, sizeof(fullScreenSwapchainDesc));
        fullScreenSwapchainDesc.RefreshRate.Numerator = 60;
        fullScreenSwapchainDesc.RefreshRate.Denominator = 1;
        fullScreenSwapchainDesc.Windowed = TRUE;

        //sd.OutputWindow = m_mainWindow;
        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
            m_mainWindow,
            &sd,
            &fullScreenSwapchainDesc,
            nullptr,
            &swapChain
        ));
        ThrowIfFailed(swapChain.As(&m_swapChain));

        ID3D12DebugDevice1* debugDevice = nullptr;
        if (SUCCEEDED(m_device->QueryInterface(__uuidof(ID3D12DebugDevice1), reinterpret_cast<void**>(&debugDevice)))) {
            debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
            debugDevice->Release();
        }
        // This sample does not support fullscreen transitions.
        //ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

        Graphics::InitCommonStates(m_device);

        CreateBuffers();

        SetMainViewport();

        // �������� ���̴� ConstBuffers
        D3D11Utils::CreateConstBuffer(m_device, m_globalConstsCPU,
            m_globalConstsGPU);
        D3D11Utils::CreateConstBuffer(m_device, m_reflectGlobalConstsCPU,
            m_reflectGlobalConstsGPU);

        // �׸��ڸ� �������� �� ����� GlobalConsts�� ���� ����
        for (int i = 0; i < MAX_LIGHTS; i++) {
            D3D11Utils::CreateConstBuffer(m_device, m_shadowGlobalConstsCPU[i],
                m_shadowGlobalConstsGPU[i]);
        }

        // ��ó�� ȿ���� ConstBuffer
        D3D11Utils::CreateConstBuffer(m_device, m_postEffectsConstsCPU,
            m_postEffectsConstsGPU);

        return true;
    }

    bool AppBase::InitGUI() {

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.DisplaySize = ImVec2(float(m_screenWidth), float(m_screenHeight));
        ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        if (!ImGui_ImplDX11_Init(m_device.Get(), m_context.Get())) {
            return false;
        }

        if (!ImGui_ImplWin32_Init(m_mainWindow)) {
            return false;
        }

        return true;
    }

    void AppBase::SetMainViewport() {

        // ����Ʈ �ʱ�ȭ
        ZeroMemory(&m_screenViewport, sizeof(D3D12_VIEWPORT));
        m_screenViewport.TopLeftX = 0.0f;
        m_screenViewport.TopLeftY = 0.0f;
        m_screenViewport.Width = static_cast<float>(m_screenWidth);
        m_screenViewport.Height = static_cast<float>(m_screenHeight);
        m_screenViewport.MinDepth = 0.0f;
        m_screenViewport.MaxDepth = 1.0f;

        ZeroMemory(&m_scissorRect, sizeof(D3D12_RECT));
        m_scissorRect.left = 0;
        m_scissorRect.top = 0;
        m_scissorRect.right = m_screenWidth;
        m_scissorRect.bottom = m_screenHeight;

        // ��� ��Ͽ� ����Ʈ�� ��ũ���� ��Ʈ ����
        // TODO. ������ ������ Set���ֱ�
        /*
        m_commandList->RSSetViewports(1, &m_screenViewport);
        m_commandList->RSSetScissorRects(1, &m_scissorRect);
        */

    }

    void AppBase::SetShadowViewport() {
        //TODO �ϴ� �굵 Pass
        // Set the viewport
        D3D11_VIEWPORT shadowViewport;
        ZeroMemory(&shadowViewport, sizeof(D3D11_VIEWPORT));
        shadowViewport.TopLeftX = 0;
        shadowViewport.TopLeftY = 0;
        shadowViewport.Width = float(m_shadowWidth);
        shadowViewport.Height = float(m_shadowHeight);
        shadowViewport.MinDepth = 0.0f;
        shadowViewport.MaxDepth = 1.0f;

        //m_context->RSSetViewports(1, &shadowViewport);
    }

    void AppBase::ComputeShaderBarrier() {

        // ����: BreadcrumbsDirectX-Graphics-Samples (DX12)
        // void CommandContext::InsertUAVBarrier(GpuResource & Resource, bool
        // FlushImmediate)

        // �����鿡�� �ִ� ����ϴ� SRV, UAV ������ 6��
        ID3D11ShaderResourceView* nullSRV[6] = {
            0,
        };
        m_context->CSSetShaderResources(0, 6, nullSRV);
        ID3D11UnorderedAccessView* nullUAV[6] = {
            0,
        };
        m_context->CSSetUnorderedAccessViews(0, 6, nullUAV, NULL);
    }

    void AppBase::KillObjects()
    {
        for (int i = m_objectList.size() - 1; i >= 0; i--)
        {
            if (m_objectList[i]->IsPendingKill())
            {
                DaerimsEngineBase::GetInstance().RemoveRigidBody(m_objectList[i]->GetPhysicsBody());
                m_objectList.erase(m_objectList.begin() + i);
            }
        }
    }

    void AppBase::CreateBuffers() {

        // ������ȭ -> float/depthBuffer(MSAA) -> resolved -> backBuffer

        // Create a RTV for each frame.
        for (UINT n = 0; n < 2; n++)//TODO. FrameCout 2��
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            Graphics::RegisterRtvHeap(m_device, m_renderTargets[n], nullptr, m_renderTargetViews[n]);
        }

        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels = {};
        msQualityLevels.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // ���� ����
        msQualityLevels.SampleCount = 4; // ���� �� ����

        // ��Ƽ ���ø� ǰ�� ���� ���� ��������
        ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));

        // msQualityLevels.NumQualityLevels�� ǰ�� ���� ���� ����
        m_numQualityLevels = msQualityLevels.NumQualityLevels;

        D3D12_RESOURCE_DESC desc = m_renderTargets[0]->GetDesc();
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        desc.MipLevels = desc.DepthOrArraySize = 1;

        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (m_useMSAA && m_numQualityLevels) {
            desc.SampleDesc.Count = 4;
            desc.SampleDesc.Quality = m_numQualityLevels - 1;
        }
        else {
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
        }
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            & desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_floatBuffer)));

        Graphics::RegisterRtvHeap(m_device, m_floatBuffer, nullptr, m_floatRTV);

        CreateDepthBuffers();
    }


    void GetHardwareAdapter(
        IDXGIFactory1* pFactory,
        IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;

        ComPtr<IDXGIFactory6> factory6;
        if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
        {
            for (
                UINT adapterIndex = 0;
                SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                    IID_PPV_ARGS(&adapter)));
                ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    // If you want a software adapter, pass in "/warp" on the command line.
                    continue;
                }

                // Check to see whether the adapter supports Direct3D 12, but don't create the
                // actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
        }

        if (adapter.Get() == nullptr)
        {
            for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    // If you want a software adapter, pass in "/warp" on the command line.
                    continue;
                }

                // Check to see whether the adapter supports Direct3D 12, but don't create the
                // actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
        }

        *ppAdapter = adapter.Detach();
    }
} // namespace hlab
#include "DAppBase.h"
#include <algorithm>
#include <directxtk/SimpleMath.h>

#include "GraphicsCommonD3D12.h"
#include "Actor.h"
#include "DaerimsEngineBase.h"
#include "Object.h"
namespace hlab {
void GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter = false);
int DAppBase::Run() {

    // Main message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();

            ImGui::NewFrame();
            ImGui::Begin("Scene Control");

            // ImGui�� �������ִ� Framerate ���
            ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

            ImGui::End();
            ImGui::Render();
            Update(ImGui::GetIO().DeltaTime);

            Render();


            // Example�� Render()���� RT ������ ������ �ʾ��� ��쿡��
            // �� ���ۿ� GUI�� �׸������� RT ����
            // ��) Render()���� ComputeShader�� ���
            // TODO �׸��� �۾� ������
            SetMainViewport();

            m_context->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(),
                NULL);

            // GUI ������
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Graphics::commandList.Get());

            // GUI ������ �Ŀ� Present() ȣ��
            m_swapChain->Present(1, 0);

            threadPool.SetUsingMainThreadUsingRendering(false);
            threadPool.cv_render_job_q_.notify_one();
        }
    }

    return 0;
}

void DAppBase::Init()
{
    LoadPipeline();
    LoadAssets();
    //LoadContexts();
}

void DAppBase::Update(float dt)
{
    // Camera
    m_camera.UpdateKeyboard(dt, m_keyPressed);
    // Light
    UpdateLights(dt);

    const Vector3 eyeWorld = m_camera.GetEyePos();
    const Matrix reflectRow = Matrix::Identity;//Matrix::CreateReflection(m_mirrorPlane);
    const Matrix viewRow = m_camera.GetViewRow();
    const Matrix projRow = m_camera.GetProjRow();
    UpdateGlobalConstants(dt, eyeWorld, viewRow, projRow, reflectRow);

    for (int i = 0; i < m_objectList.size(); i++)
    {
        m_objectList[i]->Tick(dt);
        // DNote : ���� DModel2 ������ �� ����� ������Ʈ�ϱ�
        //m_objectList[i]->GetModel()->UpdateConstantBuffers(m_context);
    }
    // Get current GPU progress against submitted workload. Resources still scheduled 
    // for GPU execution cannot be modified or else undefined behavior will result.
    const UINT64 lastCompletedFence = m_fence->GetCompletedValue();



    //m_pCurrentFrameResource->WriteConstantBuffers(&m_viewport, &m_camera, m_lightCameras, m_lights);
}

void DAppBase::UpdateLights(float dt) {

    // ȸ���ϴ� lights[1] ������Ʈ
    static Vector3 lightDev = Vector3(1.0f, 0.0f, 0.0f);

    m_globalConstBuffer.GetCpu().lights[1].position = Vector3(0.0f, 1.1f, 2.0f) + lightDev;
    Vector3 focusPosition = Vector3(0.0f, -0.5f, 1.7f);
    m_globalConstBuffer.GetCpu().lights[1].direction =
        focusPosition - m_globalConstBuffer.GetCpu().lights[1].position;
    m_globalConstBuffer.GetCpu().lights[1].direction.Normalize();
}

void DAppBase::UpdateGlobalConstants(const float& dt, const Vector3& eyeWorld,
    const Matrix& viewRow,
    const Matrix& projRow, const Matrix& refl) {

    m_globalConstBuffer.GetCpu().globalTime += dt;
    m_globalConstBuffer.GetCpu().eyeWorld = eyeWorld;
    m_globalConstBuffer.GetCpu().view = viewRow.Transpose();
    m_globalConstBuffer.GetCpu().proj = projRow.Transpose();
    m_globalConstBuffer.GetCpu().invProj = projRow.Invert().Transpose();
    m_globalConstBuffer.GetCpu().viewProj = (viewRow * projRow).Transpose();
    m_globalConstBuffer.GetCpu().invView = viewRow.Invert().Transpose();

    // �׸��� �������� ���
    m_globalConstBuffer.GetCpu().invViewProj = m_globalConstBuffer.GetCpu().viewProj.Invert();
    m_globalConstBuffer.Upload();
}
void DAppBase::LoadPipeline()
{
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
    GetHardwareAdapter(factory.Get(), &hardwareAdapter, true);
    // TODO Level �ٽ� Ȯ��
    ThrowIfFailed(D3D12CreateDevice(
        hardwareAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device)
    ));
    

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_screenWidth;
    swapChainDesc.Height = m_screenHeight;
    swapChainDesc.Format = m_backBufferFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        m_mainWindow,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(m_mainWindow, DXGI_MWA_NO_ALT_ENTER));
    // DNode : farmeIndex�� ���� �׷��ߵɰ�? �������� ���̾ƴ�
    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create Shader
    // Create descriptor heaps.

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

    m_screenViewport.TopLeftX = 0.0f;
    m_screenViewport.TopLeftY = 0.0f;
    m_screenViewport.Width = static_cast<float>(m_screenWidth);
    m_screenViewport.Height = static_cast<float>(m_screenHeight);

    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = m_screenWidth;
    m_scissorRect.bottom = m_screenHeight;
}

void DAppBase::Render() {
    // Note : ��� �������� �������������� ��û�Ѵٸ�
    // ���� ��ٸ��鼭 �� ���� ������ �׳� ���� �����忡�� ���������Ѵ�.

    SetMainViewport();

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

void DAppBase::LoadAssets()
{
    DGraphics::InitCommonStates(m_device);

    // Create temporary command list for initial GPU setup.
    ComPtr<ID3D12GraphicsCommandList> commandList;
    // DNote pipelineState nullptr ���� Ȯ��
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    // DNote �̰� ������ ��� rtvHandle �����ؾߵ�
    for (UINT i = 0; i < FrameCount; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
        DGraphics::RegisterRtvHeap(m_device, m_renderTargets[i], nullptr, rtvHandle);
    }


    // �������� ���̰��ִ� ���ҽ��� �ε�
    m_globalConstBuffer.Initialize(m_device);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue++;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        /*

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.

        // Signal and increment the fence value.
        const UINT64 fenceToWaitFor = m_fenceValue;
        // DNote : ������� ��ɵ� ���� fence �ɾ��ְ�
        ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor));
        m_fenceValue++;

        // DNote : fence �ɾ��ذ� �̺�Ʈ�� �ɾ ��ٷ���
        ThrowIfFailed(m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
        */
    }
}

void DAppBase::SetMainViewport() {

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

void DAppBase::SetCommonPipelineState(ID3D12GraphicsCommandList* pCommandList)
{
    pCommandList->SetGraphicsRootSignature(DGraphics::defaultRootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { DGraphics::srvCbvHeap.Get(),  DGraphics::rtvHeap.Get(),
         DGraphics::samplerHeap.Get()};
    pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    pCommandList->RSSetViewports(1, &m_screenViewport);
    pCommandList->RSSetScissorRects(1, &m_scissorRect);
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->SetGraphicsRootDescriptorTable(0, m_globalConstBuffer.m_gpuHandle);
    pCommandList->OMSetStencilRef(0);// DNote : Ȯ�� �ʿ�
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
}
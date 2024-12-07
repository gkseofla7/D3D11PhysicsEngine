#include "DAppBase.h"
#include <algorithm>
#include <directxtk/SimpleMath.h>

#include "GraphicsCommonD3D12.h"
#include "Actor.h"
#include "DaerimsEngineBase.h"
#include "Object.h"
#include "SwapChain.h"
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

            // ImGui가 측정해주는 Framerate 출력
            ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

            ImGui::End();
            ImGui::Render();
            Update(ImGui::GetIO().DeltaTime);

            Render();


            // Example의 Render()에서 RT 설정을 해주지 않았을 경우에도
            // 백 버퍼에 GUI를 그리기위해 RT 설정
            // 예) Render()에서 ComputeShader만 사용
            // TODO 그리기 작업 마무리
            SetMainViewport();


            // GUI 렌더링
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Graphics::commandList.Get());

            // GUI 렌더링 후에 Present() 호출
            //m_swapChain->Present(1, 0);

            //threadPool.SetUsingMainThreadUsingRendering(false);
            //threadPool.cv_render_job_q_.notify_one();
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
        // DNote : 여기 DModel2 수정후 꼭 제대로 업데이트하기
        //m_objectList[i]->GetModel()->UpdateConstantBuffers(m_context);
    }
    // Get current GPU progress against submitted workload. Resources still scheduled 
    // for GPU execution cannot be modified or else undefined behavior will result.
    const UINT64 lastCompletedFence = m_fence->GetCompletedValue();



    //m_pCurrentFrameResource->WriteConstantBuffers(&m_viewport, &m_camera, m_lightCameras, m_lights);
}

void DAppBase::UpdateLights(float dt) {

    // 회전하는 lights[1] 업데이트
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

    // 그림자 렌더링에 사용
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
    // TODO Level 다시 확인
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

    m_swapChain = make_shared<SwapChain>();
    m_swapChain->Init(m_mainWindow, factory, m_commandQueue, m_screenWidth, m_screenHeight);

    // Create Shader
    // Create descriptor heaps.

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
    m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
    m_commandList->Close();


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
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), nullptr);


        // 공통으로 사용할 텍스춰들: "Common.hlsli"에서 register(t10)부터 시작
    m_descriptorHeap.SetSRV(m_device, m_envSRV, SRV_REGISTER::t10);
    m_descriptorHeap.SetSRV(m_device, m_specularSRV, SRV_REGISTER::t11);
    m_descriptorHeap.SetSRV(m_device, m_irradianceSRV, SRV_REGISTER::t12);
    m_descriptorHeap.SetSRV(m_device, m_brdfSRV, SRV_REGISTER::t13);
    m_descriptorHeap.CommitTable(m_commandList);

    SetMainViewport();


    // Render


    {
        INT8 backIndex = m_swapChain->GetCurrentBackBufferIndex();
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTargets[backIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, // 화면 출력
            D3D12_RESOURCE_STATE_PRESENT); // 외주 결과물
    }
    m_commandList->ResourceBarrier(1, &barrier);
    m_commandList->Close();

    // 커맨드 리스트 수행
    ID3D12CommandList* cmdListArr[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);
    m_swapChain->Present(0, 0);

    WaitSync();

    m_swapChain->SwapIndex();
}

void DAppBase::LoadAssets()
{
    DGraphics::InitCommonStates(m_device);

    m_descriptorHeap.Init(m_device);
    // Create temporary command list for initial GPU setup.
    ComPtr<ID3D12GraphicsCommandList> commandList;
    // DNote pipelineState nullptr 셋팅 확인
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    // DNote 이거 쓰려면 어디에 rtvHandle 저장해야됨
    for (UINT i = 0; i < FrameCount; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
        DGraphics::RegisterRtvHeap(m_device, m_renderTargets[i], nullptr, rtvHandle);
    }


    // 공용으로 쓰이고있는 리소스들 로드
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
        // DNote : 현재까지 명령들 이제 fence 걸어주고
        ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor));
        m_fenceValue++;

        // DNote : fence 걸어준걸 이벤트에 걸어서 기다려줌
        ThrowIfFailed(m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
        */
    }
}

void DAppBase::InitCubemaps(wstring basePath, wstring envFilename,
    wstring specularFilename, wstring irradianceFilename,
    wstring brdfFilename)
{
    // BRDF LookUp Table은 CubeMap이 아니라 2D 텍스춰 입니다.
    D3D12Utils::CreateDDSTexture(m_device, m_commandQueue,(basePath + envFilename).c_str(),
        true, m_envSRV);
    D3D12Utils::CreateDDSTexture(
        m_device, m_commandQueue,(basePath + specularFilename).c_str(), true, m_specularSRV);
    D3D12Utils::CreateDDSTexture(m_device, m_commandQueue,
        (basePath + irradianceFilename).c_str(), true,
        m_irradianceSRV);
    D3D12Utils::CreateDDSTexture(m_device, m_commandQueue, (basePath + brdfFilename).c_str(),
        false, m_brdfSRV);
}

void DAppBase::SetMainViewport() {

    // 뷰포트 초기화
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

    // 명령 목록에 뷰포트와 스크리서 렉트 설정
    // TODO. 렌더링 직전에 Set해주기
    /*
    m_commandList->RSSetViewports(1, &m_screenViewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);
    */

}

void DAppBase::SetCommonPipelineState(ID3D12GraphicsCommandList* pCommandList)
{
    pCommandList->SetGraphicsRootSignature(DGraphics::defaultRootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { DGraphics::srvCbvHeap.Get(),
         DGraphics::samplerHeap.Get()};
    pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    pCommandList->RSSetViewports(1, &m_screenViewport);
    pCommandList->RSSetScissorRects(1, &m_scissorRect);
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // SRV 공용 텍스처
    pCommandList->SetGraphicsRootDescriptorTable(0, m_globalConstBuffer.m_gpuHandle);
    pCommandList->SetGraphicsRootDescriptorTable(1, m_globalConstBuffer.m_gpuHandle);
    // CBV
    pCommandList->SetGraphicsRootDescriptorTable(2, m_globalConstBuffer.m_gpuHandle);
    // Sampler
    pCommandList->SetGraphicsRootDescriptorTable(3, DGraphics::samplerHeap->GetGPUDescriptorHandleForHeapStart());

    pCommandList->OMSetStencilRef(0);// DNote : 확인 필요
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
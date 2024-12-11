#include "Engine.h"
#include "Device.h"
#include "ConstantBuffer.h"
#include "CommandQueue.h"
#include "SwapChain.h"
#include "RootSignature.h"
#include "Shader.h"
#include "GraphicsPipelineState.h"
#include "GraphicsPSO2.h"
#include "Samplers2.h"
#include "ConstantBuffer.h"
#include "DSkinnedMeshModel2.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);
namespace hlab {


LRESULT WINAPI WndProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return GEngine->MsgProc(hWnd, msg, wParam, lParam);
}
Engine::~Engine()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	DestroyWindow(m_window.hwnd);
}
void Engine::Init(const WindowInfo& info)
{
	m_window = info;	
	InitGraphics();
	InitGlobalBuffer();
	//CreateConstantBuffer(CBV_REGISTER::b0, sizeof(LightParams), 1);
	//CreateConstantBuffer(CBV_REGISTER::b1, sizeof(TransformParams), 256);
	//CreateConstantBuffer(CBV_REGISTER::b2, sizeof(MaterialParams), 256);
	CreateRenderTargetGroups();

	ResizeWindow(info.width, info.height);

	m_camera.SetAspectRatio(this->GetAspectRatio());

	InitGUI();
	
	// TODO. CommandList Close �� ���� �ʿ�
}
int Engine::Run()
{

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

			//UpdateGUI(); // �߰������� ����� GUI

			ImGui::End();
			ImGui::Render();
			Update(ImGui::GetIO().DeltaTime);

			Render();

		}
	}

	return 0;
}

bool Engine::InitMainWindow()
{

	WNDCLASSEX wc = { sizeof(WNDCLASSEX),
					 CS_CLASSDC,
					 WndProc2,
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
		std::cout << "RegisterClassEx() failed." << std::endl;
		return false;
	}

	RECT wr = { 0, 0, m_window.width, m_window.height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);
	m_window.hwnd = CreateWindow(wc.lpszClassName, L"Daerim'sGTA",
		WS_OVERLAPPEDWINDOW,
		100, // ������ ���� ����� x ��ǥ
		100, // ������ ���� ����� y ��ǥ
		wr.right - wr.left, // ������ ���� ���� �ػ�
		wr.bottom - wr.top, // ������ ���� ���� �ػ�
		NULL, NULL, wc.hInstance, NULL);

	if (!m_window.hwnd) {
		std::cout << "CreateWindow() failed." << std::endl;
		return false;
	}

	ShowWindow(m_window.hwnd, SW_SHOWDEFAULT);
	UpdateWindow(m_window.hwnd);

	SetForegroundWindow(m_window.hwnd);
	return true;
}

bool Engine::InitGUI() {

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.DisplaySize = ImVec2(float(m_window.width), float(m_window.height));
	ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	if (!ImGui_ImplDX12_Init(DEVICE.Get(), SWAP_CHAIN_BUFFER_COUNT, DXGI_FORMAT_R8G8B8A8_UNORM,
		m_rtGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)]->GetRenderTargetHeap().Get(),
		m_rtGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)]->GetRenderTargetHeap()->GetCPUDescriptorHandleForHeapStart(),
		m_rtGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)]->GetRenderTargetHeap()->GetGPUDescriptorHandleForHeapStart()
		)) 
	{
		return false;
	}

	if (!ImGui_ImplWin32_Init(m_window.hwnd))
	{
		return false;
	}

	return true;
}

void Engine::InitGraphics()
{
	// �׷��� ȭ�� ũ�⸦ ����
	m_viewport = { 0, 0, static_cast<FLOAT>(m_window.width), static_cast<FLOAT>(m_window.height), 0.0f, 1.0f };
	m_scissorRect = CD3DX12_RECT(0, 0, m_window.width, m_window.height);

	m_device = std::make_shared<Device>();
	m_device->Init();

	m_swapChain = std::make_shared< SwapChain>();
	m_swapChain->Init(m_window, m_device->GetDevice(), m_device->GetDXGI(), m_graphicsCmdQueue->GetCmdQueue());

	m_graphicsCmdQueue = std::make_shared<GraphicsCommandQueue>();
	m_graphicsCmdQueue->Init(m_device->GetDevice(), m_swapChain);

	m_graphicsDescHeap = std::make_shared< GraphicsDescriptorHeap>();
	m_graphicsDescHeap->Init(256);// ��.. Frame ������ŭ�� �־ ���������� �ͱ��ѵ�

	m_shader = std::make_shared<Shader>();
	m_shader->Init();

	m_rootSignature = std::make_shared<RootSignature>();
	m_rootSignature->Init();

	m_graphicsPipelineState = std::make_shared< GraphicsPipelineState>();
	m_graphicsPipelineState->Init();

	m_defaultGraphicsPSO = std::make_shared< GraphicsPSO2>();
	m_defaultGraphicsPSO->Init(m_rootSignature->GetGraphicsRootSignature(), m_graphicsPipelineState->GetDefaultPipelineState());

	m_samplers = std::make_shared<Samplers>();
	m_samplers->Init();
	GRAPHICS_CMD_LIST->SetGraphicsRootDescriptorTable(3, m_samplers->GetDescHeap()->GetGPUDescriptorHandleForHeapStart());

}
void Engine::InitGlobalBuffer()
{
	m_globalConstsBuffer = std::make_shared<ConstantBuffer2<GlobalConstants2>>();
	m_globalConstsBuffer->Init(CBV_REGISTER::b0, 3);

	m_envTex = std::make_shared<Texture>();
	m_irradianceTex = std::make_shared<Texture>();
	m_specularTex = std::make_shared<Texture>();
	m_brdfTex = std::make_shared<Texture>();

	
}

void Engine::InitCubemaps(wstring basePath, wstring envFilename,
	wstring specularFilename, wstring irradianceFilename,
	wstring brdfFilename) {
	// BRDF LookUp Table�� CubeMap�� �ƴ϶� 2D �ؽ��� �Դϴ�.
	m_envTex->Load((basePath + envFilename).c_str(), true);
	GRAPHICS_CMD_LIST->SetGraphicsRootShaderResourceView(1, m_envTex->GetTex2D()->GetGPUVirtualAddress());

	m_irradianceTex->Load((basePath + irradianceFilename).c_str(), true);
	GRAPHICS_CMD_LIST->SetGraphicsRootShaderResourceView(2, m_irradianceTex->GetTex2D()->GetGPUVirtualAddress());

	m_specularTex->Load((basePath + specularFilename).c_str(), true);
	GRAPHICS_CMD_LIST->SetGraphicsRootShaderResourceView(3, m_specularTex->GetTex2D()->GetGPUVirtualAddress());

	m_brdfTex->Load((basePath + brdfFilename).c_str(), true);
	GRAPHICS_CMD_LIST->SetGraphicsRootShaderResourceView(4, m_brdfTex->GetTex2D()->GetGPUVirtualAddress());
}

bool Engine::InitScene()
{
	// ���� ����
	{
		GlobalConstants2& globalConstsCPU = m_globalConstsBuffer->GetCpu();
		// ���� 0�� ����
		globalConstsCPU.lights[0].radiance = Vector3(5.0f);
		globalConstsCPU.lights[0].position = Vector3(0.0f, 1.5f, 1.1f);
		globalConstsCPU.lights[0].direction = Vector3(0.0f, -1.0f, 0.0f);
		globalConstsCPU.lights[0].spotPower = 3.0f;
		globalConstsCPU.lights[0].radius = 0.04f;
		globalConstsCPU.lights[0].type =
			LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow

		// ���� 1�� ��ġ�� ������ Update()���� ����
		globalConstsCPU.lights[1].radiance = Vector3(5.0f);
		globalConstsCPU.lights[1].spotPower = 3.0f;
		globalConstsCPU.lights[1].fallOffEnd = 20.0f;
		globalConstsCPU.lights[1].radius = 0.02f;
		globalConstsCPU.lights[1].type =
			LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow

		// ���� 2�� ������
		globalConstsCPU.lights[2].type = LIGHT_OFF;
		// DaerimGTA
		globalConstsCPU.strengthIBL = 0.1f;
		globalConstsCPU.lodBias = 0.0f;

		m_camera.Reset(Vector3(1.60851f, 0.409084f, 0.560064f), -1.65915f,
			0.0654498f);

		InitCubemaps(L"../Assets/Textures/Cubemaps/HDRI/",
			L"SampleEnvHDR.dds", L"SampleSpecularHDR.dds",
			L"SampleDiffuseHDR.dds", L"SampleBrdf.dds");

		{
			std::string path = "../Assets/Characters/Mixamo/";
			std::string characterName = "character.fbx";
			Vector3 center(0.5f, 0.1f, 1.0f);
			m_activeModel = std::make_shared<DSkinnedMeshModel2>(path, characterName);
			m_activeModel->m_materialConsts.GetCpu().albedoFactor = Vector3(1.0f);
			m_activeModel->m_materialConsts.GetCpu().roughnessFactor = 0.8f;
			m_activeModel->m_materialConsts.GetCpu().metallicFactor = 0.0f;
			m_activeModel->UpdateWorldRow(Matrix::CreateScale(0.2f) *
				Matrix::CreateTranslation(center));
			m_activeModel->SetScale(0.2f);
		}
		// EDaerimGTA
	}

	return true;
}

void Engine::Update(float dt)
{
	m_camera.UpdateKeyboard(dt, m_keyPressed);

	// �ݻ� ��� �߰�
	const Vector3 eyeWorld = m_camera.GetEyePos();
	const Matrix reflectRow; //Matrix::CreateReflection(m_mirrorPlane);
	const Matrix viewRow = m_camera.GetViewRow();
	const Matrix projRow = m_camera.GetProjRow();
	UpdateGlobalConstants(dt, eyeWorld, viewRow, projRow, reflectRow);
	//GET_SINGLE(Input)->Update();
	//GET_SINGLE(Timer)->Update();
	//GET_SINGLE(SceneManager)->Update();
	//GET_SINGLE(InstancingManager)->ClearBuffer();

	Render();
}

void Engine::Render()
{
	RenderBegin();

	//GET_SINGLE(SceneManager)->Render();
	m_activeModel->Render();

	RenderEnd();
}

void Engine::RenderBegin()
{

	// �������� ����� �ؽ����: "Common.hlsli"���� register(t10)���� ����
	GRAPHICS_CMD_LIST->SetGraphicsRootConstantBufferView(
		1, m_globalConstsBuffer->GetGpuVirtualAddress(m_swapChain->GetBackBufferIndex()));

	m_graphicsCmdQueue->RenderBegin();
}

void Engine::RenderEnd()
{
	SetMainViewport();
	m_graphicsCmdQueue->RenderEnd();
}

void Engine::SetMainViewport()
{
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = m_window.width;
	viewport.Height = m_window.height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	GRAPHICS_CMD_LIST->RSSetViewports(1, &viewport);
}

void Engine::ResizeWindow(int32 width, int32 height)
{
	m_window.width = width;
	m_window.height = height;

	RECT rect = { 0, 0, width, height };
	::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	::SetWindowPos(m_window.hwnd, 0, 100, 100, width, height, 0);
}

// ���� ��ü���� ���������� ����ϴ� Const ������Ʈ
void Engine::UpdateGlobalConstants(const float& dt, const Vector3& eyeWorld,
	const Matrix& viewRow,
	const Matrix& projRow, const Matrix& refl) 
{
	GlobalConstants2& globalCpuData = m_globalConstsBuffer->GetCpu();
	globalCpuData.globalTime += dt;
	globalCpuData.eyeWorld = eyeWorld;
	globalCpuData.view = viewRow.Transpose();
	globalCpuData.proj = projRow.Transpose();
	globalCpuData.invProj = projRow.Invert().Transpose();
	globalCpuData.viewProj = (viewRow * projRow).Transpose();
	globalCpuData.invView = viewRow.Invert().Transpose();

	// �׸��� �������� ���
	globalCpuData.invViewProj = globalCpuData.viewProj.Invert();

	m_globalConstsBuffer->Upload();
}

void Engine::CreateRenderTargetGroups()
{
	// DepthStencil
	shared_ptr<Texture> dsTexture = std::make_shared<Texture>();
	dsTexture->Create(DXGI_FORMAT_D32_FLOAT, m_window.width, m_window.height,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	// SwapChain Group
	{
		vector<RenderTarget> rtVec(SWAP_CHAIN_BUFFER_COUNT);

		for (uint32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		{
			ComPtr<ID3D12Resource> resource;
			m_swapChain->GetSwapChain()->GetBuffer(i, IID_PPV_ARGS(&resource));
			rtVec[i].target = std::make_shared<Texture>();
			rtVec[i].target->CreateFromResource(resource);
		}

		m_rtGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)] = std::make_shared<RenderTargetGroup>();
		m_rtGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)]->Create(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN, rtVec, dsTexture);
	}
}

void Engine::OnMouseMove(int mouseX, int mouseY) {

	m_mouseX = mouseX;
	m_mouseY = mouseY;

	// ���콺 Ŀ���� ��ġ�� NDC�� ��ȯ
	// ���콺 Ŀ���� ���� ��� (0, 0), ���� �ϴ�(width-1, height-1)
	// NDC�� ���� �ϴ��� (-1, -1), ���� ���(1, 1)
	m_mouseNdcX = mouseX * 2.0f / m_window.windowed - 1.0f;
	m_mouseNdcY = -mouseY * 2.0f / m_window.height + 1.0f;

	// Ŀ���� ȭ�� ������ ������ ��� ���� ����
	// ���ӿ����� Ŭ������ ���� ���� �ֽ��ϴ�.
	m_mouseNdcX = std::clamp(m_mouseNdcX, -1.0f, 1.0f);
	m_mouseNdcY = std::clamp(m_mouseNdcY, -1.0f, 1.0f);

	// ī�޶� ���� ȸ��
	m_camera.UpdateMouse(m_mouseNdcX, m_mouseNdcY);
}

void Engine::OnMouseClick(int mouseX, int mouseY) {

	m_mouseX = mouseX;
	m_mouseY = mouseY;

	m_mouseNdcX = mouseX * 2.0f / m_window.windowed - 1.0f;
	m_mouseNdcY = -mouseY * 2.0f / m_window.height + 1.0f;
}

LRESULT Engine::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	switch (msg) {
	case WM_SIZE:
		// ȭ�� �ػ󵵰� �ٲ�� SwapChain�� �ٽ� ����
		if (m_swapChain) {

			m_window.width = int(LOWORD(lParam));
			m_window.height = int(HIWORD(lParam));

			// �����찡 Minimize ��忡���� screenWidth/Height�� 0
			if (m_window.width && m_window.height) {

				std::cout << "Resize SwapChain to " << m_window.width << " "
					<< m_window.height << std::endl;

				// ViewPort ���� Reset
				// TODO. ����� ������ �ִ��� Ȯ�� �ʿ�
				m_swapChain->Init(m_window, DEVICE, m_device->GetDXGI(), m_graphicsCmdQueue->GetCmdQueue());
				vector<RenderTarget> rtVec(SWAP_CHAIN_BUFFER_COUNT);
				for (uint32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
				{
					ComPtr<ID3D12Resource> resource;
					m_swapChain->GetSwapChain()->GetBuffer(i, IID_PPV_ARGS(&resource));
					rtVec[i].target = std::make_shared<Texture>();
					rtVec[i].target->CreateFromResource(resource);
				}
				// DepthStencil
				shared_ptr<Texture> dsTexture = std::make_shared<Texture>();
				dsTexture->Create(DXGI_FORMAT_D32_FLOAT, m_window.width, m_window.height,
					CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
				m_rtGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)]->Create(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN, rtVec, dsTexture);
				//SetMainViewport();


				m_camera.SetAspectRatio(this->GetAspectRatio());
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
		//if (m_activateActor != nullptr)
		//{
		//	if (m_activateActor->MsgProc(wParam, true))
		//	{
		//		return true;
		//	}
		//}
		m_keyPressed[wParam] = true;
		//if (wParam == VK_ESCAPE) { // ESCŰ ����
		//	DestroyWindow(hwnd);
		//}
		//if (wParam == VK_SPACE) {
		//	m_lightRotate = !m_lightRotate;
		//}
		break;
	case WM_KEYUP:
		//if (m_activateActor != nullptr)
		//{
		//	if (m_activateActor->MsgProc(wParam, false))
		//	{
		//		return true;
		//	}
		//}
		if (wParam == 'F') { // fŰ ����Ī ����
			m_camera.m_useFirstPersonView = !m_camera.m_useFirstPersonView;
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

float Engine::GetAspectRatio() const
{
	return float(m_window.width) / m_window.height;
}
}

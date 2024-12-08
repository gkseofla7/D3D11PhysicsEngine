#pragma once
#include "EnginePch.h"

// AppBase�� ExampleApp�� �����ϱ� ����
// �ݺ��ؼ� ���Ǵ� ���̴� ����, ���� ���� ���� �и�
// Parameter�� ������ �� const�� �տ� �δ� ���� �Ϲ���������
// device�� ���ƻ��� �߿伺 ������ ���ܷ� �� �տ� �׽��ϴ�.
// ���ǰ� ����Ǹ鼭 ���ݾ� ����� �߰��˴ϴ�.

namespace hlab {

using Microsoft::WRL::ComPtr;
using std::shared_ptr;
using std::vector;
using std::wstring;

struct ImageInfo
{
    int width = 0;
    int height = 0;
    std::vector<uint8_t> image;
    DXGI_FORMAT pixelFormat;
};


class D3D12Utils {
  public:
      static void CreateVertexShader(
          ComPtr<ID3D12Device> device, wstring filename,
          ComPtr<ID3DBlob>& vertexShader,
          const vector<D3D_SHADER_MACRO> shaderMacros = {/* Empty default */ });
      static void CreatePixelShader(ComPtr<ID3D12Device> device,
          const wstring& filename,
          ComPtr<ID3DBlob>& pixelShader);
      static void CreatePipelineState(ComPtr<ID3D12Device>& device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc, ComPtr<ID3D12PipelineState>& OutPipelineState);
    // ShaderMacros ����� �� ����
    // {D3D_SHADER_MACRO("SKINNED", "1"), D3D_SHADER_MACRO(NULL, NULL)};
    // �� �ڿ� NULL, NULL �ʼ�

    static void CreateIndexBuffer(ComPtr<ID3D12Device> device,
        const vector<uint32_t>& indices,
        ComPtr<ID3D12Resource>& indexBuffer,
        D3D12_INDEX_BUFFER_VIEW& indexBufferView);


    template <typename T_VERTEX>
    static void CreateVertexBuffer(ComPtr<ID3D12Device> device,
        const vector<T_VERTEX>& vertices,
        ComPtr<ID3D12Resource>&	vertexBuffer,
        D3D12_VERTEX_BUFFER_VIEW& vertexBufferView) {
        uint32 bufferSize = vertices.size() * sizeof(T_VERTEX);

        D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

        DEVICE->CreateCommittedResource(
            &heapProperty,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertexBuffer));

        // Copy the triangle data to the vertex buffer.
        void* vertexDataBuffer = nullptr;
        CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
        vertexBuffer->Map(0, &readRange, &vertexDataBuffer);
        ::memcpy(vertexDataBuffer, &buffer[0], bufferSize);
        vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(T_VERTEX); // ���� 1�� ũ��
        vertexBufferView.SizeInBytes = bufferSize; // ������ ũ��	
    }

    template <typename T_CONSTANT>
    static void CreateConstBuffer(ComPtr<ID3D12Device>& device,
        const T_CONSTANT& constantBufferData,
        ComPtr<ID3D12Resource>& constantBuffer) {

        ThreadPool& tPool = ThreadPool::getInstance();
        auto func = [&device, constantBufferData, &constantBuffer]() {
            return CreateConstBufferImpl(device, constantBufferData, constantBuffer); };
        tPool.EnqueueRenderJob(func);
    }

    template <typename T_CONSTANT>
    static void CreateConstBuffer(ComPtr<ID3D12Device>& device,
        const T_CONSTANT&& constantBufferData,
        ComPtr<ID3D12Resource>& constantBuffer) {

        ThreadPool& tPool = ThreadPool::getInstance();
        auto func = [&device, constantBufferData = std::move(constantBufferData), &constantBuffer]() {
            return CreateConstBufferImpl(device, constantBufferData, constantBuffer); };
        tPool.EnqueueRenderJob(func);
    }

    template <typename T_CONSTANT>
    static void CreateConstBufferImpl(ComPtr<ID3D12Device> &device,
                                  const T_CONSTANT& constantBufferData,
                                  ComPtr<ID3D12Resource> &constantBuffer) {
        static_assert((sizeof(T_CONSTANT) % 16) == 0,
                      "Constant Buffer size must be 16-byte aligned");

        D3D12_RESOURCE_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        const UINT constantBufferSize = (sizeof(constantBufferData) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1); // must be a multiple 256 bytes
        desc.ByteWidth = constantBufferSize;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA initData;
        ZeroMemory(&initData, sizeof(initData));
        initData.pSysMem = &constantBufferData;
        initData.SysMemPitch = 0;
        initData.SysMemSlicePitch = 0;

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&constantBuffer)));
    }
    // Buffer
    template <typename T_DATA>
    static void UpdateBuffer(const T_DATA &bufferData,
                             ComPtr<ID3D12Resource> &buffer) {
        ThreadPool& tPool = ThreadPool::getInstance();
        auto func = [&bufferData, &buffer]() {
            return UpdateBufferImpl(context, bufferData, buffer); };
        tPool.EnqueueRenderJob(func);
    }

    template <typename T_DATA>
    static void UpdateBuffer(const T_DATA&& bufferData,
        ComPtr<ID3D12Resource>& buffer) {
        ThreadPool& tPool = ThreadPool::getInstance();
        auto func = [bufferData = std::move(bufferData), &buffer]() {
            return UpdateBufferImpl(context, bufferData, buffer); };
        tPool.EnqueueRenderJob(func);
    }

    template <typename T_DATA>
    static void UpdateBufferImpl(const T_DATA& bufferData,
        ComPtr<ID3D12Resource>& buffer) {
        if (!buffer) {
            std::cout << "UpdateBuffer() buffer was not initialized."
                << std::endl;
        }
        T_DATA* ConstantBufferWO;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(buffer->Map(0, &readRange, reinterpret_cast<void**>(&ConstantBufferWO)));
        memcpy(ConstantBufferWO, &bufferData, sizeof(T_DATA));
        CD3DX12_RANGE writtenRange(0, sizeof(T_DATA));
        ThrowIfFailed(buffer->Unmap(0, &writtenRange));
    }

    // Texture
    static void CreateTexture(ComPtr<ID3D12Device> device,
                  const std::string filename, const bool usSRGB,
                    ComPtr<ID3D12Resource>&texture);

    static void CreateTexture(ComPtr<ID3D12Device> device, const std::string albedoFilename,
        const std::string opacityFilename,const bool usSRGB, ComPtr<ID3D12Resource>& texture);
  
    static void CreateMetallicRoughnessTexture(
        ComPtr<ID3D12Device> device, const std::string metallicFiilename,
        const std::string roughnessFilename, ComPtr<ID3D12Resource>& texture);


    static void CreateTextureHelper(ComPtr<ID3D12Device>& device,
        const int width, const int height, const vector<uint8_t>& image,
        const DXGI_FORMAT pixelFormat, ComPtr<ID3D12Resource>& texture);


    static void CreateDDSTexture(ComPtr<ID3D12Device> &device, ComPtr<ID3D12CommandQueue> m_commandQueue,
                                 const wstring&&filename, const bool isCubeMap);


    static size_t GetPixelSize(DXGI_FORMAT pixelFormat);
private:
    static std::unordered_map<std::string, ImageInfo> imageMap;
};
} // namespace hlab

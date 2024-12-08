#pragma once
#include "D3D12Core/EnginePch.h"

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
      static void CreateRootSignature(ComPtr<ID3D12Device>& device,
          ComPtr<ID3D12RootSignature>& rootSignature, vector<CD3DX12_ROOT_PARAMETER1>& rootParams);
      static void CreatePipelineState(ComPtr<ID3D12Device>& device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc, ComPtr<ID3D12PipelineState>& OutPipelineState);
    // ShaderMacros ����� �� ����
    // {D3D_SHADER_MACRO("SKINNED", "1"), D3D_SHADER_MACRO(NULL, NULL)};
    // �� �ڿ� NULL, NULL �ʼ�

    static void CreateIndexBuffer(ComPtr<ID3D12Device> &device,
                                  const vector<uint32_t> &indices,
                                  D3D12_INDEX_BUFFER_VIEW& OutIndexBufferView);
    static void CreateIndexBuffer(ComPtr<ID3D12Device>& device,
        const vector<uint32_t>&& indices,
        D3D12_INDEX_BUFFER_VIEW& OutIndexBufferView);

    static void CreateIndexBufferImpl(ComPtr<ID3D12Device>& device,
        const vector<uint32_t>& indices,
        D3D12_INDEX_BUFFER_VIEW& OutIndexBufferView);

    template <typename T_VERTEX>
    static void CreateVertexBuffer(ComPtr<ID3D11Device>& device,
        const vector<T_VERTEX>& vertices,
        ComPtr<ID3D11Buffer>& vertexBuffer) 
    {
        ThreadPool& tPool = ThreadPool::getInstance();
        auto func = [&device, vertices, &vertexBuffer]() {
            return CreateVertexBufferImpl(device, vertices, vertexBuffer); };
        tPool.EnqueueRenderJob(func);
    }
    // Vertex
    template <typename T_VERTEX>
    static void CreateVertexBuffer(ComPtr<ID3D11Device>& device,
        const vector<T_VERTEX>&& vertices,
        ComPtr<ID3D11Buffer>& vertexBuffer)
    {
        ThreadPool& tPool = ThreadPool::getInstance();
        auto func = [&device, vertices = std::move(vertices), &vertexBuffer]() {
            return CreateVertexBufferImpl(device, vertices, vertexBuffer); };
        tPool.EnqueueRenderJob(func);
    }

    template <typename T_VERTEX>
    static void CreateVertexBufferImpl(ComPtr<ID3D11Device>& device,
        const vector<T_VERTEX>& vertices,
        ComPtr<ID3D11Buffer>& vertexBuffer) {
        // D3D11_USAGE enumeration (d3d11.h)
        // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage

        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof(bufferDesc));
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = UINT(sizeof(T_VERTEX) * vertices.size());
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0; // 0 if no CPU access is necessary.
        bufferDesc.StructureByteStride = sizeof(T_VERTEX);

        D3D11_SUBRESOURCE_DATA vertexBufferData = {
            0 }; // MS �������� �ʱ�ȭ�ϴ� ���
        vertexBufferData.pSysMem = vertices.data();
        vertexBufferData.SysMemPitch = 0;
        vertexBufferData.SysMemSlicePitch = 0;

        ThrowIfFailed(device->CreateBuffer(&bufferDesc, &vertexBufferData,
            vertexBuffer.GetAddressOf()));
    }

    // Instance
    template <typename T_INSTANCE>
    static void CreateInstanceBuffer(ComPtr<ID3D11Device>& device,
        const vector<T_INSTANCE>& instances,
        ComPtr<ID3D11Buffer>& instanceBuffer) {

        ThreadPool& tPool = ThreadPool::getInstance();
        auto func = [&device, instances, &instanceBuffer]() {
            return CreateInstanceBufferImpl(device, instances, instanceBuffer); };
        tPool.EnqueueRenderJob(func);
    }
    template <typename T_INSTANCE>
    static void CreateInstanceBuffer(ComPtr<ID3D11Device>& device,
        const vector<T_INSTANCE>&& instances,
        ComPtr<ID3D11Buffer>& instanceBuffer) {

        ThreadPool& tPool = ThreadPool::getInstance();
        auto func = [&device, instances = std::move(instances), &instanceBuffer]() {
            return CreateInstanceBufferImpl(device, instances, instanceBuffer); };
        tPool.EnqueueRenderJob(func);
    }

    template <typename T_INSTANCE>
    static void CreateInstanceBufferImpl(ComPtr<ID3D11Device> &device,
                                     const vector<T_INSTANCE> &instances,
                                     ComPtr<ID3D11Buffer> &instanceBuffer) {

        // �⺻������ VertexBuffer�� ����մϴ�.
        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof(bufferDesc));
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC; // <- �ִϸ��̼ǿ� ���
        bufferDesc.ByteWidth = UINT(sizeof(T_INSTANCE) * instances.size());
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // <- CPU���� ����
        bufferDesc.StructureByteStride = sizeof(T_INSTANCE);

        D3D11_SUBRESOURCE_DATA vertexBufferData = {
            0}; // MS �������� �ʱ�ȭ�ϴ� ���
        vertexBufferData.pSysMem = instances.data();
        vertexBufferData.SysMemPitch = 0;
        vertexBufferData.SysMemSlicePitch = 0;

        ThrowIfFailed(device->CreateBuffer(&bufferDesc, &vertexBufferData,
                                           instanceBuffer.GetAddressOf()));
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
    static void CreateTexture(ComPtr<ID3D11Device> &device,
                  ComPtr<ID3D11DeviceContext> &context,
                  const std::string filename, const bool usSRGB,
                  ComPtr<ID3D11Texture2D> &texture,
                  ComPtr<ID3D11ShaderResourceView> &textureResourceView);

    static void CreateTexture(
        ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
        const std::string albedoFilename, const std::string opacityFilename,
        const bool usSRGB, ComPtr<ID3D11Texture2D>& texture,
        ComPtr<ID3D11ShaderResourceView>& textureResourceView);
  
    static void CreateMetallicRoughnessTexture(
        ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context,
        const std::string metallicFiilename,
        const std::string roughnessFilename, ComPtr<ID3D11Texture2D>& texture,
        ComPtr<ID3D11ShaderResourceView>& srv);



    static void CreateTextureHelper(ComPtr<ID3D12Device>& device, 
        ComPtr<ID3D12GraphicsCommandList>& commandList,
        ComPtr<ID3D12CommandQueue>& commandQueue,
        const int width, const int height, const vector<uint8_t>& image,
        const DXGI_FORMAT pixelFormat,
        ComPtr<ID3D12Resource>& texture,
        CD3DX12_CPU_DESCRIPTOR_HANDLE& srvHandle);
    static void CreateTextureHelper(ComPtr<ID3D12Device>& device, 
        ComPtr<ID3D12GraphicsCommandList>& commandList,
        ComPtr<ID3D12CommandQueue>& commandQueue,
        const int width, const int height, const vector<uint8_t>&& image,
        const DXGI_FORMAT pixelFormat,
        ComPtr<ID3D12Resource>& texture,
        CD3DX12_CPU_DESCRIPTOR_HANDLE& srvHandle);
    static void CreateTextureHelperImpl(ComPtr<ID3D12Device>& device, 
        ComPtr<ID3D12GraphicsCommandList>& commandList,
        ComPtr<ID3D12CommandQueue>& commandQueue,
        const int width, const int height, const vector<uint8_t>& image,
        const DXGI_FORMAT pixelFormat,
        ComPtr<ID3D12Resource>& texture,
        CD3DX12_CPU_DESCRIPTOR_HANDLE& srvHandle);

    // ���� �Ⱦ��� �ְų� �񵿱� �Լ� ȣ�� ���ο����� ���̰� �ִ°� ����
    static void CreateUATexture(ComPtr<ID3D11Device> &device, const int width,
                                const int height, const DXGI_FORMAT pixelFormat,
                                ComPtr<ID3D11Texture2D> &texture,
                                ComPtr<ID3D11RenderTargetView> &rtv,
                                ComPtr<ID3D11ShaderResourceView> &srv,
                                ComPtr<ID3D11UnorderedAccessView> &uav);

    static void CreateTexture3D(ComPtr<ID3D11Device> &device, const int width,
                                const int height, const int depth,
                                const DXGI_FORMAT pixelFormat,
                                const vector<float> &initData,
                                ComPtr<ID3D11Texture3D> &texture,
                                ComPtr<ID3D11RenderTargetView> &rtv,
                                ComPtr<ID3D11ShaderResourceView> &srv,
                                ComPtr<ID3D11UnorderedAccessView> &uav);

    static void CreateStagingBuffer(ComPtr<ID3D11Device> &device,
                                    const UINT numElements,
                                    const UINT sizeElement,
                                    const void *initData,
                                    ComPtr<ID3D11Buffer> &buffer);

    static void CopyFromStagingBuffer(ComPtr<ID3D11DeviceContext> &context,
                                      ComPtr<ID3D11Buffer> &buffer, UINT size,
                                      void *dest);

    static void CopyToStagingBuffer(ComPtr<ID3D11DeviceContext> &context,
                                    ComPtr<ID3D11Buffer> &buffer, UINT size,
                                    void *src) {
        D3D11_MAPPED_SUBRESOURCE ms;
        context->Map(buffer.Get(), NULL, D3D11_MAP_WRITE, NULL, &ms);
        memcpy(ms.pData, src, size);
        context->Unmap(buffer.Get(), NULL);
    }

    static void CreateStructuredBuffer(ComPtr<ID3D11Device> &device,
                                       const UINT numElements,
                                       const UINT sizeElement,
                                       const void *initData,
                                       ComPtr<ID3D11Buffer> &buffer,
                                       ComPtr<ID3D11ShaderResourceView> &srv,
                                       ComPtr<ID3D11UnorderedAccessView> &uav);

    static void CreateIndirectArgsBuffer(ComPtr<ID3D11Device> &device,
                                         const UINT numElements,
                                         const UINT sizeElement,
                                         const void *initData,
                                         ComPtr<ID3D11Buffer> &buffer);

    static void CreateAppendBuffer(ComPtr<ID3D11Device> &device,
                                   const UINT numElements,
                                   const UINT sizeElement, const void *initData,
                                   ComPtr<ID3D11Buffer> &buffer,
                                   ComPtr<ID3D11ShaderResourceView> &srv,
                                   ComPtr<ID3D11UnorderedAccessView> &uav);

    static void
    CreateTextureArray(ComPtr<ID3D11Device> &device,
                       ComPtr<ID3D11DeviceContext> &context,
                       const std::vector<std::string> filenames,
                       ComPtr<ID3D11Texture2D> &texture,
                       ComPtr<ID3D11ShaderResourceView> &textureResourceView);

    static void CreateDDSTexture(ComPtr<ID3D12Device> &device, ComPtr<ID3D12CommandQueue> m_commandQueue,
                                 const wstring&&filename, const bool isCubeMap,
        CD3DX12_GPU_DESCRIPTOR_HANDLE& texResView);
    static void CreateDDSTextureImpl(ComPtr<ID3D12Device>& device, ComPtr<ID3D12CommandQueue> m_commandQueue,
        const wstring&& filename, const bool isCubeMap,
        CD3DX12_GPU_DESCRIPTOR_HANDLE& texResView);

    static ComPtr<ID3D11Texture2D>
    CreateStagingTexture(ComPtr<ID3D11Device> &device,
                         ComPtr<ID3D11DeviceContext> &context, const int width,
                         const int height, const std::vector<uint8_t> &image,
                         const DXGI_FORMAT pixelFormat, const int mipLevels,
                         const int arraySize);

    static ComPtr<ID3D11Texture3D>
    CreateStagingTexture3D(ComPtr<ID3D11Device> &device, const int width,
                           const int height, const int depth,
                           const DXGI_FORMAT pixelFormat);

    static void WriteToPngFile(ComPtr<ID3D11Device> &device,
                               ComPtr<ID3D11DeviceContext> &context,
                               ComPtr<ID3D11Texture2D> &textureToWrite,
                               const std::string filename);

    static size_t GetPixelSize(DXGI_FORMAT pixelFormat);
private:
    static std::unordered_map<std::string, ImageInfo> imageMap;
};
} // namespace hlab

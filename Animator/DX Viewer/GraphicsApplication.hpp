#pragma once

// Windows includes
//#define WIN32_LEAN_AND_MEAN // Disregard uncommon windows options, save on build time
#include <Windows.h>
#include <wrl/client.h>

// D3D includes
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>
//#include "DDSTextureLoader.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib") // Needed to fix an unresolved external for CreateDXGIFactory1
#pragma comment(lib, "dxguid.lib") // Needed to fix an unresolved external for d3dx12.h


// Shader includes
//#include "Shaders/pixelShader.h"
//#include "Shaders/vertexShader.h"
//#include "Shaders/allgreen.h"
//#include "Shaders/GeoPipeVert.h"
//#include "Shaders/GeoPipePixel.h"
//#include "Shaders/PointToTriangle.h"

// Console includes
#include <iostream>

// App includes
//#include "Shaders/Animal.h"
//#include "Shaders/axeTexture.h"
//#include "interstellar.h"
#include <vector>
#include <fstream>
#include "XTime.h"

namespace MRenderer
{

	using namespace DirectX;
	using Microsoft::WRL::ComPtr;

	using std::vector;
	class GraphicsApplication
	{
	private:
		struct Vertex
		{
			XMFLOAT4 position;
			XMFLOAT4 normal;
			XMFLOAT4 color;
			XMFLOAT2 tex;
		};

		struct InputVertex
		{
			XMFLOAT4 position;
			XMFLOAT4 normal;
			XMFLOAT2 tex;
		};

		struct Mesh
		{
			std::vector<Vertex> vertices;
			std::vector<int> indices;
		};

		struct InputMesh
		{
			std::vector<InputVertex> vertices;
			std::vector<int> indices;
		};

		struct ConstantBuffer
		{
			XMMATRIX mWorld;
			XMMATRIX mView;
			XMMATRIX mProjection;
			XMFLOAT4 vLightDir[3];
			XMFLOAT4 vLightColor[3];
			XMFLOAT4 vLightPosition[3];
			XMFLOAT4 vOutputColor;
			XMFLOAT4 vPointLightRadius;
			XMMATRIX mInverseTransposeWorld;
			XMMATRIX mViewProjection;
			//XMMATRIX 
		};

		struct Camera
		{
			XMMATRIX matrix;
			float verticalAngle;
			float horizontalAngle;
			POINT mousePos;
			bool mouseLock;
			XMFLOAT3 cameraTranslation;
			float fov;
		};

		struct InstanceData
		{
			XMFLOAT3 position;
		};

		struct RenderObject
		{
			Mesh mesh;

			ComPtr<ID3D12Resource>          vertexBuffer;
			D3D12_VERTEX_BUFFER_VIEW        vertexBufferView;

			ComPtr<ID3D12Resource>          indexBuffer;
			D3D12_INDEX_BUFFER_VIEW         indexBufferView;

			ComPtr<ID3D12Resource>          instanceBuffer;
			D3D12_VERTEX_BUFFER_VIEW        instanceBufferView;

			ComPtr<ID3D12PipelineState>     pipelineState;

			char*							pixelShaderByteCode;
			size_t							pixelShaderByteCodeSize;

			char*							vertexShaderByteCode;
			size_t							vertexShaderByteCodeSize;


		};
	public:

		// Constructors
		GraphicsApplication(int width, int height);

		// Destructors
		~GraphicsApplication();

		// Functions
		HRESULT InitializeDevice();
		HRESULT LoadAssets();
		void Update();
		void Render();
		void CleanupDevice();

		// Operators

	private:

		void WaitForPreviousFrame();
		void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter);
		void PopulateCommandList();
		void LoadMesh(Mesh& mesh1);

		bool CreateDevice();
		bool CreateCommandQueue();
		bool CreateSwapchain();
		bool CreateDescriptorHeaps();
		bool CreateFrameResources();
		bool CreateFrameAllocator();

		bool CreateRootSignature();
		bool CreatePipelineStates();
		bool CreateCommandList();
		bool CreateVertexBuffers();
		bool CreateIndexBuffers();
		bool CreateInstanceBuffers();
		bool CreateTextures();
		bool ExecuteCommandList();
		bool SetupDepthStencil();
		bool SetupRasterizer();

		bool CreateShaders();
		bool CreateConstantBuffers();
		std::string OpenFileName(const wchar_t* filter, HWND owner);



	public:

		// Windows objects.
		HINSTANCE   m_instance = nullptr;
		HWND        m_window = nullptr;
		float       m_aspectRatio = 0.0f;

		int m_windowWidth;
		int m_windowHeight;

	private:

		static const UINT FrameCount = 2;

		// Factory objects.
		ComPtr<IDXGIFactory4>				factory;
		ComPtr<IDXGIAdapter1>				hardwareAdapter;

		// Pipeline objects.
		float								m_clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		D3D12_VIEWPORT                      m_viewport;
		D3D12_RECT                          m_scissorRect;
		ComPtr<IDXGISwapChain3>             m_swapChain;
		ComPtr<ID3D12Device>                m_device;
		ComPtr<ID3D12Resource>              m_renderTargets[FrameCount];
		ComPtr<ID3D12CommandAllocator>      m_commandAllocator;
		ComPtr<ID3D12CommandQueue>          m_commandQueue;
		ComPtr<ID3D12RootSignature>         m_rootSignature;
		ComPtr<ID3D12RootSignature>         m_computeRootSignature;
		ComPtr<ID3D12DescriptorHeap>        m_rtvHeap;
		ComPtr<ID3D12PipelineState>			m_computeState;
		ComPtr<ID3D12GraphicsCommandList>   m_commandList;
		UINT                                m_rtvDescriptorSize = 0;

		// App resources.

		ComPtr<ID3D12Resource>          m_constantBuffer;
		ComPtr<ID3D12DescriptorHeap>    m_cbvHeap;
		UINT                            m_cbvDescriptorSize = 0;
		ConstantBuffer                  m_constantBufferData;
		UINT8* m_pCbvDataBegin;
		ComPtr<ID3D12Resource>          m_depthStencil;
		ComPtr<ID3D12DescriptorHeap>    m_dsvHeap;
		ComPtr<ID3D12Resource>          m_sampler;
		ComPtr<ID3D12DescriptorHeap>    m_samplerHeap;
		ComPtr<ID3D12Resource>          m_shaderResourceView;
		ComPtr<ID3D12Resource>          m_shaderResourceView1;
		ComPtr<ID3D12DescriptorHeap>    m_srvHeap;
		ComPtr<ID3D12Resource>			m_texture;
		ComPtr<ID3D12Resource>			m_texture1;
		XMMATRIX                        m_world;
		XMMATRIX                        m_view;
		XMMATRIX                        m_projection;
		Camera                          m_camera;
		float							m_directionalLightRotation = 1.0f;
		float							m_pointLightLocation = 1.0f;
		float							m_pointLightRadius = 1.0f;
		float							m_spotLightLocation = 1.0f;
		float							m_spotLightRotation = 1.0f;

		XTime							timer;



		// Asset Resources
		RenderObject DefaultCube;


		// Synchronization objects.
		UINT                m_frameIndex = 0;
		HANDLE              m_fenceEvent = nullptr;
		ComPtr<ID3D12Fence> m_fence;
		UINT64              m_fenceValue = 0;

#if defined _DEBUG
		// Debug objects.
		ComPtr<ID3D12Debug> m_debugController;
#endif

	};
}

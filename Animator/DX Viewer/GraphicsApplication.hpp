#pragma once

// Windows includes
//#define WIN32_LEAN_AND_MEAN // Disregard uncommon windows options, save on build time
#include <Windows.h>
#include <wrl/client.h>

// D3D includes
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
//#include <DirectXMath.h> // gotten from MathTypes.hpp
#include "DirectXTex.h"

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
#include "Shaders\utility.hlsl"
#include "DebugRenderer.hpp"

namespace MRenderer
{

	using namespace DirectX;
	using Microsoft::WRL::ComPtr;

	using std::vector;
	class GraphicsApplication
	{
	private:

		struct InputVertex
		{
			XMFLOAT4 position;
			XMFLOAT4 normal;
			XMFLOAT2 tex;
			XMINT4 joints;
			struct Double4 { double x, y, z, w; } weights;
		};

		struct Material
		{
			enum ComponentType { EMISSIVE = 0, DIFFUSE, SPECULAR, SHININESS, COUNT };

			struct Component
			{
				float value[3] = { 0.0f, 0.0f, 0.0f };
				float factor = 0.0f;
				int64_t input = -1;
			};

			Component& operator[](int i) { return components[i]; }

			const Component& operator[](int i) const { return components[i]; }

		private:
			Component components[COUNT];
		};

		struct Mesh
		{
			std::vector<Vertex> vertices;
			std::vector<int> indices;
			std::vector<Material> materials;
			std::vector<std::string> materialPaths;
		};

		struct InputMesh
		{
			std::vector<InputVertex> vertices;
			std::vector<int> indices;
		};

		struct InputJoint
		{
			float transform[16];
			int parentIndex;
		};

		struct ShaderLight
		{
			XMFLOAT4 Position; // 16 bytes
			//----------------------------------- (16 byte boundary)
			XMFLOAT4 Direction; // 16 bytes
			//----------------------------------- (16 byte boundary)
			XMFLOAT4 Color; // 16 bytes
			//----------------------------------- (16 byte boundary)
			float SpotAngle; // 4 bytes
			float ConstantAttenuation; // 4 bytes
			float LinearAttenuation; // 4 bytes
			float QuadraticAttenuation; // 4 bytes
			//----------------------------------- (16 byte boundary)
			int LightType; // 4 bytes
			float LightPower; // 4 bytes
			int Enabled; // 4 bytes
			int Padding; // 4 bytes
			//----------------------------------- (16 byte boundary)
		}; // Total:                           // 80 bytes (5 * 16)

		struct ShaderMaterial
		{
			XMFLOAT4 Emissive; // 16 bytes
			//----------------------------------- (16 byte boundary)
			XMFLOAT4 Ambient; // 16 bytes
			//------------------------------------(16 byte boundary)
			XMFLOAT4 Diffuse; // 16 bytes
			//----------------------------------- (16 byte boundary)
			XMFLOAT4 Specular; // 16 bytes
			//----------------------------------- (16 byte boundary)
			float EmissiveFactor; // 4 bytes
			float AmbientFactor; // 4 bytes
			float DiffuseFactor; // 4 bytes
			float SpecularFactor; // 4 bytes
			//----------------------------------- (16 byte boundary)
			float SpecularPower; // 4 bytes
			int UseTexture; // 4 bytes
			int Padding[2]; // 8 bytes
			//----------------------------------- (16 byte boundary)
		}; // Total:               // 80 bytes ( 5 * 16 )

		struct ConstantBuffer
		{
			XMMATRIX World;
			XMMATRIX InverseTransposeWorldMatrix;
			XMMATRIX MVP;
			ShaderMaterial Material;
			XMFLOAT4 EyePosition;
			XMFLOAT4 GlobalAmbient;

			ShaderLight Lights[MAX_LIGHTS];
			XMMATRIX JointTransforms[64];
		};

		struct MVP
		{
			XMMATRIX World;
			XMMATRIX View;
			XMMATRIX Projection;
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
			XMFLOAT4 position;
		};

		struct Joint
		{
			XMFLOAT4X4 transform;
			int parentIndex;
		};

		using Pose = vector<Joint>;

		struct Keyframe
		{
			double keytime;
			Pose poseData;
		};

		struct RenderObject;

		struct Animation
		{
			bool enabled = true;
			RenderObject* renderObject;
			double duration;
			double currentTime = 0.0;
			vector<Keyframe> keyframes;
			Pose bindPose;

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

			D3D12_PRIMITIVE_TOPOLOGY		PrimitiveTopology;

			Animation animation;

			XMMATRIX InverseBind[64];

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
		void LoadMesh(Mesh& mesh, Animation& animation);

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
		//bool SetupRasterizer();

		bool UpdateBuffers(RenderObject RO);

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
		ComPtr<ID3D12Resource>				textureUploadHeap;
		ComPtr<ID3D12Resource>				textureUploadHeap1;
		ComPtr<ID3D12Resource>				textureUploadHeap2;

		// Pipeline objects.
		float								m_clearColor[4] = {0.25f, 0.25f, 0.25f, 1.0f};
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
		MVP								m_MVP;
		ConstantBuffer                  m_constantBufferData;
		UINT8*							m_pCbvDataBegin;
		UINT8*							m_pDebugVertexDataBegin;
		ComPtr<ID3D12Resource>          m_depthStencil;
		ComPtr<ID3D12DescriptorHeap>    m_dsvHeap;
		ComPtr<ID3D12Resource>          m_sampler;
		ComPtr<ID3D12DescriptorHeap>    m_samplerHeap;
		ComPtr<ID3D12Resource>          m_shaderResourceView;
		ComPtr<ID3D12Resource>          m_shaderResourceView1;
		ComPtr<ID3D12DescriptorHeap>    m_srvHeap;
		ComPtr<ID3D12Resource>			m_texture;
		ComPtr<ID3D12Resource>			m_texture1;
		ComPtr<ID3D12Resource>			m_texture2;
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
		RenderObject					DefaultCube;
		RenderObject					DefaultLineRenderer;
		std::vector<RenderObject*>		RenderObjects;
		float							hue = 1.0f;


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

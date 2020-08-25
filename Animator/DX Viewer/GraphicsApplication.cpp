#include "GraphicsApplication.hpp"

namespace MRenderer
{

#pragma warning(disable: 26812) // Disable prefer enum class over enum/\.

	GraphicsApplication::GraphicsApplication(int width, int height)
	{
		m_windowWidth = width;
		m_windowHeight = height;

		m_fenceEvent = nullptr;
		m_fenceValue = 0;
		m_frameIndex = 0;
		m_rtvDescriptorSize = 0;

		m_viewport.TopLeftX = 0;
		m_viewport.TopLeftY = 0;
		m_viewport.Width = static_cast<float>(width);
		m_viewport.Height = static_cast<float>(height);
		m_viewport.MinDepth = D3D12_MIN_DEPTH;
		m_viewport.MaxDepth = D3D12_MAX_DEPTH;

		m_scissorRect.left = 0;
		m_scissorRect.top = 0;
		m_scissorRect.right = static_cast<LONG>(width);
		m_scissorRect.bottom = static_cast<LONG>(height);

		m_pCbvDataBegin = nullptr;
		m_constantBufferData = {};

		m_camera.horizontalAngle = 0.0f;
		m_camera.verticalAngle = 0.0f;
		m_camera.mouseLock = false;
		m_camera.fov = XM_PIDIV4;

		RenderObjects.push_back(&DefaultCube);
		RenderObjects.push_back(&DefaultLineRenderer);

		timer.Restart();


#pragma warning(disable: 4996) // Disable function or variable may be unsafe.
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
#pragma warning(default: 4996)
	}

	GraphicsApplication::~GraphicsApplication()
	{
		//CleanupDevice();
	}

	void GraphicsApplication::GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
	{
		*ppAdapter = nullptr;
		for (UINT adapterIndex = 0; ; ++adapterIndex)
		{
			IDXGIAdapter1* pAdapter = nullptr;
			if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter))
			{
				// No more adapters to enumerate.
				break;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				*ppAdapter = pAdapter;
				return;
			}
			pAdapter->Release();
		}
	}

	HRESULT GraphicsApplication::InitializeDevice()
	{
		HRESULT hr;

		// Get the debug interface and enable the debug layer
#if defined _DEBUG
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(m_debugController.GetAddressOf()));
		if (FAILED(hr))
		{
			std::cout << "Didn't get Debug Interface.\n";
			return E_FAIL;
		}
		m_debugController->EnableDebugLayer();
#endif

		CreateDevice();
		
		CreateCommandQueue();
		
		CreateSwapchain();
		
		CreateDescriptorHeaps();
		
		CreateFrameResources();

		CreateFrameAllocator();
		
		LoadAssets();

		return S_OK;
	}

	HRESULT GraphicsApplication::LoadAssets()
	{
		// Load Assets
		
		LoadMesh(DefaultCube.mesh, DefaultCube.animation);

		DefaultLineRenderer.animation = DefaultCube.animation;

		CreateRootSignature();

		CreateShaders();

		CreatePipelineStates();

		CreateCommandList();

		CreateVertexBuffers();

		CreateIndexBuffers();
		
		CreateInstanceBuffers();

		CreateTextures();

		ExecuteCommandList();

		CreateConstantBuffers();

		SetupDepthStencil();

		

		// Create synchronization objects and wait until assets have been uploaded to the GPU.
		{
			HRESULT hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the fence \n";
				return E_FAIL;
			}
			m_fenceValue = 1;

			// Create an event handle to use for frame synchronization.
			m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (m_fenceEvent == nullptr)
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
				if (FAILED(hr))
				{
					std::cout << "Failed to create the event handle \n";
					return E_FAIL;
				}
			}

			// Wait for the command list to execute; we are reusing the same command 
			// list in our main loop but for now, we just want to wait for setup to 
			// complete before continuing.
			WaitForPreviousFrame();

		}

		return S_OK;
	}

	void GraphicsApplication::CleanupDevice()
	{
		// Wait for the GPU to be done with all resources.
		WaitForPreviousFrame();

		CloseHandle(m_fenceEvent);
		m_swapChain.Reset();
		m_device.Reset();
		for (int i = 0; i < FrameCount; ++i)
		{
			m_renderTargets[i].Reset();
		}
		m_commandAllocator.Reset();
		m_commandQueue.Reset();
		m_rootSignature.Reset();
		m_rtvHeap.Reset();
		m_commandList.Reset();
		m_constantBuffer.Reset();
		m_cbvHeap.Reset();
		m_depthStencil.Reset();
		m_dsvHeap.Reset();
		m_fence.Reset();

#if defined _DEBUG
		m_debugController.Reset();
#endif
	}

	void GraphicsApplication::Update()
	{
		timer.Signal();
		static float speed = 1.5f;

		XMFLOAT4 lightPos = m_constantBufferData.Lights[0].Position;

		// Check for input
		if ((GetAsyncKeyState((SHORT)'W') & 0xF000))
		{
			// Move forward
			m_camera.cameraTranslation.z = timer.Delta() * speed;
		}
		if ((GetAsyncKeyState((SHORT)'S') & 0xF000))
		{
			// Move backwards
			m_camera.cameraTranslation.z = -timer.Delta() * speed;
		}
		if ((GetAsyncKeyState((SHORT)'A') & 0xF000))
		{
			m_camera.cameraTranslation.x = -timer.Delta() * speed;
		}
		if ((GetAsyncKeyState((SHORT)'D') & 0xF000))
		{
			m_camera.cameraTranslation.x = timer.Delta() * speed;
		}
		if ((GetAsyncKeyState(VK_SPACE) & 0xF000))
		{
			// Move up
			m_camera.cameraTranslation.y = timer.Delta() * speed;
		}
		if ((GetAsyncKeyState(VK_LSHIFT) & 0xF000))
		{
			// Move down
			m_camera.cameraTranslation.y = -timer.Delta() * speed;
		}
		if ((GetAsyncKeyState(VK_LEFT) & 0xF000))
		{
			lightPos.x += -timer.Delta() * speed;
		}
		if ((GetAsyncKeyState(VK_RIGHT) & 0xF000))
		{
			lightPos.x += timer.Delta() * speed;
		}
		if ((GetAsyncKeyState(VK_UP) & 0xF000))
		{
			// Move up
			lightPos.z += -timer.Delta() * speed;
		}
		if ((GetAsyncKeyState(VK_DOWN) & 0xF000))
		{
			// Move down
			lightPos.z += timer.Delta() * speed;
		}
		if ((GetAsyncKeyState(SHORT('O')) & 0xF000))
		{
			// Move up
			m_constantBufferData.Lights[0].LightPower += -timer.Delta() * speed;
		}
		if ((GetAsyncKeyState(SHORT('P')) & 0xF000))
		{
			// Move down
			m_constantBufferData.Lights[0].LightPower += timer.Delta() * speed;
		}
		if (GetAsyncKeyState('Q') & 0x1)
		{
			// Toggle lights
			m_constantBufferData.Lights[0].Enabled = !m_constantBufferData.Lights[0].Enabled;
			m_constantBufferData.Lights[1].Enabled = !m_constantBufferData.Lights[1].Enabled;
			m_constantBufferData.Lights[2].Enabled = !m_constantBufferData.Lights[2].Enabled;
		}
		static bool escDownLastFrame = false;
		if ((GetAsyncKeyState('M') & 0xF000) && !escDownLastFrame)
		{
			m_camera.mouseLock = !m_camera.mouseLock;
			escDownLastFrame = true;
			if (m_camera.mouseLock)
			{
				POINT p = { m_windowWidth / 2, m_windowHeight / 2 };
				ClientToScreen(m_window, &p);
				SetCursorPos(p.x, p.y);
			}
		}
		else if (!(GetAsyncKeyState('M') & 0xF000) && escDownLastFrame)
		{
			escDownLastFrame = false;
		}

		m_projection = XMMatrixPerspectiveFovLH(m_camera.fov, m_windowWidth / (FLOAT)m_windowHeight, 0.1f, 100.0f);


		if (m_camera.mouseLock)
		{
			GetCursorPos(&m_camera.mousePos);
			ScreenToClient(m_window, &m_camera.mousePos);
			POINT p = { m_windowWidth / 2, m_windowHeight / 2 };
			ClientToScreen(m_window, &p);
			SetCursorPos(p.x, p.y);

			m_camera.horizontalAngle -= 0.075f * timer.Delta() * static_cast<float>(m_windowWidth / 2 - m_camera.mousePos.x);
			m_camera.verticalAngle -= 0.075f * timer.Delta() * static_cast<float>(m_windowHeight / 2 - m_camera.mousePos.y);
			// Clamp y to top and bottom
			m_camera.verticalAngle = m_camera.verticalAngle > XM_PIDIV2 ? m_camera.verticalAngle = XM_PIDIV2 : m_camera.verticalAngle < -XM_PIDIV2 ? -XM_PIDIV2 : m_camera.verticalAngle;
		}

		m_constantBufferData.World = XMMatrixTranspose(m_world);
		m_MVP.World = m_world;

		XMFLOAT4 temp = {};
		XMStoreFloat4(&temp, m_camera.matrix.r[3]);
		temp.y += m_camera.cameraTranslation.y;
		m_camera.matrix = XMMatrixRotationY(m_camera.horizontalAngle);
		m_camera.matrix = XMMatrixRotationX(m_camera.verticalAngle) * m_camera.matrix;
		m_camera.matrix.r[3] = XMLoadFloat4(&temp);
		m_camera.matrix = XMMatrixTranslation(m_camera.cameraTranslation.x, 0.0f, m_camera.cameraTranslation.z) * m_camera.matrix;
		m_camera.cameraTranslation = { 0.0f, 0.0f, 0.0f };

		m_MVP.View = m_view * XMMatrixInverse(nullptr, m_camera.matrix);

		m_MVP.Projection = m_projection;

		XMStoreFloat4(&m_constantBufferData.EyePosition, XMMatrixInverse(nullptr, m_MVP.View).r[3]);

		//std::cout << "X: " << m_constantBufferData.EyePosition.x << " Y: " << m_constantBufferData.EyePosition.y << " Z: " << m_constantBufferData.EyePosition.z << '\n';

		m_constantBufferData.Lights[0].Position = lightPos;

		m_constantBufferData.InverseTransposeWorldMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, m_constantBufferData.World));
		m_constantBufferData.MVP = XMMatrixTranspose(XMMatrixMultiply(XMMatrixMultiply(m_MVP.World, m_MVP.View), m_MVP.Projection));
		memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));


		hue += timer.Delta() * 0.1f;
		if (hue > 1.0f)
			hue = 0.0f;
		XMVECTOR c = XMColorHSVToRGB({ hue, 1.0f, 1.0f, 1.0f });

		XMFLOAT4 my_color;
		my_color.x = XMVectorGetX(c) * 0.65f;
		my_color.y = XMVectorGetY(c) * 0.65f;
		my_color.z = XMVectorGetZ(c) * 0.65f;
		my_color.w = XMVectorGetW(c) * 0.65f;

		DebugRenderer::clear_lines();

		for (float i = -10.0f; i < 10.5f; i += 1.0f)
		{
			DebugRenderer::add_line({ -10.0f, 0.0f, i, 1.0f }, { 10.0f, 0.0f, i, 1.0f }, my_color);
			DebugRenderer::add_line({ i, 0.0f, -10.0f, 1.0f }, { i, 0.0f, 10.0f, 1.0f }, my_color);
		}

		static int frame = 0;
		if ((GetAsyncKeyState(SHORT('B')) & 0x1))
		{
			frame--;
			if (frame < 0)
			{
				frame = DefaultLineRenderer.animation.keyframes.size() - 1;
			}
		}
		if ((GetAsyncKeyState(SHORT('N')) & 0x1))
		{
			frame++;
			if (frame > DefaultLineRenderer.animation.keyframes.size() - 1)
			{
				frame = 0;
			}
		}
		if ((GetAsyncKeyState(SHORT('V')) & 0x1))
		{
			DefaultLineRenderer.animation.enabled = !DefaultLineRenderer.animation.enabled;
		}

		if (!DefaultLineRenderer.animation.enabled) // not animating
		{
			XMVECTOR Location;
			XMFLOAT4 position;
			XMFLOAT4 xOffset;
			XMFLOAT4 yOffset;
			XMFLOAT4 zOffset;
			XMFLOAT4 scaledDown;
			XMMATRIX transform;
			float lineLength = 0.25f;

			for (int i = 0; i < DefaultLineRenderer.animation.bindPose.size(); i++)
			{
				transform = XMLoadFloat4x4(&DefaultLineRenderer.animation.bindPose[i].transform);
				Location = transform.r[3];
				//transform = XMMatrixTranspose(transform);

				XMStoreFloat4(&position, Location);
				XMStoreFloat4(&xOffset, transform.r[0]);
				XMStoreFloat4(&yOffset, transform.r[1]);
				XMStoreFloat4(&zOffset, transform.r[2]);

				scaledDown = Float4Add(position, Float4MultiplyFloat(xOffset, lineLength));

				DebugRenderer::add_line(position, scaledDown, Color::Red);

				scaledDown = Float4Add(position, Float4MultiplyFloat(yOffset, lineLength));

				DebugRenderer::add_line(position, scaledDown, Color::Green);

				scaledDown = Float4Add(position, Float4MultiplyFloat(zOffset, lineLength));

				DebugRenderer::add_line(position, scaledDown, Color::Blue);

				if (DefaultLineRenderer.animation.bindPose[i].parentIndex > -1)
				{
					float p_x = DefaultLineRenderer.animation.bindPose[DefaultLineRenderer.animation.bindPose[i].parentIndex].transform.m[3][0];
					float p_y = DefaultLineRenderer.animation.bindPose[DefaultLineRenderer.animation.bindPose[i].parentIndex].transform.m[3][1];
					float p_z = DefaultLineRenderer.animation.bindPose[DefaultLineRenderer.animation.bindPose[i].parentIndex].transform.m[3][2];
					float p_w = DefaultLineRenderer.animation.bindPose[DefaultLineRenderer.animation.bindPose[i].parentIndex].transform.m[3][3];
					DebugRenderer::add_line({ position.x, position.y, position.z, position.w }, { p_x, p_y, p_z, p_w }, Color::White);
				}
			}

		}
		else // animating
		{
			// find a key start
			DefaultLineRenderer.animation.currentTime += timer.Delta();
			//std::cout << "Time : " << DefaultLineRenderer.animation.currentTime << "\nOutO : " << DefaultLineRenderer.animation.duration << '\n';
			if (DefaultLineRenderer.animation.currentTime > DefaultLineRenderer.animation.duration)
			{
				DefaultLineRenderer.animation.currentTime -= DefaultLineRenderer.animation.duration;
			}

			for (int i = 0; i < DefaultLineRenderer.animation.keyframes.size(); i++)
			{
				if (DefaultLineRenderer.animation.keyframes[i].keytime > DefaultLineRenderer.animation.currentTime)
				{
					frame = i;
					break;
				}
			}

			// get the 0 to 1 ratio for the two frames.
			int previousFrame = frame - 1;
			double delta;
			double t = DefaultLineRenderer.animation.currentTime;
			double t2 = DefaultLineRenderer.animation.keyframes[frame].keytime;
			if (previousFrame < 0)
			{
				previousFrame += DefaultLineRenderer.animation.keyframes.size();
				double t1 = DefaultLineRenderer.animation.keyframes[previousFrame].keytime;
				t2 = DefaultLineRenderer.animation.duration + DefaultLineRenderer.animation.keyframes[frame].keytime;
				delta = (t - t1) / (t2 - t1);//r = (t-t1) / (t2-t1)
			}
			else
			{
				double t1 = DefaultLineRenderer.animation.keyframes[previousFrame].keytime;
				delta = (t - t1) / (t2 - t1);//r = (t-t1) / (t2-t1)
			}

			// now interpolate the frames.
			//std::cout << delta << '\n';


			XMFLOAT4 positionA;
			XMFLOAT4 positionB;
			XMFLOAT4 position;
			XMFLOAT4 xOffset;
			XMFLOAT4 yOffset;
			XMFLOAT4 zOffset;
			XMFLOAT4 scaledDown;
			XMMATRIX transformA;
			XMMATRIX transformB;
			XMMATRIX transform;
			XMVECTOR quatA;
			XMVECTOR quatB;
			float lineLength = 0.25f;

			for (int i = 0; i < DefaultLineRenderer.animation.keyframes[frame].poseData.size(); i++)
			{
				transformA = XMLoadFloat4x4(&DefaultLineRenderer.animation.keyframes[previousFrame].poseData[i].transform);
				transformB = XMLoadFloat4x4(&DefaultLineRenderer.animation.keyframes[frame].poseData[i].transform);
				//transform = XMMatrixTranspose(transform);

				XMStoreFloat4(&positionA, transformA.r[3]);
				XMStoreFloat4(&positionB, transformB.r[3]);

				position = Float4Lerp(positionA, positionB, delta);

				quatA = XMQuaternionRotationMatrix(transformA);
				quatB = XMQuaternionRotationMatrix(transformB);

				quatA = XMQuaternionSlerp(quatA, quatB, delta);

				transform = XMMatrixRotationQuaternion(quatA);

				XMStoreFloat4(&xOffset, transform.r[0]);
				xOffset.w = 0.0f;
				XMStoreFloat4(&yOffset, transform.r[1]);
				yOffset.w = 0.0f;
				XMStoreFloat4(&zOffset, transform.r[2]);
				zOffset.w = 0.0f;

				scaledDown = Float4Add(position, Float4MultiplyFloat(xOffset, lineLength));

				DebugRenderer::add_line(position, scaledDown, Color::Red);

				scaledDown = Float4Add(position, Float4MultiplyFloat(yOffset, lineLength));

				DebugRenderer::add_line(position, scaledDown, Color::Green);

				scaledDown = Float4Add(position, Float4MultiplyFloat(zOffset, lineLength));

				DebugRenderer::add_line(position, scaledDown, Color::Blue);

				if (DefaultLineRenderer.animation.bindPose[i].parentIndex > -1)
				{
					float p_x = DefaultLineRenderer.animation.keyframes[frame].poseData[DefaultLineRenderer.animation.keyframes[frame].poseData[i].parentIndex].transform.m[3][0];
					float p_y = DefaultLineRenderer.animation.keyframes[frame].poseData[DefaultLineRenderer.animation.keyframes[frame].poseData[i].parentIndex].transform.m[3][1];
					float p_z = DefaultLineRenderer.animation.keyframes[frame].poseData[DefaultLineRenderer.animation.keyframes[frame].poseData[i].parentIndex].transform.m[3][2];
					float p_w = DefaultLineRenderer.animation.keyframes[frame].poseData[DefaultLineRenderer.animation.keyframes[frame].poseData[i].parentIndex].transform.m[3][3];
					DebugRenderer::add_line({ position.x, position.y, position.z, position.w }, { p_x, p_y, p_z, p_w }, Color::White);
				}
			}
		}

		D3D12_RANGE readRange;	// We do not intend to read from this resource on the CPU.
		readRange.Begin = 0;
		readRange.End = 0;
		HRESULT hr = DefaultLineRenderer.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pDebugVertexDataBegin));
		if (FAILED(hr))
		{
			std::cout << "Failed to map the vertex buffer. \n";
			exit(hr);
		}
		memcpy(m_pDebugVertexDataBegin, DebugRenderer::get_line_verts(), DebugRenderer::get_line_vert_count() * sizeof(Vertex));
		DefaultLineRenderer.vertexBuffer->Unmap(0, nullptr);
	}

	void GraphicsApplication::Render()
	{
		// Record all the commands we need to render the scene into the command list.
		PopulateCommandList();

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Present the frame.
		if (FAILED(m_swapChain->Present(1, 0)))
			std::cout << "Presenting failed\n";

		WaitForPreviousFrame();
	}

	void GraphicsApplication::PopulateCommandList()
	{
		HRESULT hr;
		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		hr = m_commandAllocator->Reset();
		if (FAILED(hr))
		{
			exit(hr);
		}

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		hr = m_commandList->Reset(m_commandAllocator.Get(), RenderObjects[0]->pipelineState.Get());
		if (FAILED(hr))
		{
			exit(hr);
		}
		// Set necessary state.
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_cbvDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(1, cbvHandle);
		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);
		//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), 2, m_cbvDescriptorSize);
		//m_commandList->SetGraphicsRootDescriptorTable(2, cbvHandle);
		//m_commandList->RSSetViewports(2, &m_viewport);
		//m_commandList->RSSetScissorRects(2, &m_scissorRect);
		//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), 3, m_cbvDescriptorSize);
		//m_commandList->SetGraphicsRootDescriptorTable(3, cbvHandle);
		//m_commandList->RSSetViewports(3, &m_viewport);
		//m_commandList->RSSetScissorRects(3, &m_scissorRect);
		// Set constant buffer

		//ID3D12DescriptorHeap* ppHeaps1[] = { m_srvHeap.Get() };
		//m_commandList->SetDescriptorHeaps(_countof(ppHeaps1), ppHeaps1);

		//m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

		D3D12_RESOURCE_BARRIER resourceBarrier;
		resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE; //
		resourceBarrier.Transition = { m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET };
		// Indicate that the back buffer will be used as a render target.
		//m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		m_commandList->ResourceBarrier(1, &resourceBarrier);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += ((INT64)m_frameIndex * m_rtvDescriptorSize);
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);


		// Record commands.
		m_commandList->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);

		m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		m_commandList->IASetPrimitiveTopology(RenderObjects[0]->PrimitiveTopology);
		m_commandList->SetPipelineState(RenderObjects[0]->pipelineState.Get());
		m_commandList->IASetVertexBuffers(0, 1, &RenderObjects[0]->vertexBufferView);
		m_commandList->IASetVertexBuffers(1, 1, &RenderObjects[0]->instanceBufferView);
		m_commandList->IASetIndexBuffer(&RenderObjects[0]->indexBufferView);
		
		m_commandList->DrawIndexedInstanced(RenderObjects[0]->mesh.indices.size(), 1, 0, 0, 0);


		m_commandList->IASetPrimitiveTopology(DefaultLineRenderer.PrimitiveTopology);
		m_commandList->SetPipelineState(DefaultLineRenderer.pipelineState.Get());
		m_commandList->IASetVertexBuffers(0, 1, &DefaultLineRenderer.vertexBufferView);
		m_commandList->IASetVertexBuffers(1, 1, &DefaultLineRenderer.instanceBufferView);
		m_commandList->DrawInstanced(DebugRenderer::get_line_vert_count(), 1, 0, 0);

		// Indicate that the back buffer will now be used to present.
		D3D12_RESOURCE_BARRIER resourceBarrier1;
		resourceBarrier1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		resourceBarrier1.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		resourceBarrier1.Transition = { m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT };
		m_commandList->ResourceBarrier(1, &resourceBarrier1);

		hr = m_commandList->Close();
		if (FAILED(hr))
		{
			exit(hr);
		}
	}

	void GraphicsApplication::WaitForPreviousFrame()
	{
		// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
		// This is code implemented as such for simplicity. More advanced samples 
		// illustrate how to use fences for efficient resource usage.

		// Signal and increment the fence value.
		const UINT64 fence = m_fenceValue;
		HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fence);
		if (FAILED(hr))
		{
			exit(hr);
		}
		m_fenceValue++;

		// Wait until the previous frame is finished.
		if (m_fence->GetCompletedValue() < fence)
		{
			hr = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
			if (FAILED(hr))
			{
				exit(hr);
			}
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	void GraphicsApplication::LoadMesh(GraphicsApplication::Mesh& mesh, GraphicsApplication::Animation& animation)
	{
		GraphicsApplication::InputMesh inputMesh;
		std::string meshFileName;
		do {

			meshFileName = OpenFileName(L"Morales Binary Mesh Files (*.mbm)\0*.mbm*\0", NULL);

		} while (meshFileName.empty());

		std::fstream file; //{ meshFileName, std::ios_base::in | std::ios_base::binary };
		file.open(meshFileName, std::ios_base::in | std::ios_base::binary);
		assert(file.is_open());

		uint32_t player_index_count;
		uint32_t player_vertex_count;
		uint32_t player_material_count;
		uint32_t player_material_path_count;
		uint32_t bindpose_joint_count;
		uint32_t joint_count;
		uint32_t frame_count;

		// Read the index count and all the indices.

		file.read((char*)&player_index_count, sizeof(uint32_t));

		inputMesh.indices.resize(player_index_count);

		file.read((char*)inputMesh.indices.data(), sizeof(uint32_t) * player_index_count);

		// Read the veretx count and all the vertices.

		file.read((char*)&player_vertex_count, sizeof(uint32_t));

		inputMesh.vertices.resize(player_vertex_count);

		file.read((char*)inputMesh.vertices.data(), sizeof(InputVertex) * player_vertex_count);

		// Read the material count and all the materials.

		file.read((char*)&player_material_count, sizeof(uint32_t));

		mesh.materials.resize(player_material_count);

		file.read((char*)mesh.materials.data(), sizeof(Material) * player_material_count);

		// Read the material path count and all the material paths.

		file.read((char*)&player_material_path_count, sizeof(uint32_t));

		mesh.materialPaths.resize(player_material_path_count);

		for (uint32_t i = 0; i < player_material_path_count; i++)
		{
			// Read each path separately.
			uint32_t path_size;
			file.read((char*)&path_size, sizeof(uint32_t));
			char* buffer = new char[path_size + 1];
			file.read(buffer, path_size * sizeof(char));
			buffer[path_size] = '\0';
			mesh.materialPaths[i] = std::string(buffer);
			delete[] buffer;

		}

		// Read the animation data
		file.read((char*)&bindpose_joint_count, sizeof(uint32_t));
		animation.bindPose.resize(bindpose_joint_count);
		vector<InputJoint> input_joints;
		input_joints.resize(bindpose_joint_count);
		file.read((char*)input_joints.data(), sizeof(Joint) * bindpose_joint_count);

		for (int i = 0; i < input_joints.size(); i++)
		{
			animation.bindPose[i].parentIndex = input_joints[i].parentIndex;

			animation.bindPose[i].transform.m[0][0] = input_joints[i].transform[0];
			animation.bindPose[i].transform.m[0][1] = input_joints[i].transform[1];
			animation.bindPose[i].transform.m[0][2] = input_joints[i].transform[2];
			animation.bindPose[i].transform.m[0][3] = input_joints[i].transform[3];

			animation.bindPose[i].transform.m[1][0] = input_joints[i].transform[4];
			animation.bindPose[i].transform.m[1][1] = input_joints[i].transform[5];
			animation.bindPose[i].transform.m[1][2] = input_joints[i].transform[6];
			animation.bindPose[i].transform.m[1][3] = input_joints[i].transform[7];

			animation.bindPose[i].transform.m[2][0] = input_joints[i].transform[8];
			animation.bindPose[i].transform.m[2][1] = input_joints[i].transform[9];
			animation.bindPose[i].transform.m[2][2] = input_joints[i].transform[10];
			animation.bindPose[i].transform.m[2][3] = input_joints[i].transform[11];

			animation.bindPose[i].transform.m[3][0] = input_joints[i].transform[12];
			animation.bindPose[i].transform.m[3][1] = input_joints[i].transform[13];
			animation.bindPose[i].transform.m[3][2] = input_joints[i].transform[14];
			animation.bindPose[i].transform.m[3][3] = input_joints[i].transform[15];
		}

		file.read((char*)&animation.duration, sizeof(double));

		file.read((char*)&joint_count, sizeof(uint32_t));
		file.read((char*)&frame_count, sizeof(uint32_t));

		animation.keyframes.resize(frame_count);
		for (uint32_t i = 0; i < frame_count; i++)
		{
			file.read((char*)&animation.keyframes[i].keytime, sizeof(double));
			animation.keyframes[i].poseData.resize(joint_count);
			input_joints.clear();
			input_joints.resize(joint_count);
			file.read((char*)input_joints.data(), sizeof(Joint) * joint_count);

			for (int j = 0; j < input_joints.size(); j++)
			{
				animation.keyframes[i].poseData[j].parentIndex = input_joints[j].parentIndex;
						  
				animation.keyframes[i].poseData[j].transform.m[0][0] = input_joints[j].transform[0];
				animation.keyframes[i].poseData[j].transform.m[0][1] = input_joints[j].transform[1];
				animation.keyframes[i].poseData[j].transform.m[0][2] = input_joints[j].transform[2];
				animation.keyframes[i].poseData[j].transform.m[0][3] = input_joints[j].transform[3];
						  															
				animation.keyframes[i].poseData[j].transform.m[1][0] = input_joints[j].transform[4];
				animation.keyframes[i].poseData[j].transform.m[1][1] = input_joints[j].transform[5];
				animation.keyframes[i].poseData[j].transform.m[1][2] = input_joints[j].transform[6];
				animation.keyframes[i].poseData[j].transform.m[1][3] = input_joints[j].transform[7];
						  															
				animation.keyframes[i].poseData[j].transform.m[2][0] = input_joints[j].transform[8];
				animation.keyframes[i].poseData[j].transform.m[2][1] = input_joints[j].transform[9];
				animation.keyframes[i].poseData[j].transform.m[2][2] = input_joints[j].transform[10];
				animation.keyframes[i].poseData[j].transform.m[2][3] = input_joints[j].transform[11];
						  															
				animation.keyframes[i].poseData[j].transform.m[3][0] = input_joints[j].transform[12];
				animation.keyframes[i].poseData[j].transform.m[3][1] = input_joints[j].transform[13];
				animation.keyframes[i].poseData[j].transform.m[3][2] = input_joints[j].transform[14];
				animation.keyframes[i].poseData[j].transform.m[3][3] = input_joints[j].transform[15];
			}
		}

		file.close();

		mesh.indices.resize(player_index_count);
		mesh.vertices.resize(player_vertex_count);

		// Example mesh conditioning if needed - this flips handedness
		for (auto& v : inputMesh.vertices)
		{
			v.position.x = -v.position.x;
			//v.position.y += 0.5f; // TODO: Make this not hardcoded
			v.normal.x = -v.normal.x;
			v.tex.y = 1.0f - v.tex.y;
		}

		int tri_count = (int)(inputMesh.indices.size() / 3);

		for (int i = 0; i < tri_count; ++i)
		{
			int* tri = inputMesh.indices.data() + i * 3;

			int temp = tri[0];
			tri[0] = tri[2];
			tri[2] = temp;
		}

		for (int i = 0; i < inputMesh.vertices.size(); i++)
		{
			mesh.vertices[i].position.x = inputMesh.vertices[i].position.x;
			mesh.vertices[i].position.y = inputMesh.vertices[i].position.y;
			mesh.vertices[i].position.z = inputMesh.vertices[i].position.z;
			mesh.vertices[i].position.w = 1.0f; //inputMesh.vertices[i].position.w; //HACK for now, mesh doesn't have w values appearantly.
			mesh.vertices[i].normal.x = inputMesh.vertices[i].normal.x;
			mesh.vertices[i].normal.y = inputMesh.vertices[i].normal.y;
			mesh.vertices[i].normal.z = inputMesh.vertices[i].normal.z;
			mesh.vertices[i].normal.w = inputMesh.vertices[i].normal.w;
			//mesh.vertices[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
			mesh.vertices[i].tex.x = inputMesh.vertices[i].tex.x;
			mesh.vertices[i].tex.y = inputMesh.vertices[i].tex.y;
		}
		for (int i = 0; i < inputMesh.indices.size(); i++)
		{
			mesh.indices[i] = inputMesh.indices[i];
		}



		std::cout << "File Loaded\n";
	}

	bool GraphicsApplication::CreateDevice()
	{
		// Create the DXGI factory and D3D12Device
		HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			std::cout << "Failed to create the IDXGI Factory 4\n";
			exit(hr);
		}

		// We don't instruct to use a WARP device
		
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		hr = D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
		if (FAILED(hr))
		{
			std::cout << "Failed to create the D3D12 Device\n";
			exit(hr);
		}

		return true;
	}

	bool GraphicsApplication::CreateCommandQueue()
	{
		// Create the Command Queue
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		HRESULT hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
		if (FAILED(hr))
		{
			std::cout << "Failed to create the Command Queue\n";
			exit(hr);
		}

		return true;
	}

	bool GraphicsApplication::CreateSwapchain()
	{
		// Create the Swap Chain
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = m_windowWidth;
		swapChainDesc.Height = m_windowHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = 0u;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		ComPtr<IDXGISwapChain1> swapChain;
		HRESULT hr = factory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_window, &swapChainDesc, &fsSwapChainDesc, nullptr, swapChain.GetAddressOf());
		if (FAILED(hr))
		{
			std::cout << "Failed to create the Swap Chain\n";
			exit(hr);
		}

		hr = swapChain.As(&m_swapChain); if (FAILED(hr))
		{
			std::cout << "Failed to create the Swap Chain\n";
			exit(hr);
		}

		return true;
	}

	bool GraphicsApplication::CreateDescriptorHeaps()
	{
		// Create the Heap Descriptor
		// Create descriptor heaps.
		{
			// Describe and create a render target view (RTV) descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = FrameCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the Descriptor Heap\n";
				exit(hr);
			}


			// Describe and create a constant buffer view (CBV) descriptor heap and a srv heap.
			D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
			cbvHeapDesc.NumDescriptors = 4;
			cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			cbvHeapDesc.NodeMask = 0;
			hr = m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the Descriptor Heap\n";
				exit(hr);
			}

			m_cbvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			dsvHeapDesc.NodeMask = 0;
			hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the Descriptor Heap\n";
				exit(hr);
			}

			//D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			//srvHeapDesc.NumDescriptors = 1;
			//srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			//srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			//hr = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));
			//if (FAILED(hr))
			//{
			//	std::cout << "Failed to create the Descriptor Heap\n";
			//	return E_FAIL;
			//}

			m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			//D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
			//samplerHeapDesc.NumDescriptors = 1;
			//samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			//samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			//samplerHeapDesc.NodeMask = 0;
			//hr = m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerHeap));
			//if (FAILED(hr))
			//{
			//	std::cout << "Failed to create the Descriptor Heap\n";
			//	return E_FAIL;
			//}
		}

		return true;
	}

	bool GraphicsApplication::CreateFrameResources()
	{
		// Create frame resources.
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

			// Create a RTV for each frame.
			for (UINT n = 0; n < FrameCount; n++)
			{
				HRESULT hr = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
				if (FAILED(hr))
				{
					std::cout << "Failed to create the Frame Resources\n";
					exit(hr);
				}
				m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
				rtvHandle.ptr += (1 * m_rtvDescriptorSize);
				//rtvHandle.ptr = 1 * m_rtvDescriptorSize; //TODO: Check this out when more info is found on descriptor handles.
			}
		}

		return true;
	}

	bool GraphicsApplication::CreateFrameAllocator()
	{
		
		// Create the command allocator.
		HRESULT hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
		if (FAILED(hr))
		{
			std::cout << "Failed to create the Command Allocator\n";
			exit(hr);
		}

		return true;
	}

	bool GraphicsApplication::CreateRootSignature()
	{
		// Check what feature support level and assign accordingly.
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
		if (FAILED(hr))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// Create a root signature that has a descriptor table with one CBV and one SRV.
		{
			CD3DX12_DESCRIPTOR_RANGE1 ranges[2] = {};
			CD3DX12_ROOT_PARAMETER1 rootParameters[2] = {};

			ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			ranges[0].NumDescriptors = 1;
			ranges[0].BaseShaderRegister = 0;
			ranges[0].RegisterSpace = 0;
			ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
			ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
			//ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

			rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
			rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
			//rootParameters[2].(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

			//rootParameters[1].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_PIXEL);
			//rootParameters[2].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);


			//rootParameters[1].InitAsShaderResourceView(0);

			D3D12_STATIC_SAMPLER_DESC sampler = {};
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.MipLODBias = 0;
			sampler.MaxAnisotropy = 0;
			sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			sampler.MinLOD = 0.0f;
			sampler.MaxLOD = D3D12_FLOAT32_MAX;
			sampler.ShaderRegister = 0;
			sampler.RegisterSpace = 0;
			sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // TODO: Check this if crash bc the ps cannot access root signature.

			D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;

			rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
			rootSignatureDesc.Desc_1_1.NumParameters = _countof(rootParameters);
			rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
			rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
			rootSignatureDesc.Desc_1_1.pStaticSamplers = &sampler;
			rootSignatureDesc.Desc_1_1.Flags = rootSignatureFlags;

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error); // This cannot create a version 1_0 root sig, may be a problem
			if (FAILED(hr))
			{
				std::cout << "Failed to serialize the root signature. \n";
				exit(hr);
			}

			hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the root signature. \n";
				exit(hr);
			}
		}

		return true;
	}

	bool GraphicsApplication::CreatePipelineStates()
	{
		// Create the pipeline state, which includes loading shaders.
		{

			//CreateDDSTextureFromFile(m_device.Get(), L"../Macaw.dds", 0, false, nullptr, );
				// Define the vertex input layout.
			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "INSTANCEPOS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 0},
			};

			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.pRootSignature = m_rootSignature.Get();
			psoDesc.PS = { DefaultCube.pixelShaderByteCode, DefaultCube.pixelShaderByteCodeSize };
			psoDesc.VS = { DefaultCube.vertexShaderByteCode, DefaultCube.vertexShaderByteCodeSize };

			D3D12_RASTERIZER_DESC rasterizerDesc;
			rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
			rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
			rasterizerDesc.FrontCounterClockwise = FALSE;
			rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			rasterizerDesc.DepthClipEnable = TRUE;
			rasterizerDesc.MultisampleEnable = FALSE;
			rasterizerDesc.AntialiasedLineEnable = FALSE;
			rasterizerDesc.ForcedSampleCount = 0;
			rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			psoDesc.RasterizerState = rasterizerDesc;

			D3D12_BLEND_DESC blendDesc;
			blendDesc.AlphaToCoverageEnable = FALSE;
			blendDesc.IndependentBlendEnable = FALSE;
			blendDesc.RenderTarget[0].BlendEnable = FALSE;
			blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			psoDesc.BlendState = blendDesc;

			psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			psoDesc.DepthStencilState.DepthEnable = TRUE;
			psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
			psoDesc.DepthStencilState.BackFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			psoDesc.DepthStencilState.FrontFace = psoDesc.DepthStencilState.BackFace;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			DefaultCube.PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&DefaultCube.pipelineState));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the graphics pipeline state. \n";
				exit(hr);
			}
		}
		// Create the pipeline state, which includes loading shaders.
		{

			//CreateDDSTextureFromFile(m_device.Get(), L"../Macaw.dds", 0, false, nullptr, );
				// Define the vertex input layout.
			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "INSTANCEPOS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 0},

			};

			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.pRootSignature = m_rootSignature.Get();
			psoDesc.PS = { DefaultLineRenderer.pixelShaderByteCode, DefaultLineRenderer.pixelShaderByteCodeSize };
			psoDesc.VS = { DefaultLineRenderer.vertexShaderByteCode, DefaultLineRenderer.vertexShaderByteCodeSize };

			D3D12_RASTERIZER_DESC rasterizerDesc;
			rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
			rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
			rasterizerDesc.FrontCounterClockwise = FALSE;
			rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			rasterizerDesc.DepthClipEnable = TRUE;
			rasterizerDesc.MultisampleEnable = FALSE;
			rasterizerDesc.AntialiasedLineEnable = FALSE;
			rasterizerDesc.ForcedSampleCount = 0;
			rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			psoDesc.RasterizerState = rasterizerDesc;

			D3D12_BLEND_DESC blendDesc;
			blendDesc.AlphaToCoverageEnable = FALSE;
			blendDesc.IndependentBlendEnable = FALSE;
			blendDesc.RenderTarget[0].BlendEnable = FALSE;
			blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			psoDesc.BlendState = blendDesc;

			psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			psoDesc.DepthStencilState.DepthEnable = TRUE;
			psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
			psoDesc.DepthStencilState.BackFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			psoDesc.DepthStencilState.FrontFace = psoDesc.DepthStencilState.BackFace;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			DefaultLineRenderer.PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&DefaultLineRenderer.pipelineState));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the graphics pipeline state. \n";
				exit(hr);
			}
		}

		return true;
	}

	bool GraphicsApplication::CreateCommandList()
	{
		// Create the command list.
		HRESULT hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), DefaultCube.pipelineState.Get(), IID_PPV_ARGS(&m_commandList));
		if (FAILED(hr))
		{
			std::cout << "Failed to create the command list. \n";
			exit(hr);
		}

		return true;
	}

	bool GraphicsApplication::SetupDepthStencil()
	{
		{
			// Create descriptor heap for depth stencil view
			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.CreationNodeMask = 1;
			heapProperties.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC resourceDesc;
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resourceDesc.Width = m_windowWidth;
			resourceDesc.Height = m_windowHeight;
			resourceDesc.Alignment = 0;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			D3D12_CLEAR_VALUE clearValue = {};
			clearValue.Format = DXGI_FORMAT_D32_FLOAT;
			clearValue.DepthStencil.Depth = 1.0f;
			clearValue.DepthStencil.Stencil = 0;


			HRESULT hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
				IID_PPV_ARGS(&m_depthStencil));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the depth stencil\n";
				exit(hr);
			}



			D3D12_DEPTH_STENCIL_VIEW_DESC depthStencildesc = {};
			depthStencildesc.Format = DXGI_FORMAT_D32_FLOAT;
			depthStencildesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			depthStencildesc.Flags = D3D12_DSV_FLAG_NONE;

			m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencildesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		}
		return true;
	}

	bool GraphicsApplication::UpdateBuffers(RenderObject RO)
	{

		return true;
	}

	bool GraphicsApplication::CreateShaders()
	{
		{
			std::fstream fin;
			//char d[MAX_PATH];

			//GetCurrentDirectoryA(MAX_PATH, d);

			fin.open("../x64/Debug/BlinnPhongPixel.cso", std::ios_base::in | std::ios_base::binary);

			assert(fin.is_open());

			fin.seekg(0, fin.end);
			size_t length = fin.tellg();
			fin.seekg(0, fin.beg);

			DefaultCube.pixelShaderByteCode = new char[length];
			DefaultCube.pixelShaderByteCodeSize = length;

			fin.read(DefaultCube.pixelShaderByteCode, length);

			fin.close();

			fin.open("../x64/Debug/BlinnPhongVertex.cso", std::ios_base::in | std::ios_base::binary);

			assert(fin.is_open());

			fin.seekg(0, fin.end);
			length = fin.tellg();
			fin.seekg(0, fin.beg);

			DefaultCube.vertexShaderByteCode = new char[length];
			DefaultCube.vertexShaderByteCodeSize = length;

			fin.read(DefaultCube.vertexShaderByteCode, length);

			fin.close();
		}

		{
			std::fstream fin;
			//char d[MAX_PATH];

			//GetCurrentDirectoryA(MAX_PATH, d);

			fin.open("../x64/Debug/DebugLinePixel.cso", std::ios_base::in | std::ios_base::binary);

			assert(fin.is_open());

			fin.seekg(0, fin.end);
			size_t length = fin.tellg();
			fin.seekg(0, fin.beg);

			DefaultLineRenderer.pixelShaderByteCode = new char[length];
			DefaultLineRenderer.pixelShaderByteCodeSize = length;

			fin.read(DefaultLineRenderer.pixelShaderByteCode, length);

			fin.close();

			fin.open("../x64/Debug/DebugLineVertex.cso", std::ios_base::in | std::ios_base::binary);

			assert(fin.is_open());

			fin.seekg(0, fin.end);
			length = fin.tellg();
			fin.seekg(0, fin.beg);

			DefaultLineRenderer.vertexShaderByteCode = new char[length];
			DefaultLineRenderer.vertexShaderByteCodeSize = length;

			fin.read(DefaultLineRenderer.vertexShaderByteCode, length);

			fin.close();
		}

		return true;
	}

	bool GraphicsApplication::CreateConstantBuffers()
	{
		// Create descriptor heap for constant buffer
		{
			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.CreationNodeMask = 1;
			heapProperties.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC resourceDesc;
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = 1024 * 64;
			resourceDesc.Alignment = 0;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			HRESULT hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				IID_PPV_ARGS(&m_constantBuffer));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the descriptor heap for the constant buffer. \n";
				exit(hr);
			}

			// Create the constant buffer view
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = (sizeof(ConstantBuffer) + 255) & ~255;
			m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

			// Create the constant buffer
			D3D12_RANGE readRange;	// We do not intend to read from this resource on the CPU.
			readRange.Begin = 0;
			readRange.End = 0;
			hr = m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin));
			if (FAILED(hr))
			{
				std::cout << "Failed to map the constant buffer. \n";
				exit(hr);
			}
			memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

			// Create the shader resource view

			//m_device->CreateShaderResourceView();

			// camera
			m_camera.matrix = XMMatrixIdentity();

			m_camera.cameraTranslation = { 0.0f, 0.0f, 0.0f };

			// world
			m_world = XMMatrixIdentity();

			// view
			XMVECTOR Eye = { 0.0f, 3.0f, 10.0f, 0.0f };
			XMVECTOR At = { 0.0f, 2.5f, 0.0f, 0.0f };
			XMVECTOR Up = { 0.0f, 1.0f, 0.0f, 0.0f };
			m_view = XMMatrixLookAtLH(Eye, At, Up);

			// projection
			m_projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, m_windowWidth / (FLOAT)m_windowHeight, 0.01f, 100.0f);


			m_constantBufferData.GlobalAmbient = { 0.2f, 0.2f, 0.2f, 0.2f };

			m_constantBufferData.Material.Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
			m_constantBufferData.Material.AmbientFactor = 1.0f;
			m_constantBufferData.Material.SpecularPower = 3.0f;

			m_constantBufferData.Material.UseTexture = 1;

			m_constantBufferData.Material.Diffuse.x = DefaultCube.mesh.materials[0][Material::DIFFUSE].value[0];
			m_constantBufferData.Material.Diffuse.y = DefaultCube.mesh.materials[0][Material::DIFFUSE].value[1];
			m_constantBufferData.Material.Diffuse.z = DefaultCube.mesh.materials[0][Material::DIFFUSE].value[2];
			m_constantBufferData.Material.Diffuse.w = 1.0f;

			m_constantBufferData.Material.DiffuseFactor = DefaultCube.mesh.materials[0][Material::DIFFUSE].factor;

			m_constantBufferData.Material.Emissive.x = DefaultCube.mesh.materials[0][Material::EMISSIVE].value[0];
			m_constantBufferData.Material.Emissive.y = DefaultCube.mesh.materials[0][Material::EMISSIVE].value[1];
			m_constantBufferData.Material.Emissive.z = DefaultCube.mesh.materials[0][Material::EMISSIVE].value[2];
			m_constantBufferData.Material.Emissive.w = 1.0f;

			m_constantBufferData.Material.EmissiveFactor = DefaultCube.mesh.materials[0][Material::EMISSIVE].factor;

			m_constantBufferData.Material.Specular.x = DefaultCube.mesh.materials[0][Material::SPECULAR].value[0];
			m_constantBufferData.Material.Specular.y = DefaultCube.mesh.materials[0][Material::SPECULAR].value[1];
			m_constantBufferData.Material.Specular.z = DefaultCube.mesh.materials[0][Material::SPECULAR].value[2];
			m_constantBufferData.Material.Specular.w = 1.0f;

			m_constantBufferData.Material.SpecularFactor = DefaultCube.mesh.materials[0][Material::SPECULAR].factor;

			for (size_t i = 0; i < MAX_LIGHTS; i++)
			{
				m_constantBufferData.Lights[i].Enabled = 0;
			}

			m_constantBufferData.Lights[0].Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			m_constantBufferData.Lights[0].LightType = ShaderLightType::POINT_LIGHT;
			m_constantBufferData.Lights[0].Enabled = 1;
			m_constantBufferData.Lights[0].LightPower = 1.0f;

			m_constantBufferData.Lights[0].ConstantAttenuation = 1.0f;
			m_constantBufferData.Lights[0].LinearAttenuation = 0.2f;
			m_constantBufferData.Lights[0].QuadraticAttenuation = 0.1f;

			m_constantBufferData.Lights[0].Position = { -1.0f, 2.5f, 2.0f, 1.0f };

			m_constantBufferData.Lights[1].Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			m_constantBufferData.Lights[1].LightType = ShaderLightType::DIRECTIONAL_LIGHT;
			m_constantBufferData.Lights[1].Enabled = 1;
			m_constantBufferData.Lights[1].LightPower = 1.0f;
										
			m_constantBufferData.Lights[1].Direction = { 0.0f, -0.5f, 1.0f, 1.0f };

			m_constantBufferData.Lights[2].Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			m_constantBufferData.Lights[2].LightType = ShaderLightType::DIRECTIONAL_LIGHT;
			m_constantBufferData.Lights[2].Enabled = 1;
			m_constantBufferData.Lights[2].LightPower = 0.25f;
										
			m_constantBufferData.Lights[2].Direction = { 0.0f, -0.5f, -1.0f, 1.0f };

			//m_constantBufferData.vLightDir[0] = { -0.577f, 0.577f, -0.577f, 1.0f };
			//m_constantBufferData.vLightDir[1] = { 10.0f, 0.0f, 0.0f, 1.0f };
			//m_constantBufferData.vLightDir[2] = { 0.1f, 0.0f, -1.0f, 1.0f };
			//
			//m_constantBufferData.vLightPosition[0] = { 0.0f, 0.0f, 0.0f, 1.0f };
			//m_constantBufferData.vLightPosition[1] = { -1.0f, 2.5f, -2.0f, 1.0f };
			//m_constantBufferData.vLightPosition[2] = { 4.0f, 2.0f, 0.0f, 1.0f };
			//
			//m_constantBufferData.vLightColor[0] = { 0.5f, 0.5f, 0.5f, 1.0f };
			//m_constantBufferData.vLightColor[1] = { 1.0f, 1.0f, 1.0f, 1.0f };
			//m_constantBufferData.vLightColor[2] = { 0.0f, 1.0f, 0.0f, 1.0f };
			//
			//m_constantBufferData.vPointLightRadius = { 1.0f, 1.0f, 1.0f, 1.0f };
			//
			//m_constantBufferData.mDiffuseColor.x = DefaultCube.mesh.materials[0][Material::DIFFUSE].value[0];
			//m_constantBufferData.mDiffuseColor.y = DefaultCube.mesh.materials[0][Material::DIFFUSE].value[1];
			//m_constantBufferData.mDiffuseColor.z = DefaultCube.mesh.materials[0][Material::DIFFUSE].value[2];
			//m_constantBufferData.mDiffuseColor.w = DefaultCube.mesh.materials[0][Material::DIFFUSE].factor;
			//
			//m_constantBufferData.mEmissiveColor.x = DefaultCube.mesh.materials[0][Material::EMISSIVE].value[0];
			//m_constantBufferData.mEmissiveColor.y = DefaultCube.mesh.materials[0][Material::EMISSIVE].value[1];
			//m_constantBufferData.mEmissiveColor.z = DefaultCube.mesh.materials[0][Material::EMISSIVE].value[2];
			//m_constantBufferData.mEmissiveColor.w = DefaultCube.mesh.materials[0][Material::EMISSIVE].factor;
			//
			//m_constantBufferData.mSpecularColor.x = DefaultCube.mesh.materials[0][Material::SPECULAR].value[0];
			//m_constantBufferData.mSpecularColor.y = DefaultCube.mesh.materials[0][Material::SPECULAR].value[1];
			//m_constantBufferData.mSpecularColor.z = DefaultCube.mesh.materials[0][Material::SPECULAR].value[2];
			//m_constantBufferData.mSpecularColor.w = DefaultCube.mesh.materials[0][Material::SPECULAR].factor;
		}

		return true;
	}

	bool GraphicsApplication::CreateVertexBuffers()
	{
		// Create the vertex buffer.
		{
			// Define the geometry for a triangle.
			//Vertex vertices[] =
			//{
			//	{ XMFLOAT4(-1.0f, 1.0f, -1.0f, 1.0f),  XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),  XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(1.0f, 1.0f, -1.0f, 1.0f),   XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),  XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
			//	{ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),    XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),  XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(-1.0f, 1.0f, 1.0f, 1.0f),   XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),  XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },

			//	{ XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, -1.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(1.0f, -1.0f, -1.0f, 1.0f),  XMFLOAT4(0.0f, -1.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
			//	{ XMFLOAT4(1.0f, -1.0f, 1.0f, 1.0f),   XMFLOAT4(0.0f, -1.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(-1.0f, -1.0f, 1.0f, 1.0f),  XMFLOAT4(0.0f, -1.0f, 0.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },

			//	{ XMFLOAT4(-1.0f, -1.0f, 1.0f, 1.0f),  XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f), XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
			//	{ XMFLOAT4(-1.0f, 1.0f, -1.0f, 1.0f),  XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(-1.0f, 1.0f, 1.0f, 1.0f),   XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },

			//	{ XMFLOAT4(1.0f, -1.0f, 1.0f, 1.0f),   XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),  XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(1.0f, -1.0f, -1.0f, 1.0f),  XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),  XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
			//	{ XMFLOAT4(1.0f, 1.0f, -1.0f, 1.0f),   XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),  XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),    XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),  XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },

			//	{ XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(1.0f, -1.0f, -1.0f, 1.0f),  XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
			//	{ XMFLOAT4(1.0f, 1.0f, -1.0f, 1.0f),   XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(-1.0f, 1.0f, -1.0f, 1.0f),  XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },

			//	{ XMFLOAT4(-1.0f, -1.0f, 1.0f, 1.0f),  XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),  XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(1.0f, -1.0f, 1.0f, 1.0f),   XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),  XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
			//	{ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),    XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),  XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
			//	{ XMFLOAT4(-1.0f, 1.0f, 1.0f, 1.0f),   XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),  XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
			//};

			const UINT vertexBufferSize = DefaultCube.mesh.vertices.size() * sizeof(Vertex);

			//DefaultCube.mesh.vertices.resize(_countof(vertices));
			//memcpy(DefaultCube.mesh.vertices.data(), vertices, vertexBufferSize);

			// Note: using upload heaps to transfer static data like vert buffers is not 
			// recommended. Every time the GPU needs it, the upload heap will be marshalled 
			// over. Please read up on Default Heap usage. An upload heap is used here for 
			// code simplicity and because there are very few verts to actually transfer.
			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.CreationNodeMask = 1;
			heapProperties.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC resourceDesc;
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = vertexBufferSize;
			resourceDesc.Alignment = 0;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;



			HRESULT hr = (m_device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&DefaultCube.vertexBuffer)));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the committed resource. \n";
				exit(hr);
			}

			// Copy the triangle data to the vertex buffer.
			UINT8* pVertexDataBegin;
			D3D12_RANGE readRange;	// We do not intend to read from this resource on the CPU.
			readRange.Begin = 0;
			readRange.End = 0;
			hr = DefaultCube.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
			if (FAILED(hr))
			{
				std::cout << "Failed to map the vertex buffer. \n";
				exit(hr);
			}
			memcpy(pVertexDataBegin, DefaultCube.mesh.vertices.data(), vertexBufferSize);
			DefaultCube.vertexBuffer->Unmap(0, nullptr);

			// Initialize the vertex buffer view.
			DefaultCube.vertexBufferView.BufferLocation = DefaultCube.vertexBuffer->GetGPUVirtualAddress();
			DefaultCube.vertexBufferView.StrideInBytes = sizeof(Vertex);
			DefaultCube.vertexBufferView.SizeInBytes = vertexBufferSize;
		}

		// Create the vertex buffer.
		{
			const UINT vertexBufferSize = DebugRenderer::get_line_vert_capacity() * sizeof(Vertex);

			//DefaultCube.mesh.vertices.resize(_countof(vertices));
			//memcpy(DefaultCube.mesh.vertices.data(), vertices, vertexBufferSize);

			// Note: using upload heaps to transfer static data like vert buffers is not 
			// recommended. Every time the GPU needs it, the upload heap will be marshalled 
			// over. Please read up on Default Heap usage. An upload heap is used here for 
			// code simplicity and because there are very few verts to actually transfer.
			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.CreationNodeMask = 1;
			heapProperties.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC resourceDesc;
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = vertexBufferSize;
			resourceDesc.Alignment = 0;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;



			HRESULT hr = (m_device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&DefaultLineRenderer.vertexBuffer)));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the committed resource. \n";
				exit(hr);
			}

			// Copy the triangle data to the vertex buffer.
			//UINT8* pVertexDataBegin;
			D3D12_RANGE readRange;	// We do not intend to read from this resource on the CPU.
			readRange.Begin = 0;
			readRange.End = 0;
			hr = DefaultLineRenderer.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pDebugVertexDataBegin));
			if (FAILED(hr))
			{
				std::cout << "Failed to map the vertex buffer. \n";
				exit(hr);
			}
			memcpy(m_pDebugVertexDataBegin, DebugRenderer::get_line_verts(), vertexBufferSize);
			DefaultLineRenderer.vertexBuffer->Unmap(0, nullptr);

			// Initialize the vertex buffer view.
			DefaultLineRenderer.vertexBufferView.BufferLocation = DefaultLineRenderer.vertexBuffer->GetGPUVirtualAddress();
			DefaultLineRenderer.vertexBufferView.StrideInBytes = sizeof(Vertex);
			DefaultLineRenderer.vertexBufferView.SizeInBytes = vertexBufferSize;
		}

		return true;
	}

	bool GraphicsApplication::CreateIndexBuffers()
	{
		{
			//int indices[] =
			//{
			//	3,1,0,
			//	2,1,3,

			//	6,4,5,
			//	7,4,6,

			//	11,9,8,
			//	10,9,11,

			//	14,12,13,
			//	15,12,14,

			//	19,17,16,
			//	18,17,19,

			//	22,20,21,
			//	23,20,22
			//};

			const UINT64 indexBufferSize = DefaultCube.mesh.indices.size() * sizeof(int);

			//DefaultCube.mesh.indices.resize(_countof(indices));
			//memcpy(DefaultCube.mesh.indices.data(), indices, indexBufferSize);


			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.CreationNodeMask = 1;
			heapProperties.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC resourceDesc;
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = indexBufferSize;
			resourceDesc.Alignment = 0;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			HRESULT hr = (m_device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&DefaultCube.indexBuffer)));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the committed resource. \n";
				exit(hr);
			}

			// Copy the index data to the index buffer.
			UINT32* pIndexDataBegin;
			D3D12_RANGE readRange;	// We do not intend to read from this resource on the CPU.
			readRange.Begin = 0;
			readRange.End = 0;
			hr = DefaultCube.indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
			if (FAILED(hr))
			{
				std::cout << "Failed to map the index buffer. \n";
				exit(hr);
			}
			memcpy(pIndexDataBegin, DefaultCube.mesh.indices.data(), sizeof(UINT32) * DefaultCube.mesh.indices.size());
			DefaultCube.indexBuffer->Unmap(0, nullptr);

			// Initialize the index buffer view.
			DefaultCube.indexBufferView.BufferLocation = DefaultCube.indexBuffer->GetGPUVirtualAddress();
			DefaultCube.indexBufferView.SizeInBytes = indexBufferSize;
			DefaultCube.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		}

		return true;
	}

	bool GraphicsApplication::CreateInstanceBuffers()
	{
		// Create the instance buffer
		{
			InstanceData instances[1];
			instances[0] = { {0.0f, 0.0f, 0.0f, 1.0f} };

			UINT instanceBufferSize = sizeof(instances);

			HRESULT hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(instanceBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&DefaultCube.instanceBuffer));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the instance buffer.\n";
				return hr;
			}

			UINT8* pInstanceDataBegin;
			hr = DefaultCube.instanceBuffer->Map(0, &CD3DX12_RANGE(0, 0),
				reinterpret_cast<void**>(&pInstanceDataBegin));
			if (FAILED(hr))
			{
				std::cout << "Failed to map the vertex buffer. \n";
				return E_FAIL;
			}
			memcpy(pInstanceDataBegin, instances, instanceBufferSize);
			DefaultCube.instanceBuffer->Unmap(0, nullptr);

			// Initialize the vertex buffer view.
			DefaultCube.instanceBufferView.BufferLocation = DefaultCube.instanceBuffer->GetGPUVirtualAddress();
			DefaultCube.instanceBufferView.StrideInBytes = sizeof(InstanceData);
			DefaultCube.instanceBufferView.SizeInBytes = instanceBufferSize;
		}
		// Create the instance buffer
		{
			InstanceData instances[1];
			instances[0] = { {5.0f, 0.0f, 0.0f, 1.0f} };

			UINT instanceBufferSize = sizeof(instances);

			HRESULT hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(instanceBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&DefaultLineRenderer.instanceBuffer));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the instance buffer.\n";
				return hr;
			}

			UINT8* pInstanceDataBegin;
			hr = DefaultLineRenderer.instanceBuffer->Map(0, &CD3DX12_RANGE(0, 0),
				reinterpret_cast<void**>(&pInstanceDataBegin));
			if (FAILED(hr))
			{
				std::cout << "Failed to map the vertex buffer. \n";
				return E_FAIL;
			}
			memcpy(pInstanceDataBegin, instances, instanceBufferSize);
			DefaultLineRenderer.instanceBuffer->Unmap(0, nullptr);

			// Initialize the vertex buffer view.
			DefaultLineRenderer.instanceBufferView.BufferLocation = DefaultLineRenderer.instanceBuffer->GetGPUVirtualAddress();
			DefaultLineRenderer.instanceBufferView.StrideInBytes = sizeof(InstanceData);
			DefaultLineRenderer.instanceBufferView.SizeInBytes = instanceBufferSize;
		}

		return true;
	}

	bool GraphicsApplication::CreateTextures()
	{
		// TODO::
		// Need to get DirectXTex to compile and run, need to make sure that the filepath is correct and points to the correct place, need to make sure that the texture shows up and populates on the model itself. 
		// Need to get other textures up and running and into the shader as well.
		// Just got the .lib file from the github repo and placed it into the project, still need to include the library since I went to bed.



		// Create the diffuse texture
		{

			TexMetadata texMetaData;
			ScratchImage scratchImage;
			//TextureUsage

			std::string filepath = DefaultCube.mesh.materialPaths[DefaultCube.mesh.materials[0][Material::DIFFUSE].input].c_str();
			std::wstring filepathw = std::wstring(filepath.begin(), filepath.end());
			LoadFromWICFile(filepathw.c_str(), WIC_FLAGS_FORCE_RGB, &texMetaData, scratchImage); //used for .png .bmp or whatever WIC has.

			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = texMetaData.width;
			textureDesc.Height = texMetaData.height;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			textureDesc.DepthOrArraySize = texMetaData.arraySize;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			HRESULT hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_texture));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the descriptor heap for the constant buffer. \n";
				return E_FAIL;
			}

			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, scratchImage.GetImageCount());
			//m_device->GetCopyableFootprints(&m_texture->GetDesc(), 0, 1, 0, nullptr, nullptr, nullptr, &RequiredSize);

			hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadHeap));
			if (FAILED(hr))
			{
				std::cout << "Cannot create committed resource\n";
				return E_FAIL;
			}

			//std::vector<UINT8> texture;
			//texture.resize(scratchImage.GetPixelsSize() * 4);
			//memcpy(texture.data(), scratchImage.GetPixels(), texture.size());
			//char temp = 0;
			//for (int i = 0; i < texture.size(); i += 4)
			//{
			//	temp = texture[i];
			//	texture[i] = texture[i + 3];
			//	texture[i + 3] = temp;
			//	temp = texture[i + 1];
			//	texture[i + 1] = texture[i + 2];
			//	texture[i + 2] = temp;
			//}

			std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
			const Image* pImages = scratchImage.GetImages();
			for (int i = 0; i < scratchImage.GetImageCount(); ++i)
			{
				auto& subresource = subresources[i];
				subresource.RowPitch = pImages[i].rowPitch;
				subresource.SlicePitch = pImages[i].slicePitch;
				subresource.pData = pImages[i].pixels;
			}

			UpdateSubresources(m_commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, subresources.size(), &subresources[0]);

			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			// Describe and create a SRV for the texture.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;

			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_cbvDescriptorSize);

			m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, cbvHandle);
		}

		// Create the emissive texture
		{

			TexMetadata texMetaData;
			ScratchImage scratchImage;
			//TextureUsage

			std::string filepath = DefaultCube.mesh.materialPaths[DefaultCube.mesh.materials[0][Material::EMISSIVE].input].c_str();
			std::wstring filepathw = std::wstring(filepath.begin(), filepath.end());
			LoadFromWICFile(filepathw.c_str(), WIC_FLAGS_FORCE_RGB, &texMetaData, scratchImage); //used for .png .bmp or whatever WIC has.

			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = texMetaData.width;
			textureDesc.Height = texMetaData.height;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			textureDesc.DepthOrArraySize = texMetaData.arraySize;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			HRESULT hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_texture1));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the descriptor heap for the constant buffer. \n";
				return E_FAIL;
			}

			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture1.Get(), 0, scratchImage.GetImageCount());
			//m_device->GetCopyableFootprints(&m_texture->GetDesc(), 0, 1, 0, nullptr, nullptr, nullptr, &RequiredSize);

			hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadHeap1));
			if (FAILED(hr))
			{
				std::cout << "Cannot create committed resource\n";
				return E_FAIL;
			}

			//std::vector<UINT8> texture;
			//texture.resize(scratchImage.GetPixelsSize() * 4);
			//memcpy(texture.data(), scratchImage.GetPixels(), texture.size());
			//char temp = 0;
			//for (int i = 0; i < texture.size(); i += 4)
			//{
			//	temp = texture[i];
			//	texture[i] = texture[i + 3];
			//	texture[i + 3] = temp;
			//	temp = texture[i + 1];
			//	texture[i + 1] = texture[i + 2];
			//	texture[i + 2] = temp;
			//}

			std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
			const Image* pImages = scratchImage.GetImages();
			for (int i = 0; i < scratchImage.GetImageCount(); ++i)
			{
				auto& subresource = subresources[i];
				subresource.RowPitch = pImages[i].rowPitch;
				subresource.SlicePitch = pImages[i].slicePitch;
				subresource.pData = pImages[i].pixels;
			}

			UpdateSubresources(m_commandList.Get(), m_texture1.Get(), textureUploadHeap1.Get(), 0, 0, subresources.size(), &subresources[0]);

			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture1.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			// Describe and create a SRV for the texture.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;

			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart(), 2, m_cbvDescriptorSize);

			m_device->CreateShaderResourceView(m_texture1.Get(), &srvDesc, cbvHandle);
		}

		// Create the specular texture
		{

			TexMetadata texMetaData;
			ScratchImage scratchImage;
			//TextureUsage

			std::string filepath = DefaultCube.mesh.materialPaths[DefaultCube.mesh.materials[0][Material::SPECULAR].input].c_str();
			std::wstring filepathw = std::wstring(filepath.begin(), filepath.end());
			LoadFromWICFile(filepathw.c_str(), WIC_FLAGS_FORCE_RGB, &texMetaData, scratchImage); //used for .png .bmp or whatever WIC has.

			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = texMetaData.width;
			textureDesc.Height = texMetaData.height;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			textureDesc.DepthOrArraySize = texMetaData.arraySize;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			HRESULT hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_texture2));
			if (FAILED(hr))
			{
				std::cout << "Failed to create the descriptor heap for the constant buffer. \n";
				return E_FAIL;
			}

			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture2.Get(), 0, scratchImage.GetImageCount());
			//m_device->GetCopyableFootprints(&m_texture->GetDesc(), 0, 1, 0, nullptr, nullptr, nullptr, &RequiredSize);

			hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadHeap2));
			if (FAILED(hr))
			{
				std::cout << "Cannot create committed resource\n";
				return E_FAIL;
			}

			//std::vector<UINT8> texture;
			//texture.resize(scratchImage.GetPixelsSize() * 4);
			//memcpy(texture.data(), scratchImage.GetPixels(), texture.size());
			//char temp = 0;
			//for (int i = 0; i < texture.size(); i += 4)
			//{
			//	temp = texture[i];
			//	texture[i] = texture[i + 3];
			//	texture[i + 3] = temp;
			//	temp = texture[i + 1];
			//	texture[i + 1] = texture[i + 2];
			//	texture[i + 2] = temp;
			//}

			std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
			const Image* pImages = scratchImage.GetImages();
			for (int i = 0; i < scratchImage.GetImageCount(); ++i)
			{
				auto& subresource = subresources[i];
				subresource.RowPitch = pImages[i].rowPitch;
				subresource.SlicePitch = pImages[i].slicePitch;
				subresource.pData = pImages[i].pixels;
			}

			UpdateSubresources(m_commandList.Get(), m_texture2.Get(), textureUploadHeap2.Get(), 0, 0, subresources.size(), &subresources[0]);

			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture2.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			// Describe and create a SRV for the texture.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;

			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart(), 3, m_cbvDescriptorSize);

			m_device->CreateShaderResourceView(m_texture2.Get(), &srvDesc, cbvHandle);
		}

		return true;
	}

	bool GraphicsApplication::ExecuteCommandList()
	{
		HRESULT hr = m_commandList->Close();
		if (FAILED(hr))
		{
			std::cout << "Failed to close the command list. \n";
			exit(hr);
		}
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		return true;
	}

	std::string GraphicsApplication::OpenFileName(const wchar_t* filter, HWND owner) 
	{
		OPENFILENAME ofn;
		wchar_t fileName[MAX_PATH] = L"";
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = owner;
		ofn.lpstrFilter = filter;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		ofn.lpstrDefExt = L"";
		std::wstring fileNameStr;
		if (GetOpenFileName(&ofn))
			fileNameStr = fileName;

		std::string returnString = std::string(fileNameStr.begin(), fileNameStr.end());

		return returnString;
	}

}
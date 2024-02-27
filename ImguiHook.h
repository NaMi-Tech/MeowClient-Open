#include <dxgi1_4.h>
#include <d3d12.h>
#include "../include/imgui/imgui_impl_dx11.h"
#include "../MeowClient/Module/ModuleManager.h"
#include "../include/imgui/imgui_impl_win32.h"
#include "../include/imgui/imgui_impl_dx12.h"
#include "../include/kiero/kiero.h"
#include "../include/font/Mojangles.h"
#include "../include/font/ProductSansRegular.h"
#include "../include/font/BahnSchrift.h"
#include "../include/font/MiSansLight.h"
#include "../include/font/MiSansRegular.h"
#include "../include/font/GenshinImpact.h"
#include "../MeowClient/Module/Modules/Client/Notifications.h"
#include "../include/animations/snowflake.hpp"
#include "../include/animations/dotMatrix.h"
#include "../include/animations/rainCode.h"

#define SNOW_LIMIT 300  // max ammount of Snow/Bubbles allowed on screen at once
std::vector<Snowflake::Snowflake> snow;
std::vector<Particle> dots;
std::vector<CodeRain> codes;

bool initContext = false;
enum SwapChain_DeviceType { INVALID_DEVICE_TYPE,
							D3D11,
							D3D12 };

typedef HRESULT(__thiscall* PresentD3D12)(IDXGISwapChain3*, UINT, UINT);
PresentD3D12 oPresentD3D12;

struct FrameContext {
	ID3D12CommandAllocator* commandAllocator = nullptr;
	ID3D12Resource* main_render_target_resource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE main_render_target_descriptor;
};

UINT buffersCounts = -1;
FrameContext* frameContext = nullptr;

ID3D12CommandQueue* d3d12CommandQueue = nullptr;

ID3D12DescriptorHeap* d3d12DescriptorHeapImGuiRender = nullptr;
ID3D12DescriptorHeap* d3d12DescriptorHeapBackBuffers = nullptr;
ID3D12GraphicsCommandList* d3d12CommandList = nullptr;
ID3D12CommandAllocator* allocator = nullptr;

// Devices
SwapChain_DeviceType deviceType = SwapChain_DeviceType::INVALID_DEVICE_TYPE;
ID3D11Device* d3d11Device = nullptr;
ID3D12Device* d3d12Device = nullptr;

void onImGuiRender(ImDrawList* d) {
	ImGuiIO& io = ImGui::GetIO();
	auto customFontMod = moduleMgr->getModule<FontChanger>();
	static ImFont* mcFont = io.Fonts->Fonts[0];
	static ImFont* pdsFont = io.Fonts->Fonts[1];
	static ImFont* bhsFont = io.Fonts->Fonts[2];
	static ImFont* mslFont = io.Fonts->Fonts[3];
	static ImFont* msrFont = io.Fonts->Fonts[4];
	static ImFont* giFont = io.Fonts->Fonts[5];
	if (customFontMod->imguiFonts.selected == 0) {
		io.FontDefault = mcFont;
		ImGuiUtils::fontSize = customFontMod->fontsize;
	} else if (customFontMod->imguiFonts.selected == 1) {
		io.FontDefault = pdsFont;
		ImGuiUtils::fontSize = customFontMod->fontsize;
	} else if (customFontMod->imguiFonts.selected == 2) {
		io.FontDefault = bhsFont;
		ImGuiUtils::fontSize = customFontMod->fontsize;
	} else if (customFontMod->imguiFonts.selected == 3) {
		io.FontDefault = mslFont;
		ImGuiUtils::fontSize = customFontMod->fontsize;
	} else if (customFontMod->imguiFonts.selected == 4) {
		io.FontDefault = msrFont;
		ImGuiUtils::fontSize = customFontMod->fontsize;
	} else if (customFontMod->imguiFonts.selected == 5) {
		io.FontDefault = giFont;
		ImGuiUtils::fontSize = customFontMod->fontsize;
	}
	ImGuiUtils::setDrawList(d);
	moduleMgr->onImGuiRender(d);
	Notifications::Render(d);
	POINT mouse;
	RECT rc = {0};
	RECT rect;
	auto window = (HWND)FindWindowA(nullptr, (LPCSTR) "Minecraft");
	GetWindowRect(window, &rect);
	ImVec2 size69 = ImVec2(rect.right - rect.left, rect.bottom - rect.top);
	static auto clickGuiMod = moduleMgr->getModule<ClickGuiMod>();
	static auto clientSettings = moduleMgr->getModule<ClientSettings>();
	if (clientSettings->clickGuiCheck) {
		if (clickGuiMod->isEnabled()) {
			ImGui::SetNextWindowPos(ImVec2(size69.x - size69.x, size69.y - size69.y), ImGuiCond_Once);
			ImGui::SetNextWindowSize(ImVec2(size69.x, size69.y));
			ImGui::SetNextWindowBgAlpha(0.f);  // Set to 0.25 for a nice background

			ImGui::Begin("HELLO!!!", 0, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
			{
				GetWindowRect(window, &rc);
				GetCursorPos(&mouse);
				// render this before anything else so it is the background
				if (clientSettings->doSnow)
					Snowflake::Update(snow, Snowflake::vec3(mouse.x, mouse.y), Snowflake::vec3(rc.left, rc.top));  // you can change a few things inside the update function

				if (clientSettings->doDotMatrix) {
					updateDotMatrix({Utils::getScreenResolution().x, Utils::getScreenResolution().y}, dots);
					drawDotMatrix(dots, 50, 0.05, false);
				}
				if (clientSettings->rainCode) {
					updateCodeRain({Utils::getScreenResolution().x, Utils::getScreenResolution().y}, codes);
					drawCodeRain(codes);
				}
			}
			ImGui::End();
			ImGui::Render();
		}
	} else {
		ImGui::SetNextWindowPos(ImVec2(size69.x - size69.x, size69.y - size69.y), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(size69.x, size69.y));
		ImGui::SetNextWindowBgAlpha(0.f);  // Set to 0.25 for a nice background

		ImGui::Begin("HELLO!!!", 0, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
		{
			GetWindowRect(window, &rc);
			GetCursorPos(&mouse);
			// render this before anything else so it is the background
			if (clientSettings->doSnow)
				Snowflake::Update(snow, Snowflake::vec3(mouse.x, mouse.y), Snowflake::vec3(rc.left, rc.top));  // you can change a few things inside the update function

			if (clientSettings->doDotMatrix) {
				updateDotMatrix({Utils::getScreenResolution().x, Utils::getScreenResolution().y}, dots);
				drawDotMatrix(dots, 50, 0.05, false);
			}
			if (clientSettings->rainCode) {
				updateCodeRain({Utils::getScreenResolution().x, Utils::getScreenResolution().y}, codes);
				drawCodeRain(codes);
			}
		}
		ImGui::End();
		ImGui::Render();
	}
};

auto hookPresentD3D12(IDXGISwapChain3* ppSwapChain, UINT syncInterval, UINT flags) -> HRESULT {
	auto window = (HWND)FindWindowA(nullptr, (LPCSTR) "Minecraft");

	if (SUCCEEDED(ppSwapChain->GetDevice(IID_PPV_ARGS(&d3d11Device)))) {
		deviceType = SwapChain_DeviceType::D3D11;
	} else if (SUCCEEDED(ppSwapChain->GetDevice(IID_PPV_ARGS(&d3d12Device)))) {
		deviceType = SwapChain_DeviceType::D3D12;
	};

	if (deviceType == SwapChain_DeviceType::INVALID_DEVICE_TYPE)
		goto out;

	if (deviceType == SwapChain_DeviceType::D3D11) {
		if (!initContext) {
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			ImGui::StyleColorsDark();
			io.Fonts->AddFontFromMemoryTTF((void*)Mojangles, sizeof(Mojangles), 50.f);  // 200.f
			io.Fonts->AddFontFromMemoryTTF((void*)ProductSansRegular, sizeof(ProductSansRegular), 50.f);
			io.Fonts->AddFontFromMemoryTTF((void*)BahnSchrift, sizeof(BahnSchrift), 50.f);
			io.Fonts->AddFontFromMemoryTTF((void*)MiSansLight, sizeof(MiSansLight), 50.f);
			io.Fonts->AddFontFromMemoryTTF((void*)MiSansRegular, sizeof(MiSansRegular), 50.f);
			io.Fonts->AddFontFromMemoryTTF((void*)GenshinImpact, sizeof(GenshinImpact), 50.f);
			io.IniFilename = NULL;

			Snowflake::CreateSnowFlakes(snow, SNOW_LIMIT, 5.f /*minimum size*/, 25.f /*maximum size*/, 0 /*imgui window x position*/, 0 /*imgui window y position*/, Utils::getScreenResolution().x, Utils::getScreenResolution().y, Snowflake::vec3(0.f, 0.005f) /*gravity*/, IM_COL32(255, 255, 255, 100) /*color*/);
			dots = createDotMatrix({Utils::getScreenResolution().x, Utils::getScreenResolution().y}, {40, 40}, Utils::getScreenResolution().x * Utils::getScreenResolution().y / 9000);
			codes = createCodeRain({Utils::getScreenResolution().x, Utils::getScreenResolution().y}, {0, 0}, Utils::getScreenResolution().x * Utils::getScreenResolution().y / 50000);

			initContext = true;
		};

		ID3D11DeviceContext* ppContext = nullptr;
		d3d11Device->GetImmediateContext(&ppContext);

		ID3D11Texture2D* pBackBuffer;
		ppSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

		ID3D11RenderTargetView* mainRenderTargetView;
		d3d11Device->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);

		pBackBuffer->Release();

		ImGui_ImplWin32_Init(window);
		ImGui_ImplDX11_Init(d3d11Device, ppContext);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame(ImVec2(Game.getGuiData()->windowSizeReal.x, Game.getGuiData()->windowSizeReal.y));
		ImGui::NewFrame();

		onImGuiRender(ImGui::GetBackgroundDrawList());

		ImGui::EndFrame();
		ImGui::Render();

		ppContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		mainRenderTargetView->Release();
		d3d11Device->Release();

	} else if (deviceType == SwapChain_DeviceType::D3D12) {
		if (!initContext) {
			ImGui::CreateContext();
			// auto& io = ImGui::GetIO();
			// io.Fonts->AddFontFromMemoryCompressedTTF(mojangles_compressed_data, mojangles_compressed_size, 22);  // Mojangles font
			// io.Fonts->Build();
		};

		DXGI_SWAP_CHAIN_DESC sdesc;
		ppSwapChain->GetDesc(&sdesc);
		sdesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		sdesc.OutputWindow = window;
		sdesc.Windowed = ((GetWindowLongPtr(window, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

		buffersCounts = sdesc.BufferCount;
		frameContext = new FrameContext[buffersCounts];

		D3D12_DESCRIPTOR_HEAP_DESC descriptorImGuiRender = {};
		descriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorImGuiRender.NumDescriptors = buffersCounts;
		descriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (d3d12DescriptorHeapImGuiRender == nullptr)
			if (FAILED(d3d12Device->CreateDescriptorHeap(&descriptorImGuiRender, IID_PPV_ARGS(&d3d12DescriptorHeapImGuiRender))))
				goto out;

		if (d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)) != S_OK)
			return false;

		for (size_t i = 0; i < buffersCounts; i++) {
			frameContext[i].commandAllocator = allocator;
		};

		if (d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, NULL, IID_PPV_ARGS(&d3d12CommandList)) != S_OK ||
			d3d12CommandList->Close() != S_OK)
			return false;

		D3D12_DESCRIPTOR_HEAP_DESC descriptorBackBuffers;
		descriptorBackBuffers.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		descriptorBackBuffers.NumDescriptors = buffersCounts;
		descriptorBackBuffers.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorBackBuffers.NodeMask = 1;

		if (d3d12Device->CreateDescriptorHeap(&descriptorBackBuffers, IID_PPV_ARGS(&d3d12DescriptorHeapBackBuffers)) != S_OK)
			return false;

		const auto rtvDescriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = d3d12DescriptorHeapBackBuffers->GetCPUDescriptorHandleForHeapStart();

		for (size_t i = 0; i < buffersCounts; i++) {
			ID3D12Resource* pBackBuffer = nullptr;

			frameContext[i].main_render_target_descriptor = rtvHandle;
			ppSwapChain->GetBuffer((UINT)i, IID_PPV_ARGS(&pBackBuffer));
			d3d12Device->CreateRenderTargetView(pBackBuffer, nullptr, rtvHandle);
			frameContext[i].main_render_target_resource = pBackBuffer;
			rtvHandle.ptr += rtvDescriptorSize;

			pBackBuffer->Release();
		};

		if (!initContext) {
			ImGui_ImplWin32_Init(window);
			ImGui_ImplDX12_Init(d3d12Device, buffersCounts,
								DXGI_FORMAT_R8G8B8A8_UNORM, d3d12DescriptorHeapImGuiRender,
								d3d12DescriptorHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(),
								d3d12DescriptorHeapImGuiRender->GetGPUDescriptorHandleForHeapStart());

			ImGuiIO& io = ImGui::GetIO();
			ImGui::StyleColorsDark();
			io.Fonts->AddFontFromMemoryTTF((void*)Mojangles, sizeof(Mojangles), 50.f);  // 200.f
			io.Fonts->AddFontFromMemoryTTF((void*)ProductSansRegular, sizeof(ProductSansRegular), 50.f);
			io.Fonts->AddFontFromMemoryTTF((void*)BahnSchrift, sizeof(BahnSchrift), 50.f);
			io.Fonts->AddFontFromMemoryTTF((void*)MiSansLight, sizeof(MiSansLight), 50.f);
			io.Fonts->AddFontFromMemoryTTF((void*)MiSansRegular, sizeof(MiSansRegular), 50.f);
			io.Fonts->AddFontFromMemoryTTF((void*)GenshinImpact, sizeof(GenshinImpact), 50.f);
			io.IniFilename = NULL;

			Snowflake::CreateSnowFlakes(snow, SNOW_LIMIT, 5.f /*minimum size*/, 25.f /*maximum size*/, 0 /*imgui window x position*/, 0 /*imgui window y position*/, Utils::getScreenResolution().x, Utils::getScreenResolution().y, Snowflake::vec3(0.f, 0.005f) /*gravity*/, IM_COL32(255, 255, 255, 100) /*color*/);
			dots = createDotMatrix({Utils::getScreenResolution().x, Utils::getScreenResolution().y}, {40, 40}, Utils::getScreenResolution().x * Utils::getScreenResolution().y / 9000);
			codes = createCodeRain({Utils::getScreenResolution().x, Utils::getScreenResolution().y}, {0, 0}, Utils::getScreenResolution().x * Utils::getScreenResolution().y / 50000);
			initContext = true;
		};

		if (d3d12CommandQueue == nullptr)
			goto out;

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame(ImVec2(Game.getGuiData()->windowSizeReal.x, Game.getGuiData()->windowSizeReal.y));
		ImGui::NewFrame();

		onImGuiRender(ImGui::GetBackgroundDrawList());

		FrameContext& currentFrameContext = frameContext[ppSwapChain->GetCurrentBackBufferIndex()];
		currentFrameContext.commandAllocator->Reset();

		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = currentFrameContext.main_render_target_resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		d3d12CommandList->Reset(currentFrameContext.commandAllocator, nullptr);
		d3d12CommandList->ResourceBarrier(1, &barrier);
		d3d12CommandList->OMSetRenderTargets(1, &currentFrameContext.main_render_target_descriptor, FALSE, nullptr);
		d3d12CommandList->SetDescriptorHeaps(1, &d3d12DescriptorHeapImGuiRender);

		ImGui::EndFrame();
		ImGui::Render();

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d12CommandList);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

		d3d12CommandList->ResourceBarrier(1, &barrier);
		d3d12CommandList->Close();

		d3d12CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&d3d12CommandList));

		d3d12DescriptorHeapBackBuffers->Release();
		d3d12CommandList->Release();
		allocator->Release();

		currentFrameContext.main_render_target_resource->Release();
		currentFrameContext.commandAllocator->Release();

		d3d12Device->Release();

		delete frameContext;
	};

	goto out;

out:
	return oPresentD3D12(ppSwapChain, syncInterval, flags);
};

typedef void(__thiscall* ExecuteCommandListsD3D12)(ID3D12CommandQueue*, UINT, ID3D12CommandList*);
ExecuteCommandListsD3D12 oExecuteCommandListsD3D12;

auto hookExecuteCommandListsD3D12(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists) -> void {
	if (!d3d12CommandQueue)
		d3d12CommandQueue = queue;

	oExecuteCommandListsD3D12(queue, NumCommandLists, ppCommandLists);
};

typedef HRESULT(__thiscall* present_t)(IDXGISwapChain3*, UINT, UINT);
present_t original_present;

typedef HRESULT(__thiscall* resize_buffers_t)(IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
resize_buffers_t original_resize_buffers;

typedef HRESULT(__thiscall* draw_indexed_t)(ID3D11DeviceContext*, UINT, UINT, INT);
draw_indexed_t original_draw_indexed;

ID3D11Device* device;

HRESULT PresentD3D(IDXGISwapChain3* swap_chain, UINT sync_interval, UINT flags) {
	static bool once = false;
	if (!once) {
		ID3D12Device* bad_device;
		if (SUCCEEDED(swap_chain->GetDevice(IID_PPV_ARGS(&bad_device)))) {
			static_cast<ID3D12Device5*>(bad_device)->RemoveDevice();
			return original_present(swap_chain, sync_interval, flags);
		}
		once = true;
	}

	if (FAILED(swap_chain->GetDevice(IID_PPV_ARGS(&device))))
		return original_present(swap_chain, sync_interval, flags);


	return original_present(swap_chain, sync_interval, flags);
}

HRESULT ResizeBuffersD3D(IDXGISwapChain3* swap_chain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT swap_chain_flags) {

	return original_resize_buffers(swap_chain, buffer_count, width, height, new_format, swap_chain_flags);
}

// Hook the DrawIndexed Function
HRESULT DrawIndexedD3D11(ID3D11DeviceContext* Context, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) {
	ID3D11Buffer* vertBuffer;
	UINT vertBufferOffset;
	UINT stride;

	Context->IAGetVertexBuffers(0, 1, &vertBuffer, &stride, &vertBufferOffset);
	if (stride == 24) {
		D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
		ID3D11DepthStencilState* depthStencil;
		UINT stencilRef;

		// get info about current depth stencil
		Context->OMGetDepthStencilState(&depthStencil, &stencilRef);
		depthStencil->GetDesc(&depthStencilDesc);
		depthStencilDesc.DepthEnable = false;  // disable depth to ignore all other geometry

		// create a new depth stencil based on current drawn one but with the depth disabled as mentioned before
		device->CreateDepthStencilState(&depthStencilDesc, &depthStencil);
		Context->OMSetDepthStencilState(depthStencil, stencilRef);

		// call original function
		original_draw_indexed(Context, IndexCount, StartIndexLocation, BaseVertexLocation);

		// release memory
		depthStencil->Release();

		return 0;
	}

	return original_draw_indexed(Context, IndexCount, StartIndexLocation, BaseVertexLocation);
}
class ImguiHook {
public:
	static void InitImGui() {
		if (kiero::init(kiero::RenderType::D3D12) == kiero::Status::Success) {
			kiero::bind(54, (void**)&oExecuteCommandListsD3D12, hookExecuteCommandListsD3D12);
			kiero::bind(140, (void**)&oPresentD3D12, hookPresentD3D12);
			//
			kiero::bind(140, (void**)&original_present, PresentD3D);
			kiero::bind(145, (void**)&original_resize_buffers, ResizeBuffersD3D);
			logF("Created hook for SwapChain::Present (DX12)!");
		} else if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success) {
			kiero::bind(8, (void**)&oPresentD3D12, hookPresentD3D12);
			//
			kiero::bind(8, (void**)&original_present, PresentD3D);
			kiero::bind(13, (void**)&original_resize_buffers, ResizeBuffersD3D);
			kiero::bind(73, (void**)&original_draw_indexed, DrawIndexedD3D11);
			logF("Created hook for SwapChain::Present (DX11)!");
		} else {
			logF("uhhhhh, fuck... didnt make one for anything :)");
		}
	}
};

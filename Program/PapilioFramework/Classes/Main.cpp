//=========================================================
//
// File: Main.cpp
//
// PapilioFramework使用アプリケーション　メイン関数
//
//									2017/02/22
//										Author Shinya Ueba
//=========================================================

//-------------------------------------------------------------------------
// インクルードヘッダ
//-------------------------------------------------------------------------
#include <Windows.h>
#include "d3dx12.h"
#include <dxgi1_4.h>
#include <wrl/client.h>



//-------------------------------------------------------------------------
// ライブラリ
//-------------------------------------------------------------------------
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")







const wchar_t windowClassName[] = L"PapilioFramework";
const wchar_t windowTitle[]		= L"PapilioFramework";
const int clientWidth = 800;
const int clientHeight = 600;
HWND hwnd = nullptr;


using Microsoft::WRL::ComPtr;
static const int frameBufferCount = 2;
ComPtr<ID3D12Device> device;
ComPtr<IDXGISwapChain3> swapChain;
ComPtr<ID3D12CommandQueue> commandQueue;
ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
ComPtr<ID3D12Resource> renderTargetList[frameBufferCount];
ComPtr<ID3D12Resource> depthStencilBuffer;
ComPtr<ID3D12CommandAllocator> commandAllocator[frameBufferCount];
ComPtr<ID3D12GraphicsCommandList> commandList;
ComPtr<ID3D12Fence> fence[frameBufferCount];
HANDLE fenceEvent;
UINT64 fenceValue[frameBufferCount];
int currentFrameIndex;
int rtvDescriptorSize;
bool warp;
bool InitializeD3D();
void FinalizeD3D();
bool Render();
bool WaitForPreviousFrame();
bool WaitForGpu();



//-------------------------------------------------------------------------
// ウィンドウプロシージャ宣言
//-------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparan);




/**
* @desc		アプリケーションメイン関数
* @param 
* @param
* @param
* @param
* @return
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int intCmdShow)
{	
	//作成するウィンドウの設定
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
	wc.lpszClassName = windowClassName;
	
	//ウィンドウを登録
	RegisterClassEx(&wc);

	//ウィンドウ枠を含めたウィンドウサイズの取得
	RECT windowRect = { 0, 0, clientWidth, clientHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	//ウィンドウの生成
	hwnd = CreateWindow(windowClassName,
						windowTitle,
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						windowRect.right - windowRect.left,
						windowRect.bottom - windowRect.top,
						nullptr,
						nullptr,
						hInstance,
						nullptr);

	//ウィンドウの表示
	ShowWindow(hwnd, intCmdShow);

	if (!InitializeD3D())
	{ 
		return -1;
	} 
	for (;;) 
	{ 
		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}    
		
		if (msg.message == WM_QUIT) 
		{	
			break;
		}    
		if (!Render())
		{ 
			break;
		} 
	}
	FinalizeD3D();

	return 0; 
}

/**
* @desc ウィンドウプロシージャ
* @param
* @param
* @param
* @param
* @return
*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{ 
	switch (msg) 
	{ 
		case WM_KEYDOWN:	if (wparam == VK_ESCAPE)
							{	
								DestroyWindow(hwnd);
								return 0;
							}
							break;
						
		case WM_DESTROY:	PostQuitMessage(0);
							return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam); 
}

/**
* @desc DirectXの初期化処理
* @param true...成功 false...失敗
*/
bool InitializeD3D(void)
{ 
	//DXGIファクトリを作成する
	ComPtr<IDXGIFactory4> dxgiFactory;
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))))
	{ 
		return false; 
	}

	ComPtr<IDXGIAdapter1> dxgiAdapter;	// デバイス情報を取得するインターフェイス
	int adapterIndex = 0;				// 列挙するデバイスのインデックス
	bool adapterFound = false;			// 目的のデバイスが見つかったかどうか
	while (dxgiFactory->EnumAdapters1(adapterIndex,&dxgiAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		dxgiAdapter->GetDesc1(&desc);
		if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) 
		{
			if (SUCCEEDED(D3D12CreateDevice( dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device),nullptr)))
			{ 
				adapterFound = true;
				break;
			}
		}    
		
		++adapterIndex;
	}
	
	if(!adapterFound)
	{
		if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter))))
		{
			return false;
		}
		
		warp = true;
	}
	
	if(FAILED(D3D12CreateDevice( dxgiAdapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))))
	{
		return false;
	}

	//コマンドキューの作成
	const D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	if(FAILED(device->CreateCommandQueue(&cqDesc,IID_PPV_ARGS(&commandQueue))))
	{
		return false;
	}

	//スワップチェーンの作成
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width	= clientWidth;
	scDesc.Height	= clientHeight;
	scDesc.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.SampleDesc.Count = 1;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = frameBufferCount; scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	ComPtr<IDXGISwapChain1> tmpSwapChain;
	if (FAILED(dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &scDesc, nullptr, nullptr, &tmpSwapChain)))
	{
		return false;
	}

	tmpSwapChain.As(&swapChain); currentFrameIndex = swapChain->GetCurrentBackBufferIndex();

	//レンダーターゲット用のデスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.NumDescriptors = frameBufferCount;
	rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvDescriptorHeap))))
	{
		return false; 
	} 
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//レンダーターゲット用のデスクリプタを作成
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < frameBufferCount; ++i)
	{
		if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargetList[i]))))
		{
			return false;
		}
		device->CreateRenderTargetView(renderTargetList[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += rtvDescriptorSize;
	}

	//コマンドアロケータを作成する
	for (int i = 0; i < frameBufferCount; ++i)
	{ 
		if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]))))
		{
			return false;
		} 
	}

	//コマンドリストを作成する
	if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[currentFrameIndex].Get(), nullptr, IID_PPV_ARGS(&commandList))))
	{ 
		return false;
	} 
	if (FAILED(commandList->Close())) 
	{
		return false;
	}

	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence->GetAddressOf()))))
	{ 
		return false; 
	}

	for (int i = 0; i < frameBufferCount; ++i) 
	{ 
		fenceValue[i] = 0; 
	}
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr); if (!fenceEvent) { return false; }




	return true;
}

/**
* @desc DirectX終了処理
*/
void FinalizeD3D(void)
{
	WaitForGpu();
	CloseHandle(fenceEvent);
}

/**
* @desc 描画処理
* @return true...成功 false...失敗
*/
bool Render(void)
{ 
	//描画完了を待つ
	if (!WaitForPreviousFrame())
	{
		return false;
	}

	//コマンドアロケータをリセットする
	if (FAILED(commandAllocator[currentFrameIndex]->Reset()))
	{ 
		return false;
	} 
	
	if (FAILED(commandList->Reset(commandAllocator[currentFrameIndex].Get(), nullptr)))
	{
		return false;
	}



	commandList->ResourceBarrier(	1,
									&CD3DX12_RESOURCE_BARRIER::Transition(renderTargetList[currentFrameIndex].Get(),
									D3D12_RESOURCE_STATE_PRESENT,
									D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += rtvDescriptorSize * currentFrameIndex;
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	const float clearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargetList[currentFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));


	if (FAILED(commandList->Close()))
	{
		return false;
	}

	//コマンドリストを実行する
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


	if (FAILED(swapChain->Present(1, 0)))
	{
		return false;
	}


	if (FAILED(commandQueue->Signal(fence->Get(), fenceValue[currentFrameIndex])))
	{ 
		return false;
	}

	return true;
}

/**
* @desc 描画完了を待つ
* @return true...成功　false...失敗
*/
bool WaitForPreviousFrame(void)
{ 
	currentFrameIndex = swapChain->GetCurrentBackBufferIndex();
	
	if (fence->Get()->GetCompletedValue()< fenceValue[currentFrameIndex])
	{ 
		if (FAILED(fence->Get()->SetEventOnCompletion(fenceValue[currentFrameIndex], fenceEvent)))
		{
			return false;
		}
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	
	++fenceValue[currentFrameIndex];
	return true;
}

/**
* @desc GPUの処理完了を待つ
* @return true...成功　false...失敗
*/
bool WaitForGpu(void)
{
	if (FAILED(commandQueue->Signal(fence->Get(),
									fenceValue[currentFrameIndex])))
	{
		return false;
	}
	
	if (FAILED(fence->Get()->SetEventOnCompletion(fenceValue[currentFrameIndex], fenceEvent)))
	{ 
		return false;
	}
	WaitForSingleObject(fenceEvent, INFINITE);
	++fenceValue[currentFrameIndex];
	return true;
}
//EOF
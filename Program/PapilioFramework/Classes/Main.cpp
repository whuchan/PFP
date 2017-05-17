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
#include <d3dcompiler.h>
#include <DirectXMath.h>

//-------------------------------------------------------------------------
// 名前空間
//-------------------------------------------------------------------------
using Microsoft::WRL::ComPtr;
using namespace DirectX;


//-------------------------------------------------------------------------
// ライブラリ
//-------------------------------------------------------------------------
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

//-------------------------------------------------------------------------
// 変数宣言
//-------------------------------------------------------------------------
const wchar_t windowClassName[] = L"PapilioFramework";
const wchar_t windowTitle[]		= L"PapilioFramework";
const int clientWidth = 800;
const int clientHeight = 600;
HWND hwnd = nullptr;

//フレームバッファの数 / ダブルバッファを設定
static const int frameBufferCount = 2;
//DirectXデバイス
ComPtr<ID3D12Device> device;
//スワップチェイン
ComPtr<IDXGISwapChain3> swapChain;
//コマンドキュー
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
//現在の描画中のフレームバッファ
int currentFrameIndex;
//レンダーターゲットビューデスクリプタのバイト数
int rtvDescriptorSize;
//WARPデバイスかどうか true...WARPデバイス false...WARPデバイスでない
bool warp;

ComPtr<ID3D12RootSignature> rootSignature;
ComPtr<ID3D12PipelineState> pso;
ComPtr<ID3D12Resource> vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
D3D12_VIEWPORT viewport;
D3D12_RECT scissorRect;


//-------------------------------------------------------------------------
// 関数宣言
//-------------------------------------------------------------------------
bool InitializeD3D();
void FinalizeD3D();
bool Render();
bool WaitForPreviousFrame();
bool WaitForGpu();

bool LoadShader(const wchar_t*, const char*, ID3DBlob**);
bool CreatePSO();
bool CreateVertexBuffer();
void DrawTriangle();


//-------------------------------------------------------------------------
// 頂点データ構造
//-------------------------------------------------------------------------
//頂点データ型
struct Vertex
{
	XMFLOAT3 position;	//座標
	XMFLOAT4 color;		//色
};

//頂点データのインップットレイアウト
const D3D12_INPUT_ELEMENT_DESC vertexLayout[] = { 
		{	
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		
		{
			"COLOR",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		} };

//-------------------------------------------------------------------------
// データ
//-------------------------------------------------------------------------
//頂点データ
const Vertex vertices[] = {
	{ XMFLOAT3(0.0f,0.5f,0.5f),XMFLOAT4(1.0f,0.0f,0.0f,1.0f) },
	{ XMFLOAT3(0.5f,-0.5f,0.5f),XMFLOAT4(0.0f,1.0f,0.0f,1.0f) },
	{ XMFLOAT3(-0.5f,-0.5f,0.5f),XMFLOAT4(0.0f,0.0f,1.0f,1.0f) },
};

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

	//Direct3D初期化
	if (!InitializeD3D())
	{
		OutputDebugString(L"Failed Initialize Direct3D");
		return -1;
	}

	//メッセージループ
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

	//Direct3D終了
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
		OutputDebugString(L"Failed Create DXGIFactory");
		return false; 
	}

	ComPtr<IDXGIAdapter1> dxgiAdapter;	// デバイス情報を取得するインターフェイス
	int adapterIndex = 0;				// 列挙するデバイスのインデックス
	bool adapterFound = false;			// 目的のデバイスが見つかったかどうか
	
	//使用可能なデバイスを列挙
	while (dxgiFactory->EnumAdapters1(adapterIndex,&dxgiAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		dxgiAdapter->GetDesc1(&desc);
		
		//ハードウェアデバイスかどうか
		if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) 
		{
			//デバイスが作成可能かを調べる DirectX11を最低バージョンとして設定
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
		//WARPデバイスかどうか調べる
		if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter))))
		{
			OutputDebugString(L"Not Direct11 Device");
			return false;
		}
		
		warp = true;
	}
	
	//Direct3Dデバイスを作成する
	if(FAILED(D3D12CreateDevice( dxgiAdapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))))
	{
		OutputDebugString(L"Failed Create Device");
		return false;
	}

	//コマンドキューの作成
	const D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	if(FAILED(device->CreateCommandQueue(&cqDesc,IID_PPV_ARGS(&commandQueue))))
	{
		OutputDebugString(L"Failed Create CommandQueue");
		return false;
	}

	//スワップチェーンの作成
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width	= clientWidth;							//フレームバッファ幅
	scDesc.Height	= clientHeight;							//フレームバッファ高さ
	scDesc.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;			//RGBA 32bitを指定
	scDesc.SampleDesc.Count = 1;							//アンチエイリアスサンプル数...	使わないので１
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = frameBufferCount;					//フレームバッファ数...			ダブルバッファに設定
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		//表示切り替え時の扱い方...		GPUに任せる
	ComPtr<IDXGISwapChain1> tmpSwapChain;
	if (FAILED(dxgiFactory->CreateSwapChainForHwnd(	commandQueue.Get(),
													hwnd,
													&scDesc,
													nullptr,				//フルスクリーンモードの動作
													nullptr,				//マルチモニター
													&tmpSwapChain)))
	{
		OutputDebugString(L"Failed Create SwapChain");
		return false;
	}
	//インターフェイスを取得
	tmpSwapChain.As(&swapChain);
	//現在の描画フレームのナンバーを取得
	currentFrameIndex = swapChain->GetCurrentBackBufferIndex();



	//レンダーターゲット用のデスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
	rtvDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//タイプ...レンダーターゲットとして設定
	rtvDesc.NumDescriptors	= frameBufferCount;					//格納できるデスクリプタ数
	rtvDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	//何も設定しない

	if (FAILED(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvDescriptorHeap))))
	{
		OutputDebugString(L"Failed Create DescriptorHeap");
		return false; 
	}
	//レンダーターゲット用のバイト数を取得
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


	//レンダーターゲット用のデスクリプタを作成
	//GPUメモリ上のデスクリプタヒープに、 CPUから値を設定する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < frameBufferCount; ++i)
	{
		//レンダーターゲットのインターフェイスを取得しています
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
			OutputDebugString(L"Failed Create CommandAllocator");
			return false;
		} 
	}

	//コマンドリストを作成する
	if (FAILED(device->CreateCommandList(0,											//0番目のGPUを指定
										D3D12_COMMAND_LIST_TYPE_DIRECT,
										commandAllocator[currentFrameIndex].Get(),
										nullptr,
										IID_PPV_ARGS(&commandList))))
	{ 
		OutputDebugString(L"Failed Create CommandList");
		return false;
	} 

	//コマンドリストを記録状態から記録完了状態にする
	if (FAILED(commandList->Close())) 
	{
		OutputDebugString(L"Failed Close CommandList");
		return false;
	}

	//フェンスを作成する
	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence->GetAddressOf()))))
	{ 
		OutputDebugString(L"Failed Create Fence");
		return false; 
	}

	for (int i = 0; i < frameBufferCount; ++i) 
	{ 
		fenceValue[i] = 0; 
	}

	//フェンスイベントを作成する
	fenceEvent = CreateEvent(	nullptr,	//子プロセスから見えなくする
								FALSE,
								FALSE,
								nullptr);
	if (!fenceEvent)
	{
		OutputDebugString(L"Failed Create FenceEvent");
		return false;
	}

	if (!CreatePSO())
	{
		return false;
	}

	if (!CreateVertexBuffer())
	{
		return false;
	}

	//ビューポートを設定する
	viewport.TopLeftX	= 0;
	viewport.TopLeftY	= 0;
	viewport.Width		= clientWidth;
	viewport.Height		= clientHeight;
	viewport.MinDepth	= 0.0f;
	viewport.MaxDepth	= 1.0f;

	scissorRect.left	= 0;
	scissorRect.top		= 0;
	scissorRect.right	= clientWidth;
	scissorRect.bottom	= clientHeight;
	

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
	
	//コマンドリストをリセットする
	if (FAILED(commandList->Reset(commandAllocator[currentFrameIndex].Get(), nullptr)))
	{
		return false;
	}


	//命令:レンダーターゲットが描画できる状態になるのを待つ
	commandList->ResourceBarrier(	1,																				//リソースバリアの数	
									&CD3DX12_RESOURCE_BARRIER::Transition(renderTargetList[currentFrameIndex].Get(),//
									D3D12_RESOURCE_STATE_PRESENT,													//保護を開始する前の状態
									D3D12_RESOURCE_STATE_RENDER_TARGET));											//保護を終了する状態
	
	//命令：描画するフレームバッファを指定する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//N番目の要素のアドレス=デスクリプタヒープの先頭アドレス+(要素のバイト数*N)
	rtvHandle.ptr += rtvDescriptorSize * currentFrameIndex;
	commandList->OMSetRenderTargets(1,			//レンダーターゲット用のデスクリプタの数
									&rtvHandle,
									FALSE,		//デスクリプタの格納状態
									nullptr);	//ステンシルバッファ用のデスクリプタのアドレス
	
	//命令：色を指定して塗りつぶす
	const float clearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	


	DrawTriangle();



	//命令：フレームバッファが表示状態になるまで命令の実行を止める
	commandList->ResourceBarrier(	1,
									&CD3DX12_RESOURCE_BARRIER::Transition(renderTargetList[currentFrameIndex].Get(),
									D3D12_RESOURCE_STATE_RENDER_TARGET,
									D3D12_RESOURCE_STATE_PRESENT));


	//コマンドリストを記録状態から記録完了状態にする
	if (FAILED(commandList->Close()))
	{
		OutputDebugString(L"Failed Close CommandList");
		return false;
	}

	//コマンドリストを実行する
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


	//フロントバッファとバックバッファを切り替える
	if (FAILED(swapChain->Present(1, 0)))
	{
		OutputDebugString(L"Failed Present SwapChain");
		return false;
	}

	//コマンドリストの命令の終了をGPUからCPUに知らせる
	if (FAILED(commandQueue->Signal(fence->Get(), fenceValue[currentFrameIndex])))
	{ 
		OutputDebugString(L"Failed Close CommandList");
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

/**
* @desc シェーダを読み込む
* @param シェーダファイルパス
* @param 頂点シェーダなら"vs_5_0",ピクセルシェーダなら"ps_5_0"
* @param シェーダを格納するID3DBlobインターフェイスのポインタ
*/
bool LoadShader(const wchar_t* filename,
				const char* target,
				ID3DBlob** blob)
{
	ComPtr<ID3DBlob> errorBuffer;
	if (FAILED(D3DCompileFromFile(	filename,
									nullptr,
									nullptr,
									"main",
									target,
									D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
									0,
									blob,
									&errorBuffer)))
	{
		if (errorBuffer)
		{
			OutputDebugStringA(
				static_cast<char*>(errorBuffer->GetBufferPointer()));
		}
		return false;
	}
	return true;
}

/**
* @desc ルートシグネチャとPSOを作成する
*/
bool CreatePSO()
{
	ComPtr<ID3DBlob> vertexShaderBlob;
	if (!LoadShader(L"Resources/VertexShader.hlsl", "vs_5_0", &vertexShaderBlob))
	{
		return false;
	}

	ComPtr<ID3DBlob> pixelShaderBlob;
	if (!LoadShader(L"Resources/PixelShader.hlsl", "ps_5_0", &pixelShaderBlob))
	{
		return false;
	}

	D3D12_ROOT_SIGNATURE_DESC rsDesc = {
		0,
		nullptr,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	};

	ComPtr<ID3DBlob> signatureBlob;
	if (FAILED(D3D12SerializeRootSignature(&rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		&signatureBlob,
		nullptr)))
	{
		return false;
	}

	if (FAILED(device->CreateRootSignature(
		0,
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature))))
	{
		return false;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = {
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize()
	};

	psoDesc.PS = {
		pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize()
	};

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.InputLayout.pInputElementDescs = vertexLayout;
	psoDesc.InputLayout.NumElements = sizeof(vertexLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	
	if (warp)
	{
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
	}

	if (FAILED(device->CreateGraphicsPipelineState(&psoDesc,
			IID_PPV_ARGS(&pso))))
	{
		return false;
	}
	return true;
}

/**
* @desc 頂点バッファを作成する
*/
bool CreateVertexBuffer(void)
{
	//頂点バッファを作成する
	if (FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer))))
	{
		return false;
	}
	vertexBuffer->SetName(L"Vertex buffer");

	//頂点バッファに頂点データをコピーする
	void* pVertexDataBegin;
	const D3D12_RANGE readRange = { 0,0 };
	if (FAILED(vertexBuffer->Map(0, &readRange, &pVertexDataBegin)))
	{
		return false;
	}
	memcpy(pVertexDataBegin,vertices,sizeof(vertices));
	vertexBuffer->Unmap(0,nullptr);

	//頂点バッファビューを作成する
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes	= sizeof(Vertex);
	vertexBufferView.SizeInBytes	= sizeof(vertices);

	return true;
}

/**
* 三角形を描画する
*/
void DrawTriangle()
{
	commandList->SetPipelineState(pso.Get());
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1,&scissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0,1,&vertexBufferView);
	commandList->DrawInstanced(_countof(vertices),1,0,0);
}

//EOF
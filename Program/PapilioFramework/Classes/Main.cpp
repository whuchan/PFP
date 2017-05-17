//=========================================================
//
// File: Main.cpp
//
// PapilioFramework�g�p�A�v���P�[�V�����@���C���֐�
//
//									2017/02/22
//										Author Shinya Ueba
//=========================================================

//-------------------------------------------------------------------------
// �C���N���[�h�w�b�_
//-------------------------------------------------------------------------
#include <Windows.h>
#include "d3dx12.h"
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

//-------------------------------------------------------------------------
// ���O���
//-------------------------------------------------------------------------
using Microsoft::WRL::ComPtr;
using namespace DirectX;


//-------------------------------------------------------------------------
// ���C�u����
//-------------------------------------------------------------------------
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

//-------------------------------------------------------------------------
// �ϐ��錾
//-------------------------------------------------------------------------
const wchar_t windowClassName[] = L"PapilioFramework";
const wchar_t windowTitle[]		= L"PapilioFramework";
const int clientWidth = 800;
const int clientHeight = 600;
HWND hwnd = nullptr;

//�t���[���o�b�t�@�̐� / �_�u���o�b�t�@��ݒ�
static const int frameBufferCount = 2;
//DirectX�f�o�C�X
ComPtr<ID3D12Device> device;
//�X���b�v�`�F�C��
ComPtr<IDXGISwapChain3> swapChain;
//�R�}���h�L���[
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
//���݂̕`�撆�̃t���[���o�b�t�@
int currentFrameIndex;
//�����_�[�^�[�Q�b�g�r���[�f�X�N���v�^�̃o�C�g��
int rtvDescriptorSize;
//WARP�f�o�C�X���ǂ��� true...WARP�f�o�C�X false...WARP�f�o�C�X�łȂ�
bool warp;

ComPtr<ID3D12RootSignature> rootSignature;
ComPtr<ID3D12PipelineState> pso;
ComPtr<ID3D12Resource> vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
D3D12_VIEWPORT viewport;
D3D12_RECT scissorRect;


//-------------------------------------------------------------------------
// �֐��錾
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
// ���_�f�[�^�\��
//-------------------------------------------------------------------------
//���_�f�[�^�^
struct Vertex
{
	XMFLOAT3 position;	//���W
	XMFLOAT4 color;		//�F
};

//���_�f�[�^�̃C���b�v�b�g���C�A�E�g
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
// �f�[�^
//-------------------------------------------------------------------------
//���_�f�[�^
const Vertex vertices[] = {
	{ XMFLOAT3(0.0f,0.5f,0.5f),XMFLOAT4(1.0f,0.0f,0.0f,1.0f) },
	{ XMFLOAT3(0.5f,-0.5f,0.5f),XMFLOAT4(0.0f,1.0f,0.0f,1.0f) },
	{ XMFLOAT3(-0.5f,-0.5f,0.5f),XMFLOAT4(0.0f,0.0f,1.0f,1.0f) },
};

//-------------------------------------------------------------------------
// �E�B���h�E�v���V�[�W���錾
//-------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparan);




/**
* @desc		�A�v���P�[�V�������C���֐�
* @param 
* @param
* @param
* @param
* @return
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int intCmdShow)
{	
	//�쐬����E�B���h�E�̐ݒ�
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
	wc.lpszClassName = windowClassName;
	
	//�E�B���h�E��o�^
	RegisterClassEx(&wc);

	//�E�B���h�E�g���܂߂��E�B���h�E�T�C�Y�̎擾
	RECT windowRect = { 0, 0, clientWidth, clientHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	//�E�B���h�E�̐���
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

	//�E�B���h�E�̕\��
	ShowWindow(hwnd, intCmdShow);

	//Direct3D������
	if (!InitializeD3D())
	{
		OutputDebugString(L"Failed Initialize Direct3D");
		return -1;
	}

	//���b�Z�[�W���[�v
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

	//Direct3D�I��
	FinalizeD3D();

	return 0; 
}

/**
* @desc �E�B���h�E�v���V�[�W��
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
* @desc DirectX�̏���������
* @param true...���� false...���s
*/
bool InitializeD3D(void)
{ 
	//DXGI�t�@�N�g�����쐬����
	ComPtr<IDXGIFactory4> dxgiFactory;
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))))
	{ 
		OutputDebugString(L"Failed Create DXGIFactory");
		return false; 
	}

	ComPtr<IDXGIAdapter1> dxgiAdapter;	// �f�o�C�X�����擾����C���^�[�t�F�C�X
	int adapterIndex = 0;				// �񋓂���f�o�C�X�̃C���f�b�N�X
	bool adapterFound = false;			// �ړI�̃f�o�C�X�������������ǂ���
	
	//�g�p�\�ȃf�o�C�X���
	while (dxgiFactory->EnumAdapters1(adapterIndex,&dxgiAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		dxgiAdapter->GetDesc1(&desc);
		
		//�n�[�h�E�F�A�f�o�C�X���ǂ���
		if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) 
		{
			//�f�o�C�X���쐬�\���𒲂ׂ� DirectX11���Œ�o�[�W�����Ƃ��Đݒ�
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
		//WARP�f�o�C�X���ǂ������ׂ�
		if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter))))
		{
			OutputDebugString(L"Not Direct11 Device");
			return false;
		}
		
		warp = true;
	}
	
	//Direct3D�f�o�C�X���쐬����
	if(FAILED(D3D12CreateDevice( dxgiAdapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))))
	{
		OutputDebugString(L"Failed Create Device");
		return false;
	}

	//�R�}���h�L���[�̍쐬
	const D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	if(FAILED(device->CreateCommandQueue(&cqDesc,IID_PPV_ARGS(&commandQueue))))
	{
		OutputDebugString(L"Failed Create CommandQueue");
		return false;
	}

	//�X���b�v�`�F�[���̍쐬
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width	= clientWidth;							//�t���[���o�b�t�@��
	scDesc.Height	= clientHeight;							//�t���[���o�b�t�@����
	scDesc.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;			//RGBA 32bit���w��
	scDesc.SampleDesc.Count = 1;							//�A���`�G�C���A�X�T���v����...	�g��Ȃ��̂łP
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = frameBufferCount;					//�t���[���o�b�t�@��...			�_�u���o�b�t�@�ɐݒ�
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		//�\���؂�ւ����̈�����...		GPU�ɔC����
	ComPtr<IDXGISwapChain1> tmpSwapChain;
	if (FAILED(dxgiFactory->CreateSwapChainForHwnd(	commandQueue.Get(),
													hwnd,
													&scDesc,
													nullptr,				//�t���X�N���[�����[�h�̓���
													nullptr,				//�}���`���j�^�[
													&tmpSwapChain)))
	{
		OutputDebugString(L"Failed Create SwapChain");
		return false;
	}
	//�C���^�[�t�F�C�X���擾
	tmpSwapChain.As(&swapChain);
	//���݂̕`��t���[���̃i���o�[���擾
	currentFrameIndex = swapChain->GetCurrentBackBufferIndex();



	//�����_�[�^�[�Q�b�g�p�̃f�X�N���v�^�q�[�v�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
	rtvDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//�^�C�v...�����_�[�^�[�Q�b�g�Ƃ��Đݒ�
	rtvDesc.NumDescriptors	= frameBufferCount;					//�i�[�ł���f�X�N���v�^��
	rtvDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	//�����ݒ肵�Ȃ�

	if (FAILED(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvDescriptorHeap))))
	{
		OutputDebugString(L"Failed Create DescriptorHeap");
		return false; 
	}
	//�����_�[�^�[�Q�b�g�p�̃o�C�g�����擾
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


	//�����_�[�^�[�Q�b�g�p�̃f�X�N���v�^���쐬
	//GPU��������̃f�X�N���v�^�q�[�v�ɁA CPU����l��ݒ肷��
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < frameBufferCount; ++i)
	{
		//�����_�[�^�[�Q�b�g�̃C���^�[�t�F�C�X���擾���Ă��܂�
		if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargetList[i]))))
		{
			return false;
		}
		device->CreateRenderTargetView(renderTargetList[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += rtvDescriptorSize;
	}

	//�R�}���h�A���P�[�^���쐬����
	for (int i = 0; i < frameBufferCount; ++i)
	{ 
		if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]))))
		{
			OutputDebugString(L"Failed Create CommandAllocator");
			return false;
		} 
	}

	//�R�}���h���X�g���쐬����
	if (FAILED(device->CreateCommandList(0,											//0�Ԗڂ�GPU���w��
										D3D12_COMMAND_LIST_TYPE_DIRECT,
										commandAllocator[currentFrameIndex].Get(),
										nullptr,
										IID_PPV_ARGS(&commandList))))
	{ 
		OutputDebugString(L"Failed Create CommandList");
		return false;
	} 

	//�R�}���h���X�g���L�^��Ԃ���L�^������Ԃɂ���
	if (FAILED(commandList->Close())) 
	{
		OutputDebugString(L"Failed Close CommandList");
		return false;
	}

	//�t�F���X���쐬����
	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence->GetAddressOf()))))
	{ 
		OutputDebugString(L"Failed Create Fence");
		return false; 
	}

	for (int i = 0; i < frameBufferCount; ++i) 
	{ 
		fenceValue[i] = 0; 
	}

	//�t�F���X�C�x���g���쐬����
	fenceEvent = CreateEvent(	nullptr,	//�q�v���Z�X���猩���Ȃ�����
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

	//�r���[�|�[�g��ݒ肷��
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
* @desc DirectX�I������
*/
void FinalizeD3D(void)
{
	WaitForGpu();
	CloseHandle(fenceEvent);
}

/**
* @desc �`�揈��
* @return true...���� false...���s
*/
bool Render(void)
{ 
	//�`�抮����҂�
	if (!WaitForPreviousFrame())
	{
		return false;
	}

	//�R�}���h�A���P�[�^�����Z�b�g����
	if (FAILED(commandAllocator[currentFrameIndex]->Reset()))
	{ 
		return false;
	} 
	
	//�R�}���h���X�g�����Z�b�g����
	if (FAILED(commandList->Reset(commandAllocator[currentFrameIndex].Get(), nullptr)))
	{
		return false;
	}


	//����:�����_�[�^�[�Q�b�g���`��ł����ԂɂȂ�̂�҂�
	commandList->ResourceBarrier(	1,																				//���\�[�X�o���A�̐�	
									&CD3DX12_RESOURCE_BARRIER::Transition(renderTargetList[currentFrameIndex].Get(),//
									D3D12_RESOURCE_STATE_PRESENT,													//�ی���J�n����O�̏��
									D3D12_RESOURCE_STATE_RENDER_TARGET));											//�ی���I��������
	
	//���߁F�`�悷��t���[���o�b�t�@���w�肷��
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//N�Ԗڂ̗v�f�̃A�h���X=�f�X�N���v�^�q�[�v�̐擪�A�h���X+(�v�f�̃o�C�g��*N)
	rtvHandle.ptr += rtvDescriptorSize * currentFrameIndex;
	commandList->OMSetRenderTargets(1,			//�����_�[�^�[�Q�b�g�p�̃f�X�N���v�^�̐�
									&rtvHandle,
									FALSE,		//�f�X�N���v�^�̊i�[���
									nullptr);	//�X�e���V���o�b�t�@�p�̃f�X�N���v�^�̃A�h���X
	
	//���߁F�F���w�肵�ēh��Ԃ�
	const float clearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	


	DrawTriangle();



	//���߁F�t���[���o�b�t�@���\����ԂɂȂ�܂Ŗ��߂̎��s���~�߂�
	commandList->ResourceBarrier(	1,
									&CD3DX12_RESOURCE_BARRIER::Transition(renderTargetList[currentFrameIndex].Get(),
									D3D12_RESOURCE_STATE_RENDER_TARGET,
									D3D12_RESOURCE_STATE_PRESENT));


	//�R�}���h���X�g���L�^��Ԃ���L�^������Ԃɂ���
	if (FAILED(commandList->Close()))
	{
		OutputDebugString(L"Failed Close CommandList");
		return false;
	}

	//�R�}���h���X�g�����s����
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


	//�t�����g�o�b�t�@�ƃo�b�N�o�b�t�@��؂�ւ���
	if (FAILED(swapChain->Present(1, 0)))
	{
		OutputDebugString(L"Failed Present SwapChain");
		return false;
	}

	//�R�}���h���X�g�̖��߂̏I����GPU����CPU�ɒm�点��
	if (FAILED(commandQueue->Signal(fence->Get(), fenceValue[currentFrameIndex])))
	{ 
		OutputDebugString(L"Failed Close CommandList");
		return false;
	}

	return true;
}

/**
* @desc �`�抮����҂�
* @return true...�����@false...���s
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
* @desc GPU�̏���������҂�
* @return true...�����@false...���s
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
* @desc �V�F�[�_��ǂݍ���
* @param �V�F�[�_�t�@�C���p�X
* @param ���_�V�F�[�_�Ȃ�"vs_5_0",�s�N�Z���V�F�[�_�Ȃ�"ps_5_0"
* @param �V�F�[�_���i�[����ID3DBlob�C���^�[�t�F�C�X�̃|�C���^
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
* @desc ���[�g�V�O�l�`����PSO���쐬����
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
* @desc ���_�o�b�t�@���쐬����
*/
bool CreateVertexBuffer(void)
{
	//���_�o�b�t�@���쐬����
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

	//���_�o�b�t�@�ɒ��_�f�[�^���R�s�[����
	void* pVertexDataBegin;
	const D3D12_RANGE readRange = { 0,0 };
	if (FAILED(vertexBuffer->Map(0, &readRange, &pVertexDataBegin)))
	{
		return false;
	}
	memcpy(pVertexDataBegin,vertices,sizeof(vertices));
	vertexBuffer->Unmap(0,nullptr);

	//���_�o�b�t�@�r���[���쐬����
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes	= sizeof(Vertex);
	vertexBufferView.SizeInBytes	= sizeof(vertices);

	return true;
}

/**
* �O�p�`��`�悷��
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
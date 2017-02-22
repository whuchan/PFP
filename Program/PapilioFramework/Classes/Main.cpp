//=========================================================
//
// File: Main.cpp
//
// PapilioFramework使用アプリケーション　メイン関数
//
//									2016/12/18
//										Author Shinya Ueba
//=========================================================

//-------------------------------------------------------------------------
// インクルードヘッダ
//-------------------------------------------------------------------------
#include <Windows.h>

const wchar_t windowClassName[] = L"DX12TutorialApp";
const wchar_t windowTitle[]		= L"DX12Tutorial";
const int clientWidth = 800;
const int clientHeight = 600;
HWND hwnd = nullptr;


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

	for (;;) 
	{
		MSG msg = {}; while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} 
		if (msg.message == WM_QUIT)
		{ 
			break;
		}
	}

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
//EOF
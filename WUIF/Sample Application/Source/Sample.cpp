/*Copyright 2017 Jonathan Campbell

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.*/
#include "stdafx.h"
#include "WUIF.h"
#include "DirectX\DirectX.h"
#include "Application\Application.h"
#include "Window\Window.h"

#pragma comment(lib, "d3d12")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "shcore")
#ifdef _DEBUG
#pragma comment(lib, "dxguid") //needed for dxgi debug features
#endif // _DEBUG
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")

using namespace WUIF;

//set flags for application
const WUIF::FLAGS::WUIF_Flags WUIF::Application::flags = WUIF::FLAGS::D3D11 | WUIF::FLAGS::D2D;
const D3D_FEATURE_LEVEL WUIF::DXResources::minDXFL = D3D_FEATURE_LEVEL_11_0;

//uncomment to indicate to hybrid graphics systems to prefer the discrete part by default

extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}


bool MenuCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	bool handled = false;
	switch (LOWORD(wParam))
	{
	case ID_FILE_EXI:
	{
		PostQuitMessage(0);
		handled = true;
		break;
	}
	case ID_HELP_ABOUT:
	{
		handled = true;
		break;
	}
	}
	return handled;
}

void draw(Window *pThis)
{
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pBlackBrush;
	ThrowIfFailed(
		pThis->DXres->d2dres.d2dDeviceContext->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Yellow),
			&pBlackBrush
		)
	);
	if (pThis->DXres->d2dres.d2dDeviceContext)
	{
		pThis->DXres->d2dres.d2dDeviceContext->BeginDraw();
		pThis->DXres->d2dres.d2dDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
		// pThis->DXres->d2dres.d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
		D2D1_SIZE_F rtSize = pThis->DXres->d2dres.d2dDeviceContext->GetSize();
		D2D1_RECT_F rectangle1 = D2D1::RectF(
			rtSize.width / 2 - 50.0f,
			rtSize.height / 2 - 50.0f,
			rtSize.width / 2 + 50.0f,
			rtSize.height / 2 + 50.0f
		);
		pThis->DXres->d2dres.d2dDeviceContext->FillRectangle(
			&rectangle1,
			pBlackBrush.Get());

		ThrowIfFailed(
			pThis->DXres->d2dres.d2dDeviceContext->EndDraw()
		);
	}
}

void CreateRenderTargets(D3D11Resources* pThis)
{
	//create the render target view
	ThrowIfFailed(pThis->DXres->DXGIres.dxgiSwapChain1->GetBuffer(0, IID_PPV_ARGS(&pThis->d3d11BackBuffer)));
	ThrowIfFailed(pThis->d3d11Device1->CreateRenderTargetView(pThis->d3d11BackBuffer.Get(), NULL, &pThis->d3d11RenderTargetView));
	pThis->d3d11ImmediateContext->OMSetRenderTargets(1, &pThis->d3d11RenderTargetView, 0);
	D3D11_VIEWPORT vp;
	pThis->DXres->winparent->getWindowDPI();
	vp.Width = static_cast<float>(pThis->DXres->winparent->Scale(pThis->DXres->winparent->props.width()));
	vp.Height = static_cast<float>(pThis->DXres->winparent->Scale(pThis->DXres->winparent->props.height()));
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pThis->d3d11ImmediateContext->RSSetViewports(1, &vp);
}



int AppMain(int argc, LPWSTR *argv)
{
	//WinStruct appWin;
	HBRUSH blkbgrnd = CreateSolidBrush(RGB(0, 0, 0));
	HBRUSH whitebgrnd = CreateSolidBrush(RGB(255, 255, 255));
	App->mainWindow->props.background((HBRUSH)GetStockObject(WHITE_BRUSH));//((HBRUSH)COLOR_WINDOW + 1);
	App->mainWindow->props.windowname(L"Sample Application");
	App->mainWindow->props.menuname(MAKEINTRESOURCEW(IDR_MENU1));
	App->mainWindow->props.height(300);
	App->mainWindow->props.width(320);
	App->mainWindow->props.top(100);
	App->mainWindow->props.left(100);
	App->mainWindow->props.minwidth(100);
	App->mainWindow->props.minheight(100);
	App->mainWindow->props.style(WS_OVERLAPPEDWINDOW);
	App->mainWindow->WndProc_map[WM_COMMAND] = MenuCommand;
	App->mainWindow->drawroutines.push_front(draw);
	//App->mainWindow->props.allowfsexclusive(true);
	App->mainWindow->DXres->d3d11res.createresources = CreateRenderTargets;

	/*
	Window *cw = new Window();
	App->mainWindow->Next = cw;
	cw->props.background((HBRUSH)GetStockObject(WHITE_BRUSH));
	cw->props.height(50);
	cw->props.width(50);
	cw->props.left(105);
	cw->props.top(150);
	cw->props.style(WS_POPUP);
	*/

	Run(NULL);
	DeleteObject(blkbgrnd);

	return 0;
}
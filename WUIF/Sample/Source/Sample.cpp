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
#include "..\Headers\stdafx.h"
#include "..\Headers\Sample.h"

#define USED3D12
#define USED3D11
#define USED2D
#define MIND3DFL11_0
#define USEDISCRETE

#include "..\..\Include\WUIF.h"
#include <DirectXColors.h>

using namespace WUIF;

/*bool MenuCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
}*/

void draw(Window *pThis)
{
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pBlackBrush;
    ThrowIfFailed(
        pThis->GFX->D2D->d2dDeviceContext->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::Yellow),
            &pBlackBrush
        )
    );
    if (pThis->GFX->D2D->d2dDeviceContext)
    {
        pThis->GFX->D2D->d2dDeviceContext->BeginDraw();
        pThis->GFX->D2D->d2dDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
        //pThis->GFX->D2D->d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Red));
        D2D1_SIZE_F rtSize = pThis->GFX->D2D->d2dDeviceContext->GetSize();
        D2D1_RECT_F rectangle1 = D2D1::RectF(
            rtSize.width / 2 - 50.0f,
            rtSize.height / 2 - 50.0f,
            rtSize.width / 2 + 50.0f,
            rtSize.height / 2 + 50.0f
        );
        pThis->GFX->D2D->d2dDeviceContext->FillRectangle(
            &rectangle1,
            pBlackBrush.Get());

        ThrowIfFailed(
            pThis->GFX->D2D->d2dDeviceContext->EndDraw()
        );
    }
}

void CreateRenderTargets(D3D11Resources* pThis)
{
    //create the render target view
    ThrowIfFailed(pThis->window.GFX->DXGI->dxgiSwapChain1->GetBuffer(0, IID_PPV_ARGS(&pThis->d3d11BackBuffer)));
    ThrowIfFailed(pThis->d3d11Device1->CreateRenderTargetView(pThis->d3d11BackBuffer.Get(), NULL, &pThis->d3d11RenderTargetView));
    pThis->d3d11ImmediateContext->OMSetRenderTargets(1, pThis->d3d11RenderTargetView.GetAddressOf(), 0);
    D3D11_VIEWPORT vp;
    pThis->window.getWindowDPI();
    vp.Width = static_cast<float>(pThis->window.Scale(pThis->window.property.width()));
    vp.Height = static_cast<float>(pThis->window.Scale(pThis->window.property.height()));
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pThis->d3d11ImmediateContext->RSSetViewports(1, &vp);
}



int main(int argc, char *argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    //WinStruct appWin;
    App::mainWindow->property.background(DirectX::Colors::Aqua);
    App::mainWindow->property.windowname(TEXT("Sample Application"));
    //App::mainWindow->property.menuname(MAKEINTRESOURCEW(IDR_MENU1));
    App::mainWindow->property.height(300);
    App::mainWindow->property.width(320);
    App::mainWindow->property.top(100);
    App::mainWindow->property.left(100);
    App::mainWindow->property.minwidth(100);
    App::mainWindow->property.minheight(100);
    App::mainWindow->property.style(WS_OVERLAPPEDWINDOW);
    //App::mainWindow->WndProc_map[WM_COMMAND] = MenuCommand;
    App::mainWindow->drawroutines.push_front(draw);
    //App::mainWindow->props.allowfsexclusive(true);
    //App::mainWindow->DXres->d3d11res.RegisterResourceCreation(CreateRenderTargets, 1);

    Run(NULL);
    //App::mainWindow->GFX->D3D11->UnregisterResourceCreation(CreateRenderTargets);

    return 0;
}

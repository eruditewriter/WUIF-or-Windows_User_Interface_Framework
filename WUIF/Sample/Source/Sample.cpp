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



using namespace WUIF;

bool MenuCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, Window* pWin)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(pWin);

    bool handled = false;
    switch (LOWORD(wParam))
    {
    case ID_FILE_EXIT:
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
    case ID_FILE_CHANGEICON:
    {
        pWin->icon(LoadIcon(NULL, IDI_ERROR));
        pWin->iconsm(reinterpret_cast<HICON>(LoadImage(NULL, IDI_WARNING, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)));
    }
    }
    return handled;
}

void draw(Window *pThis)
{
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pBlackBrush;
    ThrowIfFailed(
        pThis->d2dDeviceContext->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::Yellow),
            &pBlackBrush
        )
    );
    if (pThis->d2dDeviceContext)
    {
        pThis->d2dDeviceContext->BeginDraw();
        pThis->d2dDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
        //pThis->GFX->D2D->d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Red));
        D2D1_SIZE_F rtSize = pThis->d2dDeviceContext->GetSize();
        D2D1_RECT_F rectangle1 = D2D1::RectF(
            rtSize.width / 2 - 50.0f,
            rtSize.height / 2 - 50.0f,
            rtSize.width / 2 + 50.0f,
            rtSize.height / 2 + 50.0f
        );
        pThis->d2dDeviceContext->FillRectangle(
            &rectangle1,
            pBlackBrush.Get());

        ThrowIfFailed(
            pThis->d2dDeviceContext->EndDraw()
        );
    }
}

void CreateRenderTargets(Window* pThis)
{
    //create the render target view
    ThrowIfFailed(pThis->dxgiSwapChain1->GetBuffer(0, IID_PPV_ARGS(&pThis->d3d11BackBuffer)));
    ThrowIfFailed(pThis->d3d11Device1->CreateRenderTargetView(pThis->d3d11BackBuffer.Get(), NULL, &pThis->d3d11RenderTargetView));
    pThis->d3d11ImmediateContext->OMSetRenderTargets(1, pThis->d3d11RenderTargetView.GetAddressOf(), 0);
    D3D11_VIEWPORT vp;
    pThis->getWindowDPI();
    vp.Width = static_cast<float>(pThis->Scale(pThis->width()));
    vp.Height = static_cast<float>(pThis->Scale(pThis->height()));
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pThis->d3d11ImmediateContext->RSSetViewports(1, &vp);
}

LRESULT CALLBACK SampleWindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main(int argc, char *argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    int retval = 0;
    if (SUCCEEDED(App::mainWindow->windowname(TEXT("Sample Application"))))
    {
        WNDCLASSEX wndclass = {}; //zero out
        wndclass.lpszClassName = TEXT("SampleClass");
        wndclass.cbSize = sizeof(WNDCLASSEX);
        wndclass.style = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc = SampleWindowProc;
        wndclass.hInstance = App::hInstance;
        wndclass.hIcon = LoadIcon(NULL, IDI_WARNING);
        wndclass.hCursor = LoadCursor(NULL, IDC_UPARROW);
        wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wndclass.hIconSm = reinterpret_cast<HICON>(LoadImage(NULL, IDI_INFORMATION, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
        ATOM sclassatom = RegisterClassEx(&wndclass);


        //App::mainWindow->classatom(sclassatom);
        App::mainWindow->background(DirectX::Colors::Aqua);
        App::mainWindow->menu(MAKEINTRESOURCE(IDR_MENU1));
        App::mainWindow->height(300);
        App::mainWindow->width(320);
        App::mainWindow->top(100);
        App::mainWindow->left(100);
        App::mainWindow->minwidth(100);
        App::mainWindow->minheight(100);
        App::mainWindow->style(WS_OVERLAPPEDWINDOW);
        App::mainWindow->WndProc_map[WM_COMMAND] = MenuCommand;
        App::mainWindow->drawroutines.push_front(draw);
        //App::mainWindow->allowfsexclusive(true);
        //App::mainWindow->DXres->d3d11res.RegisterResourceCreation(CreateRenderTargets, 1);

        Window* win2 = new Window();
        win2->classatom(sclassatom);
        win2->background(DirectX::Colors::Red);
        win2->menu(MAKEINTRESOURCE(IDR_MENU1));
        win2->height(300);
        win2->width(320);
        win2->top(200);
        win2->left(200);
        win2->minwidth(100);
        win2->minheight(100);
        win2->style(WS_OVERLAPPEDWINDOW);
        win2->WndProc_map[WM_COMMAND] = MenuCommand;
        win2->drawroutines.push_front(draw);


        Run(NULL);
        //App::mainWindow->GFX->D3D11->UnregisterResourceCreation(CreateRenderTargets);
    }
    else
    {
        retval = 1;
    }
    return retval;
}
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
#include "WUIF_Error.h"
#include "GFX\GFX.h"
#include "GFX\DXGI\DXGI.h"
#include "GFX\D3D\D3D12.h"
#include "GFX\D3D\D3D11.h"
#include "GFX\D2D\D2D.h"
#include "Application\Application.h"
#include "Window\Window.h"

using namespace WUIF;

GFXResources::GFXResources(Window &win) :
    DXGI(nullptr),
    D3D12(nullptr),
    D3D11(nullptr),
    D2D(nullptr),
    window(win)
{
    DXGI  = DBG_NEW DXGIResources(win);
    D3D11 = DBG_NEW D3D11Resources(win);
    D3D12 = DBG_NEW D3D12Resources(win);
    D2D = DBG_NEW D2DResources(win);
}

GFXResources::~GFXResources()
{
    delete DXGI;
    delete D3D11;
    delete D3D12;
    delete D2D;
}

void GFXResources::HandleDeviceLost()
{
#if defined(_DEBUG)
    HRESULT reason = D3D11->d3d11Device1->GetDeviceRemovedReason();
    DebugPrint(_T("Device removed! DXGI_ERROR code: 0x%X\n"), reason);
    DXGI->dxgiInfoQueue.Reset();
    DXGI->dxgiDebug.Reset();
    D3D11->d3dInfoQueue.Reset();
    D3D11->d3dDebug.Reset();
#endif
    DXGI->dxgiSwapChain1 = nullptr;
    DXGI->dxgiDevice.Reset();
    D3D11->d3d11ImmediateContext.Reset();
    D3D11->d3d11Device1.Reset();
    D3D12->d3d11on12Device.Reset();
    D3D12->d3dCommQueue.Reset();
    D3D12->d3d12Device.Reset();

    if (deviceNotify != nullptr)
    {
        // Notify the renderers that device resources need to be released.
        // This ensures all references to the existing swap chain are released so that a new one can be created.
        deviceNotify->OnDeviceLost();
    }

    if (WUIF::App::GFXflags & FLAGS::D3D12)
    {
        //create D3D12 device
        D3D12Resources::CreateStaticResources();
    }
    else
    {
        //create D3D11 device
        D3D11Resources::CreateStaticResources();
    }
    if (App::GFXflags & FLAGS::D2D)
    {
        D2DResources::CreateStaticResources();
    }
    //recreate the swap chain and resources for each window
    if (!App::Windows.empty()) //Windows is empty if all windows have been closed
    {
        for (std::forward_list<Window*>::iterator i = App::Windows.begin(); i != App::Windows.end(); ++i)
        {
            if (App::GFXflags & FLAGS::D3D12)
            {
                //static_cast<Window*>(*i)->GFX->D3D12->CreateDeviceResources();
            }
            else
            {
                static_cast<Window*>(*i)->GFX->D3D11->CreateDeviceResources();
            }
            if (App::GFXflags & FLAGS::D2D)
            {
                static_cast<Window*>(*i)->GFX->D2D->CreateDeviceResources();
            }
        }
    }
    if (deviceNotify != nullptr)
    {
        // Notify the renderers that resources can now be created again
        deviceNotify->OnDeviceRestored();
    }
    //App::paintmutex.unlock();
}

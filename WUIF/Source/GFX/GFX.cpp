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
#include "GFX\GFX.h"
#include "Application\Application.h"
#include "Window\Window.h"

namespace WUIF {

    namespace App {
        extern std::mutex veclock;
        extern bool vecwrite;
        extern std::condition_variable vecready;
        extern std::vector<Window*> Windows;
        extern bool is_vecwritable();
    }

    GFXResources::GFXResources(Window * const winptr) : D3D12Resources(winptr)
    {
    }

    void GFXResources::HandleDeviceLost()
    {
        DebugPrint(TEXT("Entering GFXResources::HandleDeviceLost"));
#ifdef _DEBUG
        //get reason for device removal
        HRESULT reason = d3d11Device1->GetDeviceRemovedReason();
        DebugPrint(TEXT("Device removed! DXGI_ERROR code: 0x%X\n"), reason);
        //reset all debug graphics resources
        dxgiInfoQueue.Reset();
        dxgiDebug.Reset();
        d3dInfoQueue.Reset();
        d3dDebug.Reset();
#endif
        //reset all graphics resources
        dxgiSwapChain1 = nullptr;
        dxgiDevice.Reset();
        d3d11ImmediateContext.Reset();
        d3d11Device1.Reset();
        d3d11on12Device.Reset();
        d3dCommQueue.Reset();
        d3d12Device.Reset();

        /*Notify the renderers that device resources need to be released.
        This ensures all references to the existing swap chain are released so that a new one can
        be created.*/
        if (deviceNotify != nullptr)
        {
            deviceNotify->OnDeviceLost();
        }

        //recreate static resources
        (WUIF::App::GFXflags & FLAGS::D3D12) ? CreateD3D12StaticResources() : CreateD3D11StaticResources();
        if (App::GFXflags & FLAGS::D2D)
        {
            CreateD2DStaticResources();
        }

        //recreate the swap chain and resources for each window
        //std::vector<Window*> winlist = App::GetWindows();
        WINVECLOCK
        if (!App::Windows.empty()) //Windows is empty if all windows have been closed
        {
            for (std::vector<Window*>::iterator i = App::Windows.begin(); i != App::Windows.end(); ++i)
            {
                if (App::GFXflags & FLAGS::D3D12)
                {
                    //static_cast<Window*>(*i)->GFX->CreateD3D12DeviceResources();
                }
                else
                {
                    static_cast<Window*>(*i)->CreateD3D11DeviceResources();
                }
                if (App::GFXflags & FLAGS::D2D)
                {
                    static_cast<Window*>(*i)->CreateD2DDeviceResources();
                }
            }
        }
        WINVECUNLOCK

        //Notify the renderers that resources can now be created again
        if (deviceNotify != nullptr)
        {
            deviceNotify->OnDeviceRestored();
        }
    }
}
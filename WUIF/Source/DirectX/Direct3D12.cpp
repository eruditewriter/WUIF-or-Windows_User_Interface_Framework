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
#include "WUIF_Main.h"
#include "WUIF_Error.h"
#include "GFX\GFX.h"
#include "GFX\D3D\D3D12.h"
#include "GFX\D3D\D3D11.h"
#include "GFX\DXGI\DXGI.h"
#include "Application\Application.h"
#include "Window\Window.h"

using namespace WUIF;
using namespace Microsoft::WRL;

ComPtr<ID3D12Device>       D3D12Resources::d3d12Device = nullptr;
ComPtr<ID3D12CommandQueue> D3D12Resources::d3dCommQueue = nullptr;
ComPtr<ID3D11On12Device>   D3D12Resources::d3d11on12Device = nullptr;

D3D12Resources::D3D12Resources(Window &win) :
    window(win)
{

}

D3D12Resources::~D3D12Resources()
{
    ReleaseNonStaticResources();
}

void D3D12Resources::CreateStaticResources()
{
    if (DXGIResources::dxgiFactory == nullptr)  //first time run or we've lost the dxgi factory
    {
        //enumerate the adapters and pick the first adapter to support the feature set to make the D3D11 device
        DXGIResources::GetDXGIAdapterandFactory();
    }

    HINSTANCE lib = LoadLibraryW(L"D3D12.dll");
    if (lib)
    {
        //function signature PFN_D3D12_CREATE_DEVICE is provided as a typedef by d3d12.h for D3D12CreateDevice
        PFN_D3D12_CREATE_DEVICE created3d12device = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(lib, "D3D12CreateDevice");
        ThrowIfFailed(created3d12device(DXGIResources::dxgiAdapter.Get(), App::minD3DFL, IID_PPV_ARGS(&d3d12Device)));
        FreeLibrary(lib);
    }
    else
    {
        throw WUIF_exception(_T("Unable to load D3D12.DLL"));
    }
    
    //If we are using D2D we need to setup D3D11on12
    if (App::GFXflags & WUIF::FLAGS::D2D)
        //if (parent->d2dres.d2dFactory != nullptr)
    {
        UINT d3dFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
        d3dFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        //D3D12 Device created, now setup the swap chain
        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        ThrowIfFailed(d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d3dCommQueue)));

        // Create an 11 device wrapped around the 12 device and share 12's command queue.
        //function signature PFN_D3D11ON12_CREATE_DEVICE is provided as a typedef by d3d12.h for D3D11On12CreateDevice
        lib = LoadLibrary(_T("D3D11.dll"));
        if (lib)
        {
            D3D_FEATURE_LEVEL featureLevels[] =
            {
                D3D_FEATURE_LEVEL_12_1,
                D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0,
                D3D_FEATURE_LEVEL_9_3,
                D3D_FEATURE_LEVEL_9_2,
                D3D_FEATURE_LEVEL_9_1
            };
            PFN_D3D11ON12_CREATE_DEVICE created3d11on12device = (PFN_D3D11ON12_CREATE_DEVICE)GetProcAddress(lib, "D3D11On12CreateDevice");
            ThrowIfFailed(created3d11on12device(
                d3d12Device.Get(), //Specifies a pre-existing D3D12 device to use for D3D11 interop
                d3dFlags,
                featureLevels,
                _countof(featureLevels),
                reinterpret_cast<IUnknown**>(d3dCommQueue.GetAddressOf()),
                1,
                0,
                reinterpret_cast<ID3D11Device**>(D3D11Resources::d3d11Device1.ReleaseAndGetAddressOf()),
                reinterpret_cast<ID3D11DeviceContext**>(D3D11Resources::d3d11ImmediateContext.ReleaseAndGetAddressOf()),
                nullptr
            ));
            FreeLibrary(lib);
        }
        else
        {
            throw WUIF_exception(_T("Unable to load D3D11.DLL"));
        }
        // Query the 11On12 device from the 11 device.
        ThrowIfFailed(D3D11Resources::d3d11Device1.As(&d3d11on12Device));

        ThrowIfFailed(d3d11on12Device.As(&DXGIResources::dxgiDevice));
    }
}

void D3D12Resources::ReleaseNonStaticResources()
{

}
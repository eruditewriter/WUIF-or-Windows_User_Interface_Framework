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
#include "GFX/GFX.h"
#include "Application/Application.h"
#include "Window/Window.h"

using namespace WUIF;
using namespace Microsoft::WRL;

WUIF::D3D12libAPI *WUIF::d3d12libAPI = nullptr;

ComPtr<ID3D12Device>       D3D12Resources::d3d12Device     = nullptr;
ComPtr<ID3D12CommandQueue> D3D12Resources::d3dCommQueue    = nullptr;
ComPtr<ID3D11On12Device>   D3D12Resources::d3d11on12Device = nullptr;

D3D12Resources::D3D12Resources(Window * const winptr) : D2DResources(winptr)
{ }

D3D12Resources::~D3D12Resources()
{
    ReleaseD3D12NonStaticResources();
}

void D3D12Resources::CreateD3D12StaticResources()
{
    DebugPrint(TEXT("CreateD3D11StaticResources"));
    if (DXGIResources::dxgiFactory == nullptr)  //first time run or we've lost the dxgi factory
    {
        //enumerate the adapters and pick the first adapter to support the feature set to make the D3D12 device
        DXGIResources::GetDXGIAdapterandFactory();
    }
    ThrowIfFailed(d3d12libAPI->D3D12CreateDevice(DXGIResources::dxgiAdapter.Get(), App::minD3DFL, IID_PPV_ARGS(d3d12Device.ReleaseAndGetAddressOf())));

#ifdef USETHIS
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
        ThrowIfFailed(d3d12libAPI->D3D11On12CreateDevice(
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
        // Query the 11On12 device from the 11 device.
        ThrowIfFailed(D3D11Resources::d3d11Device1.As(&d3d11on12Device));

        ThrowIfFailed(d3d11on12Device.As(&DXGIResources::dxgiDevice));
    }
#endif
}

void D3D12Resources::ReleaseD3D12NonStaticResources()
{
}
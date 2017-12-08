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
#include "GFX\D3D\D3D11.h"
#include "GFX\DXGI\DXGI.h"
//#include "DirectX\DirectX.h"
#include "Application\Application.h"
#include "Window\Window.h"

using namespace Microsoft::WRL;
using namespace WUIF;

ComPtr<ID3D11Device1>        D3D11Resources::d3d11Device1          = nullptr;
ComPtr<ID3D11DeviceContext1> D3D11Resources::d3d11ImmediateContext = nullptr;
//the following variables are for D3D11 debug purposes
#ifdef _DEBUG
ComPtr<ID3D11Debug> D3D11Resources::d3dDebug        = nullptr;
ComPtr<ID3D11InfoQueue>D3D11Resources::d3dInfoQueue = nullptr;
#endif

D3D11Resources::D3D11Resources(Window &win) :
    d3d11BackBuffer(nullptr),
    d3d11RenderTargetView(nullptr),
    window(win)
{
    //createresources = CreateRenderTargets;
}

D3D11Resources::~D3D11Resources()
{
    ReleaseNonStaticResources();
}

void D3D11Resources::CreateStaticResources()
{
    if (DXGIResources::dxgiFactory == nullptr)  //we've lost the dxgi factory
    {
        /*enumerate the adapters and pick the first adapter to support the feature set to make the
        D3D11 device*/
        DXGIResources::GetDXGIAdapterandFactory();
    }

    UINT d3d11Flags = 0; //D3D11_CREATE_DEVICE_FLAG
    if (App::GFXflags & WUIF::FLAGS::D2D) //using Direct2D
    {
        /*This flag adds support for surfaces with a different color channel ordering than the API
        default. It is required for compatibility with Direct2D.*/
        d3d11Flags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    }
    /*This array defines the set of DirectX hardware feature levels this app will support. if you
    include D3D_FEATURE_LEVEL_12_0 and/or D3D_FEATURE_LEVEL_12_1 in your array when run on versions
    of Windows prior to Windows 10 then you get a return code of E_INVALIDARG*/
    const D3D_FEATURE_LEVEL featureLevels[] =
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
    HRESULT hr = NULL;
    D3D_FEATURE_LEVEL returnedFeatureLevel;
    ComPtr<ID3D11Device> d3d11Device;
    ComPtr<ID3D11DeviceContext> d3d11Context;

#ifdef _DEBUG
    // Check for SDK Layer debug support.
    if (SUCCEEDED(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
        0,
        D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
        nullptr,                    // Any feature level will do.
        0,
        D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION
        nullptr,                    // No need to keep the D3D device reference.
        nullptr,                    // No need to know the feature level.
        nullptr                     // No need to keep the D3D device context reference.
    )))
    {
        //SDK layer debug support is available, set proper flag to initialize it at device creation
        d3d11Flags |= D3D11_CREATE_DEVICE_DEBUG;
    }
#else
    //Causes the Direct3D runtime to ignore registry settings that turn on the debug layer.
    d3d11Flags |= D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY;
#endif //_DEBUG
#ifdef REFDRIVER
    // Create Direct3D reference device
    if (FAILED(D3D11CreateDevice(
        nullptr,				   // Specify nullptr to use the default adapter.
        D3D_DRIVER_TYPE_REFERENCE, // Create a device using the hardware graphics driver.
        0,                         // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
        d3d11Flags,		           // Set debug and Direct2D compatibility flags.
        featureLevels,             // List of feature levels this app can support.
        _countof(featureLevels),   // Size of the list above.
        D3D11_SDK_VERSION,         // Always set this to D3D11_SDK_VERSION
        &d3d11Device,              // Returns the Direct3D device created.
        &returnedFeatureLevel,     // Returns feature level of device created.
        &d3d11Context              // Returns the device immediate context.
    )))
    {
        throw WUIF_exception(_T("Direct3D 11 device creation failed!"));
    }
    #else
    // Create Direct3D device and context
    if (App::winversion >= OSVersion::WIN10)
    {
        hr = D3D11CreateDevice(
            DXGIResources::dxgiAdapter.Get(),
            D3D_DRIVER_TYPE_UNKNOWN,    // With an actual adapter we must use TYPE_UNKNOWN
            0,                          // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE
            d3d11Flags,				    // Set debug and Direct2D compatibility flags.
            featureLevels,              // List of feature levels this app can support.
            _countof(featureLevels),    // Size of the list above.
            D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION
            d3d11Device.GetAddressOf(), // Returns the Direct3D device created.
            &returnedFeatureLevel,      // Returns feature level of device created.
            d3d11Context.GetAddressOf() // Returns the device immediate context.
        );
    }
    else
    {
        hr = D3D11CreateDevice(
            DXGIResources::dxgiAdapter.Get(),
            D3D_DRIVER_TYPE_UNKNOWN,     // With an actual adapter we must use TYPE_UNKNOWN
            0,                           // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE
            d3d11Flags,				     // Set debug and Direct2D compatibility flags.
            &featureLevels[2],           // List of feature levels this app can support -D3D12 levels
            _countof(featureLevels) - 2, // Size of the list above.
            D3D11_SDK_VERSION,           // Always set this to D3D11_SDK_VERSION
            d3d11Device.GetAddressOf(),  // Returns the Direct3D device created.
            &returnedFeatureLevel,       // Returns feature level of device created.
            d3d11Context.GetAddressOf()  // Returns the device immediate context.
        );
    }
    /*platform only supports DirectX 11.0 or D3D_FEATURE_LEVEL_12_0 and/or D3D_FEATURE_LEVEL_12_1
    in array when run on versions < Windows 10 since some older graphics adapters (i.e. laptops,
    integrated graphics chips) do not have 11.1 support fall back to 11.0 device. If the Direct3D
    11.1 runtime is present on the computer and pFeatureLevels is set to NULL, this function won't
    create a D3D_FEATURE_LEVEL_11_1 device. To create a D3D_FEATURE_LEVEL_11_1 device, you must
    explicitly provide a D3D_FEATURE_LEVEL array that includes D3D_FEATURE_LEVEL_11_1. If you
    provide a D3D_FEATURE_LEVEL array that contains D3D_FEATURE_LEVEL_11_1 on a computer that
    doesn't have the Direct3D 11.1 runtime installed, this function immediately fails with
    E_INVALIDARG*/
    if (hr == E_INVALIDARG)
    {
        #ifndef _DEBUG
        //not supported until D3D11.1
        d3d11Flags = d3d11Flags & ~D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY;
        #endif
        hr = D3D11CreateDevice(DXGIResources::dxgiAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, 0,
            d3d11Flags, &featureLevels[3], _countof(featureLevels) - 3,
            D3D11_SDK_VERSION, d3d11Device.ReleaseAndGetAddressOf(), &returnedFeatureLevel,
            d3d11Context.ReleaseAndGetAddressOf());
    }
    //if we failed to create device >= 11_0 then attempt to create WARP if allowed
    if (FAILED(hr))  // If the initialization fails, fall back to the WARP device if allowed
    {
        if (App::GFXflags & FLAGS::WARP)
        {
            //we are allowed to create a warp device
            {
                if (FAILED(D3D11CreateDevice(
                    nullptr,
                    D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                    0,
                    d3d11Flags,
                    &featureLevels[2],
                    _countof(featureLevels) - 2,
                    D3D11_SDK_VERSION,
                    d3d11Device.ReleaseAndGetAddressOf(),
                    &returnedFeatureLevel,
                    d3d11Context.ReleaseAndGetAddressOf())))
                {
                    throw WUIF_exception(_T("Direct3D 11 device creation failed!"));
                }
            }
        }
        else
        {
            throw WUIF_exception(_T("Direct3D 11 device creation failed!"));
        }
    }
    #endif //REFDRIVER

    //see https://blogs.msdn.microsoft.com/chuckw/2012/11/30/direct3d-sdk-debug-layer-tricks/
    #ifdef _DEBUG
    if (SUCCEEDED(d3d11Device.As(&d3dDebug)))
    {
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
            D3D11_MESSAGE_ID hide[] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                // Add more message IDs here as needed
            };

            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
    #endif // _DEBUG

    /*We now get the ID#D11Device1 and ID3D11DeviceContext1 interfaces since we require Windows 7
    SP1 with KB 2670838 might as well use 11.1 interface*/
    ThrowIfFailed(d3d11Device.As(&d3d11Device1));
    d3d11Device.Reset();
    ThrowIfFailed(d3d11Context.As(&d3d11ImmediateContext));
    d3d11Context.Reset();
    //get the IDXGIDevice
    ThrowIfFailed(d3d11Device1.As(&DXGIResources::dxgiDevice));
    return;
}

void D3D11Resources::CreateDeviceResources()
{
    CreateRenderTargets();
    if (!createresources.empty())
    {
        size_t count = createresources.size();
        for (unsigned int i = 1; i < UINT_MAX; ++i)
        {
            size_t const j = createresources.count(i);
            if (j > 0)
            {
                auto range = createresources.equal_range(i);
                for (auto k = range.first; k != range.second; ++k)
                {
                    count--;
                    reinterpret_cast<FPTR>(k->second)(this);
                }
            }
            if (count == 0)
            {
                break;
            }
        }
    }
}

void D3D11Resources::CreateRenderTargets()
{
    //create the render target view
    ThrowIfFailed(window.GFX->DXGI->dxgiSwapChain1->GetBuffer(0, IID_PPV_ARGS(&d3d11BackBuffer)));
    ThrowIfFailed(d3d11Device1->CreateRenderTargetView(d3d11BackBuffer.Get(), NULL, &d3d11RenderTargetView));
    d3d11ImmediateContext->OMSetRenderTargets(1, d3d11RenderTargetView.GetAddressOf(), 0);
    D3D11_VIEWPORT vp;
    window.getWindowDPI();
    vp.Width = static_cast<float>(window.Scale(window.property.width()));
    vp.Height = static_cast<float>(window.Scale(window.property.height()));
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    d3d11ImmediateContext->RSSetViewports(1, &vp);
}

void D3D11Resources::ReleaseNonStaticResources()
{
    d3d11BackBuffer.Reset();
    d3d11RenderTargetView.Reset();
}

void D3D11Resources::RegisterResourceCreation(FPTR func, unsigned int priority)
{
    createresources.insert({ priority, func });
}

void D3D11Resources::UnregisterResourceCreation(FPTR func)
{
    for (std::unordered_multimap<unsigned int, FPTR>::iterator i = createresources.begin(); i != createresources.end(); i++)
    {
        if (i->second == func)
        {
            createresources.erase(i);
            break;
        }
    }
}
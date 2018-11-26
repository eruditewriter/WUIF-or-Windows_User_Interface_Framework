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

using namespace Microsoft::WRL;
using namespace WUIF;

//define static variables
ComPtr<ID3D11Device1>        D3D11Resources::d3d11Device1          = nullptr;
ComPtr<ID3D11DeviceContext1> D3D11Resources::d3d11ImmediateContext = nullptr;
#ifdef _DEBUG
//the following variables are for D3D11 debug purposes
ComPtr<ID3D11Debug>          D3D11Resources::d3dDebug              = nullptr;
ComPtr<ID3D11InfoQueue>      D3D11Resources::d3dInfoQueue          = nullptr;
#endif

D3D11Resources::D3D11Resources(Window * const winptr) : DXGIResources(winptr), d3d11BackBuffer(nullptr), d3d11RenderTargetView(nullptr)
{
    //createresources = CreateD3D11RenderTargets;
}

D3D11Resources::~D3D11Resources()
{
    ReleaseD3D11NonStaticResources();
}

void D3D11Resources::CreateD3D11StaticResources()
{
    DebugPrint(TEXT("CreateD3D11StaticResources"));
    if (DXGIResources::dxgiFactory == nullptr)  //first time run or we've lost the dxgi factory
    {
        //enumerate the adapters and pick the first adapter to support the feature set to make the D3D11 device
        DXGIResources::GetDXGIAdapterandFactory();
    }
    /*Creates a device that supports BGRA formats. Required for Direct2D interoperability with Direct3D resources.*/
    if (App::GFXflags & FLAGS::D2D)
        d3d11createDeviceFlags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    #ifdef _DEBUG
    /*To use these flags, you must have D3D11*SDKLayers.dll installed; otherwise, device creation fails. To get D3D11_1SDKLayers.dll,
    install the SDK for Windows 8. Check for d3d11_1sdklayers.dll*/
    HINSTANCE d3d11sdklayers = LoadLibraryEx(TEXT("d3d11_1sdklayers.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (d3d11sdklayers)
    {
        d3d11createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
        /*If you use this flag and the current driver does not support shader debugging, device creation fails. Shader debugging requires a
        driver that is implemented to the WDDM for Windows 8 (WDDM 1.2). This value is not supported until Direct3D 11.1.*/
        //d3d11createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUGGABLE; //this doesn't seem to actually work
        FreeLibrary(d3d11sdklayers);
    }
    #endif
    #ifndef _DEBUG
    /*You can set this flag in your app, typically in release builds only, to prevent end users from using the DirectX Control Panel to
    monitor how the app uses Direct3D. MS shipped the last version of the DirectX SDK in June 2010. You can also set this flag in your app
    to prevent Direct3D debugging tools, such as Visual Studio Ultimate 2012, from hooking your app. Windows 8.1:  This flag doesn't
    prevent Visual Studio 2013 and later running on Windows 8.1 and later from hooking your app; instead use
    ID3D11DeviceContext2::IsAnnotationEnabled. This flag still prevents Visual Studio 2013 and later running on Windows 8 and earlier from
    hooking your app. This value is not supported until Direct3D 11.1.*/
    d3d11createDeviceFlags |= D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY;
    #endif
    /*This array defines the set of DirectX hardware feature levels this app will support. if you
    include D3D_FEATURE_LEVEL_12_0 and/or D3D_FEATURE_LEVEL_12_1 in your array when run on versions
    of Windows prior to Windows 10 then you get a return code of E_INVALIDARG. On <Win10 we
    pass &featureLevels[2]*/
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
    bool looped = false;
createdeviceagain:
    #ifdef REFDRIVER
    // Create Direct3D reference device
    if (FAILED(D3D11CreateDevice(
        nullptr,				   // Specify nullptr to use the default adapter.
        D3D_DRIVER_TYPE_REFERENCE, // Create a device using the hardware graphics driver.
        0,                         // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
        d3d11createDeviceFlags,		           // Set debug and Direct2D compatibility flags.
        featureLevels,             // List of feature levels this app can support.
        _countof(featureLevels),   // Size of the list above.
        D3D11_SDK_VERSION,         // Always set this to D3D11_SDK_VERSION
        &d3d11Device,              // Returns the Direct3D device created.
        &returnedFeatureLevel,     // Returns feature level of device created.
        &d3d11Context              // Returns the device immediate context.
    )))
    {
        SetLastError(WE_GRAPHICS_INVALID_DISPLAY_ADAPTER);
        throw WUIF_exception(TEXT("Direct3D 11 device creation failed!"));
    }
    #else
    // Create Direct3D device and context
    if (App::winversion >= OSVersion::WIN10)
    {
        hr = D3D11CreateDevice(
            dxgiAdapter.Get(),
            D3D_DRIVER_TYPE_UNKNOWN,    // With an actual adapter we must use TYPE_UNKNOWN
            0,                          // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE
            d3d11createDeviceFlags,				    // Set debug and Direct2D compatibility flags.
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
            dxgiAdapter.Get(),
            D3D_DRIVER_TYPE_UNKNOWN,     // With an actual adapter we must use TYPE_UNKNOWN
            0,                           // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE
            d3d11createDeviceFlags,				     // Set debug and Direct2D compatibility flags.
            &featureLevels[2],           // List of feature levels this app can support -D3D12 levels
            _countof(featureLevels) - 2, // Size of the list above.
            D3D11_SDK_VERSION,           // Always set this to D3D11_SDK_VERSION
            d3d11Device.GetAddressOf(),  // Returns the Direct3D device created.
            &returnedFeatureLevel,       // Returns feature level of device created.
            d3d11Context.GetAddressOf()  // Returns the device immediate context.
        );
    }
    #ifdef _DEBUG
    if ((hr == DXGI_ERROR_UNSUPPORTED) && (looped == false))//should only occur with D3D11_CREATE_DEVICE_DEBUGGABLE
    {
        d3d11createDeviceFlags &= ~(D3D11_CREATE_DEVICE_DEBUGGABLE);
        looped = true; //allow only one loop
        goto createdeviceagain;
    }
    #endif
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
        //remove flags that are D3D11.1
        #ifdef _DEBUG
        d3d11createDeviceFlags &= ~(D3D11_CREATE_DEVICE_DEBUGGABLE);
        #endif
        #ifndef _DEBUG
        d3d11createDeviceFlags &= ~(D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY);
        #endif
        hr = D3D11CreateDevice(dxgiAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, 0,
            d3d11createDeviceFlags, &featureLevels[3], _countof(featureLevels) - 3,
            D3D11_SDK_VERSION, d3d11Device.ReleaseAndGetAddressOf(), &returnedFeatureLevel,
            d3d11Context.ReleaseAndGetAddressOf());
    }
    if (FAILED(hr))
    {
        SetLastError(WE_GRAPHICS_INVALID_DISPLAY_ADAPTER);
        throw WUIF_exception(TEXT("Direct3D 11 device creation failed!"));
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

    /*We now get the ID#D11Device1 and ID3D11DeviceContext1 interfaces since we require Windows 7 SP1 with KB 2670838 might as well use
    11.1 interface*/
    d3d11Device1.Reset();
    ThrowIfFailed(d3d11Device.As(&d3d11Device1));
    d3d11Device.Reset();
    d3d11ImmediateContext.Reset();
    ThrowIfFailed(d3d11Context.As(&d3d11ImmediateContext));
    d3d11Context.Reset();
    //get the IDXGIDevice
    dxgiDevice.Reset();
    ThrowIfFailed(d3d11Device1.As(&dxgiDevice));

    DebugPrint(TEXT("D3D11 Device and Context created"));

    return;
}

void D3D11Resources::CreateD3D11DeviceResources()
{
    CreateD3D11RenderTargets();
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

void D3D11Resources::CreateD3D11RenderTargets()
{
    //create the render target view
    ThrowIfFailed(dxgiSwapChain1->GetBuffer(0, IID_PPV_ARGS(&d3d11BackBuffer)));
    ThrowIfFailed(d3d11Device1->CreateRenderTargetView(d3d11BackBuffer.Get(), NULL, &d3d11RenderTargetView));
    d3d11ImmediateContext->OMSetRenderTargets(1, d3d11RenderTargetView.GetAddressOf(), 0);
    D3D11_VIEWPORT vp;
    win->getWindowDPI();
    vp.Width = static_cast<float>(win->Scale(win->width()));
    vp.Height = static_cast<float>(win->Scale(win->height()));
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    d3d11ImmediateContext->RSSetViewports(1, &vp);
}

void D3D11Resources::ReleaseD3D11NonStaticResources()
{
    d3d11BackBuffer.Reset();
    d3d11RenderTargetView.Reset();
}

#ifdef _MSC_VER
#pragma warning(push)
//Avoid unnamed objects with custom construction and destruction
#pragma warning(disable: 26444)
#endif
void D3D11Resources::RegisterD3D11ResourceCreation(FPTR func, unsigned int priority)
{
    createresources.insert({ priority, func });
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif
}

#ifdef _MSC_VER
#pragma warning(push)
//Avoid unnamed objects with custom construction and destruction
#pragma warning(disable: 26444)
#endif
void D3D11Resources::UnregisterD3D11ResourceCreation(FPTR func)
{
    for (std::unordered_multimap<unsigned int, FPTR>::iterator i = createresources.begin(); i != createresources.end(); i++)
    {
        if (i->second == func)
        {
            createresources.erase(i);
            break;
        }
    }
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif
}
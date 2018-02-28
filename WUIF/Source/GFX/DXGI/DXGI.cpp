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



using namespace Microsoft::WRL;
using namespace WUIF;

//define static resources
ComPtr<IDXGIFactory2>  DXGIResources::dxgiFactory   = nullptr;
ComPtr<IDXGIAdapter1>  DXGIResources::dxgiAdapter   = nullptr;
ComPtr<IDXGIDevice>    DXGIResources::dxgiDevice    = nullptr;
#ifdef _DEBUG
ComPtr<IDXGIDebug>     DXGIResources::dxgiDebug     = nullptr;
ComPtr<IDXGIInfoQueue> DXGIResources::dxgiInfoQueue = nullptr;
#endif

DXGIResources::DXGIResources(Window * const winptr) :
    dxgiBackBuffer(nullptr),
    dxgiSwapChain1(nullptr),
    dxgiSwapChainDesc1({}),
    _HDRsupport(false),
    win(winptr)
    //tearingsupport(false)
{
    /**assign default values to DXGI_SWAP_CHAIN_DESC1**
    *BufferCount = value that describes the number of buffers in the swap chain - use
     double-buffering to minimize latency - 2 is required for flip effects.
    *SampleDesc.Count - number of multi-samples per pixel - flip effects can't use multi-sampling
     so set to 1
    *AlphaMode = value that identifies the transparency behavior of the swap-chain back buffer -
     DXGI_ALPHA_MODE_IGNORE indicates to ignore the transparency behavior and is mandatory for
     CreateSwapChainForHwnd
    *Format = structure that describes the display format - DXGI_FORMAT_B8G8R8A8_UNORM is the most
     common swap chain format.
    *BufferUsage = value that describes the surface usage and CPU access options for the back
     buffer -  DXGI_USAGE_RENDER_TARGET_OUTPUT indicates to use the surface or resource as an
     output render target
    *SwapEffect = value that describes the presentation model - DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
     indicates to use a flip presentation model and it persists the contents of the back buffer
     (cannot be used with multi-sampling)
       NB: MS documentation states that DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL is not available on Win 7
           but testing shows that it is available with Platform Update for Windows 7
    *Flags = value specifies options for swap-chain behavior
      *DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING - It is recommended to always use the tearing flag when
       it is available - this will be disabled if not supported
      *DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH - enable an application to switch modes by calling
       IDXGISwapChain::ResizeTarget. When switching from windowed to full-screen mode, the display
       mode or monitor resolution will be changed to match the dimensions of the application window
    **these values are already 0 on initialization and do not need to be set - noted for reference
      *Width = resolution width - 0 indicates match the size of the window
      *Height = resolution height - 0 indicates match the size of the window
      *SampleDesc.Quality = image quality level - default sampler mode, with no anti-aliasing, has
       a count of 1 and a quality level of 0
      *Stereo = specifies whether the full-screen display mode or the swap-chain back buffer is
       stereo (actual value is FALSE)
      *Scaling = DXGI_SCALING-typed value that identifies resize behavior - win7 only supports
       DXGI_SCALING_STRETCH (0) */
    dxgiSwapChainDesc1.BufferCount      = 2;
    dxgiSwapChainDesc1.SampleDesc.Count = 1;
    dxgiSwapChainDesc1.AlphaMode        = DXGI_ALPHA_MODE_IGNORE;
    dxgiSwapChainDesc1.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
    dxgiSwapChainDesc1.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgiSwapChainDesc1.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    dxgiSwapChainDesc1.Flags            = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
}

DXGIResources::~DXGIResources()
{
    dxgiBackBuffer.Reset();
    dxgiSwapChain1.Reset();
}

void DXGIResources::GetDXGIAdapterandFactory()
{
    DebugPrint(TEXT("Entering GetDXGIAdapterandFactory"));
    //First, create the DXGI Factory if it doesn't already exist
    if (!dxgiFactory)
    {
        DebugPrint(TEXT("Getting new DXGI Factory"));
        #ifdef _DEBUG
        /*In Windows 8, any DXGI factory created while DXGIDebug.dll was present on the system
        would load and use it. Starting in Windows 8.1, apps explicitly request that DXGIDebug.dll
        be loaded instead. Use CreateDXGIFactory2 and specify the DXGI_CREATE_FACTORY_DEBUG flag to
        request DXGIDebug.dll; the DLL will be loaded if it is present on the system.*/
        if (App::winversion >= OSVersion::WIN8_1)
        {
            if (App::GFXflags & FLAGS::D3D12) //already tested for Win10 in WUIF.cpp/InitResources
            {
                /*The D3D12 debug device is not enabled through a creation flag like it is in
                Direct3D 11. The debug device is only present if the Graphics Tools Windows 10
                optional feature is enabled.*/
                /*function signature PFN_D3D12_GET_DEBUG_INTERFACE is provided as a typedef by
                d3d12.h for D3D12GetDebugInterface*/
                PFN_D3D12_GET_DEBUG_INTERFACE pfnd3d12getdebuginterface =
                    reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(
                        GetProcAddress(App::libD3D12, "D3D12GetDebugInterface"));
                if (pfnd3d12getdebuginterface)
                {
                    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;   //Direct3D 12 debug controller
                    ThrowIfFailed(pfnd3d12getdebuginterface(IID_PPV_ARGS(debugController.ReleaseAndGetAddressOf())));
                    debugController->EnableDebugLayer();
                    debugController.Reset();
                }
            }
            //CreateDXGIFactory2 is available on Win > 8.1 and is only needed to load the debug dll
            using PFN_CREATE_DXGI_FACTORY_2 = HRESULT(WINAPI *)(UINT, REFIID, _Out_ void**);
            //GetModuleHandle could return NULL - dxgi.dll should be loaded and covered by _ASSERTE
            #ifdef _MSC_VER
            #pragma warning(suppress: 6387)
            #endif
            PFN_CREATE_DXGI_FACTORY_2 pfncreatefactory2 = reinterpret_cast<PFN_CREATE_DXGI_FACTORY_2>(GetProcAddress(GetModuleHandle(TEXT("dxgi.dll")), "CreateDXGIFactory2"));
            //assert if GetProcAddress fails by returning NULL
            ThrowIfFalse(pfncreatefactory2);
            ThrowIfFailed(pfncreatefactory2(DXGI_CREATE_FACTORY_DEBUG,
                IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
        }
        else
        {
        #endif //_DEBUG
            //Only need CreateDXGIFactory2 for debug so we can use CreateDXGIFactory1
            {
                ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
            }
        #ifdef _DEBUG
        }
        /*get the DXGI Debug and InfoQueue resources (MS says only available Win 8 or greater but
        have loaded on Win7 with Platform update and latest SDK)*/
        HINSTANCE lib = LoadLibrary(TEXT("dxgidebug.dll"));
        if (lib)
        {
            using PFN_DXGI_GET_DEBUG_INTERFACE = HRESULT(WINAPI *)(REFIID, void**);
            PFN_DXGI_GET_DEBUG_INTERFACE pfndxgigetdebuginterface =
                reinterpret_cast<PFN_DXGI_GET_DEBUG_INTERFACE>(GetProcAddress(lib, "DXGIGetDebugInterface"));
            if (pfndxgigetdebuginterface)
            {
                pfndxgigetdebuginterface(IID_PPV_ARGS(dxgiDebug.ReleaseAndGetAddressOf()));
                if (SUCCEEDED(pfndxgigetdebuginterface(
                    IID_PPV_ARGS(dxgiInfoQueue.ReleaseAndGetAddressOf()))))
                {
                    dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                    dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
                }
            }
            FreeLibrary(lib);
            lib = NULL;
        }
        #endif // _DEBUG
    }
    //factory exists - if we have an adapter release and re-enumerate
    DebugPrint(TEXT("Getting DXGI Adapter"));
    dxgiAdapter.Reset();
    ComPtr<IDXGIAdapter1> adapter;
    DXGI_ADAPTER_DESC1 desc = {};
    if (App::GFXflags & FLAGS::D3D12)
    {
        /*enumerate the hardware adapters until we find the first one that supports the minimum D3D
        feature level*/
        for (unsigned int adapterIndex = 0;
            DXGI_ERROR_NOT_FOUND !=
            dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf());
            ++adapterIndex)
        {
            if (FAILED(adapter->GetDesc1(&desc)))
            {
                //invalid adapter, continue along
                continue;
            }
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter for now
                continue;
            }
            //check to see if the adapter supports D3D12 but don't create the actual device yet
            /*Function signature PFN_D3D12_CREATE_DEVICE is provided as a typedef by d3d12.h for
            D3D12CreateDevice*/
            PFN_D3D12_CREATE_DEVICE pfnd3d12createdevice =
                reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(App::libD3D12,
                    "D3D12CreateDevice"));
            if (pfnd3d12createdevice)
            {
                if (SUCCEEDED(pfnd3d12createdevice(adapter.Get(), App::minD3DFL, _uuidof(ID3D12Device),
                    nullptr)))
                {
                    //adapter is good, now assign adapter to dxgiAdapter using Swap
                    DebugPrint(TEXT("Found D3D12 Adapter"));
                    adapter.Swap(dxgiAdapter);
                    return;
                }
            }
            else
            {
                //couldn't load PFN_D3D12_CREATE_DEVICE see if we can fallback to D3D11
                goto nod3d12adapter;
            }
        }
        //we didn't find a D3D12 hardware adapter, try, if allowed, to fallback to a warp device
        if (FLAGS::WARP & App::GFXflags)
        {
            ComPtr<IDXGIFactory4> dxgiFactory4;
            if (SUCCEEDED(dxgiFactory.As(&dxgiFactory4)))
            {
                if (SUCCEEDED(dxgiFactory4->EnumWarpAdapter(IID_PPV_ARGS(&adapter))))
                {
                    //adapter is good, now assign adapter to dxgiAdapter using Swap
                    DebugPrint(TEXT("Found D3D12 WARP Adapter: %s"), desc.Description);
                    adapter.Swap(dxgiAdapter);
                    dxgiFactory4.Reset();
                    return;
                }
                dxgiFactory4.Reset();
            }
        }
        //no capable WARP adapter or PFN_D3D12_CREATE_DEVICE not found
        nod3d12adapter:
            if (App::GFXflags & FLAGS::D3D11)
            {
                //no suitable D3D12 adapter but D3D11 allowed - so remove the GFXFlag for D3D12
                DebugPrint(TEXT("No D3D12 Adapter, fallback to D3D11"));
                const WUIF::FLAGS::GFX_Flags flagtempval = (App::GFXflags & (~FLAGS::D3D12));
                changeconst(&const_cast<WUIF::FLAGS::GFX_Flags&>(App::GFXflags),
                    &flagtempval);
                if (WUIF::App::libD3D12)
                {
                    FreeLibrary(WUIF::App::libD3D12);
                    WUIF::App::libD3D12 = NULL;
                }
            }
            else
            {
                //if D3D12 only throw an error
                SetLastError(WE_GRAPHICS_INVALID_DISPLAY_ADAPTER);
                throw WUIF_exception(TEXT("This application requires Direct3D 12 and there is not an available adapter!"));
            }
    }
    //no D3D12 adapter or D3D11 only, we've checked and faulted if D3D12 only, so try D3D11
    bool renderdriver = false;
    int  renderindex = 0;
    D3D_FEATURE_LEVEL returnedFeatureLevel;
    /*This array defines the set of DirectX hardware feature levels this app will support. if you
    include D3D_FEATURE_LEVEL_12_0 and/or D3D_FEATURE_LEVEL_12_1 in your array when run on versions
    of Windows prior to Windows 10 then you get a return code of E_INVALIDARG. On <Win10 we
    pass &featureLevels[2]*/
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
    for (unsigned int adapterIndex = 0; DXGI_ERROR_NOT_FOUND !=
        dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf());
        ++adapterIndex)
    {
        if (FAILED(adapter->GetDesc1(&desc)))
        {
            //invalid adapter so continue
            continue;
        }
        // Don't select the Basic Render Driver adapter for now
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            if (renderdriver)
            {
                //already found a basic render driver so ignore this one
                continue;
            }
            else
            {
                //mark we have a basic render driver in case no actual hardware is present (i.e. virtual machine)
                renderdriver = true;
                //mark the index we found it at
                renderindex = adapterIndex;
                continue;
            }
        }
        HRESULT hr = NULL;
        /*Set ppDevice and ppImmediateContext to NULL to determine which feature level is supported
        by looking at pFeatureLevel. If the Direct3D 11.1 runtime is present on the computer and
        pFeatureLevels is set to NULL, this function won't create a D3D_FEATURE_LEVEL_11_1 device.
        To create a D3D_FEATURE_LEVEL_11_1 device, you must explicitly provide a D3D_FEATURE_LEVEL
        array that includes D3D_FEATURE_LEVEL_11_1. If you provide a D3D_FEATURE_LEVEL array that
        contains D3D_FEATURE_LEVEL_11_1 on a computer that doesn't have the Direct3D 11.1 runtime
        installed, this function immediately fails with E_INVALIDARG*/
        if (App::winversion >= OSVersion::WIN10)
        {
            hr = (D3D11CreateDevice(
                adapter.Get(),
                D3D_DRIVER_TYPE_UNKNOWN, //must use D3D_DRIVER_TYPE_UNKNOWN if pAdapter is not null
                NULL,
                D3D11_CREATE_DEVICE_SINGLETHREADED,
                featureLevels,
                _countof(featureLevels),
                D3D11_SDK_VERSION,
                nullptr,
                &returnedFeatureLevel,
                nullptr));
        }
        else
        {
            hr = (D3D11CreateDevice(
                adapter.Get(),
                D3D_DRIVER_TYPE_UNKNOWN, //must use D3D_DRIVER_TYPE_UNKNOWN if pAdapter is not null
                NULL,
                D3D11_CREATE_DEVICE_SINGLETHREADED,
                &featureLevels[2],
                _countof(featureLevels) - 2,
                D3D11_SDK_VERSION,
                nullptr,
                &returnedFeatureLevel,
                nullptr));
        }
        if (SUCCEEDED(hr))
        {
            if (returnedFeatureLevel >= App::minD3DFL)
            {
                //adapter is good, now assign adapter to dxgiAdapter using Swap
                DebugPrint(TEXT("Found D3D11 Adapter: %s"), desc.Description);
                adapter.Swap(dxgiAdapter);
                return;
            }
        }
        else
        { //for 11.0 only systems we will get an E_INVALIDARG return
            if ((hr == E_INVALIDARG) && (App::minD3DFL == D3D_FEATURE_LEVEL_11_0))
            {
                hr = D3D11CreateDevice(
                    adapter.Get(),
                    D3D_DRIVER_TYPE_UNKNOWN, //must use D3D_DRIVER_TYPE_UNKNOWN if we pAdapter is not null
                    NULL,
                    D3D11_CREATE_DEVICE_SINGLETHREADED,
                    &featureLevels[3],
                    _countof(featureLevels) - 3,
                    D3D11_SDK_VERSION,
                    nullptr,
                    &returnedFeatureLevel,
                    nullptr);
                if (SUCCEEDED(hr))
                {
                    if (returnedFeatureLevel == App::minD3DFL)
                    {
                        //adapter is good, now assign adapter to dxgiAdapter using Swap
                        DebugPrint(TEXT("Found D3D11 Adapter: %s"), desc.Description);
                        adapter.Swap(dxgiAdapter);
                        return;
                    }
                }
            }
        }
    }
    //no suitable hardware adapters found, check if we found a Basic Render Driver and use that
    if ((renderdriver) && (App::GFXflags & FLAGS::WARP))
    {
        dxgiFactory->EnumAdapters1(renderindex, adapter.ReleaseAndGetAddressOf());
        HRESULT hr = D3D11CreateDevice(
            adapter.Get(),
            D3D_DRIVER_TYPE_UNKNOWN,
            NULL,
            D3D11_CREATE_DEVICE_SINGLETHREADED,
            featureLevels,
            _countof(featureLevels),
            D3D11_SDK_VERSION,
            nullptr,
            &returnedFeatureLevel,
            nullptr);
        if (SUCCEEDED(hr))
        {
            if (returnedFeatureLevel >= App::minD3DFL)
            {
                //adapter is good, now assign adapter to dxgiAdapter using Swap
                DebugPrint(TEXT("Found D3D11 WARP Adapter: %s"), desc.Description);
                adapter.Swap(dxgiAdapter);
                return;
            }
        }
        else
        {
            if ((hr == E_INVALIDARG) && (App::minD3DFL == D3D_FEATURE_LEVEL_11_0))
            {
                hr = D3D11CreateDevice(
                    adapter.Get(),
                    D3D_DRIVER_TYPE_UNKNOWN, //must use D3D_DRIVER_TYPE_UNKNOWN if pAdapter is not null
                    NULL,
                    D3D11_CREATE_DEVICE_SINGLETHREADED,
                    &featureLevels[1],
                    _countof(featureLevels) - 1,
                    D3D11_SDK_VERSION,
                    nullptr,
                    &returnedFeatureLevel,
                    nullptr);
                if (SUCCEEDED(hr))
                {
                    if (returnedFeatureLevel == App::minD3DFL)
                    {
                        //adapter is good, now assign adapter to dxgiAdapter using Swap
                        DebugPrint(TEXT("Found D3D11 WARP Adapter: %s"), desc.Description);
                        adapter.Swap(dxgiAdapter);
                        return;
                    }
                }
            }
            else
            {
                adapter.Reset();
                SetLastError(WE_GRAPHICS_INVALID_DISPLAY_ADAPTER);
                throw WUIF_exception(TEXT("Unable to find a suitable graphics adapter to run application"));
            }
        }
    }
    else
    {
        adapter.Reset();
        SetLastError(WE_GRAPHICS_INVALID_DISPLAY_ADAPTER);
        throw WUIF_exception(TEXT("Unable to find a suitable graphics adapter to run application!"));
    }
}

void DXGIResources::CreateSwapChain()
{
    if (dxgiSwapChain1)
    {
        // If the swap chain already exists, resize it.
        // Clear the previous window size specific context.
        ID3D11RenderTargetView* nullViews[] = { nullptr };
        win->d3d11ImmediateContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
        win->ReleaseD3D11NonStaticResources();
        win->d3d11RenderTargetView.Reset();
        win->d3d11BackBuffer.Reset();
        if (App::GFXflags & WUIF::FLAGS::D2D)
        {
            if (win->d2dDeviceContext)
            {
                win->d2dDeviceContext->SetTarget(nullptr);
            }
            win->d2dRenderTarget.Reset();
        }
        dxgiBackBuffer.Reset();
        win->d3d11ImmediateContext->Flush();
        win->d3d11ImmediateContext->ClearState();

        //Perform checks for flag values that are not supported in older OS
        if (App::winversion < OSVersion::WIN10)
        {
            SetLastError(WE_WRONGOSFORAPP);
            if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED)
            {
                throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 10 or higher!"));
            }
            if (App::winversion < OSVersion::WIN8_1)
            {
                if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
                {
                    throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 8.1 or higher!"));
                }
                if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER)
                {
                    throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 8.1 or higher!"));
                }
                if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO)
                {
                    throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 8.1 or higher!"));
                }
                if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO)
                {
                    throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 8.1 or higher!"));
                }

                if (App::winversion == OSVersion::WIN7)
                {
                    if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT)
                    {
                        throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 8 or higher!"));
                    }
                    if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER)
                    {
                        throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 8 or higher!"));
                    }
                    if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY)
                    {
                        throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 8 or higher!"));
                    }
                }
            }
            SetLastError(WE_OK);
        }
        if (win->allowfsexclusive())
        {
            dxgiSwapChainDesc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; //must be present if we allow fullscreen exclusive mode
        }
        HRESULT hr = dxgiSwapChain1->ResizeBuffers(
            dxgiSwapChainDesc1.BufferCount,         //number of buffers in the swap chain (0 = preserve existing number of buffers)
            win->actualwidth(),  //new width of the back buffer
            win->actualheight(),	//new height of the back buffer
            dxgiSwapChainDesc1.Format,              //DXGI_FORMAT value for the new format of the back buffer (0 = keep existing format)
            dxgiSwapChainDesc1.Flags);	            //DXGI_SWAP_CHAIN_FLAG values specifying options for swap-chain behavior

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            win->HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
            // and correctly set up the new device.
            return;
        }
        else
        {
            ThrowIfFailed(hr);
        }
    }
    else
    {
        //Perform checks for values that are not supported in older OS and if possible substitute a fallback
        if (App::winversion < OSVersion::WIN10)
        {
            SetLastError(WE_WRONGOSFORAPP);
            if (dxgiSwapChainDesc1.Scaling == DXGI_SCALING_ASPECT_RATIO_STRETCH)
            {
                //DXGI_SCALING_ASPECT_RATIO_STRETCH is only supported on Win10
                //we perform a substitution to equivalent legacy Win32 DXGI_SCALING_STRETCH
                dxgiSwapChainDesc1.Scaling = DXGI_SCALING_STRETCH;
            }
            if (dxgiSwapChainDesc1.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD)
            {
                //only supported in >=Win10
                throw WUIF_exception(TEXT("SwapChain Swap Effect requires Windows version 10 or higher!"));
            }
            if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED)
            {
                throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 10 or higher!"));
            }
            if (App::winversion < OSVersion::WIN8_1)
            {
                if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
                {
                    throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 8.1 or higher!"));
                }
                if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER)
                {
                    throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 8.1 or higher!"));
                }
                if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO)
                {
                    throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 8.1 or higher!"));
                }
                if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO)
                {
                    throw WUIF_exception(TEXT("SwapChain Flag requires Windows version 8.1 or higher!"));
                }
                if (App::winversion == OSVersion::WIN7)
                {
                    //following formats are not supported on Win7
                    switch (dxgiSwapChainDesc1.Format)
                    {
                    case DXGI_FORMAT_B5G6R5_UNORM:
                    case DXGI_FORMAT_B5G5R5A1_UNORM:
                    case DXGI_FORMAT_AYUV:
                    case DXGI_FORMAT_Y410:
                    case DXGI_FORMAT_Y416:
                    case DXGI_FORMAT_NV12:
                    case DXGI_FORMAT_P010:
                    case DXGI_FORMAT_P016:
                    case DXGI_FORMAT_420_OPAQUE:
                    case DXGI_FORMAT_YUY2:
                    case DXGI_FORMAT_Y210:
                    case DXGI_FORMAT_Y216:
                    case DXGI_FORMAT_NV11:
                    case DXGI_FORMAT_AI44:
                    case DXGI_FORMAT_IA44:
                    case DXGI_FORMAT_P8:
                    case DXGI_FORMAT_A8P8:
                    case DXGI_FORMAT_B4G4R4A4_UNORM:
                    {
                        throw WUIF_exception(TEXT("SwapChain Format requires Windows version 8 or higher!"));
                    }
                    break;
                    default:
                        break;
                    }
                }
            }
            SetLastError(WE_OK);
        }
        //set the description width/height to the scaled values
        dxgiSwapChainDesc1.Width = win->actualwidth();
        dxgiSwapChainDesc1.Height = win->actualheight();
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fs = {};
        fs.Windowed = TRUE;
        //validate if tearing support is available
        if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
        {
            BOOL allowTearing = FALSE;
            ComPtr<IDXGIFactory5> factory5;
            HRESULT hr = dxgiFactory.As(&factory5);
            if (SUCCEEDED(hr))
            {
                hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }
            if (FAILED(hr))
            {
                dxgiSwapChainDesc1.Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                DebugPrint(TEXT("WARNING: Variable refresh rate displays not supported/n"));
            }
        }
        if (win->allowfsexclusive())
        {
            dxgiSwapChainDesc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; //must be present if we allow fullscreen exclusive mode
        }
        // Create DXGI swap chain targeting a window handle
        HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(
            win->d3d11Device1.Get(), //D3D11 - this is a pointer to the Direct3D device for the swap chain
                                                //D3D12 - this is a pointer to a direct command queue
            win->hWnd,             //The HWND handle that is associated with the swap chain
            &dxgiSwapChainDesc1,                //A pointer to a DXGI_SWAP_CHAIN_DESC1 structure for the swap-chain description
            &fs,	//recommendation is to create a Windowed swap chain and then allow a change to full-screen
                    //Set it to NULL to create a windowed swap chain.
            NULL,   //A pointer to the IDXGIOutput interface for the output to restrict content to
            &dxgiSwapChain1); //A pointer to a variable that receives a pointer to the IDXGISwapChain1 interface for the swap chain
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            win->HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
            // and correctly set up the new device.
            return;
        }
        else
        {
            ThrowIfFailed(hr);
        }
    }

    /*we'll handle ALT-ENTER for fullscreen toggle
    Applications that want to handle mode changes or Alt+Enter themselves should call MakeWindowAssociation
    with the DXGI_MWA_NO_WINDOW_CHANGES flag after swap chain creation.*/
    //if (!DXres->winparent->property.allowfsexclusive())
    {
        ThrowIfFailed(dxgiFactory->MakeWindowAssociation(win->hWnd, DXGI_MWA_NO_ALT_ENTER));
    }
    if ((App::winversion >= OSVersion::WIN10) && (win->enableHDR))
    {
        DetectHDRSupport();
    }
    ComPtr<IDXGIDevice1> dxgiDevice1;
    dxgiDevice.As(&dxgiDevice1);
    // Ensure that DXGI doesn't queue more than one frame at a time.
    dxgiDevice1->SetMaximumFrameLatency(1);
    dxgiDevice1.Reset();
    // Get the back buffer as an IDXGISurface (Direct2D doesn't accept an ID3D11Texture2D directly as a render target)
    ThrowIfFailed(dxgiSwapChain1->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer)));
}

void DXGIResources::DetectHDRSupport()
{
    static bool checkSupport = true;
    // Output information is cached on the DXGI Factory. If it is stale we need to create
    // a new factory and re-enumerate the displays
    if (!dxgiFactory->IsCurrent())
    {
        dxgiFactory.Reset();
        GetDXGIAdapterandFactory();
    }
    // Get information about the display we are presenting to.
    ComPtr<IDXGIOutput> output;
    ComPtr<IDXGIOutput6> output6;
    ThrowIfFailed(dxgiSwapChain1->GetContainingOutput(&output));
    if (SUCCEEDED(output.As(&output6)))
    {
        DXGI_OUTPUT_DESC1 outputDesc;
        ThrowIfFailed(output6->GetDesc1(&outputDesc));
        _HDRsupport = (outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
        //DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R16G16B16A16_FLOAT
        DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
        switch (dxgiSwapChainDesc1.Format)
        {
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            // The application creates the HDR10 signal.
            colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
            break;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            // The system creates the HDR10 signal; application uses linear values.
            colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
            break;
        default:
            break;
        }
        ComPtr<IDXGISwapChain3> swapChain3;
        if (SUCCEEDED(dxgiSwapChain1.As(&swapChain3)))
        {
            UINT colorSpaceSupport = 0;
            if (SUCCEEDED(swapChain3->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport))
                && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
            {
                ThrowIfFailed(swapChain3->SetColorSpace1(colorSpace));
            }
            swapChain3.Reset();
        }
    }
}
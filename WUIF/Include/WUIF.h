/*Copyright (c) 2018 Jonathan Campbell

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.*/

// WUIF.h : Windows UI Framework general header file for applications
#pragma once
#ifdef _MSC_VER
    //linker options
    #ifdef _UNICODE
        #pragma comment(linker, "/SUBSYSTEM:WINDOWS,6.01 /ENTRY:wWinMainCRTStartup")
    #else
        #pragma comment(linker, "/SUBSYSTEM:WINDOWS,6.01 /ENTRY:WinMainCRTStartup")
    #endif // _UNICODE
    #pragma comment(lib, "dxgi")
    #ifdef _DEBUG
        #pragma comment(lib, "dxguid") //needed for dxgi debug features
    #endif // _DEBUG
    #ifdef USED3D12
        #pragma comment(lib, "d3d12")
    #endif // USED3D12
    #pragma comment(lib, "d3d11")
    #pragma comment(lib, "d2d1")
    #pragma comment(lib, "dwrite")

    //using Common Controls V6
    #pragma comment(linker,"\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif // _MSC_VER

#include "../Headers/WUIF_Main.h"
#include "../Headers/WUIF_Error.h"
#include "../Headers/Application/Application.h"
#include "../Headers/Window/Window.h"
#include "../Headers/GFX/GFX.h"


//define to indicate on hybrid graphics systems to prefer the discrete part by default
#ifdef USEDISCRETE
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement                  = 0x00000001;
    __declspec(dllexport) int   AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#ifdef USED2D
#define _USED2D
#endif
#ifdef USED3D12
#define _USED3D12
#endif
#ifdef USED3D11
#define _USED3D11
#endif
#ifdef USEWARP
#define _USEWARP
#endif

#ifdef MINVER_WIN7
#define _MINVER OSVersion::WIN7
#endif
#ifdef MINVER_WIN8
#define _MINVER OSVersion::WIN8
#endif
#ifdef MINVER_WIN8_1
#define _MINVER OSVersion::WIN8_1
#endif
#ifdef MINVER_WIN10
#define _MINVER OSVersion::WIN10
#endif
#ifdef MINVER_WIN10_1511
#define _MINVER OSVersion::WIN10_1511
#endif
#ifdef MINVER_WIN10_1607
#define _MINVER OSVersion::WIN10_1607
#endif
#ifdef MINVER_WIN10_1703
#define _MINVER OSVersion::WIN10_1703
#endif
#ifdef MINVER_WIN10_1709
#define _MINVER OSVersion::WIN10_1709
#endif
#ifdef MINVER_WIN10_1803
#define _MINVER OSVersion::WIN10_1803
#endif
#ifdef MINVER_WIN10_1809
#define _MINVER OSVersion::WIN10_1809
#endif
#ifndef _MINVER
#define _MINVER OSVersion::WIN7
#endif

/*This is a hack. WUIF::App::GFXflags should be set once and not modified, so in theory it should be a "const" variable. However we want
the flags to be modified by environment detection. That is for example, if there is no D3D12 allow a fallback to D3D11 and we modify the
value of WUIF::App::GFXflags after environment detection and allow no further changes by the programmer. The changeconst function gives the
flexibility needed but for "changeconst" function to work the flag must be in writable memory and that can only be achieved by declaring it
in assembly code. If we declare it here we get two definitions and now nothing will compile. So we declare a class tempInitializer and
assign the value to "appgfxflag" and in WUIF_Main.h place appgfxflag's value into "WUIF::App::GFXflags" which is what is actually used. We
use a class so that after the value is placed into WUIF::App::GFXflags we can delete the memory overhead of appgfxflag.*/
namespace WUIF {
    class tempInitializer {
    public:
        const FLAGS::GFX_Flags appgfxflag;
        const OSVersion minosversion;
        tempInitializer(FLAGS::GFX_Flags flagvalue, OSVersion osvalue) : appgfxflag(flagvalue), minosversion(osvalue) {}
    };

    #ifndef USERD3D11CREATEDEVICEFLAGS
    /*D3D11_CREATE_DEVICE_FLAG (defined in WUIF_D3D11.h)*/
    UINT D3D11Resources::d3d11createDeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
    #endif
    #ifndef USED2DCREATEFACTORYFLAGS
    D2D1_FACTORY_TYPE   D2DResources::d2d1factorytype = D2D1_FACTORY_TYPE_SINGLE_THREADED;
    #endif
    #ifndef  USEDWRITEFACTORYFLAGS
    DWRITE_FACTORY_TYPE D2DResources::dwfactorytype = DWRITE_FACTORY_TYPE_SHARED;
    #endif
}

#define _NOGFXSET //use for compile time assert if no gfx preprocessor macro flag defined
#ifdef _USED2D //set application flag based on defines - D2D requires D3D11
    #undef _NOGFXSET
    #ifdef _USED3D12
        #ifdef _USEWARP
            namespace WUIF { extern tempInitializer *tempInit = CRT_NEW tempInitializer((FLAGS::D2D | FLAGS::D3D11 | FLAGS::D3D12 | FLAGS::WARP), _MINVER); }
        #else
            namespace WUIF { extern tempInitializer *tempInit = CRT_NEW tempInitializer((FLAGS::D2D | FLAGS::D3D11 | FLAGS::D3D12), _MINVER); }
        #endif //_USEWARP
    #else
        #ifdef _USEWARP
            namespace WUIF { extern tempInitializer *tempInit = CRT_NEW tempInitializer((FLAGS::D2D | FLAGS::D3D11 | FLAGS::WARP), _MINVER); }
        #else
            namespace WUIF { extern tempInitializer *tempInit = CRT_NEW tempInitializer((FLAGS::D2D | FLAGS::D3D11), _MINVER); }
        #endif //_USEWARP
    #endif //_USED3D12
    //perform undefs so we skip the next checks
    #undef _USED2D
    #undef _USED3D11
    #undef _USED3D12
    #undef _USEWARP
#endif //_USED2D
#ifdef _USED3D12
    #undef _NOGFXSET
    #ifdef _USED3D11
        #ifdef _USEWARP
            namespace WUIF { extern tempInitializer *tempInit = CRT_NEW tempInitializer((FLAGS::D3D11 | FLAGS::D3D12 | FLAGS::WARP), _MINVER); }
        #else
            namespace WUIF { extern tempInitializer *tempInit = CRT_NEW tempInitializer((FLAGS::D3D11 | FLAGS::D3D12), _MINVER); }
        #endif //_USEWARP
    #else
        #ifdef _USEWARP
            namespace WUIF { extern tempInitializer *tempInit = CRT_NEW tempInitializer((FLAGS::D3D12 | FLAGS::WARP), _MINVER); }
        #else
            namespace WUIF { extern tempInitializer *tempInit = CRT_NEW tempInitializer((FLAGS::D3D12), _MINVER); }
        #endif //_USEWARP
    #endif //_USED3D11
    #undef _USED2D
    #undef _USED3D11
    #undef _USED3D12
    #undef _USEWARP
#endif //_USED3D12
#ifdef _USED3D11 //we can skip D2D, and D3D12 checks, they were already handled
    #undef _NOGFXSET
    #ifdef _USEWARP
        namespace WUIF { extern tempInitializer *tempInit = CRT_NEW tempInitializer((FLAGS::D3D11 | FLAGS::WARP), _MINVER); }
    #else
        namespace WUIF { extern tempInitializer *tempInit = CRT_NEW tempInitializer((FLAGS::D3D11), _MINVER); }
    #endif //_USEWARP
    #undef _USED2D //undef for consistency
    #undef _USED3D11
    #undef _USED3D12
    #undef _USEWARP
#endif
#ifdef _NOGFXSET
    static_assert(0, "GFX preprocessor macro not set!");
#endif

#define _NOMIND3D //use for compile time assert if no mind3dfl preprocessor macro flag defined
#ifdef MIND3DFL11_0
    #undef _NOMIND3D
    const D3D_FEATURE_LEVEL WUIF::App::minD3DFL = D3D_FEATURE_LEVEL_11_0;
#endif
#ifdef MIND3DFL11_1
    #undef _NOMIND3D
    const D3D_FEATURE_LEVEL WUIF::App::minD3DFL = D3D_FEATURE_LEVEL_11_1;
#endif
#ifdef MIND3DFL12_0
    #undef _NOMIND3D
    const D3D_FEATURE_LEVEL WUIF::App::minD3DFL = D3D_FEATURE_LEVEL_12_0;
#endif
#ifdef MIND3DFL12_1
    #undef _NOMIND3D
    const D3D_FEATURE_LEVEL WUIF::App::minD3DFL = D3D_FEATURE_LEVEL_12_1;
#endif
#ifdef _NOMIND3D
    static_assert(0, "minimum D3D Feature Level preprocessor macro not set!");
#endif
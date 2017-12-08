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
// WUIF.h : Windows UI Framework general header file
//
#pragma once
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "d3d11")
#ifdef USED3D12
#pragma comment(lib, "d3d12")
#endif // USED3D12
#pragma comment(lib, "dxgi")
#pragma comment(lib, "shcore")
#ifdef _DEBUG
#pragma comment(lib, "dxguid") //needed for dxgi debug features
#endif // _DEBUG

#ifdef _UNICODE
#pragma comment(linker, "/SUBSYSTEM:WINDOWS,6.01 /ENTRY:wWinMainCRTStartup")
#else
#pragma comment(linker, "/SUBSYSTEM:WINDOWS,6.01 /ENTRY:WinMainCRTStartup")
#endif

#include "..\Headers\WUIF_Main.h"
#include "..\Headers\WUIF_Error.h"
#include "..\Headers\Application\Application.h"
#include "..\Headers\Window\Window.h"
#include "..\Headers\GFX\GFX.h"
#include "..\Headers\GFX\DXGI\DXGI.h"
#include "..\Headers\GFX\D3D\D3D11.h"
#include "..\Headers\GFX\D3D\D3D12.h"
#include "..\Headers\GFX\D2D\D2D.h"


//define to indicate on hybrid graphics systems to prefer the discrete part by default
#ifdef USEDISCRETE
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#define warpflag  1u << (1 - 1)
#define d2dflag   1u << (2 - 1)
#define d3d11flag 1u << (3 - 1)
#define d3d12flag 1u << (4 - 1)

#ifdef USED2D //set application flag based on defines - D2D requires D3D11
    #ifdef USED3D12
        #ifdef USEWARP
            const unsigned int WUIF::flagexpr = d2dflag | d3d11flag | d3d12flag | warpflag;
        #else
            const unsigned int WUIF::flagexpr = d2dflag | d3d11flag | d3d12flag;
        #endif //USEWARP
    #else
        #ifdef USEWARP
            const unsigned int WUIF::flagexpr = d2dflag | d3d11flag | warpflag;
        #else
            const unsigned int WUIF::flagexpr = d2dflag | d3d11flag;
        #endif //USEWARP
    #endif //USED3D12
    //perform undefs so we skip the next checks
    #undef USED2D
    #undef USED3D11
    #undef USED3D12
    #undef USEWARP
#endif //USED2D
#ifdef USED3D12
    #ifdef USED3D11
        #ifdef USEWARP
            const unsigned int WUIF::flagexpr = d3d11flag | d3d12flag | warpflag;
        #else
            const unsigned int WUIF::flagexpr = d3d11flag | d3d12flag;
        #endif //USEWARP
    #else
        #ifdef USEWARP
            const unsigned int WUIF::flagexpr = d3d12flag | warpflag;
        #else
            const unsigned int WUIF::flagexpr = d3d12flag;
        #endif //USEWARP
    #endif //USED3D11
    #undef USED2D
    #undef USED3D11
    #undef USED3D12
    #undef USEWARP
#endif //USED3D12
#ifdef USED3D11 //we can skip D2D, and D3D12 checks, they were already handled
    #ifdef USEWARP
        const unsigned int WUIF::flagexpr = d3d11flag | warpflag;
    #else
        const unsigned int WUIF::flagexpr = d3d11flag;
    #endif
    #undef USED2D //undef for consistency
    #undef USED3D11
    #undef USED3D12
    #undef USEWARP
#endif //USED3D11

#ifdef MIND3DFL11_0
const D3D_FEATURE_LEVEL WUIF::App::minD3DFL = D3D_FEATURE_LEVEL_11_0;
#endif
#ifdef MIND3DFL11_1
const D3D_FEATURE_LEVEL WUIF::App::minD3DFL = D3D_FEATURE_LEVEL_11_1;
#endif
#ifdef MIND3DFL12_0
const D3D_FEATURE_LEVEL WUIF::App::minD3DFL = D3D_FEATURE_LEVEL_12_0;
#endif
#ifdef MIND3DFL12_1
const D3D_FEATURE_LEVEL WUIF::App::minD3DFL = D3D_FEATURE_LEVEL_12_1;
#endif

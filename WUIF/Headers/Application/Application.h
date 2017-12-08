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
#pragma once
#include <forward_list>
#include <stdint.h> //needed for uint_fast8_t
#include <d3dcommon.h> //needed for D3D_FEATURE_LEVEL
#include "..\Bitfield.h"

namespace WUIF {

    namespace FLAGS {
        BIT_FIELD(uint_fast8_t, GFX_Flags);
        BIT_MASK(GFX_Flags, WARP,  1);
        BIT_MASK(GFX_Flags, D2D,   2);
        BIT_MASK(GFX_Flags, D3D11, 3);
        BIT_MASK(GFX_Flags, D3D12, 4);
    }

    enum class OSVersion {
        UNKNOWN = 0,
        WIN7,
        WIN8,
        WIN8_1,
        WIN10,
        WIN10_1511, //WIN10_1511 - November Update
        WIN10_1607, //WIN10_1607 - Anniversary Update
        WIN10_1703, //WIN10_1703 - Creators Update
        WIN10_1709, //WIN10_1709 - Fall Creators Update
        WIN10_1803  //WIN10_1803 - ???
    };

    class Window;

    namespace App {
        extern const int               nCmdShow;
        extern const LPTSTR            lpCmdLine;
        extern const HINSTANCE         hInstance;
        extern const OSVersion         winversion; //which version of Windows OS app is running on
        extern const FLAGS::GFX_Flags  GFXflags;   //flags defined in code stating which configuration to run - declared in WUIF_Main.h
        extern const D3D_FEATURE_LEVEL minD3DFL;   //minimum D3DFL for the application - declared in WUIF.h

        extern       Window            *mainWindow;
        extern       std::forward_list<Window*> Windows;

        extern       HINSTANCE         libD3D12;

        //functions
        extern void(*ExceptionHandler)(void);
    };
}
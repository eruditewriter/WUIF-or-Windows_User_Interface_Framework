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
#include <vector>
#include <unordered_map>
#include <d3dcommon.h> //needed for D3D_FEATURE_LEVEL
#include <ShellScalingApi.h>
#include "..\WUIF_Const.h"

namespace WUIF {
    extern void __fastcall changeconst(_In_ void *var, _In_ const void *value);

    class Window;

    namespace App {
        extern const volatile int              nCmdShow;
        extern const volatile LPTSTR           lpCmdLine;
        extern const volatile HINSTANCE        hInstance;
        extern const volatile OSVersion        winversion; //which version of Windows OS app is running on
        extern const volatile FLAGS::GFX_Flags GFXflags;   //flags defined in code stating which configuration to run - declared in WUIF_Main.h
        extern const volatile PROCESS_DPI_AWARENESS * const volatile processdpiawareness; //pointer to value of processes' PROCESS_DPI_AWARENESS
        extern const volatile DPI_AWARENESS_CONTEXT * const volatile processdpiawarenesscontext; //pointer to value of processes' DPI_AWARENESS_CONTEXT
        extern const D3D_FEATURE_LEVEL  minD3DFL;   //minimum D3DFL for the application - declared in WUIF.h

        extern       Window            *mainWindow;

        extern       HINSTANCE          libD3D12;

        //functions
        extern inline const std::vector<Window*>& GetWindows();
        extern void(*ExceptionHandler)(void);

        /*Global user WindowProcedure function - this is a general WndProc for all windows of the
        application*/
        extern std::unordered_map<int, WNDPROC> GWndProc_map;
    };
}

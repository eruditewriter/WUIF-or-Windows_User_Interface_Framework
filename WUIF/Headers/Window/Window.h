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
#include <unordered_map>
#include <forward_list>
#include <wrl/client.h> //needed for ComPtr
#include <dxgi1_5.h>    //needed for DXGI resources
#include "WindowProperties.h"
#include "GFX\GFX.h"

namespace WUIF {

    class Window;

    typedef void(*winptr)(Window*);
    struct wndprocThunk;

    class Window : public WindowProperties, public GFXResources
    {
    public:
         Window();
        ~Window();

        //variables
        HWND		 hWnd;
        HWND         hWndParent;
        bool         enableHDR;

        //user WindowProcedure function
        typedef bool(*WndProc)(HWND, UINT, WPARAM, LPARAM, Window*);
        std::unordered_map<int, WndProc> WndProc_map;

        //functions
        void        DisplayWindow();
        UINT        getWindowDPI();
        void        ToggleFullScreen();
        void        Present();

        std::forward_list<winptr> drawroutines;

        ATOM classatom() const { return _classatom; }
        void classatom(_In_ const ATOM v);
    private:
        ATOM         _classatom; //class for window creation
        WNDPROC       cWndProc;  //sub-classed original WndProc;
        long          instance;  //number of window instances created
        wndprocThunk *thunk;	 //thunk for overloading WndProc with pointer to class object

        bool          standby;   //if window is occluded we won't present any frames

        //functions
        WNDPROC pWndProc();      //returns a pointer to the window's WndProc thunk
        //default WndProc for windows
        #if defined(_M_IX86)     //if compiling for x86
        static LRESULT CALLBACK _WndProc(_In_ Window*, _In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM);
        #elif defined(_M_AMD64)  //if compiling for x64
        static LRESULT CALLBACK _WndProc(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM, _In_ Window*);
        #endif
        //sub-classed substitute T_SC_WindowProc
        static LRESULT CALLBACK T_SC_WindowProc(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM);
    };
}
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


namespace WUIF {

    class GFXResources;
    class DXResources;
    class Application;
    class Window;
    class WinStruct;

    typedef void(*winptr)(Window*);
    struct wndprocThunk;

    class Window
    {
        friend WindowProperties;
    public:
         Window();
        ~Window();



        ATOM wndclassatom;

        //class for window creation
        WindowProperties property;
        HWND		     hWnd;
        HWND             hWndParent;
        GFXResources     *GFX;


        bool             enableHDR;

        //user WindowProcedure function
        typedef bool(*WndProc)(HWND, UINT, WPARAM, LPARAM);
        std::unordered_map<int, WndProc> WndProc_map;

        //functions
        WNDCLASSEX wndclass(); //returns class object registered with window
        void       DisplayWindow();
        void       Present();
        void       ToggleFullScreen();
        UINT       getWindowDPI();
        int        Scale(int x) { return (_fullscreen ? x : MulDiv(x, scaleFactor, 100)); };
        bool       fullscreen() { return _fullscreen; }
        std::forward_list<winptr> drawroutines;

    private:
        long             instance;    //number of window instances created
        bool             initialized; //whether the window has been registered and created
        wndprocThunk*    thunk;	      //thunk for overloading WndProc with pointer to class object
        UINT             scaleFactor; //dpi scaling factor
        bool             _fullscreen; //is window "fullscreen"
        bool             standby;     //if window is occluded we won't present any frames

                                      //functions
        #if defined(_M_IX86)    //if compiling for x86
        static LRESULT CALLBACK _WndProc( Window*, HWND, UINT,   WPARAM, LPARAM  );
        #elif defined(_M_AMD64) //if compiling for x64
        static LRESULT CALLBACK _WndProc( HWND,    UINT, WPARAM, LPARAM, Window* );
        #endif
        WNDPROC          pWndProc(); //returns a pointer to the window's WndProc thunk
    };

}
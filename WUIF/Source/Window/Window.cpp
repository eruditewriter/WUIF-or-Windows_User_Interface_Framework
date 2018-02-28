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
#include <string> //needed for int to string conversion
#include <ShellScalingApi.h> //needed for PROCESS_DPI_AWARENESS, PROCESS_PER_MONITOR_DPI_AWARE, MONITOR_DPI_TYPE, MDT_EFFECTIVE_DPI
#include "Application\Application.h"
#include "Window\Window.h"
#include "Window\WndProcThunk.h"
#include "GFX\GFX.h"

HANDLE WUIF::wndprocThunk::heapaddr = NULL;
long   WUIF::wndprocThunk::thunkInstances = 0;

namespace{
    //cumulative counting of number of windows created
    static long numwininstances = 0;
}

namespace WUIF {
    namespace App {
        std::mutex veclock;
        bool vecwrite = true;
        std::condition_variable vecready;
        //internal Window collection vector - not exposed publicly
        std::vector<Window*> Windows;
        /*Function to return a copy of the Window collection vector*/
        inline const std::vector<Window*>& GetWindows()
        {
            return Windows;
        }

        bool is_vecwritable()
        {
            return vecwrite;
        }
    }

    Window::Window() : WindowProperties(this), GFXResources(this),
        hWnd(NULL),
        hWndParent(NULL),
        enableHDR(false),
        _classatom(NULL),
        cWndProc(NULL),
        instance(0),
        thunk(nullptr),
        standby(false)
    {
        //initialize thunk
        thunk = CRT_NEW wndprocThunk;
        if (!thunk->Init(this, reinterpret_cast<DWORD_PTR>(_WndProc)))
        {
            delete thunk;
            thunk = nullptr;
            SetLastError(WE_CRITFAIL);
            throw WUIF_exception(TEXT("WndProc thunk failed to initialize properly!"));
        }
        //increment instance - use InterlockedIncrement to avoid any conflicts
        instance = InterlockedIncrement(&numwininstances);

        WINVECLOCK
        App::Windows.push_back(this);
        WINVECUNLOCK
    }

    Window::~Window()
    {
        if (thunk != nullptr)
        {
            delete thunk;
            thunk = nullptr;
        }
        int i = 0;
        WINVECLOCK
        for (std::vector<Window*>::iterator winlist = App::Windows.begin(); winlist != App::Windows.end(); winlist++)
        {
            if (*winlist == this)
            {
                App::Windows.erase(App::Windows.begin() + i);
                break;
            }
            i++;
        }
        WINVECUNLOCK
        if (_classatom)
        {
            if (!cWndProc)
            {
                //Before calling this function, an application must destroy all windows created with the specified class.
                if (UnregisterClass(MAKEINTATOM(_classatom), App::hInstance))
                {
                    _classatom = NULL;
                }
            }
        }
    }

    void Window::classatom(_In_ const ATOM v)
    {
        if (!initialized)
        {
            _classatom = v;
        }
    }

    /*inline WNDPROC Window::pWndProc()
    inline function to return a pointer to the window's WndProc thunk or throw an exception
    */
    WNDPROC Window::pWndProc()
    {
        return (thunk ? thunk->GetThunkAddress() : throw WUIF_exception(TEXT("Invalid thunk pointer!")));
    }

    /*LRESULT CALLBACK App::T_SC_WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
    This is a temporary WindowProc to assist with sub-classing. It will change the original window's 
    class WndProc pointer to the window's thunk, which points to the default _WndProc. This must
    happen in WM_NCCREATE. The *this is passed as lpCreateParams from the CreateWindowEx call in
    DisplayWindow(). It's a static class function so we can keep pWndProc private*/
    LRESULT CALLBACK Window::T_SC_WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_NCCREATE:
        {
            //change the WndProc from the class to the Window thunk
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(reinterpret_cast<WUIF::Window*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams)->pWndProc()));
            //now call the WM_NCCREATE function of _WndProc to finish
            return SendMessage(hwnd, WM_NCCREATE, wParam, lParam);
        }
        break;
        default:
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        }
    }


    /*void Window::DisplayWindow()
    If the window is not initialized it will register the class and create the window and then
    show this window
    */
    void Window::DisplayWindow()
    {
        if (!initialized)
        {
            //get the number of digits in 'instance'
            int digits = 1;
            int pten = 10;
            while (pten <= instance) {
                digits++;
                pten *= 10;
            }
            if (!_classatom)
            {
                /*assign values to the WNDCLASSEX structure prior to calling RegisterClassEx
                cbSize        - size, in bytes, of this structure. Set this member to sizeof(WNDCLASSEX)
                style         - Window Class Styles to be applied - using CS_HREDRAW | CS_VREDRAW
                lpfnWndProc   - a pointer to the window procedure, here it is a pointer to wndprocThunk
                cbClsExtra    - number of extra bytes to allocate following the structure - not used
                cbWndExtra    - number of extra bytes to allocate following the window instance - not used
                hInstance     - handle to the instance that contains the window procedure for the class
                hIcon         - handle to the class icon
                hCursor       - handle to the class cursor
                hbrBackground - handle to the class background brush - ignored but using default of
                                    (HBRUSH)(COLOR_WINDOW + 1) - background handled by D3D or D2D
                lpszMenuName  - pointer to a null-terminated character string that specifies the
                                    resource name of the class menu - not used
                lpszClassName - pointer to a null-terminated string specifying the window class name
                hIconSm       - handle to a small icon that is associated with the window class
                */

                /*create a string for the class name. use 'W' and append the instance thus guaranteeing
                uniqueness) [i.e. 'W1']*/
                LPCTSTR pszFormat = TEXT("W%d");
                LPTSTR classname = CRT_NEW TCHAR[digits + 2]; //+1 for 'W' and +1 for null terminator
                /*write 'W + [character equivalent of instance number]' to classname; use
                StringCchPrintf as the mechanism to convert instance to a character equivalent*/
                ThrowIfFailed(StringCchPrintf(classname, digits + 2, pszFormat, instance));
                WNDCLASSEX wndclass    = {}; //zero out
                wndclass.lpszClassName = classname;
                wndclass.cbSize        = sizeof(WNDCLASSEX);
                wndclass.style         = CS_HREDRAW | CS_VREDRAW;
                wndclass.lpfnWndProc   = pWndProc();
                wndclass.hInstance     = App::hInstance;
                wndclass.hIcon         = _icon;
                wndclass.hCursor       = _classcursor;
                wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
                wndclass.hIconSm       = _iconsm;

                /*register our WNDCLASSEX (wndclass) for the window and store result in our class atom
                variable. We will use the class atom instead of the class name*/
                _classatom = RegisterClassEx(&wndclass);
                delete[] classname;
                if (_classatom == 0)
                {
                    //failed
                    throw WUIF_exception(TEXT("Unable to register window class!"));
                }
            }
            else
            {
                //class already registered, get the WndProc and subclass
                WNDCLASSEX wndclass = {};
                wndclass.cbSize = sizeof(WNDCLASSEX);
                ThrowIfFalse(GetClassInfoEx(App::hInstance, MAKEINTATOM(_classatom), &wndclass));
                if (wndclass.lpfnWndProc != Window::T_SC_WindowProc)
                {
                    //create a dummy window as we need an hwnd to change the class WndProc
                    HWND tempwin = CreateWindowEx(NULL, MAKEINTATOM(_classatom), NULL, NULL, 0, 0, 0, 0, NULL, NULL, App::hInstance, NULL);
                    if (tempwin)
                    {
                        if (SetClassLongPtr(tempwin, GCLP_WNDPROC, reinterpret_cast<LONG_PTR>(Window::T_SC_WindowProc)))
                            cWndProc = wndclass.lpfnWndProc;
                        DestroyWindow(tempwin);
                    }
                }
                _icon        = wndclass.hIcon;
                _iconsm      = wndclass.hIconSm;
                _classcursor = wndclass.hCursor;
            }

            if (_windowname == nullptr)
            {
                //create a string for the window name. Use 'Window' and append the instance
                LPCTSTR pszFormat2 = TEXT("Window%d");
                _windowname = CRT_NEW TCHAR[digits + 7]; //+6 for 'Window' and +1 for null terminator
                ThrowIfFailed(StringCchPrintf(_windowname, digits + 7, pszFormat2, instance));
            }

            /*Check if the window is requesting a different dpi thread context from the main (only
            in Win10 1607). If it's different change the thread context prior to creating the
            window and then change back*/
            DPI_AWARENESS_CONTEXT dpicontext = NULL;
            HMODULE lib = NULL;
            using PFN_SET_THREAD_DPI_AWARENESS_CONTEXT = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
            PFN_SET_THREAD_DPI_AWARENESS_CONTEXT pfnsetthreaddpiawarenesscontext = nullptr;
            if ((_threaddpiawarenesscontext != NULL) && (App::winversion >= OSVersion::WIN10_1607) && (hWndParent == NULL))
            {
                lib = GetModuleHandle(TEXT("user32.dll"));
                if (lib)
                {
                    using PFN_ARE_DPI_AWARENESS_CONTEXTS_EQUAL = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT, DPI_AWARENESS_CONTEXT);
                    PFN_ARE_DPI_AWARENESS_CONTEXTS_EQUAL pfnaredpiawarenesscontextsequal = (PFN_ARE_DPI_AWARENESS_CONTEXTS_EQUAL)GetProcAddress(lib, "AreDpiAwarenessContextsEqual");
                    if (pfnaredpiawarenesscontextsequal)
                    {
                        using PFN_GET_THREAD_DPI_AWARENESS_CONTEXT = DPI_AWARENESS_CONTEXT(WINAPI*)(void);
                        PFN_GET_THREAD_DPI_AWARENESS_CONTEXT pfngetthreaddpiawarenesscontext = (PFN_GET_THREAD_DPI_AWARENESS_CONTEXT)GetProcAddress(lib, "GetThreadDpiAwarenessContext");
                        if (pfngetthreaddpiawarenesscontext)
                        {
                            if ((pfnaredpiawarenesscontextsequal(pfngetthreaddpiawarenesscontext(), _threaddpiawarenesscontext)) == TRUE)
                            {
                                pfnsetthreaddpiawarenesscontext = (PFN_SET_THREAD_DPI_AWARENESS_CONTEXT)GetProcAddress(lib, "SetThreadDpiAwarenessContext");
                                if (pfnsetthreaddpiawarenesscontext)
                                {
                                    dpicontext = pfnsetthreaddpiawarenesscontext(_threaddpiawarenesscontext);
                                }
                            }
                        }
                    }
                }
            }

            //NB: we store the window handle in the NCCREATE message of our WndProc
            hWnd = CreateWindowEx(_exstyle, //extended window style
                MAKEINTATOM(_classatom),  //null-terminated string created by a previous call to the RegisterClassEx function
                _windowname,	            //the window name
                _style & (~WS_VISIBLE),     //the style of the window being created, make sure the WS_VISIBLE bit is off as we do not want the window to show yet
                _left,			            //initial horizontal position of the window
                _top,			            //initial vertical position of the window
                _width,			            //width, in device units, of the window
                _height,		            //height, in device units, of the window
                hWndParent,		            //handle to the parent or owner window of the window being created
                _menu,			            //handle to a menu, or specifies a child-window identifier
                App::hInstance,	            //handle to the instance of the module to be associated with the window
                reinterpret_cast<LPVOID>(this)); //pointer to a value to be passed to the window through the CREATESTRUCT structure
            if (dpicontext != NULL) //restore dpi thread context
            {
                if (pfnsetthreaddpiawarenesscontext)
                {
                    pfnsetthreaddpiawarenesscontext(dpicontext);
                }
            }
            if (hWnd == NULL)
            {
                throw WUIF_exception(TEXT("Unable to create window"));
            }
            initialized = true; //window class is registered and window is "created"
            ShowWindow(hWnd, _cmdshow); //show the window
            UpdateWindow(hWnd); //now paint the window
        }
        return;
    }

    UINT Window::getWindowDPI()
    {
        if (((App::processdpiawarenesscontext != nullptr) && (*App::processdpiawarenesscontext == DPI_AWARENESS_CONTEXT_UNAWARE)) || ((App::processdpiawareness != nullptr) && (*App::processdpiawareness == PROCESS_DPI_UNAWARE)))
        {
            return 96;
        }
        if (App::winversion > OSVersion::WIN8_1)  //Windows 10 provides GetDpiForWindow, a per monitor DPI scaling
        {
            HMODULE lib = GetModuleHandle(TEXT("user32.dll"));
            if (lib)
            {
                //Windows 10 uses GetDpiForWindow to get the dynamic DPI
                using GETDPIFORWINDOW = UINT(WINAPI *)(HWND);
                GETDPIFORWINDOW getdpiforwindow = (GETDPIFORWINDOW)GetProcAddress(lib, "GetDpiForWindow");
                if (getdpiforwindow)
                {
                    UINT dpi = getdpiforwindow(hWnd);
                    scaleFactor = MulDiv(dpi, 100, 96);
                    return dpi;
                }
            }
        }
        if (App::winversion > OSVersion::WIN8) //either not Win10 or GetDpiForWindow didn't load, try Win 8.1 function
        {
            //Windows 8.1 has the GetDpiForMonitor function
            HINSTANCE lib = LoadLibrary(TEXT("shcore.dll"));
            if (lib)
            {
                using GETDPIFORMONITOR = HRESULT(WINAPI *)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
                GETDPIFORMONITOR getdpi = (GETDPIFORMONITOR)GetProcAddress(lib, "GetDpiForMonitor");
                if (getdpi)
                {
                    HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                    UINT dpiy = 0;
                    UINT dpix = 0;
                    /*MDT_EFFECTIVE_DPI - The effective DPI. This value should be used when determining the
                    correct scale factor for scaling UI elements. This incorporates the scale factor set by
                    the user for this specific display.*/
                    getdpi(hMonitor, MDT_EFFECTIVE_DPI, &dpix, &dpiy); //uDpi will be set to the monitor's dpi
                    scaleFactor = MulDiv(dpix, 100, 96);
                    FreeLibrary(lib);
                    return dpix;
                }
                FreeLibrary(lib);
            }
        }
        /*
        if (DXres->d2dres.d2dFactory)  //if D2D is available we can fallback to using it to get the system DPI
        {
        FLOAT dpiX, dpiY;
        DXres->d2dres.d2dFactory->ReloadSystemMetrics();
        DXres->d2dres.d2dFactory->GetDesktopDpi(&dpiX, &dpiY);
        scaleFactor = MulDiv(static_cast<int>(dpiX), 100, 96);
        return static_cast<int>(dpiX);
        }*/
        int dpi = GetDeviceCaps(GetDC(hWnd), LOGPIXELSX);
        scaleFactor = MulDiv(dpi, 100, 96);
        return dpi;
    }

    void Window::ToggleFullScreen()
    {
        /*You may not release a swap chain in full-screen mode because doing so may create
        thread contention (which will cause DXGI to raise a non-continuable exception).
        Before releasing a swap chain, first switch to windowed mode*/
        if (_fullscreen)
        {
            _fullscreen = false;
            //if already fullscreen we are leaving fullscreen
            //switching full screen state will override our position variables - so store
            int width = _prevwidth;
            int height = _prevheight;
            int left = _prevleft;
            int top = _prevtop;
            _prevwidth = _width;
            _prevheight = _height;
            _prevleft = _left;
            _prevtop = _top;
            if (_allowfsexclusive)
            {
                HRESULT hr = dxgiSwapChain1->SetFullscreenState(FALSE, NULL);
                //reset position variables to proper non-fullscreen values
                _width = width;
                _height = height;
                _left = left;
                _top = top;
                getWindowDPI();
                SetWindowPos(hWnd, HWND_TOP, _left, _top, Scale(_width), Scale(_height), SWP_SHOWWINDOW);
                /*
                DXGI_SWAP_CHAIN_DESC desc;
                DXres->DXGIres.dxgiSwapChain1->GetDesc(&desc);
                getWindowDPI();
                desc.BufferDesc.Width = Scale(width);
                desc.BufferDesc.Height = Scale(height);
                DXres->DXGIres.dxgiSwapChain1->ResizeTarget(&desc.BufferDesc);
                */
                if ((hr != S_OK) && (hr != DXGI_STATUS_MODE_CHANGE_IN_PROGRESS))
                {
                    _fullscreen = true;
                    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
                    {
                        MessageBox(NULL, TEXT("Unable to toggle fullscreen mode at the moment!"), TEXT("Mode-Switch Error"), (MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
                        return;
                    }
                    else
                    {
                        SetLastError(hr);
                        throw WUIF_exception(TEXT("Critical error toggling full-screen switch!"));
                    }
                }
            }
            else
            {
                //reset position variables to proper non-fullscreen values
                _width = width;
                _height = height;
                _left = left;
                _top = top;
                DWORD styletemp = _exstyle; //temp variable to hold the DWORD styles
                _exstyle = _prevexstyle;
                _prevexstyle = styletemp;
                styletemp = _style;
                _style = _prevstyle;
                _prevstyle = styletemp;
                //set the styles back to pre-fullscreen values
                SetWindowLongPtr(hWnd, GWL_EXSTYLE, _exstyle);
                SetWindowLongPtr(hWnd, GWL_STYLE, _style);
                //set the window position back to pre-fullscreen values
                getWindowDPI();
                SetWindowPos(hWnd, HWND_TOP, _left, _top, Scale(_width), Scale(_height), SWP_SHOWWINDOW);
                /*
                DXres->DXGIres.CreateSwapChain();
                //setup D3D dependent resources
                if (DXres->useD3D12)
                {
                //pThis->DXres->d3d12res.CreateD12Resources();
                }
                else //use D3D11
                {
                DXres->d3d11res.CreateDeviceResources();
                }
                if (App::flags & WUIF::FLAGS::D2D)
                {
                DXres->d2dres.CreateDeviceResources();
                }
                SendMessage(hWnd, WM_PAINT, NULL, NULL);
                */
            }
        }
        else //windowed
        {
            _fullscreen = true;
            //in case of multi-monitor setup, get which monitor is nearest to window
            MONITORINFO info = {};
            /*You must set the cbSize member of the structure to sizeof(MONITORINFO) or sizeof(MONITORINFOEX)
            before calling the GetMonitorInfo function. Doing so lets the function determine the type of
            structure you are passing to it.*/
            info.cbSize = sizeof(MONITORINFO);
            HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
            GetMonitorInfo(monitor, &info);
            if (_allowfsexclusive) //fsexclusive routine
            {
                int width = _width;
                int height = _height;
                int left = _left;
                int top = _top;
                DXGI_SWAP_CHAIN_DESC desc = {};
                dxgiSwapChain1->GetDesc(&desc);
                desc.BufferDesc.Width = info.rcMonitor.right - info.rcMonitor.left;
                desc.BufferDesc.Height = info.rcMonitor.bottom - info.rcMonitor.top;
                dxgiSwapChain1->ResizeTarget(&desc.BufferDesc);
                HRESULT hr = dxgiSwapChain1->SetFullscreenState(TRUE, NULL);
                //HRESULT hr = DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;
                /*it is advisable to call ResizeTarget again with the RefreshRate member of DXGI_MODE_DESC
                zeroed out. This amounts to a no-operation instruction in DXGI, but it can avoid issues
                with the refresh rate*/
                dxgiSwapChain1->GetDesc(&desc);
                desc.BufferDesc.RefreshRate.Denominator = 0;
                desc.BufferDesc.RefreshRate.Numerator = 0;
                dxgiSwapChain1->ResizeTarget(&desc.BufferDesc);
                _prevwidth = width;
                _prevheight = height;
                _prevleft = left;
                _prevtop = top;
                if ((hr != S_OK) && (hr != DXGI_STATUS_MODE_CHANGE_IN_PROGRESS))
                {
                    _fullscreen = false;
                    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
                    {
                        //revert window
                        width = _prevwidth;
                        height = _prevheight;
                        left = _prevleft;
                        top = _prevtop;
                        _width = width;
                        _height = height;
                        _left = left;
                        _top = top;
                        desc.BufferDesc.Width = width;
                        desc.BufferDesc.Height = height;
                        dxgiSwapChain1->ResizeTarget(&desc.BufferDesc);
                        //SetWindowPos(hWnd, HWND_NOTOPMOST, _prevleft, _prevtop, _actualwidth, _actualheight, SWP_SHOWWINDOW);
                        MessageBox(NULL, TEXT("Unable to toggle fullscreen mode at the moment!/nThe resource is not currently available, but it might become available later."), TEXT("Mode-Switch Error"), (MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
                    }
                    else
                    {
                        SetLastError(hr);
                        throw WUIF_exception(TEXT("Critical error toggling full-screen switch!"));
                    }
                }
            }
            else //non fs exclusive
            {
                //set the values to the fullscreen, borderless "windowed" mode
                //don't use the function as we are changing multiple values, just make one SetWindowPos call
                _prevwidth = _width;
                _prevheight = _height;
                _prevleft = _left;
                _prevtop = _top;
                //set the styles
                exstyle(WS_EX_APPWINDOW | WS_EX_TOPMOST);
                //style(WS_POPUP | WS_VISIBLE);
                DWORD newstyle = _style & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME);
                style(newstyle);
                //set the window position
                SetWindowPos(hWnd, HWND_TOPMOST, info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right - info.rcMonitor.left,
                    info.rcMonitor.bottom - info.rcMonitor.top, SWP_SHOWWINDOW);
            }
        }
        return;
    }

    void Window::Present()
    {
        if (App::GFXflags & FLAGS::D3D12)
        {
            //clear backbuffer
        }
        else
        {
            if (d3d11RenderTargetView)
            {
                //clear backbuffer
                d3d11ImmediateContext->ClearRenderTargetView(d3d11RenderTargetView.Get(), background());
            }
        }
        if (!drawroutines.empty())
        {
            for (std::forward_list<winptr>::iterator dr = drawroutines.begin(); dr != drawroutines.end(); ++dr)
            {
                (*dr)(this);
            }
        }

        /*
        DXGI_PRESENT_PARAMETERS parameters = { 0 };
        parameters.DirtyRectsCount = 0;
        parameters.pDirtyRects = nullptr;
        parameters.pScrollRect = nullptr;
        parameters.pScrollOffset = nullptr;
        */

        /*When using sync interval 0, it is recommended to always pass the tearing
        flag when it is supported, even when presenting in windowed mode.
        However, this flag cannot be used if the app is in fullscreen mode as a
        result of calling SetFullscreenState.*/
        UINT presentflags = 0;
        if (!((_allowfsexclusive) && (_fullscreen)))
        {
            if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
            {
                presentflags |= DXGI_PRESENT_ALLOW_TEARING;
            }
        }
        if (standby)
        {
            presentflags |= DXGI_PRESENT_TEST;
        }
        HRESULT hr = dxgiSwapChain1->Present(0, presentflags);
        if ((hr == S_OK) && (standby))
        {
            //take window out of standby
            standby = false;
            presentflags &= ~DXGI_PRESENT_TEST;
            hr = dxgiSwapChain1->Present(0, presentflags);
        }
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            HandleDeviceLost();
        }
        /*IDXGISwapChain1::Present1 will inform you if your output window is entirely occluded via
        DXGI_STATUS_OCCLUDED. When this occurs it is recommended that your application go into
        standby mode (by calling IDXGISwapChain1::Present1 with DXGI_PRESENT_TEST) since resources
        used to render the frame are wasted. Using DXGI_PRESENT_TEST will prevent any data from
        being presented while still performing the occlusion check. Once IDXGISwapChain1::Present1
        returns S_OK, you should exit standby mode; do not use the return code to switch to standby
        mode as doing so can leave the swap chain unable to relinquish full-screen mode.*/
        if (hr == DXGI_STATUS_OCCLUDED)
        {
            standby = true;
        }
        if (!dxgiFactory->IsCurrent())
        {
            dxgiFactory.Reset();
            GetDXGIAdapterandFactory();
        }
    }
}
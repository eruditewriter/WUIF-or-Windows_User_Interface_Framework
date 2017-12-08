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
#include "WUIF_Main.h"
#include "WUIF_Error.h"
#include "Application\Application.h"
#include "Window\Window.h"
#include "Window\WndProcThunk.h"
#include "GFX\GFX.h"
#include "GFX\DXGI\DXGI.h"
#include "GFX\D3D\D3D11.h"
#include "GFX\D3D\D3D12.h"
#include "GFX\D2D\D2D.h"


HANDLE WUIF::wndprocThunk::heapaddr          = NULL;
long   WUIF::wndprocThunk::thunkInstances    = 0;

namespace {
    static long  numwininstances = 0;
}

namespace WUIF {

    Window::Window() :
        GFX(nullptr),
        wndclassatom(NULL),
        property(this),
        hWnd(NULL),
        hWndParent(NULL),
        enableHDR(false),
        thunk(nullptr),
        instance(0),
        initialized(false),
        _fullscreen(false),
        standby(false)
    {
        //increment instance - use InterlockedIncrement to avoid any conflicts
        instance = InterlockedIncrement(&numwininstances);
        GFX = DBG_NEW GFXResources(*this);

        //register thunk
        thunk = DBG_NEW wndprocThunk;
        if (!(thunk->Init(this, (DWORD_PTR)_WndProc))) //get our thunk's address
        {
            throw WUIF_exception(_T("WndProc thunk failed to initialize properly!"));
        }
        App::Windows.push_front(this);
    }

    Window::~Window()
    {
        delete GFX;
        GFX = nullptr;
        delete thunk;
        thunk = nullptr;
        App::Windows.remove(this);
    }

    void Window::DisplayWindow()
    {
        //assign values to the WNDCLASSEX structure prior to calling RegisterClassEx
        WNDCLASSEX wndclass = {};                                    //zero out
        wndclass.cbSize = sizeof(WNDCLASSEX);
        wndclass.style = property.classtyle();                     //Window Class Styles to be applied
        wndclass.lpfnWndProc = pWndProc();                            //a pointer to the window procedure, here it is a pointer to wndprocThunk
        wndclass.cbClsExtra = 0;							            //number of extra bytes to allocate following the window-class structure
        wndclass.cbWndExtra = 0;							            //number of extra bytes to allocate following the window instance
        wndclass.hInstance = App::hInstance;                       //handle to the instance that contains the window procedure for the class
        wndclass.hIcon = property.icon();                          //handle to the class icon
        wndclass.hCursor = property.cursor();	                    //handle to the class cursor
        wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);                    //handle to the class background brush
        wndclass.lpszMenuName = property.menuname();				        //pointer to a null-terminated character string that specifies the resource name of the class menu
        #ifdef _UNICODE
        std::wstring s = L"Window" + std::to_wstring(instance); //create a string for the class name. use Window and append the window instance (guaranteeing uniqueness)
        #else
        std::string s = "Window" + std::to_string(instance); //create a string for the class name. use Window and append the window instance (guaranteeing uniqueness)
        #endif // _UNICODE
        wndclass.lpszClassName = s.c_str();					            //pointer to a null-terminated string specifying the window class name - a generic default
        wndclass.hIconSm = property.iconsm();					    //handle to a small icon that is associated with the window class

                                                                //register our WNDCLASSEX (wndclass) for the window and store in our class atom variable. We use the class atom instead of the class name
        wndclassatom = RegisterClassEx(&wndclass);
        if (wndclassatom == 0) //failed
        {
            throw WUIF_exception(_T("Unable to register window class!"));
        }

        //check if the window is requesting a different dpi thread context from the main (only in Win10)
        //if it's different change the thread context prior to creating the window and then change back
        DPI_AWARENESS_CONTEXT dpicontext = NULL;
        HINSTANCE lib = NULL;
        if (App::winversion >= OSVersion::WIN10)
        {
            lib = LoadLibrary(_T("user32.dll"));
            if (lib)
            {
                using AREDPICONTEXTSEQUAL = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT, DPI_AWARENESS_CONTEXT);
                AREDPICONTEXTSEQUAL aredpiawarenesscontextsequal = (AREDPICONTEXTSEQUAL)GetProcAddress(lib, "AreDpiAwarenessContextsEqual");
                if (aredpiawarenesscontextsequal) //if this fails it will fall through to the next case and try SetProcessDpiAwareness
                {
                    using GETTHREADDPICONTEXT = DPI_AWARENESS_CONTEXT(WINAPI*)(void);
                    GETTHREADDPICONTEXT getthreaddpiawarenesscontext = (GETTHREADDPICONTEXT)GetProcAddress(lib, "GetThreadDpiAwarenessContext");
                    if (getthreaddpiawarenesscontext)
                    {
                        if (!(aredpiawarenesscontextsequal(getthreaddpiawarenesscontext(), property.dpiawareness())))
                        {
                            using SETTHREADDPIAWARE = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
                            SETTHREADDPIAWARE setthreaddpiaware = (SETTHREADDPIAWARE)GetProcAddress(lib, "SetThreadDpiAwarenessContext");
                            if (setthreaddpiaware)
                            {
                                dpicontext = setthreaddpiaware(property.dpiawareness());
                            }
                        }
                    }
                }
            }
        }

        //NB: we store the window handle in the NCCREATE message of our WndProc
        hWnd = CreateWindowEx(property.exstyle(), //extended window style
            MAKEINTATOM(wndclassatom),         //null-terminated string created by a previous call to the RegisterClassEx function
            property.windowname(),		           //the window name
            property.style(),                     //the style of the window being created, make sure the WS_VISIBLE bit is off as we do not want the window to show yet
            property.left(),			           //initial horizontal position of the window
            property.top(),			           //initial vertical position of the window
            property.width(),			           //width, in device units, of the window
            property.height(),			           //height, in device units, of the window
            hWndParent,		                   //handle to the parent or owner window of the window being created
            property.menu(),			           //handle to a menu, or specifies a child-window identifier
            App::hInstance,		               //handle to the instance of the module to be associated with the window
            nullptr);                          //pointer to a value to be passed to the window through the CREATESTRUCT structure
        if (hWnd == NULL)//pointer to a value to be passed to the window through the CREATESTRUCT structure
        {
            throw WUIF_exception(_T("Unable to create window"));
        }
        initialized = true; //window class is registered and window is "created"
        if (dpicontext != NULL) //restore dpi thread context
        {
            using SETTHREADDPIAWARE = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
            SETTHREADDPIAWARE setthreaddpiaware = (SETTHREADDPIAWARE)GetProcAddress(lib, "SetThreadDpiAwarenessContext");
            if (setthreaddpiaware)
            {
                setthreaddpiaware(dpicontext);
            }
        }
        if (lib)
        {
            FreeLibrary(lib);
        }
        ShowWindow(hWnd, property.cmdshow()); //show the window
        UpdateWindow(hWnd); //now paint the window
        return;
    }

    WNDPROC Window::pWndProc() //returns a pointer to the window's WndProc thunk
    {
        return (thunk ? thunk->GetThunkAddress() : throw WUIF_exception(TEXT("Invalid thunk pointer!")));
    };

    WNDCLASSEX	Window::wndclass()
    {
        WNDCLASSEX winclass = {};
        if ((GetClassInfoEx(App::hInstance, MAKEINTATOM(wndclassatom), &winclass)) != 0)
        {
            winclass = {};
        }
        return winclass;
    }


    UINT Window::getWindowDPI()
    {
        if (App::winversion > OSVersion::WIN8_1)  //Windows 10 provides GetDpiForWindow, a per monitor DPI scaling
        {
            HINSTANCE lib = LoadLibrary(_T("user32.dll"));
            if (lib)
            {
                //Windows 10 uses GetDpiForWindow to get the dynamic DPI
                using GETDPIFORWINDOW = UINT(WINAPI *)(HWND);
                GETDPIFORWINDOW getdpiforwindow = (GETDPIFORWINDOW)GetProcAddress(lib, "GetDpiForWindow");
                if (getdpiforwindow)
                {
                    UINT dpi = getdpiforwindow(hWnd);
                    scaleFactor = MulDiv(dpi, 100, 96);
                    FreeLibrary(lib);
                    return dpi;
                }
                FreeLibrary(lib);
            }
        }
        if (App::winversion > OSVersion::WIN8) //either not Win10 or GetDpiForWindow didn't load, try Win 8.1 function
        {
            //Windows 8.1 has the GetDpiForMonitor function
            HINSTANCE lib = LoadLibrary(_T("shcore.dll"));
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
            int width = property._prevwidth;
            int height = property._prevheight;
            int left = property._prevleft;
            int top = property._prevtop;
            property._prevwidth = property._width;
            property._prevheight = property._height;
            property._prevleft = property._left;
            property._prevtop = property._top;
            if (property._allowfsexclusive)
            {
                HRESULT hr = GFX->DXGI->dxgiSwapChain1->SetFullscreenState(FALSE, NULL);
                //reset position variables to proper non-fullscreen values
                property._width = width;
                property._height = height;
                property._left = left;
                property._top = top;
                getWindowDPI();
                SetWindowPos(hWnd, HWND_TOP, property._left, property._top, Scale(property._width), Scale(property._height), SWP_SHOWWINDOW);
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
                        MessageBox(NULL, _T("Unable to toggle fullscreen mode at the moment!"), _T("Mode-Switch Error"), (MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
                        return;
                    }
                    else
                    {
                        throw WUIF_exception(_T("Critical error toggling full-screen switch!"));
                    }
                }
            }
            else
            {
                //reset position variables to proper non-fullscreen values
                property._width = width;
                property._height = height;
                property._left = left;
                property._top = top;
                DWORD styletemp = property._exstyle; //temp variable to hold the DWORD styles
                property._exstyle = property._prevexstyle;
                property._prevexstyle = styletemp;
                styletemp = property._style;
                property._style = property._prevstyle;
                property._prevstyle = styletemp;
                //set the styles back to pre-fullscreen values
                SetWindowLongPtr(hWnd, GWL_EXSTYLE, property._exstyle);
                SetWindowLongPtr(hWnd, GWL_STYLE, property._style);
                //set the window position back to pre-fullscreen values
                getWindowDPI();
                SetWindowPos(hWnd, HWND_TOP, property._left, property._top, Scale(property._width), Scale(property._height), SWP_SHOWWINDOW);
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
            if (property._allowfsexclusive) //fsexclusive routine
            {
                int width = property._width;
                int height = property._height;
                int left = property._left;
                int top = property._top;
                DXGI_SWAP_CHAIN_DESC desc = {};
                GFX->DXGI->dxgiSwapChain1->GetDesc(&desc);
                desc.BufferDesc.Width = info.rcMonitor.right - info.rcMonitor.left;
                desc.BufferDesc.Height = info.rcMonitor.bottom - info.rcMonitor.top;
                GFX->DXGI->dxgiSwapChain1->ResizeTarget(&desc.BufferDesc);
                HRESULT hr = GFX->DXGI->dxgiSwapChain1->SetFullscreenState(TRUE, NULL);
                //HRESULT hr = DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;
                /*it is advisable to call ResizeTarget again with the RefreshRate member of DXGI_MODE_DESC
                zeroed out. This amounts to a no-operation instruction in DXGI, but it can avoid issues
                with the refresh rate*/
                GFX->DXGI->dxgiSwapChain1->GetDesc(&desc);
                desc.BufferDesc.RefreshRate.Denominator = 0;
                desc.BufferDesc.RefreshRate.Numerator = 0;
                GFX->DXGI->dxgiSwapChain1->ResizeTarget(&desc.BufferDesc);
                property._prevwidth = width;
                property._prevheight = height;
                property._prevleft = left;
                property._prevtop = top;
                if ((hr != S_OK) && (hr != DXGI_STATUS_MODE_CHANGE_IN_PROGRESS))
                {
                    _fullscreen = false;
                    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
                    {
                        //revert window
                        width = property._prevwidth;
                        height = property._prevheight;
                        left = property._prevleft;
                        top = property._prevtop;
                        property._width = width;
                        property._height = height;
                        property._left = left;
                        property._top = top;
                        desc.BufferDesc.Width = width;
                        desc.BufferDesc.Height = height;
                        GFX->DXGI->dxgiSwapChain1->ResizeTarget(&desc.BufferDesc);
                        //SetWindowPos(hWnd, HWND_NOTOPMOST, property._prevleft, property._prevtop, property._actualwidth, property._actualheight, SWP_SHOWWINDOW);
                        MessageBox(NULL, _T("Unable to toggle fullscreen mode at the moment!/nThe resource is not currently available, but it might become available later."), _T("Mode-Switch Error"), (MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
                    }
                    else
                    {
                        throw WUIF_exception(_T("Critical error toggling full-screen switch!"));
                    }
                }
            }
            else //non fs exclusive
            {
                //set the values to the fullscreen, borderless "windowed" mode
                //don't use the function as we are changing multiple values, just make one SetWindowPos call
                property._prevwidth = property._width;
                property._prevheight = property._height;
                property._prevleft = property._left;
                property._prevtop = property._top;
                //set the styles
                property.exstyle(WS_EX_APPWINDOW | WS_EX_TOPMOST);
                //property.style(WS_POPUP | WS_VISIBLE);
                DWORD newstyle = property._style & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME);
                property.style(newstyle);
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
            if (GFX->D3D11->d3d11RenderTargetView)
            {
                //clear backbuffer
                GFX->D3D11->d3d11ImmediateContext->ClearRenderTargetView(GFX->D3D11->d3d11RenderTargetView.Get(), property.background());
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
        if (!((property._allowfsexclusive) && (_fullscreen)))
        {
            if (GFX->DXGI->dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
            {
                presentflags |= DXGI_PRESENT_ALLOW_TEARING;
            }
        }
        if (standby)
        {
            presentflags |= DXGI_PRESENT_TEST;
        }
        HRESULT hr = GFX->DXGI->dxgiSwapChain1->Present(0, presentflags);
        if ((hr == S_OK) && (standby))
        {
            //take window out of standby
            standby = false;
            presentflags &= ~DXGI_PRESENT_TEST;
            hr = GFX->DXGI->dxgiSwapChain1->Present(0, presentflags);
        }
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            GFX->HandleDeviceLost();
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
        if (!GFX->DXGI->dxgiFactory->IsCurrent())
        {
            GFX->DXGI->dxgiFactory.Reset();
            GFX->DXGI->GetDXGIAdapterandFactory();
        }
    }

}
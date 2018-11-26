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
#include <string> //string.h needed for int to string conversion
/*ShellScalingApi.h needed for PROCESS_DPI_AWARENESS, PROCESS_PER_MONITOR_DPI_AWARE,
MONITOR_DPI_TYPE, and MDT_EFFECTIVE_DPI*/
#include <ShellScalingApi.h>
#include "Application/Application.h"
#include "Window/Window.h"
#include "Window/WndProcThunk.h"
//#include "GFX/GFX.h"

HANDLE WUIF::wndprocThunk::heapaddr       = NULL;
long   WUIF::wndprocThunk::thunkInstances = 0;

namespace{
    //cumulative count of number of windows created
    static long numwininstances = 0;
}

namespace WUIF {
    /*"Hidden" WUIF::App namespace for establishing and using a Window collection*/
    namespace App {
        //mutex for accessing Windows vector
        std::mutex veclock;
        bool vecwrite = true;
        std::condition_variable vecready;
        std::vector<Window*> Windows; //internal Window collection vector - not exposed publicly

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

    //constructor
    Window::Window() noexcept(false):
        GFXResources(this),
        enableHDR(false),
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
            SetLastError(WE_THUNK_HOOK_FAIL);
            throw WUIF_exception(TEXT("WndProc thunk failed to initialize properly!"));
        }
        //increment instance - use InterlockedIncrement to avoid any conflicts
        instance = InterlockedIncrement(&numwininstances);

        //add to Windows collection
        WINVECLOCK
        App::Windows.push_back(this);
        WINVECUNLOCK
    }

    //destructor
    #ifdef _MSC_VER
    #pragma warning(push)
    //Avoid unnamed objects with custom construction and destruction
    #pragma warning(disable: 26444)
    #endif
    Window::~Window()
    {
        int i = 0;
        WINVECLOCK
            for (std::vector<Window*>::iterator winlist = App::Windows.begin();
                    winlist != App::Windows.end(); winlist++)
            {
                if (*winlist == this)
                {
                    App::Windows.erase(App::Windows.begin() + i);
                    break;
                }
                i++;
            }
            if (App::Windows.size() == 0)
                App::Windows.shrink_to_fit();
        WINVECUNLOCK
        if (thunk != nullptr)
        {
            delete thunk;
            thunk = nullptr;
        }
        if (_classatom)
        {
            if (!cWndProc)
            {
                /*Before calling this function, an application must destroy all windows created
                with the specified class.*/
                if (UnregisterClass(MAKEINTATOM(_classatom), App::hInstance))
                {
                    _classatom = NULL;
                }
            }
        }
        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif
    }

    /*inline WNDPROC Window::pWndProc()
    inline function to return a pointer to the window's WndProc thunk or throw an exception
    */
    WNDPROC Window::pWndProc()
    {
        if (thunk)
        {
            return thunk->GetThunkAddress();
        }
        //if our thunk somehow does not exist
        SetLastError(WE_THUNK_HOOK_FAIL);
        throw WUIF_exception(TEXT("Invalid thunk pointer!"));
    }

    /*LRESULT CALLBACK App::T_SC_WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam,
                                            _In_ LPARAM lParam)
    This is a temporary WindowProc to assist with sub-classing. It will change the original
    window's class WndProc pointer to the window's thunk, which points to the default _WndProc.
    This must happen in WM_NCCREATE. The *this is passed as lpCreateParams from the CreateWindowEx
    call in DisplayWindow(). It's a static class function so we can keep pWndProc private*/
    LRESULT CALLBACK Window::T_SC_WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam,
                                             _In_ LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_NCCREATE:
        {
            //change the WndProc from the class to the Window thunk
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(
                reinterpret_cast<WUIF::Window*>(
                    reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams)->pWndProc()));
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
                cbSize        - size, in bytes, of this structure. Set this member to
                                    sizeof(WNDCLASSEX)
                style         - Window Class Styles to be applied - using CS_HREDRAW | CS_VREDRAW
                lpfnWndProc   - a pointer to the window procedure, here it is a pointer to
                                    wndprocThunk
                cbClsExtra    - number of extra bytes to allocate following the structure- not used
                cbWndExtra    - number of extra bytes to allocate following the window instance-
                                    not used
                hInstance     - handle to the instance that contains the window procedure for the
                                    class
                hIcon         - handle to the class icon
                hCursor       - handle to the class cursor
                hbrBackground - handle to the class background brush - ignored but using default of
                                    (HBRUSH)(COLOR_WINDOW + 1) - background handled by D3D or D2D
                lpszMenuName  - pointer to a null-terminated character string that specifies the
                                    resource name of the class menu - not used
                lpszClassName - pointer to a null-terminated string specifying the window class name
                hIconSm       - handle to a small icon that is associated with the window class
                */

                /*create a unique string for the class name. Write 'W + [character equivalent of
                instance number]' to classname, thus guaranteeing uniqueness (i.e. 'W1'). Use
                StringCchPrintf as the mechanism to convert instance to a character equivalent.
                Size of array is calculated as digits +1 for 'W' and +1 for null terminator, so
                digits + 2. A unique pointer is used just in case an exception is thrown.*/
                std::unique_ptr<TCHAR[]> classname(CRT_NEW TCHAR[static_cast<size_t>(digits) + 2]);
                ThrowIfFailed(StringCchPrintf(classname.get(),
                                              static_cast<size_t>(digits) + 2,
                                              TEXT("W%d"),
                                              instance));

                //set defaults for _icon, _iconsm, and _classcursor if they are NULL
                if (_icon == NULL)
                {
                    _icon = LoadIcon(NULL, IDI_APPLICATION);
                }
                if (_iconsm == NULL)
                {
                    _iconsm = reinterpret_cast<HICON>(
                        LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                   GetSystemMetrics(SM_CYSMICON), LR_SHARED));
                }
                if (_classcursor == NULL)
                {
                    _classcursor = LoadCursor(NULL, IDC_ARROW);
                }

                WNDCLASSEX wndclass    = {}; //zero out
                wndclass.lpszClassName = classname.get();
                wndclass.cbSize        = sizeof(WNDCLASSEX);
                wndclass.style         = CS_HREDRAW | CS_VREDRAW;
                wndclass.lpfnWndProc   = pWndProc();
                wndclass.hInstance     = App::hInstance;
                wndclass.hIcon         = _icon;
                wndclass.hCursor       = _classcursor;
                wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
                wndclass.hIconSm       = _iconsm;


                /*register our WNDCLASSEX (wndclass) for the window and store result in our class
                atom variable. We will use the class atom instead of the class name*/
                _classatom = RegisterClassEx(&wndclass);
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
                ThrowIfFalse(GetClassInfoEx(App::hInstance, MAKEINTATOM(_classatom), &wndclass), GetLastError());

                //check if the class WndProc == T_SC_WindowProc, if so we already sub-classed
                if (wndclass.lpfnWndProc != Window::T_SC_WindowProc)
                {
                    //create a dummy window as we need an hwnd to change the class WndProc
                    HWND tempwin = CreateWindowEx(NULL, MAKEINTATOM(_classatom), NULL, NULL, 0, 0,
                                                   0, 0, NULL, NULL, App::hInstance, NULL);
                    if (tempwin)
                    {
                        if (SetClassLongPtr(tempwin, GCLP_WNDPROC, reinterpret_cast<LONG_PTR>(Window::T_SC_WindowProc)))
                        {
                            //all new windows for this class will use T_SC_WindowProc
                            //save the old WndProc
                            cWndProc = wndclass.lpfnWndProc;
                            DestroyWindow(tempwin);
                        }
                        else
                        {
                            //DestroyWindow will overwrite LastError - save and restore
                            DWORD err = GetLastError();
                            DestroyWindow(tempwin);
                            SetLastError(err);
                            throw WUIF_exception(TEXT("Unable to create subclass!"));
                        }
                    }
                    else
                    {
                        throw WUIF_exception(TEXT("Unable to create subclass!"));
                    }
                }
                //set the window properties
                _icon        = wndclass.hIcon;
                _iconsm      = wndclass.hIconSm;
                _classcursor = wndclass.hCursor;
            }

            if (_windowname == nullptr)
            {
                //create a string for the window name. Use 'Window' and append the instance
                _windowname = CRT_NEW TCHAR[static_cast<size_t>(digits) + 8]; //+7 for 'Window%20' and +1 for null terminator
                ThrowIfFailed(StringCchPrintf(_windowname, static_cast<size_t>(digits) + 8, TEXT("Window %d"), instance));
            }

            /*Check if the window is requesting a different dpi thread context from the main (only
            in Win10 1607). If it's different change the thread context prior to creating the
            window and then change back*/
            DPI_AWARENESS_CONTEXT dpicontext = NULL;
            HMODULE lib = NULL;
            using PFN_SET_THREAD_DPI_AWARENESS_CONTEXT = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
            PFN_SET_THREAD_DPI_AWARENESS_CONTEXT pfnsetthreaddpiawarenesscontext = nullptr;
            if ((App::winversion >= OSVersion::WIN10_1607) && (_threaddpiawarenesscontext != NULL)
                    && (_hWndParent == NULL))
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
                            if ((pfnaredpiawarenesscontextsequal(pfngetthreaddpiawarenesscontext(),                             _threaddpiawarenesscontext)) == FALSE)
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
            /*CreateWindowEx:
            DWORD     dwExStyle    = extended window style,
            LPCTSTR   lpClassName  = null-terminated string created by a previous call to the
                                        RegisterClassEx function,
            LPCTSTR   lpWindowName = the window name,
            DWORD     dwStyle      = the style of the window being created, we make sure the
                                        WS_VISIBLE bit is off as we do not want the window to show
                                        yet,
            int       x            = initial horizontal position of the window,
            int       y            = initial vertical position of the window,
            int       nWidth       = width, in device units, of the window,
            int       nHeight      = height, in device units, of the window,
            HWND      hWndParent   = handle to the parent or owner window of the window being
                                        created,
            HMENU     hMenu        = handle to a menu, or specifies a child-window identifier,
            HINSTANCE hInstance    = handle to the instance of the module to be associated with the
                                        window,
            LPVOID    lpParam      = pointer to a value to be passed to the window through the
                                        CREATESTRUCT structure - here the *this pointer for the
                                        window
            Returns handle for the create window and it's stored in this->_hWnd. NB: It is also
            assigned in NCCREATE of this->_WndProc,  but it is also assigned here for instances
            such as sub-classing where this might not happen.*/
            _hWnd = CreateWindowEx(_exstyle,
                                   MAKEINTATOM(_classatom),
                                   _windowname,
                                   _style & (~WS_VISIBLE),
                                   _left,
                                   _top,
                                   _width,
                                   _height,
                                   _hWndParent,
                                   _menu,
                                   App::hInstance,
                                   reinterpret_cast<LPVOID>(this));
            if (_hWnd == NULL)
            {
                throw WUIF_exception(TEXT("Unable to create window"));
            }
            if (dpicontext != NULL) //restore dpi thread context
            {
                if (pfnsetthreaddpiawarenesscontext)
                {
                    pfnsetthreaddpiawarenesscontext(dpicontext);
                }
            }
            initialized = true; //window class is registered and window is "created"
            ShowWindow(_hWnd, _cmdshow); //show the window
            UpdateWindow(_hWnd); //now paint the window
            _style   = static_cast<DWORD>(GetWindowLongPtr(_hWnd, GWL_STYLE));
            _exstyle = static_cast<DWORD>(GetWindowLongPtr(_hWnd, GWL_EXSTYLE));
        }
        return;
    }

    UINT Window::getWindowDPI()
    {
        //Windows 10 provides GetDpiForWindow, a per monitor DPI scaling
        if (App::winversion >= OSVersion::WIN10)
        {
            //on windows 10_1607 and greater we check for
            if (App::winversion >= OSVersion::WIN10_1607)
            {
                if (App::processdpiawarenesscontext != nullptr)
                {
                    HMODULE lib = GetModuleHandle(TEXT("user32.dll"));
                    if (lib)
                    {
                        using PFN_ARE_DPI_AWARENESS_CONTEXTS_EQUAL = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT, DPI_AWARENESS_CONTEXT);
                        PFN_ARE_DPI_AWARENESS_CONTEXTS_EQUAL pfnaredpiawarenesscontextsequal = (PFN_ARE_DPI_AWARENESS_CONTEXTS_EQUAL)GetProcAddress(lib, "AreDpiAwarenessContextsEqual");
                        if (pfnaredpiawarenesscontextsequal)
                        {
                            if (pfnaredpiawarenesscontextsequal(*App::processdpiawarenesscontext, DPI_AWARENESS_CONTEXT_UNAWARE))
                            {
                                scaleFactor = 100;
                                return 96;
                            }
                        }

                    }
                }
            }
            HMODULE lib = GetModuleHandle(TEXT("user32.dll"));
            if (lib)
            {
                //Windows 10 uses GetDpiForWindow to get the dynamic DPI
                using GETDPIFORWINDOW = UINT(WINAPI *)(HWND);
                GETDPIFORWINDOW getdpiforwindow = (GETDPIFORWINDOW)GetProcAddress(lib, "GetDpiForWindow");
                if (getdpiforwindow)
                {
                    UINT dpi = getdpiforwindow(_hWnd);
                    scaleFactor = ((dpi == 96) ? 100 : MulDiv(dpi, 100, 96));
                    return dpi;
                }
            }
        }
        if ((App::processdpiawareness != nullptr) && (*App::processdpiawareness == PROCESS_DPI_UNAWARE))
        {
            scaleFactor = 100;
            return 96;
        }
        //either not Win10 or GetDpiForWindow didn't load, try Win 8.1 function
        if (App::winversion >= OSVersion::WIN8_1)
        {
            //Windows 8.1 has the GetDpiForMonitor function
            HINSTANCE lib = LoadLibrary(TEXT("shcore.dll"));
            if (lib)
            {
                using GETDPIFORMONITOR = HRESULT(WINAPI *)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
                GETDPIFORMONITOR getdpi = (GETDPIFORMONITOR)GetProcAddress(lib, "GetDpiForMonitor");
                if (getdpi)
                {
                    HMONITOR hMonitor = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST);
                    UINT dpiy = 0;
                    UINT dpix = 0;
                    /*MDT_EFFECTIVE_DPI - The effective DPI. This value should be used when
                    determining the correct scale factor for scaling UI elements. This incorporates
                    the scale factor set by the user for this specific display. dpix will be set to
                    the monitor's dpi*/
                    getdpi(hMonitor, MDT_EFFECTIVE_DPI, &dpix, &dpiy);
                    scaleFactor = ((dpix == 96) ? 100 : MulDiv(dpix, 100, 96));
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
        int dpi = GetDeviceCaps(GetDC(_hWnd), LOGPIXELSX);
        scaleFactor = ((dpi == 96) ? 100 : MulDiv(dpi, 100, 96));
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
                SetWindowPos(_hWnd, HWND_TOP, _left, _top, Scale(_width), Scale(_height), SWP_SHOWWINDOW);
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
                SetWindowLongPtr(_hWnd, GWL_EXSTYLE, _exstyle);
                SetWindowLongPtr(_hWnd, GWL_STYLE, _style);
                //set the window position back to pre-fullscreen values
                getWindowDPI();
                SetWindowPos(_hWnd, HWND_TOP, _left, _top, Scale(_width), Scale(_height), SWP_SHOWWINDOW);
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
            HMONITOR monitor = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST);
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
                SetWindowPos(_hWnd, HWND_TOPMOST, info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right - info.rcMonitor.left,
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
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
#include "WUIF.h"
#include <string> //needed for int to string conversion
#include <ShellScalingApi.h> //needed for PROCESS_DPI_AWARENESS, PROCESS_PER_MONITOR_DPI_AWARE, MONITOR_DPI_TYPE, MDT_EFFECTIVE_DPI
#include "DirectX\DirectX.h"
#include "Application\Application.h"
#include "Window\Window.h"


HANDLE WUIF::wndprocThunk::heapaddr = NULL;
long   WUIF::wndprocThunk::thunkInstances = 0;

namespace {
	static long  numwininstances = 0;
}

namespace WUIF {

	Window::Window() : Window(App)
	{

	}

	Window::Window(Application *_app) :
		wndclassatom(NULL),
		props(this),
		hWnd(NULL),
		hWndParent(NULL),
		DXres(nullptr),
		enableHDR(false),
		Next(nullptr),
		thunk(nullptr),
		instance(0),
		initialized(false),
		_fullscreen(false),
		standby(false)
	{
		//increment instance - use InterlockedIncrement to avoid any conflicts
        instance = InterlockedIncrement(&numwininstances);
		DXres = new DXResources(this);

		//register thunk
		thunk = new wndprocThunk;
		if (!(thunk->Init(this, (DWORD_PTR)_WndProc))) //get our thunk's address
		{
			throw WUIF_exception(_T("WndProc thunk failed to initialize properly!"));
		}
		if (_app->Windows != nullptr)
		{
			if (_app->Windows->Next != nullptr)
			{
				Window *temp = _app->Windows->Next;
				while (temp->Next != nullptr)
				{
					temp = temp->Next;
				}
				temp->Next = this;
			}
			else
			{
				_app->Windows->Next = this;
			}
		}
	}

	Window::~Window()
	{
		delete DXres;
		DXres = nullptr;
		delete thunk;
		thunk = nullptr;
		if (App->Windows == nullptr)
		{
			ASSERT(false); //if there's a window still active App->Windows should never be nullptr
		}
		if (App->Windows != this)
		{
			if (App->Windows->Next != nullptr)
			{
				Window *temp = App->Windows->Next;
				while (temp->Next != this)
				{
					temp = temp->Next;
				}
				temp->Next = this->Next;
			}
			else
			{
				ASSERT(false); //we aren't the first window in the list, so there should be a next
			}
		}
		else
		{
			if (this->Next == nullptr)
			{
				App->Windows = nullptr;
			}
			else
			{
				App->Windows = this->Next;
			}
		}
	}

	void Window::DisplayWindow()
	{
		//assign values to the WNDCLASSEX structure prior to calling RegisterClassEx
		WNDCLASSEX wndclass = {};                                    //zero out
		wndclass.cbSize = sizeof(WNDCLASSEX);
		wndclass.style = props.classtyle();                     //Window Class Styles to be applied
		wndclass.lpfnWndProc = pWndProc();                            //a pointer to the window procedure, here it is a pointer to wndprocThunk
		wndclass.cbClsExtra = 0;							            //number of extra bytes to allocate following the window-class structure
		wndclass.cbWndExtra = 0;							            //number of extra bytes to allocate following the window instance
		wndclass.hInstance = *App->hInstance;                       //handle to the instance that contains the window procedure for the class
		wndclass.hIcon = props.icon();                          //handle to the class icon
		wndclass.hCursor = props.cursor();	                    //handle to the class cursor
		wndclass.hbrBackground = props.background();                    //handle to the class background brush
		wndclass.lpszMenuName = props.menuname();				        //pointer to a null-terminated character string that specifies the resource name of the class menu
		std::wstring s = L"Window" + std::to_wstring(instance); //create a string for the class name. use Window and append the window instance (guaranteeing uniqueness)
		wndclass.lpszClassName = s.c_str();					            //pointer to a null-terminated string specifying the window class name - a generic default
		wndclass.hIconSm = props.iconsm();					    //handle to a small icon that is associated with the window class

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
		if (*App->winversion >= OSVersion::WIN10)
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
						if (!(aredpiawarenesscontextsequal(getthreaddpiawarenesscontext(), props.dpiawareness())))
						{
							using SETTHREADDPIAWARE = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
							SETTHREADDPIAWARE setthreaddpiaware = (SETTHREADDPIAWARE)GetProcAddress(lib, "SetThreadDpiAwarenessContext");
							if (setthreaddpiaware)
							{
								dpicontext = setthreaddpiaware(props.dpiawareness());
							}
						}
					}
				}
			}
		}

		//NB: we store the window handle in the NCCREATE message of our WndProc
		hWnd = CreateWindowEx(props.exstyle(), //extended window style
			MAKEINTATOM(wndclassatom),         //null-terminated string created by a previous call to the RegisterClassEx function
			props.windowname(),		           //the window name
			props.style(),                     //the style of the window being created, make sure the WS_VISIBLE bit is off as we do not want the window to show yet
			props.left(),			           //initial horizontal position of the window
			props.top(),			           //initial vertical position of the window
			props.width(),			           //width, in device units, of the window
			props.height(),			           //height, in device units, of the window
			hWndParent,		                   //handle to the parent or owner window of the window being created
			props.menu(),			           //handle to a menu, or specifies a child-window identifier
			*App->hInstance,		           //handle to the instance of the module to be associated with the window
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
		ShowWindow(hWnd, props.cmdshow()); //show the window
		UpdateWindow(hWnd); //now paint the window
		return;
	}

	WNDCLASSEX	Window::wndclass()
	{
		WNDCLASSEX winclass = {};
		if ((GetClassInfoEx(*App->hInstance, MAKEINTATOM(wndclassatom), &winclass)) != 0)
		{
			winclass = {};
		}
		return winclass;
	}


	UINT Window::getWindowDPI()
	{
		if (*App->winversion > OSVersion::WIN8_1)  //Windows 10 provides GetDpiForWindow, a per monitor DPI scaling
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
		if (*App->winversion > OSVersion::WIN8) //either not Win10 or GetDpiForWindow didn't load, try Win 8.1 function
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
			int width = props._prevwidth;
			int height = props._prevheight;
			int left = props._prevleft;
			int top = props._prevtop;
			props._prevwidth = props._width;
			props._prevheight = props._height;
			props._prevleft = props._left;
			props._prevtop = props._top;
			if (props._allowfsexclusive)
			{
				HRESULT hr = DXres->DXGIres.dxgiSwapChain1->SetFullscreenState(FALSE, NULL);
				//reset position variables to proper non-fullscreen values
				props._width = width;
				props._height = height;
				props._left = left;
				props._top = top;
				getWindowDPI();
				SetWindowPos(hWnd, HWND_TOP, props._left, props._top, Scale(props._width), Scale(props._height), SWP_SHOWWINDOW);
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
				props._width = width;
				props._height = height;
				props._left = left;
				props._top = top;
				DWORD styletemp = props._exstyle; //temp variable to hold the DWORD styles
				props._exstyle = props._prevexstyle;
				props._prevexstyle = styletemp;
				styletemp = props._style;
				props._style = props._prevstyle;
				props._prevstyle = styletemp;
				//set the styles back to pre-fullscreen values
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, props._exstyle);
				SetWindowLongPtr(hWnd, GWL_STYLE, props._style);
				//set the window position back to pre-fullscreen values
				getWindowDPI();
				SetWindowPos(hWnd, HWND_TOP, props._left, props._top, Scale(props._width), Scale(props._height), SWP_SHOWWINDOW);
				/*
				DXres->DXGIres.CreateSwapChain();
				//setup D3D dependent resources
				if (DXres->usingD3D12)
				{
				//pThis->DXres->d3d12res.CreateD12Resources();
				}
				else //use D3D11
				{
				DXres->d3d11res.CreateDeviceResources();
				}
				if (App->flags & WUIF::FLAGS::D2D)
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
			if (props._allowfsexclusive) //fsexclusive routine
			{
				int width = props._width;
				int height = props._height;
				int left = props._left;
				int top = props._top;
				DXGI_SWAP_CHAIN_DESC desc = {};
				DXres->DXGIres.dxgiSwapChain1->GetDesc(&desc);
				desc.BufferDesc.Width = info.rcMonitor.right - info.rcMonitor.left;
				desc.BufferDesc.Height = info.rcMonitor.bottom - info.rcMonitor.top;
				DXres->DXGIres.dxgiSwapChain1->ResizeTarget(&desc.BufferDesc);
				HRESULT hr = DXres->DXGIres.dxgiSwapChain1->SetFullscreenState(TRUE, NULL);
				//HRESULT hr = DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;
				/*it is advisable to call ResizeTarget again with the RefreshRate member of DXGI_MODE_DESC
				zeroed out. This amounts to a no-operation instruction in DXGI, but it can avoid issues
				with the refresh rate*/
				DXres->DXGIres.dxgiSwapChain1->GetDesc(&desc);
				desc.BufferDesc.RefreshRate.Denominator = 0;
				desc.BufferDesc.RefreshRate.Numerator = 0;
				DXres->DXGIres.dxgiSwapChain1->ResizeTarget(&desc.BufferDesc);
				props._prevwidth = width;
				props._prevheight = height;
				props._prevleft = left;
				props._prevtop = top;
				if ((hr != S_OK) && (hr != DXGI_STATUS_MODE_CHANGE_IN_PROGRESS))
				{
					_fullscreen = false;
					if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
					{
						//revert window
						int width = props._prevwidth;
						int height = props._prevheight;
						int left = props._prevleft;
						int top = props._prevtop;
						props._width = width;
						props._height = height;
						props._left = left;
						props._top = top;
						desc.BufferDesc.Width = width;
						desc.BufferDesc.Height = height;
						DXres->DXGIres.dxgiSwapChain1->ResizeTarget(&desc.BufferDesc);
						//SetWindowPos(hWnd, HWND_NOTOPMOST, props._prevleft, props._prevtop, props._actualwidth, props._actualheight, SWP_SHOWWINDOW);
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
				props._prevwidth = props._width;
				props._prevheight = props._height;
				props._prevleft = props._left;
				props._prevtop = props._top;
				//set the styles
				props.exstyle(WS_EX_APPWINDOW | WS_EX_TOPMOST);
				//props.style(WS_POPUP | WS_VISIBLE);
				DWORD newstyle = props._style & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME);
				props.style(newstyle);
				//set the window position
				SetWindowPos(hWnd, HWND_TOPMOST, info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right - info.rcMonitor.left,
					info.rcMonitor.bottom - info.rcMonitor.top, SWP_SHOWWINDOW);
			}
		}
		return;
	}

	void Window::Present()
	{
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
		if (!((props._allowfsexclusive) && (_fullscreen)))
		{
			if (DXres->DXGIres.dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
			{
				presentflags |= DXGI_PRESENT_ALLOW_TEARING;
			}
		}
		if (standby)
		{
			presentflags |= DXGI_PRESENT_TEST;
		}
		HRESULT hr = DXres->DXGIres.dxgiSwapChain1->Present(0, presentflags);
		if ((hr == S_OK) && (standby))
		{
			//take window out of standby
			standby = false;
			presentflags &= ~DXGI_PRESENT_TEST;
			hr = DXres->DXGIres.dxgiSwapChain1->Present(0, presentflags);
		}
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			DXres->HandleDeviceLost();
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
		if (!DXres->DXGIres.dxgiFactory->IsCurrent())
		{
			DXres->DXGIres.dxgiFactory.Reset();
			DXres->DXGIres.GetDXGIAdapterandFactory();
		}
	}

}
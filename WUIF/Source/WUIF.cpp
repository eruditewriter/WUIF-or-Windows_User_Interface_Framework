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
#include <shellapi.h> //needed for CommandLineToArgvwW
#include <ShellScalingApi.h> //needed for PROCESS_DPI_AWARENESS, PROCESS_PER_MONITOR_DPI_AWARE, MONITOR_DPI_TYPE, MDT_EFFECTIVE_DPI
#include <strsafe.h> //needed for ErrorExit StringCchPrintf
#include "DirectX\DirectX.h" //needed for CheckOS and InitResources
#include "Application\Application.h"
#include "Window\Window.h"

//WUIF global definition
namespace WUIF { Application* App; }

namespace {
	void ErrorExit(LPTSTR lpszMessage)
	{
		LPVOID  lpMsgBuf;
		LPVOID  lpDisplayBuf;
		DWORD   err    = GetLastError();
        LPCTSTR pszTxt = _T("An unrecoverable critical error was encountered and the application is now terminating!\n\n%s\n\nReported error is %d: %s");
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPTSTR>(&lpMsgBuf),
			0, NULL);
        lpDisplayBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlen(reinterpret_cast<LPCTSTR>(lpMsgBuf))
            + lstrlen(reinterpret_cast<LPCTSTR>(lpszMessage)) + 118) * sizeof(TCHAR));
        if (lpDisplayBuf)
        {
            StringCchPrintf(reinterpret_cast<LPTSTR>(lpDisplayBuf),
                LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                pszTxt,
                lpszMessage, err, lpMsgBuf);
            MessageBox(NULL, reinterpret_cast<LPCTSTR>(lpDisplayBuf), _T("Critical Error"), (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
        }
        else //LocalAlloc failed - print fallback message
        {
            MessageBox(NULL, _T("Critical Error! Unable to retrieve error message."), _T("Critical Error"), (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
        }
		LocalFree(lpMsgBuf);
		HeapFree(GetProcessHeap(), NULL, lpDisplayBuf);
		throw WUIF_terminate_exception{};
	}

	WUIF::OSVersion OSCheck()
	{
		WUIF::OSVersion winversion = WUIF::OSVersion::WIN7;
		//determine D3D11 level by seeing which ID3D11Devices we can create
		Microsoft::WRL::ComPtr<ID3D11Device>  d3d11Device0; //Direct3D 11 device
		if (SUCCEEDED(D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       //There is no need to create a real hardware device.
			0,
			0,
			nullptr,                    //Any feature level will do.
			0,
			D3D11_SDK_VERSION,
			&d3d11Device0,              //D3D device reference.
			nullptr,                    //No need to know the feature level.
			nullptr                     //No need to keep the D3D device context reference.
		)))
		{
			Microsoft::WRL::ComPtr<ID3D11Device5> d3d11Device5; //Direct3D device (Win 10 Creators Update)
			if (SUCCEEDED(d3d11Device0.As(&d3d11Device5))) //ID3D11Device5 support still 11.4 version
			{
				winversion = WUIF::OSVersion::WIN10_1703;
				d3d11Device5.Reset();
			}
			else
			{
				Microsoft::WRL::ComPtr<ID3D11Device4> d3d11Device4;
				if (SUCCEEDED(d3d11Device0.As(&d3d11Device4))) //11.4 support
				{
					winversion = WUIF::OSVersion::WIN10_1511;
					d3d11Device4.Reset();
				}
				else
				{
					Microsoft::WRL::ComPtr<ID3D11Device3> d3d11Device3;
					if (SUCCEEDED(d3d11Device0.As(&d3d11Device3))) //11.3 support
					{
						winversion = WUIF::OSVersion::WIN10;
						d3d11Device3.Reset();
					}
					else
					{
						Microsoft::WRL::ComPtr<ID3D11Device2> d3d11Device2;
						if (SUCCEEDED(d3d11Device0.As(&d3d11Device2))) //11.2 support
						{
							winversion = WUIF::OSVersion::WIN8_1;
							d3d11Device2.Reset();
						}
						else
						{
							Microsoft::WRL::ComPtr<ID3D11Device1> d3d11Device1;
							if (SUCCEEDED(d3d11Device0.As(&d3d11Device1))) //11.1 support
							{
								winversion = WUIF::OSVersion::WIN7;
								d3d11Device1.Reset();
							}
							else //we failed to make an 11.1 device so we must not be on Win 7 SP1 with KB 2670838 or minimum supported
							{
								throw WUIF_exception(_T("You need at least Windows 7 SP1 with KB 2670838"));
							}
						}
					}
				}
			}
			//no definitive way to determine Win 8 versus Win 7 so make an API call that should give us a reasonable confidence
			if (winversion == WUIF::OSVersion::WIN7)  //windows 8 and windows 7 look alike at the moment
			{
				HINSTANCE lib = LoadLibrary(_T("kernel32.dll"));
				if (lib)
				{
					using CREATEFILE2 = HANDLE(WINAPI*)(REFIID, void**);
					CREATEFILE2 createfiletwo = (CREATEFILE2)GetProcAddress(lib, "CreateFile2");
					if (createfiletwo)
					{
						winversion = WUIF::OSVersion::WIN8;
					}
					FreeLibrary(lib);
				}
			}
			d3d11Device0.Reset();
		}
		else
		{
			ASSERT(false);
		}
		return winversion;
	}

	void InitResources()
	{
		//Use D3D12 only = ((WUIF::App->flags & WUIF::FLAGS::D3D12)) && (!(WUIF::App->flags & WUIF::FLAGS::D3D11))
		//determine if D3D12 is requested and if D3D11 is allowed as a fallback
		if (WUIF::App->flags & WUIF::FLAGS::D3D12) //if use D3D12 or D3D12 only
		{
			if (*WUIF::App->winversion < WUIF::OSVersion::WIN10) //if D3D12 we must be on Win10
			{
				if ((WUIF::App->flags & WUIF::FLAGS::D3D12) && (!(WUIF::App->flags & WUIF::FLAGS::D3D11))) //if we are D3D12 only and not on Win 10 must throw an error
				{
					throw WUIF_exception(_T("This application requires Windows 10 or greater and Direct3D 12"));
				}
			}
			else
			{
				WUIF::DXResources::usingD3D12 = true;
			}
		}
		WUIF::DXGIResources::GetDXGIAdapterandFactory();
		if (WUIF::DXResources::usingD3D12 == false)
		{
			//create D3D11 device
			WUIF::D3D11Resources::CreateStaticResources();
		}
		else
		{
			//create D3D12 device
			WUIF::D3D12Resources::CreateStaticResources();
		}
		if (WUIF::App->flags & WUIF::FLAGS::D2D)
		{
			WUIF::D2DResources::CreateStaticResources();
		}
		return;
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	//Retrieves or modifies the state of the _crtDbgFlag flag to control the allocation behavior of the debug heap manager (debug version only)
#ifdef _DEBUG
	//get current bits
	int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	//clear the upper 16 bits and OR in the desired frequency and other flags
	tmp = (tmp & 0x0000FFFF) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF;
	//set the bits
	_CrtSetDbgFlag(tmp);
#endif

	//set the DPI AWARENESS
	//first we test for SetThreadDpiAwarenessContext in user32.dll (it is not supported in < Win10 Anniversary Update?)
	//we must load the function or the application will error out on an unsupported OS version on startup
	HINSTANCE lib = LoadLibrary(_T("user32.dll"));
    if (lib)
    {
        using SETTHREADDPIAWARE = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
        SETTHREADDPIAWARE setthreaddpiaware = (SETTHREADDPIAWARE)GetProcAddress(lib, "SetThreadDpiAwarenessContext");
        if (setthreaddpiaware) //if this fails it will fall through to the next case and try SetProcessDpiAwareness
        {
            //DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 is available in Win10 Creators Update only
            DPI_AWARENESS_CONTEXT retval = setthreaddpiaware(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            if (retval == NULL) //failed try DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
            {
                setthreaddpiaware(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
            }
        }
        else
        {
            //fallback to SetProcessDpiAwareness a Win 8.1 way to set the DPI AWARENESS
            FreeLibrary(lib);
            lib = LoadLibrary(_T("shcore.dll"));
            if (lib)
            {
                using SETPROCDPIAWARENESS = HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS);
                SETPROCDPIAWARENESS setprocdpiawareness = (SETPROCDPIAWARENESS)GetProcAddress(lib, "SetProcessDpiAwareness");
                if (setprocdpiawareness)
                {
                    setprocdpiawareness(PROCESS_PER_MONITOR_DPI_AWARE); //this will fail if it's in the manifest
                }
            }
            else
            {
                lib = LoadLibrary(_T("user32.dll"));
                if (lib)
                {
                    //use Vista/Win7/Win8 SetProcessDPIAware - this isn't guaranteed to be in later operating systems so we shouldn't link statically
                    using SETPROCDPIAWARE = BOOL(WINAPI*)(void);
                    SETPROCDPIAWARE setprocdpiaware = (SETPROCDPIAWARE)GetProcAddress(lib, "SetProcessDPIAware");
                    if (setprocdpiaware)
                    {
                        setprocdpiaware();
                    }
                    FreeLibrary(lib);
                }
            }
        }
        //no dpi awareness call available (must be set in manifest), do nothing
        if (lib)
        {
            FreeLibrary(lib);
        }
	}
	else
	{
		ASSERT(false);
	}

	WUIF::App = new WUIF::Application(hInstance, lpCmdLine, nCmdShow, OSCheck());

	if ((!(WUIF::App->flags & WUIF::FLAGS::D3D11)) && (!(WUIF::App->flags & WUIF::FLAGS::D3D12)))
	{
		delete WUIF::App;
		WUIF::App = nullptr;
#ifdef _DEBUG
		ASSERT(false);
#else
		ErrorExit(_T("Developer did not specify which Direct3D version to use!"));
#endif // _DEBUG
	}
	InitResources();

	int retcode = 0;
	LPWSTR *argv = nullptr; /*An array of null-terminated strings representing command-line arguments entered by the user
							of the program. By convention, argv[0] is the command with which the program is invoked,
							argv[1] is the first command-line argument, and so on, until argv[argc], which is always NULL.*/
	int argc = 0;       /*An integer that contains the count of arguments that follow in argv.
						The argc parameter is always greater than or equal to 1	*/

						//Note: if lpCmdLine is an empty string the function returns the path to the current executable file
	argv = CommandLineToArgvW(lpCmdLine, &argc);
	try
	{
		retcode = AppMain(argc, argv);
		/*CommandLineToArgvW allocates a block of contiguous memory for pointers to the argument strings,
		and for the argument strings themselves; the calling application must free the memory used by the
		argument list when it is no longer needed. To free the memory, use a single call to the LocalFree
		function.*/
		LocalFree(argv);
		if (WUIF::App != nullptr)
		{
			delete WUIF::App;
			WUIF::App = nullptr;
		}
	}
	catch (const WUIF_exception &e)
	{
		LocalFree(argv);
		retcode = ERROR_PROCESS_ABORTED;
		if (WUIF::App != nullptr)
		{
			if (WUIF::App->ExceptionHandler != nullptr)
			{
				try {
					WUIF::App->ExceptionHandler();
					delete WUIF::App;
					WUIF::App = nullptr;
				}
				catch (...)
				{
					if (WUIF::App != nullptr)
					{
						delete WUIF::App;
						WUIF::App = nullptr;
					}
				}
			}
		}
		try {
			ErrorExit(e.WUIFWhat());
		}
		catch (...) {}
	}
	catch (...) //catch all other unhandled exceptions
	{
		LocalFree(argv);
		retcode = ERROR_PROCESS_ABORTED;
		if (WUIF::App != nullptr)
		{
			if (WUIF::App->ExceptionHandler != nullptr)
			{
				try {
					WUIF::App->ExceptionHandler();
					delete WUIF::App;
					WUIF::App = nullptr;
				}
				catch (...)
				{
					if (WUIF::App != nullptr)
					{
						delete WUIF::App;
						WUIF::App = nullptr;
					}
				}
			}
		}
		try {
			ErrorExit(_T("Critical Unhandled Exception!"));
		}
		catch (...) {}
	}

	//release all static resources
	WUIF::D2DResources::d2dDevice.Reset();
	WUIF::D2DResources::wicFactory.Reset();
	CoUninitialize();
	WUIF::D2DResources::dwFactory.Reset();
	WUIF::D2DResources::d2dFactory.Reset();
	/*calling ReportLiveDeviceObjects just after releasing all your Direct3D objects just before
	releasing the ID3D11Device should report that all objects reference counts are 0--although the
	'internal' reference counts might still be > 0 since a few objects are alive as long as the
	device is alive for internal defaults. It can also help to call ClearState and then Flush on
	the immediate context just before doing the report to ensure nothing is being kept alive by
	being bound to the render pipeline or because of lazy destruction. This information is output
	to the Debug Window in VS*/
#ifdef _DEBUG
	if (WUIF::D3D11Resources::d3d11ImmediateContext != nullptr)
	{
		WUIF::D3D11Resources::d3d11ImmediateContext->ClearState();
		WUIF::D3D11Resources::d3d11ImmediateContext->Flush();
	}
#endif
	WUIF::D3D11Resources::d3d11ImmediateContext.Reset();
	WUIF::D3D11Resources::d3d11Device1.Reset();
	WUIF::D3D12Resources::d3d11on12Device.Reset();
	WUIF::D3D12Resources::d3dCommQueue.Reset();
	WUIF::D3D12Resources::d3d12Device.Reset();
	WUIF::DXGIResources::dxgiDevice.Reset();
	WUIF::DXGIResources::dxgiAdapter.Reset();
	WUIF::DXGIResources::dxgiFactory.Reset();
#ifdef _DEBUG
	//output debug information for d3d11 and dxgi
	WUIF::D3D11Resources::d3dInfoQueue.Reset();
	if (WUIF::D3D11Resources::d3dDebug != nullptr)
	{
		//d3d11 device ref count should be 2 (1 is d3dDebug, and because d3dDebug is alive there is still a ref to an ID3D11Device
		WUIF::D3D11Resources::d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
		WUIF::D3D11Resources::d3dDebug.Reset();
	}
	WUIF::DXGIResources::dxgiInfoQueue.Reset();
	if (WUIF::DXGIResources::dxgiDebug != nullptr)
	{
		//report will be blank if there's no problem
		WUIF::DXGIResources::dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
		WUIF::DXGIResources::dxgiDebug.Reset();
	}
#endif // _DEBUG
	return retcode;
}


WUIF_exception::WUIF_exception(const LPTSTR msg) throw()
{
	wmsg = msg;
}


int WUIF::Run(int accelresource)
{
	//display the main window and any currently defined windows
	App->mainWindow->DisplayWindow();
	if (App->Windows->Next != nullptr)
	{
		Window *temp = App->Windows->Next;
		temp->DisplayWindow();
		while (temp->Next != nullptr)
		{
			temp = temp->Next;
			temp->DisplayWindow();
		}
	}
	HACCEL hAccelTable = NULL;
	if (accelresource)
	{
		HACCEL hAccelTable = LoadAcceleratorsW(*App->hInstance, MAKEINTRESOURCEW(accelresource));
	}

	// Main message loop:
	MSG msg = {};

	/*
	BOOL bRet;
	while ((bRet = GetMessageW(&msg, App->mainWindow->hWnd, 0, 0)) != 0)
	{
	if (bRet == -1)
	{
	return bRet;
	}
	if ((!hAccelTable) || (!TranslateAcceleratorW(msg.hwnd, hAccelTable, &msg)))
	{
	TranslateMessage(&msg);
	DispatchMessageW(&msg);
	}
	}*/
	do {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if ((!hAccelTable) || (!TranslateAcceleratorW(msg.hwnd, hAccelTable, &msg)))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}
		else
		{
			App->mainWindow->Present();
		}
	} while (WM_QUIT != msg.message);
	if (msg.wParam == WE_WNDPROC_EXCEPTION)
	{
		throw WUIF_terminate_exception{};
	}
	return (int)msg.wParam;
}
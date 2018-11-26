/*Copyright (c) 2018 Jonathan Campbell

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
#include "WUIF_Main.h"
#include "Application/Application.h"
#include "Window/Window.h"
#include "GFX/GFX.h" //needed for InitResources
#include "Utils/ErrorExit.h"
#include "Utils/SetDPIAwareness.h"
#include "Utils/OSCheck.h"
#include "Utils/CommandLineToArgvA.h"


namespace WUIF {
    //forward declaration - GFX_Flags is defined in WUIF_Const.h and appgfxflag is defined in WUIF.h
    //extern const FLAGS::GFX_Flags appgfxflag;
    class tempInitializer {
    public:
        const FLAGS::GFX_Flags appgfxflag;
        const OSVersion minosversion;
        tempInitializer(FLAGS::GFX_Flags flagvalue, OSVersion osvalue) : appgfxflag(flagvalue), minosversion(osvalue) {}
    };
    extern tempInitializer *tempInit; //definition in WUIF.h

    //definition for WUIF_exception::WUIF_exception found in WUIF_Error.h
    WUIF_exception::WUIF_exception(_In_ const LPTSTR msg) noexcept
    {
        wmsg = msg;
        return;
    }

    namespace App {
        extern std::mutex veclock;
        extern bool vecwrite;
        extern std::condition_variable vecready;
        extern std::vector<Window*> Windows;
        extern bool is_vecwritable();
    }
}

namespace //private namespace for this file only
{
    //special exception class for use in WUIF::Run in case an exception occurs in WndProc
    struct WUIF_WNDPROC_terminate_exception {};

    /*void InitResources()
    This function initializes the requested global graphics resources
    */
    void InitResources()
    {
        PrintEnter(TEXT("::InitResources"));
        //call GetDXGIAdapterandFactory as we do not know if the graphics card is D3D11 or 12 capable
        WUIF::DXGIResources::GetDXGIAdapterandFactory();
        if (WUIF::App::GFXflags & WUIF::FLAGS::D3D12)
        {
            WUIF::DebugPrint(TEXT("Using D3D12"));
            //create D3D12 device
            WUIF::D3D12Resources::CreateD3D12StaticResources();
        }
        if (WUIF::App::GFXflags & WUIF::FLAGS::D3D11)
        {
            WUIF::DebugPrint(TEXT("Using D3D11"));
            //create D3D11 device
            WUIF::D3D11Resources::CreateD3D11StaticResources();
        }
        if (WUIF::App::GFXflags & WUIF::FLAGS::D2D)
        {
            //Initialize the Windows Imaging Component (WIC) Factory
            WUIF::ThrowIfFailed(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
            WUIF::D2DResources::CreateD2DStaticResources();
        }
        PrintExit(TEXT("::InitResources"));
        return;
    } //end InitResources


    /*void ReleaseResources()
    Releases all global graphics resources prior to exiting application
    */
    void ReleaseResources()
    {
        PrintEnter(TEXT("::ReleaseResources"));
        if (WUIF::d3d12libAPI)
        {
            delete WUIF::d3d12libAPI;
            WUIF::d3d12libAPI = nullptr;
            //FreeLibrary(WUIF::App::libD3D12);
        }
        WINVECLOCK
        if (!WUIF::App::Windows.empty()) //Windows is empty if all windows have been closed
        {
            for (std::vector<WUIF::Window*>::reverse_iterator i = WUIF::App::Windows.rbegin();
                i != WUIF::App::Windows.rend(); i = WUIF::App::Windows.rbegin())
            {
                if (static_cast<WUIF::Window*>(*i)->isInitialized())
                {
                    WINVECUNLOCK
                    DestroyWindow(static_cast<WUIF::Window*>(*i)->hWnd());
                }
                else
                {
                    WINVECUNLOCK
                    delete static_cast<WUIF::Window*>(*i);
                }
                guard.lock();
                WUIF::App::vecready.wait(guard, WUIF::App::is_vecwritable);
                WUIF::App::vecwrite = false;
                //reload the iterator as delete will change the vector
                if (WUIF::App::Windows.empty())
                    break;
            }
        }
        WINVECUNLOCK

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
            #pragma warning (suppress: 26498)
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
        PrintExit(TEXT("::ReleaseResources"));
        return;
    }
}


/*int APIENTRY wWinMain
Main entry point for application. Two prototypes exist - one for UNICODE and one for ASCII/MBCS versions of the application.*/
#ifdef _UNICODE
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    PrintEnter(TEXT("::wWinMain"));
    WUIF::DebugPrint(TEXT("Application Start - Unicode build"));
#else
int APIENTRY  WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR  lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    PrintEnter(TEXT("::WinMain"));
    WUIF::DebugPrint(TEXT("Application Start - MBCS build"));
#endif

    #if defined(_MSC_VER) && defined(_DEBUG)
    //setup debug heap manager
    //get current bits
    int newFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    //clear the upper 16 bits and OR in the desired frequency and other flags
    newFlag = (newFlag & 0x0000FFFF) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF;
    //set the bits
    _CrtSetDbgFlag(newFlag);
    //ensure crt memory dump is output to the debug window
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    #endif

    /*Best practice is that all applications call the process-wide SetErrorMode function with a parameter of SEM_FAILCRITICALERRORS at
    startup. This is to prevent error mode dialogs from hanging the application.*/
    SetErrorMode(SEM_FAILCRITICALERRORS);
    SetLastError(WE_OK);
    int retcode = 0;

    //wrap the main branch in a "try". Any downline exceptions will bubble-up and be caught here for a clean termination of the program
    try
    {
        /*WUIF::App::GFXflags is a constant variable. However it is impossible to determine at compile time whether or not Direct3D 12
        will be permissible. This application could be running on Windows 8.1 or less. Another variable could be used indicating whether
        D3D12 is available, but that would have to be a non-const, again because determination can only be made at run-time. To prevent
        errors it's best to have a constant with compile time check. To get around this we do a clever trick through the changeconst
        function. For "changeconst" function to work the flag must be in writable memory and that can only be achieved by declaring
        WUIF::App::GFXflags in assembly code. If we declare it here we get two definitions and now nothing will compile. So we declare a
        class tempInitializer and assign the value to "appgfxflag" and in WUIF_Main.h place appgfxflag's value into "WUIF::App::GFXflags"
        which is what is actually used. We use a class so that after the value is placed into WUIF::App::GFXflags we can delete the memory
        overhead of appgfxflag.*/
        WUIF::changeconst(&const_cast<WUIF::FLAGS::GFX_Flags&>(WUIF::App::GFXflags), &WUIF::tempInit->appgfxflag);
        //check the OS version which sets WUIF::App::winversion and loads D3D12.dll if needed
        OSCheck(WUIF::tempInit->minosversion);
        //tempInit no longer needed so free up the memory
        delete WUIF::tempInit;
        WUIF::tempInit = nullptr;
        //set the other run-time constants
        WUIF::changeconst(&const_cast<HINSTANCE&>(WUIF::App::hInstance), &hInstance);
        WUIF::changeconst(&const_cast<LPTSTR&>(WUIF::App::lpCmdLine), &lpCmdLine);
        WUIF::changeconst(&const_cast<int&>(WUIF::App::nCmdShow), &nCmdShow);

        //set DPI Awareness if not defined in the manifest
        SetDPIAwareness();
        WUIF::DebugPrint(TEXT("Initializing Resources"));
        WUIF::DXGIResources::GetDXGIAdapterandFactory(); //Calling this here will allow us to determine if a D3D12 adapter is available
        if (WUIF::App::GFXflags & WUIF::FLAGS::D3D12)
        {
            WUIF::DebugPrint(TEXT("Using D3D12"));
            //create D3D12 device
            WUIF::D3D12Resources::CreateD3D12StaticResources();
        }
        if (WUIF::App::GFXflags & WUIF::FLAGS::D3D11)
        {
            WUIF::DebugPrint(TEXT("Using D3D11"));
            //create D3D11 device
            WUIF::D3D11Resources::CreateD3D11StaticResources();
        }
        if (WUIF::App::GFXflags & WUIF::FLAGS::D2D)
        {
            //Initialize the Windows Imaging Component (WIC) Factory
            WUIF::ThrowIfFailed(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
            WUIF::D2DResources::CreateD2DStaticResources();
        }

        WUIF::App::mainWindow = CRT_NEW WUIF::Window();
        //set the WUIF::Window cmdshow variable to nCmdShow
        WUIF::App::mainWindow->cmdshow(nCmdShow);

        /*Pass the command line parameters as classic C main argc and argv
        argc - An integer that contains the count of arguments that follow in argv. The argc parameter is always greater than or equal to 1.
        argv - an array of null-terminated strings representing command-line arguments entered by the user of the program. By convention,
                 argv[0] is the command with which the program is invoked, argv[1] is the first command-line argument, and so on, until
                 argv[argc], which is always NULL. By standard these are multi-byte strings*/
        int    argc  = 0;
        LPSTR *argv  = CommandLineToArgvA(GetCommandLineA(), &argc); //get command line as multi-byte strings (NOT wide!)

        #ifdef DEBUGOUTPUTFULL
        //send to debug output contents of argc and argv
        //this is a special case and it doesn't fit WUIF::DebugPrint as elsewhere
        char t[24] = "argv has   arguments\r\n\0";
        t[9] = static_cast<char>('0' + (argc-1));
        OutputDebugStringA(t);
        for (int i = 0; i < argc; i++)
        {
            char s[22] = "argv[ ] is: ";
            s[5] = static_cast<char>('0' + i);
            OutputDebugStringA(s);
            OutputDebugStringA(argv[i]);
            OutputDebugStringA("\r\n\0");
        }
        #endif
        WUIF::DebugPrint(TEXT("Executing 'main'"));

        //call the application entry point and pass argc and argv
        retcode = main(argc, argv);

        //add a newline just in case prior output messages haven't
        OutputDebugString(TEXT(""));
        WUIF::DebugPrint(TEXT("Exited 'main'"));
        //free memory allocated to argv in CommandLineToArgvA
        HeapFree(GetProcessHeap(), NULL, argv);
        WUIF::DebugPrint(TEXT("Releasing resources"));
        ReleaseResources();
    }
    catch (const WUIF::WUIF_exception &e)
    {
        //Release resources could clear error save and reset
        DWORD error = GetLastError();
        WUIF::DebugPrint(TEXT("WUIF exception encountered"));
        retcode = ERROR_PROCESS_ABORTED;
        if (WUIF::tempInit)
        {
            delete WUIF::tempInit;
            WUIF::tempInit = nullptr;
        }
        try
        {
            //execute user created exception handler if registered
            if (WUIF::App::ExceptionHandler != nullptr)
            {
                try { WUIF::App::ExceptionHandler(); }
                catch (...) { } //ignore any exceptions
            }
            ::ReleaseResources();
            SetLastError(error);
            ErrorExit(e.WUIFWhat());
        }
        catch (...) { }
    }
    catch (const WUIF_WNDPROC_terminate_exception)
    {
        try
        {
            //execute user created exception handler if registered
            if (WUIF::App::ExceptionHandler != nullptr)
            {
                try { WUIF::App::ExceptionHandler(); }
                catch (...) {}
            }
            ::ReleaseResources();
            //ErrorExit already called
        }
        catch (...) {}
    }
    catch (...) //catch all other unhandled exceptions
    {
        //Release resources could clear error save and reset
        DWORD error = GetLastError();
        WUIF::DebugPrint(TEXT("exception encountered"));
        retcode = ERROR_PROCESS_ABORTED;
        //C6001 using uninitialized memory
        #pragma warning(suppress: 6001)
        if (WUIF::tempInit)
        {
            delete WUIF::tempInit;
            WUIF::tempInit = nullptr;
        }
        try
        {
            if (WUIF::App::ExceptionHandler != nullptr)
            {
                try { WUIF::App::ExceptionHandler(); }
                catch (...) {}
            }
            ::ReleaseResources();
            SetLastError(error);
            ErrorExit(TEXT("Critical Unhandled Exception!"));
        }
        catch (...) {}
    }
    WUIF::DebugPrint(TEXT("Terminating Application"));
    #ifdef _UNICODE
    PrintExit(TEXT("::wWinMain"));
    #else
    PrintExit(TEXT("::WinMain"));
    #endif
    //_CrtDumpMemoryLeaks();
    return retcode;
}

/*int WUIF::Run(_In_opt_ int accelresource)
Main loop for the application

int accelresource - acceleration resources

Return value
int
   Returns a success failure code from msg.wParam
*/
int WUIF::Run(_In_opt_ int accelresource)
{
    //display the main window and any currently defined windows
    App::mainWindow->DisplayWindow();
    {
        WINVECLOCK
        for (std::vector<Window*>::iterator i = App::Windows.begin(); i != App::Windows.end(); ++i)
        {
            if (*i != App::mainWindow)
            {
                static_cast<Window*>(*i)->DisplayWindow();
            }
        }
        WINVECUNLOCK
    }
    HACCEL hAccelTable = nullptr;
    if (accelresource)
    {
        hAccelTable = LoadAccelerators(App::hInstance, MAKEINTRESOURCE(accelresource));
    }

    // Main message loop:
    MSG msg = {};
    while (WM_QUIT != msg.message)
    {
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
            //update the scene
            //renderer->Update();
            //render frames during idle time
            //renderer->Render();
            //present the frame to the screen
            App::mainWindow->Present();
        }
    }
    if (msg.wParam == WE_WNDPROC_EXCEPTION)
    {
        SetLastError(WE_WNDPROC_EXCEPTION);
        throw WUIF_WNDPROC_terminate_exception{};
    }
    return static_cast<int>(msg.wParam);
}
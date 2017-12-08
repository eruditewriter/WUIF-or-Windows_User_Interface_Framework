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
#include <stdio.h>
#include <strsafe.h>         //needed for ErrorExit's StringCchPrintf
#include <ShellScalingApi.h> //needed for PROCESS_DPI_AWARENESS, PROCESS_PER_MONITOR_DPI_AWARE, MONITOR_DPI_TYPE, MDT_EFFECTIVE_DPI
#include <shellapi.h>
#include <mbstring.h> //needed for _ismbblead
#include "WUIF_Main.h"
#include "WUIF_Error.h"
#include "Application\Application.h"
#include "Window\Window.h"
#include "GFX\GFX.h" //needed for CheckOS and InitResources
#include "GFX\DXGI\DXGI.h"
#include "GFX\D3D\D3D12.h"
#include "GFX\D3D\D3D11.h"
#include "GFX\D2D\D2D.h"


WUIF::WUIF_exception::WUIF_exception(_In_ const LPTSTR msg) throw()
{
    wmsg = msg;
}

//private namespace for this file only
namespace {
    struct WUIF_WNDPROC_terminate_exception {};

    void ErrorExit(_In_ LPTSTR lpszMessage)
    {
              DWORD   numchars = 0;
              LPVOID  lpMsgBuf = nullptr;
        const DWORD   err      = GetLastError();

        if (err & 0x20000000L) //application defined error
        {
            LPTSTR lpSource = nullptr;
            switch (err)
            {
                case WE_FAIL:
                {
                    lpSource = _T("FAIL");
                }
                    break;
                case WE_CRITFAIL:
                {
                    lpSource = _T("CRITFAIL");
                }
                break;
                case WE_INVALIDARG: {
                    lpSource = _T("INVALIDARG");
                }
                break;
                case WE_D3D12_NOT_FOUND:
                {
                    lpSource = _T("D3D12_NOT_FOUND");
                }
                break;
                case WE_WNDPROC_EXCEPTION:
                {
                    lpSource = _T("WNDPROC_EXCEPTION");
                }
                break;
                default:
                {
                    lpSource = _T("UNDEFINED_WUIF_EXCEPTION");
                }
            }
            numchars = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS,
              lpSource,
              NULL,
              NULL,
              reinterpret_cast<LPTSTR>(&lpMsgBuf),
              0,
              NULL);
        }
        else
        {
            numchars = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
              NULL,
              err,
              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
              reinterpret_cast<LPTSTR>(&lpMsgBuf),
              0,
              NULL);
        }
        LPVOID lpDisplayBuf = nullptr;
        if (numchars != 0)
        {
            /*allocate room for: pszTxt size (pszTxtsize) + length of lpszMessage + sizeof err
            (dword) + 1 for negative sign + lpMsgBuf (numchars)*/
            constexpr unsigned int pszTxtsize = 118;
            lpDisplayBuf = HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
                                     (pszTxtsize + lstrlen(static_cast<LPCTSTR>(lpszMessage))
                                        + (sizeof(DWORD) + 1) + numchars) * sizeof(TCHAR));
        }
        HRESULT hr = E_FAIL;
        if (lpDisplayBuf)
        {
            LPCTSTR pszTxt = _T("An unrecoverable critical error was encountered and the application is now terminating!\n\n%s\n\nReported error is %d: %s");
            hr = StringCchPrintf(static_cast<LPTSTR>(lpDisplayBuf),
                                 HeapSize(GetProcessHeap(), NULL, lpDisplayBuf) / sizeof(TCHAR),
                                 pszTxt,
                                 lpszMessage, err, lpMsgBuf);
        }
        if ((hr == S_OK) | (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
        {
            MessageBox(NULL,
                       static_cast<LPCTSTR>(lpDisplayBuf),
                       _T("Critical Error"),
                       (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
        }
        else
        {
            MessageBox(NULL,
                       _T("An unrecoverable critical error was encountered and the application is now terminating! Unable to retrieve error message."),
                       _T("Critical Error"),
                       (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
        }
        /*LocalFree is not in the modern SDK, so it cannot be used to free the result buffer.
        Instead, use HeapFree (GetProcessHeap(), allocatedMessage). In this case, this is the same
        as calling LocalFree on memory.*/
        if (lpDisplayBuf)
        {
            HeapFree(GetProcessHeap(), NULL, lpDisplayBuf);
        }
        if (lpMsgBuf)
        {
            HeapFree(GetProcessHeap(), NULL, lpMsgBuf);
        }
    }

    LPSTR* CommandLineToArgvA(_In_opt_ LPCSTR lpCmdLine, _Out_ int *pNumArgs)
    {
        if (!pNumArgs)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }
        *pNumArgs = 0;
        /*follow CommandLinetoArgvW and if lpCmdLine is NULL return the path to the executable.
        Use 'programname' so that we don't have to allocate MAX_PATH * sizeof(CHAR) for argv
        every time. Since this is ANSI the return can't be greater than MAX_PATH (260
        characters)*/
        CHAR programname[MAX_PATH] = {};
        /*pnlength = the length of the string that is copied to the buffer, in characters, not
        including the terminating null character*/
        DWORD pnlength = GetModuleFileNameA(NULL, programname, MAX_PATH);
        if (pnlength == 0) //error getting program name
        {
            //GetModuleFileNameA will SetLastError
            return NULL;
        }
        if (*lpCmdLine == NULL)
        {

            /*In keeping with CommandLineToArgvW the caller should make a single call to HeapFree
            to release the memory of argv. Allocate a single block of memory with space for two
            pointers (representing argv[0] and argv[1]). argv[0] will contain a pointer to argv+2
            where the actual program name will be stored. argv[1] will be nullptr per the C++
            specifications for argv. Hence space required is the size of a LPSTR (char*) multiplied
            by 2 [pointers] + the length of the program name (+1 for null terminating character)
            multiplied by the sizeof CHAR. HeapAlloc is called with HEAP_GENERATE_EXCEPTIONS flag,
            so if there is a failure on allocating memory an exception will be generated.*/
            LPSTR *argv = static_cast<LPSTR*>(HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
                (sizeof(LPSTR) * 2) + ((pnlength + 1) * sizeof(CHAR))));
            memcpy(argv + 2, programname, pnlength+1); //add 1 for the terminating null character
            argv[0] = reinterpret_cast<LPSTR>(argv + 2);
            argv[1] = nullptr;
            *pNumArgs = 1;
            return argv;
        }
        /*We need to determine the number of arguments and the number of characters so that the
        proper amount of memory can be allocated for argv. Our argument count starts at 1 as the
        first "argument" is the program name even if there are no other arguments per specs.*/
        int argc        = 1;
        int numchars    = 0;
        LPCSTR templpcl = lpCmdLine;
        bool in_quotes  = false;  //'in quotes' mode is off (false) or on (true)
        /*first scan the program name and copy it. The handling is much simpler than for other
        arguments. Basically, whatever lies between the leading double-quote and next one, or a
        terminal null character is simply accepted. Fancier handling is not required because the
        program name must be a legal NTFS/HPFS file name. Note that the double-quote characters are
        not copied.*/
        do {
            if (*templpcl == '"')
            {
                //don't add " to character count
                in_quotes = !in_quotes;
                templpcl++; //move to next character
                continue;
            }
            ++numchars; //count character
            templpcl++; //move to next character
            if (_ismbblead(*templpcl) != 0) //handle MBCS
            {
                ++numchars;
                templpcl++; //skip over trail byte
            }
        } while (*templpcl != '\0' && (in_quotes || (*templpcl != ' ' && *templpcl != '\t')));
        //parsed first argument
        if (*templpcl == '\0')
        {
            /*no more arguments, rewind and the next for statement will handle*/
            templpcl--;
        }
        //loop through the remaining arguments
        int slashcount       = 0; //count of backslashes
        bool countorcopychar = true; //count the character or not
        for (;;)
        {
            if (*templpcl)
            {
                //next argument begins with next non-whitespace character
                while (*templpcl == ' ' || *templpcl == '\t')
                    ++templpcl;
            }
            if (*templpcl == '\0')
                break; //end of arguments

            ++argc; //next argument - increment argument count
            //loop through this argument
            for (;;)
            {
                /*Rules:
                  2N     backslashes   + " ==> N backslashes and begin/end quote
                  2N + 1 backslashes   + " ==> N backslashes + literal "
                  N      backslashes       ==> N backslashes*/
                slashcount     = 0;
                countorcopychar = true;
                while (*templpcl == '\\')
                {
                    //count the number of backslashes for use below
                    ++templpcl;
                    ++slashcount;
                }
                if (*templpcl == '"')
                {
                    //if 2N backslashes before, start/end quote, otherwise count.
                    if (slashcount % 2 == 0) //even number of backslashes
                    {
                        if (in_quotes && *(templpcl +1) == '"')
                        {
                            in_quotes = !in_quotes; //NB: parse_cmdline omits this line
                            templpcl++; //double quote inside quoted string
                        }
                        else
                        {
                            //skip first quote character and count second
                            countorcopychar = false;
                            in_quotes = !in_quotes;
                        }
                    }
                    slashcount /= 2;
                }
                //count slashes
                while (slashcount--)
                {
                    ++numchars;
                }
                if (*templpcl == '\0' || (!in_quotes && (*templpcl == ' ' || *templpcl == '\t')))
                {
                    //at the end of the argument - break
                    break;
                }
                if (countorcopychar)
                {
                    if (_ismbblead(*templpcl) != 0) //should copy another character for MBCS
                    {
                        ++templpcl; //skip over trail byte
                        ++numchars;
                    }
                    ++numchars;
                }
                ++templpcl;
            }
            //add a count for the null-terminating character
            ++numchars;
        }
        /*allocate memory for argv. Allocate a single block of memory with space for argc number of
        pointers. argv[0] will contain a pointer to argv+argc where the actual program name will be
        stored. argv[argc] will be nullptr per the C++ specifications. Hence space required is the
        size of a LPSTR (char*) multiplied by argc + 1 pointers + the number of characters counted
        above multiplied by the sizeof CHAR. HeapAlloc is called with HEAP_GENERATE_EXCEPTIONS
        flag, so if there is a failure on allocating memory an exception will be generated.*/
        LPSTR *argv = static_cast<LPSTR*>(HeapAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
            (sizeof(LPSTR) * (argc+1)) + (numchars * sizeof(CHAR))));
        //now loop through the commandline again and split out arguments
        in_quotes      = false;
        templpcl       = lpCmdLine;
        //C6011 - Dereferencing null pointer - dereferencing NULL pointer 'argv'
        #pragma warning(suppress: 6011)
        argv[0]        = reinterpret_cast<LPSTR>(argv + argc+1);
        LPSTR tempargv = reinterpret_cast<LPSTR>(argv + argc+1);
        do {
            if (*templpcl == '"')
            {
                in_quotes = !in_quotes;
                templpcl++; //move to next character
                continue;
            }
            *tempargv++ = *templpcl;
            templpcl++; //move to next character
            if (_ismbblead(*templpcl) != 0) //should copy another character for MBCS
            {
                *tempargv++ = *templpcl; //copy second byte
                templpcl++; //skip over trail byte
            }
        } while (*templpcl != '\0' && (in_quotes || (*templpcl != ' ' && *templpcl != '\t')));
        //parsed first argument
        if (*templpcl == '\0')
        {
            //no more arguments, rewind and the next for statement will handle
            templpcl--;
        }
        else
        {
            //end of program name - add null terminator
            *tempargv = '\0';
        }
        int currentarg   = 1;
        argv[currentarg] = ++tempargv;
        //loop through the remaining arguments
        slashcount      = 0; //count of backslashes
        countorcopychar = true; //count the character or not
        for (;;)
        {
            if (*templpcl)
            {
                //next argument begins with next non-whitespace character
                while (*templpcl == ' ' || *templpcl == '\t')
                    ++templpcl;
            }
            if (*templpcl == '\0')
                break; //end of arguments
            argv[currentarg] = ++tempargv; //copy address of this argument string
            //next argument - loop through it's characters
            for (;;)
            {
                /*Rules:
                  2N     backslashes   + " ==> N backslashes and begin/end quote
                  2N + 1 backslashes   + " ==> N backslashes + literal "
                  N      backslashes       ==> N backslashes*/
                slashcount      = 0;
                countorcopychar = true;
                while (*templpcl == '\\')
                {
                    //count the number of backslashes for use below
                    ++templpcl;
                    ++slashcount;
                }
                if (*templpcl == '"')
                {
                    //if 2N backslashes before, start/end quote, otherwise copy literally.
                    if (slashcount % 2 == 0) //even number of backslashes
                    {
                        if (in_quotes && *(templpcl+1) == '"')
                        {
                            in_quotes = !in_quotes; //NB: parse_cmdline omits this line
                            templpcl++; //double quote inside quoted string
                        }
                        else
                        {
                            //skip first quote character and count second
                            countorcopychar = false;
                            in_quotes       = !in_quotes;
                        }
                    }
                    slashcount /= 2;
                }
                //copy slashes
                while (slashcount--)
                {
                    *tempargv++ = '\\';
                }
                if (*templpcl == '\0' || (!in_quotes && (*templpcl == ' ' || *templpcl == '\t')))
                {
                    //at the end of the argument - break
                    break;
                }
                if (countorcopychar)
                {
                    *tempargv++ = *templpcl;
                    if (_ismbblead(*templpcl) != 0) //should copy another character for MBCS
                    {
                        ++templpcl; //skip over trail byte
                        *tempargv++ = *templpcl;
                    }
                }
                ++templpcl;
            }
            //null-terminate the argument
            *tempargv = '\0';
            ++currentarg;
        }
        argv[argc] = nullptr;
        *pNumArgs = argc;
        return argv;
    }

    #ifdef _MSC_VER
    #pragma warning(push)
    //GetModuleHandle could return NULL - this should never happen as these are core dll's for any application
    #pragma warning(disable: 6387)
    //using x = pfnx - value pointed to is assigned only once, mark it as a pointer to const
    #pragma warning(disable: 26496)
    #endif
    const WUIF::OSVersion OSCheck()
    {
        WUIF::DebugPrint(_T("OSCheck"));
        /*check for Win10 by loading D3D12.dll, then check for specific version.
        If no d3d12 and hence not Win10 no need to try and load a bunch of non existing processes.
        Be more efficient and skip straight to win8/8.1/7 checks*/
        WUIF::App::libD3D12 = LoadLibrary(_T("D3D12.dll"));
        if (WUIF::App::libD3D12) //app is on Win10
        {
            WUIF::DebugPrint(_T("Windows 10 detected"));
            if (!(WUIF::App::GFXflags & WUIF::FLAGS::D3D12))
            {
                FreeLibrary(WUIF::App::libD3D12);
                WUIF::App::libD3D12 = NULL;
            }
            //set the dpi awareness for Win10
            using PFN_SET_THREAD_DPI_AWARENESS_CONTEXT = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
            PFN_SET_THREAD_DPI_AWARENESS_CONTEXT pfnsetthreaddpiawarenesscontext = reinterpret_cast<PFN_SET_THREAD_DPI_AWARENESS_CONTEXT>(GetProcAddress(GetModuleHandle(_T("user32.dll")), "SetThreadDpiAwarenessContext"));
            if (pfnsetthreaddpiawarenesscontext) //if this fails it will fall through to the next case and try SetProcessDpiAwareness
            {
                //DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 should be available in >= Win10 Creators Update
                DPI_AWARENESS_CONTEXT dpicontext = pfnsetthreaddpiawarenesscontext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
                if (dpicontext == NULL) //failed try DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
                {
                    pfnsetthreaddpiawarenesscontext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
                }
            }
            //determine which Win10 version
            HMODULE lib = GetModuleHandle(_T("kernel32.dll"));
            //check for Win10 1709 Fall Creators Update
            using PFN_GET_USER_DEFAULT_GEO_NAME = int(WINAPI*)(LPWSTR, int);
            PFN_GET_USER_DEFAULT_GEO_NAME pfngetuserdefaultgeoname = reinterpret_cast<PFN_GET_USER_DEFAULT_GEO_NAME>(GetProcAddress(lib, "GetUserDefaultGeoName"));
            if (pfngetuserdefaultgeoname)
            {
                WUIF::DebugPrint(_T("Windows 10 OS version is %d"), 1709);
                return WUIF::OSVersion::WIN10_1709;
            }
            //check for Win10 1703 Creators Update
            using PFN_MAP_VIEW_OF_FILE2 = void(*)(HANDLE, HANDLE, ULONG64, PVOID, SIZE_T, ULONG, ULONG);
            PFN_MAP_VIEW_OF_FILE2 pfnmapviewoffile2 = reinterpret_cast<PFN_MAP_VIEW_OF_FILE2>(GetProcAddress(lib, "MapViewOfFile2"));
            if (pfnmapviewoffile2)
            {
                WUIF::DebugPrint(_T("Windows 10 OS version is %d"), 1703);
                return WUIF::OSVersion::WIN10_1703;
            }
            //check for Win10 1607 Anniversary Update
            using PFN_SET_THREAD_DESCRIPTION = HRESULT(WINAPI*)(HANDLE, PCWSTR);
            PFN_SET_THREAD_DESCRIPTION pfnsetthreaddescription = reinterpret_cast<PFN_SET_THREAD_DESCRIPTION>(GetProcAddress(lib, "SetThreadDescription"));
            if (pfnsetthreaddescription)
            {
                WUIF::DebugPrint(_T("Windows 10 OS version is %d"), 1607);
                return WUIF::OSVersion::WIN10_1607;
            }
            //check for Win10 1511 November Update
            using PFN_IS_WOW64_PROCESS2 = BOOL(WINAPI*)(HANDLE, USHORT, USHORT);
            PFN_IS_WOW64_PROCESS2 pfniswow64process2 = reinterpret_cast<PFN_IS_WOW64_PROCESS2>(GetProcAddress(lib, "IsWow64Process2"));
            if (pfniswow64process2)
            {
                WUIF::DebugPrint(_T("Windows 10 OS version is %d"), 1511);
                return WUIF::OSVersion::WIN10_1511;
            }
            WUIF::DebugPrint(_T("Windows 10 OS version is initial release"));
            return WUIF::OSVersion::WIN10;
        }
        else
        {
            //Use D3D12 only = ((WUIF::App::GFXflags & WUIF::FLAGS::D3D12) && (!(WUIF::App::GFXflags & WUIF::FLAGS::D3D11)))
            if ((WUIF::App::GFXflags & WUIF::FLAGS::D3D12) && (!(WUIF::App::GFXflags & WUIF::FLAGS::D3D11)))
            {
                //if we are D3D12 only and not on Win 10 must throw an error
                SetLastError(WE_D3D12_NOT_FOUND);
                throw WUIF::WUIF_exception(_T("This application requires Direct3D 12, D3D12.dll not found!"));
            }
            WUIF::FLAGS::GFX_Flags t_flag = WUIF::App::GFXflags & (~WUIF::FLAGS::D3D12);
            changeconst(&const_cast<WUIF::FLAGS::GFX_Flags&>(WUIF::App::GFXflags), &t_flag);

            HINSTANCE shcorelib = LoadLibrary(_T("shcore.dll"));
            if (shcorelib)
            {
                //set dpi awareness for Win8.1
                using PFN_SET_PROCESS_DPI_AWARENESS = HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS);
                PFN_SET_PROCESS_DPI_AWARENESS pfnsetprocessdpiawareness = reinterpret_cast<PFN_SET_PROCESS_DPI_AWARENESS>(GetProcAddress(shcorelib, "SetProcessDpiAwareness"));
                if (pfnsetprocessdpiawareness)
                {
                    pfnsetprocessdpiawareness(PROCESS_PER_MONITOR_DPI_AWARE); //this will fail if it's in the manifest
                    FreeLibrary(shcorelib);
                    WUIF::DebugPrint(_T("Windows 8.1 detected"));
                    return WUIF::OSVersion::WIN8_1;
                }
                FreeLibrary(shcorelib);
                shcorelib = NULL;
            }
            //use Vista/Win7/Win8 SetProcessDPIAware - this isn't guaranteed to be in later operating systems so we shouldn't link statically
            using PFN_SET_PROCESS_DPI_AWARE = BOOL(WINAPI*)(void);
            PFN_SET_PROCESS_DPI_AWARE pfnsetprocessdpiaware = reinterpret_cast<PFN_SET_PROCESS_DPI_AWARE>(GetProcAddress(GetModuleHandle(_T("user32.dll")), "SetProcessDPIAware"));
            if (pfnsetprocessdpiaware)
            {
                pfnsetprocessdpiaware();
            }
            //check for Win 8
            using PFN_CREATE_FILE2 = HANDLE(WINAPI*)(REFIID, void**);
            PFN_CREATE_FILE2 pfncreatefile2 = reinterpret_cast<PFN_CREATE_FILE2>(GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "CreateFile2"));
            if (pfncreatefile2)
            {
                WUIF::DebugPrint(_T("Windows 8 detected"));
                return WUIF::OSVersion::WIN8;
            }
        }
        WUIF::DebugPrint(_T("Windows version detected 7"));
        return WUIF::OSVersion::WIN7;
        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif
    } //end OSCheck

    void InitResources()
    {
        WUIF::DebugPrint(_T("Initializing Resources"));
        WUIF::DXGIResources::GetDXGIAdapterandFactory();
        if (WUIF::App::GFXflags & WUIF::FLAGS::D3D12)
        {
            //create D3D12 device
            WUIF::D3D12Resources::CreateStaticResources();
            WUIF::DebugPrint(_T("Using D3D12 = True"));
        }
        if (WUIF::App::GFXflags & WUIF::FLAGS::D3D11)
        {
            //create D3D11 device
            WUIF::D3D11Resources::CreateStaticResources();
            WUIF::DebugPrint(_T("Using D3D11 = True"));
        }
        if (WUIF::App::GFXflags & WUIF::FLAGS::D2D)
        {
            //Initialize the Windows Imaging Component (WIC) Factory
            #ifdef _DEBUG
            ThrowIfFailed(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
            #else
            WUIF::ThrowIfFailed(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
            #endif _DEBUG
            WUIF::D2DResources::CreateStaticResources();
        }
        return;
    } //end InitResources

    void ReleaseResources()
    {
        if (WUIF::App::libD3D12)
        {
            FreeLibrary(WUIF::App::libD3D12);
        }
        if (!WUIF::App::Windows.empty()) //Windows is empty if all windows have been closed
        {
            for (std::forward_list<WUIF::Window*>::iterator i = WUIF::App::Windows.begin();
                    i != WUIF::App::Windows.end();
                    ++i)
            {
                delete static_cast<WUIF::Window*>(*i);
                if (WUIF::App::Windows.empty())
                    break;
            }
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
    }
}

#ifdef _UNICODE
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
#else
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
#endif
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    #ifdef _UNICODE
    WUIF::DebugPrint(_T("Application Start - Unicode enabled"));
    #else
    WUIF::DebugPrint(_T("Application Start - ANSI enabled"));
    #endif

    /*Retrieves or modifies the state of the _crtDbgFlag flag to control the allocation behavior of
    the debug heap manager (debug version only)*/
    //get current bits
    int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    //clear the upper 16 bits and OR in the desired frequency and other flags
    tmp = (tmp & 0x0000FFFF) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF;
    //set the bits
    _CrtSetDbgFlag(tmp);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);

    /*Best practice is that all applications call the process-wide SetErrorMode function with a
    parameter of SEM_FAILCRITICALERRORS at startup. This is to prevent error mode dialogs from
    hanging the application.*/
    SetErrorMode(SEM_FAILCRITICALERRORS);

    int retcode = 0;
    try
    {
        #ifdef _DEBUG
        WUIF::FLAGS::GFX_Flags tempflag = 0;
        if (WUIF::flagexpr & 1u << (1 - 1))
        {
            tempflag |= WUIF::FLAGS::WARP;
        }
        if (WUIF::flagexpr & 1u << (2 - 1))
        {
            tempflag |= WUIF::FLAGS::D2D;
        }
        if (WUIF::flagexpr & 1u << (3 - 1))
        {
            tempflag |= WUIF::FLAGS::D3D11;
        }
        if (WUIF::flagexpr & 1u << (4 - 1))
        {
            tempflag |= WUIF::FLAGS::D3D12;
        }
        changeconst(&const_cast<WUIF::FLAGS::GFX_Flags&>(WUIF::App::GFXflags), &tempflag);
        #else
        changeconst(&const_cast<WUIF::FLAGS::GFX_Flags&>(WUIF::App::GFXflags), &const_cast<unsigned int&>(WUIF::flagexpr));
        #endif
        //assert if GFXflags is zero
        _ASSERTE(WUIF::App::GFXflags);
        //ensure either D3D11 or D3D12 is declared for use - if neither is set assert
        _ASSERTE(WUIF::App::GFXflags & (WUIF::FLAGS::D3D11 | WUIF::FLAGS::D3D12));
        changeconst(&const_cast<HINSTANCE&>(WUIF::App::hInstance), &hInstance);
        changeconst(&const_cast<LPTSTR&>(WUIF::App::lpCmdLine), &lpCmdLine);
        changeconst(&const_cast<int&>(WUIF::App::nCmdShow), &nCmdShow);
        WUIF::OSVersion osv = OSCheck();
        changeconst(&const_cast<WUIF::OSVersion&>(WUIF::App::winversion), &osv);

        WUIF::App::mainWindow = DBG_NEW WUIF::Window();
        //set the cmdshow variable to the nCmdShow passed into the application
        WUIF::App::mainWindow->property.cmdshow(nCmdShow);
        //create application
        #pragma warning(suppress: 26409) //avoid calling new and delete explicitly
        //WUIF::App = DBG_NEW WUIF::Application(hInstance, lpCmdLine, nCmdShow, OSCheck());

        InitResources();

        /*argc - An integer that contains the count of arguments that follow in argv. The argc
            parameter is always greater than or equal to 1.
          argv - an array of null-terminated strings representing command-line arguments entered by
            the user of the program. By convention, argv[0] is the command with which the program
            is invoked, argv[1] is the first command-line argument, and so on, until argv[argc],
            which is always NULL.*/
        int    argc  = 0;
        LPSTR *argv  = CommandLineToArgvA(GetCommandLineA(), &argc);

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
        WUIF::DebugPrint(_T("Executing 'main'"));
        //call the application entry point and pass argc and argv
        retcode = main(argc, argv);
        OutputDebugString(_T("\r\n\0"));//add a newline just in case other output messages before haven't
        WUIF::DebugPrint(_T("Exited 'main'"));
        //free memory allocate to argv in CommandLineToArgvA
        HeapFree(GetProcessHeap(), NULL, argv);
        WUIF::DebugPrint(_T("Releasing resources"));
        ReleaseResources();
    }
    catch (const WUIF::WUIF_exception &e)
    {
        WUIF::DebugPrint(_T("WUIF exception encountered"));
        retcode = ERROR_PROCESS_ABORTED;
        try
        {
            if (WUIF::App::ExceptionHandler != nullptr)
            {
                try { WUIF::App::ExceptionHandler(); }
                catch (...) { }
            }
            ReleaseResources();
            ErrorExit(e.WUIFWhat());
        }
        catch (...) { }
    }
    catch (const WUIF_WNDPROC_terminate_exception)
    {
        WUIF::DebugPrint(_T("WUIF_WNDPROC_terminate exception encountered"));
        retcode = ERROR_PROCESS_ABORTED;
        try {
            if (WUIF::App::ExceptionHandler != nullptr)
            {
                try { WUIF::App::ExceptionHandler(); }
                catch (...) {}
            }
            ReleaseResources();
            //ErrorExit already called
        }
        catch (...) {}
    }
    catch (...) //catch all other unhandled exceptions
    {
        WUIF::DebugPrint(_T("exception encountered"));
        retcode = ERROR_PROCESS_ABORTED;
        try {
            if (WUIF::App::ExceptionHandler != nullptr)
            {
                try { WUIF::App::ExceptionHandler(); }
                catch (...) {}
            }
            ReleaseResources();
            ErrorExit(_T("Critical Unhandled Exception!"));
        }
        catch (...) {}
    }
    WUIF::DebugPrint(_T("Terminating Applicaiton"));
    return retcode;
}


int WUIF::Run(_In_opt_ int accelresource)
{
    //display the main window and any currently defined windows
    App::mainWindow->DisplayWindow();
    for (std::forward_list<Window*>::iterator i = App::Windows.begin(); i != App::Windows.end(); ++i)
    {
        if (*i != App::mainWindow)
        {
            static_cast<Window*>(*i)->DisplayWindow();
        }
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
            App::mainWindow->Present();
        }
    }
    if (msg.wParam == WE_WNDPROC_EXCEPTION)
    {
        throw WUIF_WNDPROC_terminate_exception{};
    }
    return static_cast<int>(msg.wParam);
}
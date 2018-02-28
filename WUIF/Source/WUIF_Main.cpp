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
#include <strsafe.h>         //needed for ErrorExit's StringCchPrintf
#include <ShellScalingApi.h> //needed for PROCESS_DPI_AWARENESS, PROCESS_PER_MONITOR_DPI_AWARE, MONITOR_DPI_TYPE, MDT_EFFECTIVE_DPI
#include <mbstring.h>        //needed for _ismbblead in CommandLineToArgvA
#include "WUIF_Main.h"
#include "Application\Application.h"
#include "Window\Window.h"
#include "GFX\GFX.h" //needed for CheckOS and InitResources

//handy macro for D3D12 only flag setting in WUIF::App::GFXflags
#define D3D12ONLY (WUIF::App::GFXflags & WUIF::FLAGS::D3D12) && (!(WUIF::App::GFXflags & WUIF::FLAGS::D3D11))

namespace WUIF {
    //forward declaration - this is defined in WUIF.h
    extern const FLAGS::GFX_Flags flagexpr;

    //definition for WUIF_exception::WUIF_exception found in WUIF_Error.h
    WUIF_exception::WUIF_exception(_In_ const LPTSTR msg) noexcept
    {
        wmsg = msg;
    }

    namespace App {
        extern std::mutex veclock;
        extern bool vecwrite;
        extern std::condition_variable vecready;
        extern std::vector<Window*> Windows;
        extern bool is_vecwritable();
    }
}


//private namespace for this file only
namespace {
    //internal pointers for App::processdpiawareness and App::processdpiawarenesscontext
    PROCESS_DPI_AWARENESS _processdpiawareness        = PROCESS_DPI_UNAWARE;
    DPI_AWARENESS_CONTEXT _processdpiawarenesscontext = DPI_AWARENESS_CONTEXT_UNAWARE;

    //special exception class for use in WUIF::Run in case an exception occurs in WndProc
    struct WUIF_WNDPROC_terminate_exception {};

    /*void ErrorExit(_In_ LPTSTR lpszMessage)
    Executes when an exception occurs. Displays a MessageBox with an error message and the string
    passed to lpszMessage.

    LPTSTR lpszMessage[in] - string to be displayed along with error message in MessageBox

    Return results - none
    */
    void ErrorExit(_In_ LPTSTR lpszMessage)
    {
              DWORD   numchars = 0;
              LPVOID  lpMsgBuf = nullptr;
        const DWORD   err      = GetLastError();

        //if a WUIF error constant add appropriate text for error message
        if (err & 0x20000000L) //application defined error
        {
            LPTSTR lpSource = nullptr;
            switch (err)
            {
                case WE_OK:
                {
                    lpSource = TEXT("OK");
                }
                case WE_FAIL:
                {
                    lpSource = TEXT("FAIL");
                }
                break;
                case WE_CRITFAIL:
                {
                    lpSource = TEXT("CRITFAIL");
                }
                break;
                case WE_INVALIDARG: {
                    lpSource = TEXT("INVALIDARG");
                }
                break;
                case WE_WRONGOSFORAPP: {
                    lpSource = TEXT("WRONGOSFORAPP");
                }
                break;
                case WE_D3D12_NOT_FOUND:
                {
                    lpSource = TEXT("D3D12_NOT_FOUND");
                }
                break;
                case WE_WNDPROC_EXCEPTION:
                {
                    lpSource = TEXT("WNDPROC_EXCEPTION");
                }
                break;
                case WE_GRAPHICS_INVALID_DISPLAY_ADAPTER:
                {
                    lpSource = TEXT("GRAPHICS_INVALID_DISPLAY_ADAPTER");
                }
                break;
                default:
                {
                    lpSource = TEXT("UNDEFINED_WUIF_EXCEPTION");
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
            //system error - format message with system error codes and locale language
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
                                     (pszTxtsize + lstrlen(lpszMessage)
                                        + (sizeof(DWORD) + 1) + numchars) * sizeof(TCHAR));
        }
        HRESULT hr = E_FAIL;
        if (lpDisplayBuf)
        {
            LPCTSTR pszTxt = TEXT("An unrecoverable critical error was encountered and the application is now terminating!\n\n%s\n\nReported error is %d: %s");
            hr = StringCchPrintf(static_cast<LPTSTR>(lpDisplayBuf),
                                 HeapSize(GetProcessHeap(), NULL, lpDisplayBuf) / sizeof(TCHAR),
                                 pszTxt,
                                 lpszMessage, err, lpMsgBuf);
        }
        if ((hr == S_OK) | (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
        {
            MessageBox(NULL,
                       static_cast<LPCTSTR>(lpDisplayBuf),
                       TEXT("Critical Error"),
                       (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
        }
        else
        {
            MessageBox(NULL,
                       TEXT("An unrecoverable critical error was encountered and the application is now terminating! Unable to retrieve error message."),
                       TEXT("Critical Error"),
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

    /*LPSTR* CommandLineToArgvA(_In_ LPCSTR lpCmdLine, _Out_ int *pNumArgs)
    Takes the ASCII command line string and splits it into separate args.
    Equivalent to CommandLineToArgvW

    LPCSTR lpCmdLine[in] - should be GetCommandLineA()
    int *pNumArgs[out] - pointer to the number of args (ie. argc value of main(argc,argv[]) )

    Return results
        LPSTR* - pointer to the argv[] array
    */
    LPSTR* CommandLineToArgvA(_In_ LPCSTR lpCmdLine, _Out_ int *pNumArgs)
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
        const DWORD pnlength = GetModuleFileNameA(NULL, programname, MAX_PATH);
        if (pnlength == 0) //error getting program name
        {
            //GetModuleFileNameA will SetLastError
            return NULL;
        }
        if (lpCmdLine == nullptr)
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


    /*void OSCheck()
    Performs a check for the Windows OS Version and sets WUIF::App::winversion
    */
    #ifdef _MSC_VER
    #pragma warning(push)
    //GetModuleHandle could return NULL - this should never happen as these are core dll's for any application
    #pragma warning(disable: 6387)
    //using x = pfnx - value pointed to is assigned only once, mark it as a pointer to const
    //#pragma warning(disable: 26496)
    #endif
    void OSCheck()
    {
        WUIF::DebugPrint(TEXT("Performing OSCheck and setting DPI Awareness"));
        /*check for Win10 by loading D3D12.dll, then check for specific version.
        If no d3d12 and hence not Win10 no need to try and load a bunch of non existing processes.
        Be more efficient and skip straight to win8/8.1/7 checks*/
        WUIF::OSVersion osver = WUIF::OSVersion::WIN7;

        WUIF::App::libD3D12 = LoadLibrary(TEXT("D3D12.dll"));
        if (WUIF::App::libD3D12) //app is on Win10, check for which version
        {
            WUIF::DebugPrint(TEXT("Windows 10 detected"));
            //if not using D3D12 do not store libD3D12 for future use, set to NULL instead
            if (!(WUIF::App::GFXflags & WUIF::FLAGS::D3D12))
            {
                FreeLibrary(WUIF::App::libD3D12);
                WUIF::App::libD3D12 = NULL;
            }
            //determine which Win10 version
            HMODULE lib = GetModuleHandle(TEXT("kernel32.dll"));
            WUIF::ThrowIfFalse(lib);
            //check for Win10 1709 Fall Creators Update
            using PFN_GET_USER_DEFAULT_GEO_NAME = int(WINAPI*)(LPWSTR, int);
            PFN_GET_USER_DEFAULT_GEO_NAME pfngetuserdefaultgeoname = reinterpret_cast<PFN_GET_USER_DEFAULT_GEO_NAME>(GetProcAddress(lib, "GetUserDefaultGeoName"));
            if (pfngetuserdefaultgeoname)
            {
                WUIF::DebugPrint(TEXT("Windows 10 OS version is %i"), 1709);
                osver = WUIF::OSVersion::WIN10_1709;
            }
            else
            {
                //check for Win10 1703 Creators Update
                using PFN_MAP_VIEW_OF_FILE2 = void(*)(HANDLE, HANDLE, ULONG64, PVOID, SIZE_T, ULONG, ULONG);
                PFN_MAP_VIEW_OF_FILE2 pfnmapviewoffile2 = reinterpret_cast<PFN_MAP_VIEW_OF_FILE2>(GetProcAddress(lib, "MapViewOfFile2"));
                if (pfnmapviewoffile2)
                {
                    WUIF::DebugPrint(TEXT("Windows 10 OS version is %i"), 1703);
                    osver = WUIF::OSVersion::WIN10_1703;
                }
                else
                {
                    //check for Win10 1607 Anniversary Update
                    using PFN_SET_THREAD_DESCRIPTION = HRESULT(WINAPI*)(HANDLE, PCWSTR);
                    PFN_SET_THREAD_DESCRIPTION pfnsetthreaddescription = reinterpret_cast<PFN_SET_THREAD_DESCRIPTION>(GetProcAddress(lib, "SetThreadDescription"));
                    if (pfnsetthreaddescription)
                    {
                        WUIF::DebugPrint(TEXT("Windows 10 OS version is %i"), 1607);
                        osver = WUIF::OSVersion::WIN10_1607;
                    }
                    else
                    {
                        //check for Win10 1511 November Update
                        using PFN_IS_WOW64_PROCESS2 = BOOL(WINAPI*)(HANDLE, USHORT, USHORT);
                        PFN_IS_WOW64_PROCESS2 pfniswow64process2 = reinterpret_cast<PFN_IS_WOW64_PROCESS2>(GetProcAddress(lib, "IsWow64Process2"));
                        if (pfniswow64process2)
                        {
                            WUIF::DebugPrint(TEXT("Windows 10 OS version is %i"), 1511);
                            osver = WUIF::OSVersion::WIN10_1511;
                        }
                        else
                        {
                            WUIF::DebugPrint(TEXT("Windows 10 OS version is initial release"));
                            osver = WUIF::OSVersion::WIN10;
                        }
                    }
                }
            }
        }
        else
        {
            if (D3D12ONLY)
            {
                //if we are D3D12 only and not on Win 10 must throw an error
                SetLastError(WE_D3D12_NOT_FOUND);
                throw WUIF::WUIF_exception(TEXT("This application requires Direct3D 12, D3D12.dll not found!"));
            }
            /*Remove D3D12 flag if set, as D3D12 isn't available.
            Quicker to call changeconst than do a if (d3d12 set) type of expression*/
            const WUIF::FLAGS::GFX_Flags flagtempval = (WUIF::App::GFXflags & (~WUIF::FLAGS::D3D12));
            WUIF::changeconst(&const_cast<WUIF::FLAGS::GFX_Flags&>(WUIF::App::GFXflags), &flagtempval);
            //check for Win 8.1
            HMODULE lib = GetModuleHandle(TEXT("kernel32.dll"));
            WUIF::ThrowIfFalse(lib);
            using PFN_QUERY_PROTECTED_POLICY = BOOL(WINAPI*)(LPCGUID, PULONG_PTR);
            PFN_QUERY_PROTECTED_POLICY pfnqueryprotectedpolicy = reinterpret_cast<PFN_QUERY_PROTECTED_POLICY>(GetProcAddress(lib, "QueryProtectedPolicy"));
            if (pfnqueryprotectedpolicy)
            {
                WUIF::DebugPrint(TEXT("Windows 8.1 detected"));
                osver = WUIF::OSVersion::WIN8_1;
            }
            else
            {
                //check for Win 8
                using PFN_CREATE_FILE2 = HANDLE(WINAPI*)(REFIID, void**);
                PFN_CREATE_FILE2 pfncreatefile2 = reinterpret_cast<PFN_CREATE_FILE2>(GetProcAddress(lib, "CreateFile2"));
                if (pfncreatefile2)
                {
                    WUIF::DebugPrint(TEXT("Windows 8 detected"));
                    osver = WUIF::OSVersion::WIN8;
                }
                else
                {
                    WUIF::DebugPrint(TEXT("Windows version detected 7"));
                    osver = WUIF::OSVersion::WIN7;
                }
            }
        }
        WUIF::changeconst(&const_cast<WUIF::OSVersion&>(WUIF::App::winversion), &osver);
        return;
        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif
    } //end OSCheck

    /*void SetDPIAwareness()
    Sets the DPI Awareness by the proper method for the OS Version. Attempts to set the highest
    dpi awareness. If it fails it attempts lower dpi awareness until all fail. If all fail, dpi
    awareness will be default DPI_UNAWARE. Sets App::processdpiawareness and
    App::processdpiawarenesscontext (will be nullptr if not Win10 or supported). Using a manifest
    to set DPI awareness will supersede this function.*/
    #ifdef _MSC_VER
    #pragma warning(push)
    //GetModuleHandle could return NULL - tested via ThowIfFalse
    #pragma warning(disable: 6387)
    //dereferencing NULL pointer pfn*
    #pragma warning(disable: 6011)
    #endif
    void SetDPIAwareness()
    {
        HINSTANCE shcorelib = nullptr;
        //set the dpi awareness for Win10 1703 (Creators Update) or greater
        if (WUIF::App::winversion >= WUIF::OSVersion::WIN10_1703)
        {
            HMODULE lib = GetModuleHandle(TEXT("user32.dll"));
            WUIF::ThrowIfFalse(lib);
            using PFN_IS_VALID_DPI_AWARENESS_CONTEXT = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
            PFN_IS_VALID_DPI_AWARENESS_CONTEXT pfnisvaliddpiawarenesscontext = reinterpret_cast<PFN_IS_VALID_DPI_AWARENESS_CONTEXT>(GetProcAddress(lib, "IsValidDpiAwarenessContext"));
            WUIF::ThrowIfFalse(pfnisvaliddpiawarenesscontext);
            //DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 should be available in >= Win10 Creators Update
            if (pfnisvaliddpiawarenesscontext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
            {
                using PFN_SET_PROCESS_DPI_AWARENESS_CONTEXT = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
                PFN_SET_PROCESS_DPI_AWARENESS_CONTEXT pfnsetprocessdpiawarenesscontext = reinterpret_cast<PFN_SET_PROCESS_DPI_AWARENESS_CONTEXT>(GetProcAddress(lib, "SetProcessDpiAwarenessContext"));
                WUIF::ThrowIfFalse(pfnsetprocessdpiawarenesscontext);
                if (pfnsetprocessdpiawarenesscontext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
                {
                    _processdpiawareness        = PROCESS_PER_MONITOR_DPI_AWARE;
                    _processdpiawarenesscontext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
                    void *val = &_processdpiawareness;
                    WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
                    val = &_processdpiawarenesscontext;
                    WUIF::changeconst(&const_cast<DPI_AWARENESS_CONTEXT*&>(WUIF::App::processdpiawarenesscontext), &val);
                    WUIF::DebugPrint(TEXT("SetProcessDpiAwarenessContext used with DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2"), _processdpiawarenesscontext);
                }
                else
                {
                    const DWORD err = GetLastError();
                    WUIF::DebugPrint(TEXT("SetProcessDpiAwarenessContext failed with %u"), err);
                    goto getprocessawareness;
                }
            }
        }
        else
        {
            if (WUIF::App::winversion >= WUIF::OSVersion::WIN8_1)
            {
                shcorelib = LoadLibrary(TEXT("shcore.dll"));
                WUIF::ThrowIfFalse(shcorelib);
                //set dpi awareness for Win8.1
                using PFN_SET_PROCESS_DPI_AWARENESS = HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS);
                PFN_SET_PROCESS_DPI_AWARENESS pfnsetprocessdpiawareness = reinterpret_cast<PFN_SET_PROCESS_DPI_AWARENESS>(GetProcAddress(shcorelib, "SetProcessDpiAwareness"));
                WUIF::ThrowIfFalse(pfnsetprocessdpiawareness);
                HRESULT hr = pfnsetprocessdpiawareness(PROCESS_PER_MONITOR_DPI_AWARE);
                if (hr == S_OK)
                {
                    _processdpiawareness = PROCESS_PER_MONITOR_DPI_AWARE;
                    void * val = &_processdpiawareness;
                    WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
                    WUIF::DebugPrint(TEXT("SetProcessDpiAwareness used with PROCESS_PER_MONITOR_DPI_AWARE"));
                }
                else
                {
                    WUIF::DebugPrint(TEXT("SetProcessDpiAwareness failed with %d"), hr);
                    //Query the current process context by calling GetProcessDpiAwareness
                getprocessawareness:
                    if (!shcorelib)
                    {
                        shcorelib = LoadLibrary(TEXT("shcore.dll"));
                    }
                    using PFN_GET_PROCESS_DPI_AWARENESS = HRESULT(WINAPI*)(HANDLE, PROCESS_DPI_AWARENESS*);
                    PFN_GET_PROCESS_DPI_AWARENESS pfngetprocessdpiawareness = reinterpret_cast<PFN_GET_PROCESS_DPI_AWARENESS>(GetProcAddress(shcorelib, "GetProcessDpiAwareness"));
                    WUIF::ThrowIfFalse(pfngetprocessdpiawareness);
                    hr = pfngetprocessdpiawareness(NULL, &_processdpiawareness);
                    if (hr == S_OK)
                    {
                        void * val = &_processdpiawareness;
                        WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
                        WUIF::DebugPrint(TEXT("GetProcessDpiAwareness reports PROCESS_DPI_AWARENESS is %i"), _processdpiawareness);
                    }
                    else
                    {
                        WUIF::DebugPrint(TEXT("Unable to get current DPI Awareness."));
                    }
                }
                FreeLibrary(shcorelib);
            }
            else
            {
                /*use Vista/Win7/Win8 SetProcessDPIAware and IsProcessDPIAware - these are not
                guaranteed to be in later operating systems so we shouldn't link statically*/
                using PFN_SET_PROCESS_DPI_AWARE = BOOL(WINAPI*)(void);
                PFN_SET_PROCESS_DPI_AWARE pfnsetprocessdpiaware = reinterpret_cast<PFN_SET_PROCESS_DPI_AWARE>(GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetProcessDPIAware"));
                WUIF::ThrowIfFalse(pfnsetprocessdpiaware);
                if (pfnsetprocessdpiaware())
                {
                    WUIF::DebugPrint(TEXT("SetProcessDPIAware succeeded"));
                    _processdpiawareness = PROCESS_SYSTEM_DPI_AWARE;
                    void * val = &_processdpiawareness;
                    WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
                }
                else
                {
                    WUIF::DebugPrint(TEXT("SetProcessDPIAware failed"));
                    //retrieve is dpi aware and store
                    using PFN_IS_PROCESS_DPI_AWARE = BOOL(WINAPI*)(void);
                    PFN_IS_PROCESS_DPI_AWARE pfnisprocessdpiaware = reinterpret_cast<PFN_IS_PROCESS_DPI_AWARE>(GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "IsProcessDPIAware"));
                    WUIF::ThrowIfFalse(pfnisprocessdpiaware);
                    if (pfnisprocessdpiaware())
                    {
                        WUIF::DebugPrint(TEXT("Process is DPI Aware already"));
                        _processdpiawareness = PROCESS_SYSTEM_DPI_AWARE;
                        void * val = &_processdpiawareness;
                        WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
                    }
                    else
                    {
                        WUIF::DebugPrint(TEXT("Process is not DPI Aware already"));
                        _processdpiawareness = PROCESS_DPI_UNAWARE;
                        void * val = &_processdpiawareness;
                        WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
                    }
                }

            }
        }
        return;
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif
    }

    /*void InitResources()
    This function initializes the requested global graphics resources
    */
    void InitResources()
    {
        WUIF::DebugPrint(TEXT("Initializing Resources"));
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
        return;
    } //end InitResources


    /*void ReleaseResources()
    Releases all global graphics resources prior to exiting application
    */
    void ReleaseResources()
    {
        if (WUIF::App::libD3D12)
        {
            FreeLibrary(WUIF::App::libD3D12);
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
                    DestroyWindow(static_cast<WUIF::Window*>(*i)->hWnd);
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
    }
}


/*int APIENTRY wWinMain
Main entry point for application. Two prototypes exist - one for UNICODE and one for ASCII/MBCS
versions of the application. IFDEF will set appropriate prototype.
*/
#ifdef _UNICODE
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
#else
int APIENTRY  WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR  lpCmdLine, _In_ int nCmdShow)
#endif
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    #ifdef _UNICODE
    WUIF::DebugPrint(TEXT("Application Start - Unicode build"));
    #else
    WUIF::DebugPrint(TEXT("Application Start - MBCS build"));
    #endif

    #ifdef _MSC_VER
    /*Retrieves or modifies the state of the _crtDbgFlag flag to control the allocation behavior of
    the debug heap manager (debug version only)*/
    //get current bits
    int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    //clear the upper 16 bits and OR in the desired frequency and other flags
    tmp = (tmp & 0x0000FFFF) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF;
    //set the bits
    _CrtSetDbgFlag(tmp);
    //ensure crt memory dump is output to the debug window
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    #endif

    /*Best practice is that all applications call the process-wide SetErrorMode function with a
    parameter of SEM_FAILCRITICALERRORS at startup. This is to prevent error mode dialogs from
    hanging the application.*/
    SetErrorMode(SEM_FAILCRITICALERRORS);
    SetLastError(WE_OK);

    int retcode = 0;
    try
    {
        //throw if GFXflags is zero
        if (!(WUIF::flagexpr))
        {

            SetLastError(WE_CRITFAIL);
            throw WUIF::WUIF_exception(TEXT("Developer did not set WUIF::App:GFXflags to a value!"));
        }
        //ensure either D3D11 or D3D12 is declared for use - if neither is set throw
        if (!(WUIF::flagexpr & (WUIF::FLAGS::D3D11 | WUIF::FLAGS::D3D12)))
        {
            SetLastError(WE_CRITFAIL);
            throw WUIF::WUIF_exception(TEXT("Developer did not set a flag indicating which Direct3D version to use!"));
        }
        WUIF::changeconst(&const_cast<WUIF::FLAGS::GFX_Flags&>(WUIF::App::GFXflags), &WUIF::flagexpr);
        OSCheck();
        SetDPIAwareness();
        WUIF::changeconst(&const_cast<HINSTANCE&>(WUIF::App::hInstance), &hInstance);
        WUIF::changeconst(&const_cast<LPTSTR&>(WUIF::App::lpCmdLine), &lpCmdLine);
        WUIF::changeconst(&const_cast<int&>(WUIF::App::nCmdShow), &nCmdShow);

        InitResources();

        WUIF::App::mainWindow = CRT_NEW WUIF::Window();
        //set the cmdshow variable to the nCmdShow passed into the application
        WUIF::App::mainWindow->cmdshow(nCmdShow);
        //create application

        /*argc - An integer that contains the count of arguments that follow in argv. The argc
            parameter is always greater than or equal to 1.
          argv - an array of null-terminated strings representing command-line arguments entered by
            the user of the program. By convention, argv[0] is the command with which the program
            is invoked, argv[1] is the first command-line argument, and so on, until argv[argc],
            which is always NULL.*/
        int    argc  = 0;
        LPSTR *argv  = CommandLineToArgvA(GetCommandLineA(), &argc);

        #ifdef DEBUGOUTPUT
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

        //add a newline just in case other output messages before haven't
        OutputDebugString(TEXT(""));
        WUIF::DebugPrint(TEXT("Exited 'main'"));
        //free memory allocated to argv in CommandLineToArgvA
        HeapFree(GetProcessHeap(), NULL, argv);
        WUIF::DebugPrint(TEXT("Releasing resources"));
        ::ReleaseResources();
    }
    catch (const WUIF::WUIF_exception &e)
    {
        WUIF::DebugPrint(TEXT("WUIF exception encountered"));
        retcode = ERROR_PROCESS_ABORTED;
        try
        {
            if (WUIF::App::ExceptionHandler != nullptr)
            {
                try { WUIF::App::ExceptionHandler(); }
                catch (...) { }
            }
            ::ReleaseResources();
            ErrorExit(e.WUIFWhat());
        }
        catch (...) { }
    }
    catch (const WUIF_WNDPROC_terminate_exception)
    {
        WUIF::DebugPrint(TEXT("WUIF_WNDPROC_terminate exception encountered"));
        retcode = ERROR_PROCESS_ABORTED;
        try {
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
        WUIF::DebugPrint(TEXT("exception encountered"));
        retcode = ERROR_PROCESS_ABORTED;
        try {
            if (WUIF::App::ExceptionHandler != nullptr)
            {
                try { WUIF::App::ExceptionHandler(); }
                catch (...) {}
            }
            ::ReleaseResources();
            ErrorExit(TEXT("Critical Unhandled Exception!"));
        }
        catch (...) {}
    }
    WUIF::DebugPrint(TEXT("Terminating Applicaiton"));
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
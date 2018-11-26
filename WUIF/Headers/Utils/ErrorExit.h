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
#pragma once
#include <Windows.h>
#include <strsafe.h>         //needed for ErrorExit's StringCchPrintf
#include "WUIF_Error.h"

/*void ErrorExit(_In_ LPTSTR lpszMessage)
    Executes when an exception occurs. Displays a MessageBox with an error message and the string passed to lpszMessage.

    LPTSTR lpszMessage[in] - string to be displayed along with error message in MessageBox

    Return results - none
    */
void ErrorExit(_In_ LPTSTR lpszMessage)
{
    PrintEnter(TEXT("::ErrorExit"));
    DWORD   numchars = 0;
    LPVOID  lpMsgBuf = nullptr;
    const DWORD   err = GetLastError();

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
            case WE_INVALIDARG:
            {
                lpSource = TEXT("INVALIDARG");
            }
            break;
            case WE_WRONGOSFORAPP:
            {
                lpSource = TEXT("WRONGOSFORAPP");
            }
            break;
            case WE_THUNK_HOOK_FAIL:
            {
                lpSource = TEXT("THUNK_HOOK_FAIL");
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
        /*allocate room for: pszTxt size (pszTxtsize) + length of lpszMessage + sizeof err(DWORD) + 1 for
        negative sign + lpMsgBuf (numchars)*/
        constexpr int pszTxtsize = 118; //size of const char array of pszTxt's text below
        lpDisplayBuf = HeapAlloc(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
                                 (static_cast<size_t>(pszTxtsize) + static_cast<size_t>(lstrlen(lpszMessage))
                                  + (sizeof(DWORD) + 1) + numchars) * sizeof(TCHAR));
    }
    HRESULT hr = E_FAIL;
    if (lpDisplayBuf)
    {
        LPCTSTR pszTxt = TEXT("An unrecoverable critical error was encountered and the application is now terminating!\n\n%s\n\nReported error is %u: %s");
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
    /*LocalFree is not in the modern SDK, so it cannot be used to free the result buffer which was traditional.
    Instead, use HeapFree (GetProcessHeap(), allocatedMessage). In this case, this is the same as calling LocalFree
    on memory.*/
    if (lpDisplayBuf)
    {
        HeapFree(GetProcessHeap(), NULL, lpDisplayBuf);
    }
    if (lpMsgBuf)
    {
        HeapFree(GetProcessHeap(), NULL, lpMsgBuf);
    }
    PrintExit(TEXT("::ErrorExit"));
    return;
}
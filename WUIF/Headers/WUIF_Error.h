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
//include Windows.h before including this file

//Define declarations for debugging

#include<strsafe.h>

//WUIF ERROR CODES
/*Per winerror.h

Values are 32 bit values laid out as follows:

3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
+---+-+-+-----------------------+-------------------------------+
|Sev|C|R|     Facility          |               Code            |
+---+-+-+-----------------------+-------------------------------+

where

Sev - is the severity code

00 - Success
01 - Informational
10 - Warning
11 - Error

C - is the Customer code flag

R - is a reserved bit

Facility - is the facility code

Code - is the facility's status code

Hence:
Sev
0x2 (0010) - Customer defined Success
0x6 (0110) - Customer defined Informational
0xA (1010) - Customer defined Warning
0xE (1110) - Customer defined Error
*/

//make sure to add to ErrorExit in WUIF_Main.cpp
constexpr unsigned long WE_OK                               = 0x20000000L;
constexpr unsigned long WE_FAIL                             = 0xA0000001L;
constexpr unsigned long WE_CRITFAIL                         = 0xE000042BL;
constexpr unsigned long WE_INVALIDARG                       = 0xA0070057L;
constexpr unsigned long WE_WRONGOSFORAPP                    = 0xA00401FAL;
constexpr unsigned long WE_D3D12_NOT_FOUND                  = 0xA87E0001L;
constexpr unsigned long WE_WNDPROC_EXCEPTION                = 0xE0007574L;
constexpr unsigned long WE_GRAPHICS_INVALID_DISPLAY_ADAPTER = 0xA0262002L;

namespace WUIF {
#ifdef DEBUGOUTPUT
    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 26481) //don't use pointer arithmetic
    #endif
    inline void DebugPrint(_In_ const TCHAR *format, ...)
    {
        /*max message length for use with OutputDebugString, first DWORD bytes contain process
        identifier the other is for the message*/
        constexpr int bufsize = 4096 - sizeof(DWORD);
        TCHAR buf[bufsize] = {};
        LPTSTR buffer = buf;
        LPTSTR bufend = nullptr;
        size_t numremaining = 0;
        //C26494 - variable is uninitialized: we do not initialize a va_list
        #ifdef _MSC_VER
        #pragma warning(suppress: 26494)
        #endif
        va_list args;
        va_start(args, format);
        //write the string to the buffer, parsing the arguments
        StringCchVPrintfEx(buffer, bufsize, &bufend, &numremaining, STRSAFE_FILL_BEHIND_NULL | STRSAFE_IGNORE_NULLS, format, args);
        va_end(args);
        //check if there's room to write the new line and null termination characters
        if (numremaining < 3)
        {
            /*we need three spots at the end of the buffer to write the newline and null
            termination. If we have less than 3 (because '\0' is included in the count) we need to
            set bufend to the bufsize - 3*/
            bufend = &buf[bufsize - 3];
        }
        //add new line and null termination, overwriting existing null termination
        *bufend++ = '\r';
        *bufend++ = '\n';
        *bufend   = '\0';
        //send string to debug console
        OutputDebugString(buffer);
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif //_MSC_VER
    }
#else
    inline void DebugPrint(_In_ const TCHAR *format, ...) { UNREFERENCED_PARAMETER(format); }
#endif //DEBUGOUTPUT

    //WUIF Exception structures
    class WUIF_exception {
    private:
        LPTSTR wmsg;
    public:
        explicit WUIF_exception(_In_ const LPTSTR msg) noexcept;

        const LPTSTR WUIFWhat() const { return wmsg; };
    };
} //end namespace WUIF

#define AssertIfFailed(expr) _ASSERTE(SUCCEEDED(expr))

//#define ThrowIfFailed(expr)  _ASSERTE(SUCCEEDED(expr))
//#define ThrowIfFalse(expr)   _ASSERTE(expr)

namespace WUIF {
    //inline void AssertIfFailed(HRESULT hr) {}
    inline void ThrowIfFailed(HRESULT hr)
    {
        #ifdef _DEBUG
        _ASSERTE(hr >= 0);
        #else
        if (hr < 0) {
            SetLastError(hr);
            throw WUIF_exception(TEXT("Critical Failure"));
        }
        #endif
    }
    inline void ThrowIfFalse(bool expr)
    {
        #ifdef _DEBUG
        _ASSERTE(expr);
        #else
        if (expr == false) {
            SetLastError(WE_CRITFAIL);
            throw WUIF_exception(TEXT("Critical Failure"));
        }
        #endif
    }
}
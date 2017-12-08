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

#include <stdarg.h>  //needed for _vstprintf_s
#ifdef _DEBUG
//_CRTDBG_MAP_ALLOC define is needed for debug heap manager and evaluating "new" and should come
//before including crtdbg.h
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>  //crtdbg.h is needed for _ASSERTE and for map_alloc
#endif _DEBUG
#ifndef _UNICODE
#include <stdio.h>
#endif //_UNICODE
#include <stdlib.h>

namespace WUIF {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 26481) //don't use pointer arithmetic
#endif
    inline void DebugPrint(const TCHAR *format, ...)
    {
        constexpr int bufsize = 4096 - sizeof(DWORD); //max message length for use with OutputDebugString, first DWORD bytes contain process identifier the other is for the message
        TCHAR buf[bufsize] = {};
        TCHAR *output = &buf[0];
#pragma warning(suppress: 26494) //variable is uninitialized, we do not initialize a va_list
        va_list args;
        va_start(args, format);
        const int n = _vstprintf_s(&buf[0], bufsize - 3, format, args); //buf-3 is room for CR/LF/NUL
        va_end(args);
        output += (n < 0) ? bufsize - 3 : n; //set output to the last character, n is negative if _vstprintf_s fails so set to end of max buf size
        while ((output > &buf[0]) && (isspace(output[-1]))) //remove line terminations (and consequently trailing whitespace)
            *--output = '\0';
        *output++ = '\r';
        *output++ = '\n';
        *output = '\0';
        OutputDebugString(&buf[0]);
#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER
    }

    //WUIF Exception structures
    class WUIF_exception {
    private:
        LPTSTR wmsg;
    public:
        explicit WUIF_exception(_In_ const LPTSTR msg) throw();

        const LPTSTR WUIFWhat() const { return wmsg; };
    };
} //end namespace WUIF
#ifdef _DEBUG
    //DBG_NEW is the definition for the debug heap manager and evaluating "new"
    #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
    #define ThrowIfFalse(expr)   _ASSERTE(expr)
    #define ThrowIfFailed(expr)  _ASSERTE(SUCCEEDED(expr))
    #define AssertIfFailed(expr) _ASSERTE(SUCCEEDED(expr))

#else //define declarations for release build
    #define DBG_NEW new

namespace WUIF {
    inline void AssertIfFailed(HRESULT hr) {}
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (hr < 0) {
            SetLastError(hr); throw WUIF_exception(_T("Critical Failure"));
        }
    }
    inline void ThrowIfFalse(bool expr)
    {
        if (expr == false) {
            throw WUIF_exception(_T("Critical Failure"));
        }
    }
}
#endif //_DEBUG

#include <winerror.h>
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

const unsigned long WE_OK                = 0x20000000L;
const unsigned long WE_FAIL              = 0xA0000001L;
const unsigned long WE_CRITFAIL          = 0xE000042BL;
const unsigned long WE_INVALIDARG        = 0xA0070057L;
const unsigned long WE_D3D12_NOT_FOUND   = 0xA87E0001L;
const unsigned long WE_WNDPROC_EXCEPTION = 0xE0007574L;
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
//#include <strsafe.h>
#include <windows.h>
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <strsafe.h>
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif  // _DEBUG
//#include <stdexcept>
//#include <winerror.h>

class WUIF_exception {
private:
	LPTSTR wmsg;
public:
	explicit WUIF_exception(const LPTSTR msg) throw();

	const LPTSTR WUIFWhat() const { return wmsg; };
};


#ifdef _DEBUG //declarations for use with debugging
#define ASSERT(expr) _ASSERTE(expr)
#define AssertIfFailed(expr) ASSERT(SUCCEEDED(expr))
#define ThrowIfFailed(expr)  ASSERT(SUCCEEDED(expr))

//debug output function
inline void TRACE(TCHAR const * const format, ...)
{
	va_list args;
	va_start(args, format);
	TCHAR output[512];
	vswprintf_s(output, format, args);
	OutputDebugString(output);
	va_end(args);
}
#else //define declarations for release
#define ASSERT(expr) (expr)
#define VERIFY(expr) (expr)
#define TRACE __noop

inline void ThrowIfFailed(HRESULT hr)
{
	if (hr < 0) {
		SetLastError(hr); throw WUIF_exception(_T("Critical Failure"));
	}
}
#endif //declarations for use with debugging

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
2 (0010) - Customer defined Success
6 (0110) - Customer defined Informational
A (1010) - Customer defined Warning
E (1110) - Customer defined Error
*/
/*
typedef long WUF_ERR;
#define WUF_ERR_TYPEDEF(_sc) ((WUF_ERR)_sc)
#define WE_OK              WUF_ERR_TYPEDEF(0x00000000L)
#define WE_FAIL		       WUF_ERR_TYPEDEF(0x66600000L)
#define WE_INVALIDARG      WUF_ERR_TYPEDEF(0x66600001L)
#define WE_D3D12_NOT_FOUND WUF_ERR_TYPEDEF(0xA87E0001L)
*/
const long WE_OK = 0x20000000L;
const long WE_FAIL = 0xA0000001L;
const long WE_CRITFAIL = 0xE000042BL;
const long WE_INVALIDARG = 0xA0070057L;
const long WE_D3D12_NOT_FOUND = 0xA87E0001L;
const long WE_WNDPROC_EXCEPTION = 0xE0007574L;


struct WUIF_terminate_exception {};
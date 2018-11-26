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
#include <type_traits>
#include <windows.h>
#include <d3d12.h>
#include "WUIF_Const.h"

namespace WUIF
{
    namespace App { extern const volatile OSVersion  winversion; }

    class ProcPtr
    {
    public:
        explicit ProcPtr(_In_opt_ FARPROC ptr) : _ptr(ptr) {}

        template <typename T, typename = std::enable_if_t<std::is_function_v<T>>>
        operator T *() const
        {
            return reinterpret_cast<T *>(_ptr);
        }

    private:
        FARPROC _ptr;
    };

    class DllHelper
    {
    public:
        explicit DllHelper(_In_ LPCTSTR filename, _In_ OSVersion minver) :
            _module(LoadLibraryEx(filename, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
        {
            if ((App::winversion >= minver) && (!_module))
            {
                SetLastError(ERROR_DLL_NOT_FOUND);
                LPCTSTR pszFormat = TEXT("%s %s");
                TCHAR* pszTxt = TEXT("Failed to load library");
                TCHAR pszDest[MAXCHAR];
                StringCchPrintf(pszDest, MAXCHAR, pszFormat, pszTxt, filename);
                throw WUIF::WUIF_exception(pszDest);
            }
        }

        ~DllHelper() { FreeLibrary(_module); }

        ProcPtr assign(_In_ LPCSTR proc_name, _In_ OSVersion minver) const
        {
            FARPROC _procptr = GetProcAddress(_module, proc_name);
            if ((App::winversion >= minver) && (!_procptr))
            {
                SetLastError(ERROR_PROC_NOT_FOUND);
                LPCTSTR pszFormat = TEXT("%s %s");
                TCHAR* pszTxt = TEXT("Failed to find procedure");
                TCHAR pszDest[MAXCHAR];
                StringCchPrintf(pszDest, MAXCHAR, pszFormat, pszTxt, proc_name);
                throw WUIF::WUIF_exception(pszDest);
            }
            return ProcPtr(_procptr);
        }

        inline HMODULE module() const { return _module; } //return the handle received from LoadLibrary

    private:
        HMODULE   _module;
    };

    class ModuleHelper
    {
    public:
        explicit ModuleHelper(_In_ LPCTSTR filename, _In_ OSVersion minver) :
            _module(GetModuleHandle(filename))
        {
            if ((App::winversion >= minver) && (!_module))
            {
                SetLastError(ERROR_DLL_NOT_FOUND);
                LPCTSTR pszFormat = TEXT("%s %s");
                TCHAR* pszTxt = TEXT("Failed to get handle for library");
                TCHAR pszDest[MAXCHAR];
                StringCchPrintf(pszDest, MAXCHAR, pszFormat, pszTxt, filename);
                throw WUIF::WUIF_exception(pszDest);
            }
        }

        ProcPtr assign(_In_ LPCSTR proc_name, _In_ OSVersion minver) const
        {
            FARPROC _procptr = GetProcAddress(_module, proc_name);
            if ((App::winversion >= minver) && (!_procptr))
            {
                SetLastError(ERROR_PROC_NOT_FOUND);
                LPCTSTR pszFormat = TEXT("%s %s");
                TCHAR* pszTxt = TEXT("Failed to find procedure");
                TCHAR pszDest[MAXCHAR];
                StringCchPrintf(pszDest, MAXCHAR, pszFormat, pszTxt, proc_name);
                throw WUIF::WUIF_exception(pszDest);
            }
            return ProcPtr(_procptr);
        }

        inline HMODULE module() const { return _module; } //return the handle received from LoadLibrary

    private:
        HMODULE   _module;
    };
}
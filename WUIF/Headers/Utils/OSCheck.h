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
#include <strsafe.h>     //needed for StringCchPrintf
#include "WUIF_Const.h"
#include "WUIF_Error.h"
#include "Application/Application.h"

namespace {
    /*void OSCheck()
    Performs a check for the Windows OS Version and sets WUIF::App::winversion and loads D3D12.dll if needed

    Return results - none
    */
    void OSCheck(_In_ WUIF::OSVersion minversion)
    {
        PrintEnter(TEXT("::OSCheck"));
        WUIF::OSVersion osver = WUIF::OSVersion::WIN7;
        //determine which Win10 version
        HMODULE lib = GetModuleHandle(TEXT("kernel32.dll"));
        if (!lib)
        {
            SetLastError(ERROR_DLL_NOT_FOUND);
            throw WUIF::WUIF_exception(TEXT("Failed to get handle for kernel32.dll"));
        }
        //check for Win10 1809 October Update
        if (GetProcAddress(lib, "ClosePseudoConsole"))
        {
            WUIF::DebugPrint(TEXT("Windows 10 OS version is %i"), 1809);
            osver = WUIF::OSVersion::WIN10_1809;
        }
        else
        {
            //check for Win10 1803 April Update
            if (GetProcAddress(lib, "CopyFileFromAppW"))
            {
                WUIF::DebugPrint(TEXT("Windows 10 OS version is %i"), 1803);
                osver = WUIF::OSVersion::WIN10_1803;
            }
            else
            {
                //check for Win10 1709 Fall Creators Update
                if (GetProcAddress(lib, "GetUserDefaultGeoName"))
                {
                    WUIF::DebugPrint(TEXT("Windows 10 OS version is %i"), 1709);
                    osver = WUIF::OSVersion::WIN10_1709;
                }
                else
                {
                    //check for Win10 1703 Creators Update
                    if (GetProcAddress(lib, "WerRegisterCustomMetadata"))
                    {
                        WUIF::DebugPrint(TEXT("Windows 10 OS version is %i"), 1703);
                        osver = WUIF::OSVersion::WIN10_1703;
                    }
                    else
                    {
                        //check for Win10 1607 Anniversary Update
                        if (GetProcAddress(lib, "SetThreadDescription"))
                        {
                            WUIF::DebugPrint(TEXT("Windows 10 OS version is %i"), 1607);
                            osver = WUIF::OSVersion::WIN10_1607;
                        }
                        else
                        {
                            //the following two procedures are in kernelbase.dll not kernel32.dll as MS states
                            HMODULE kernelbaselib = GetModuleHandle(TEXT("kernelbase.dll"));
                            if (!kernelbaselib)
                            {
                                SetLastError(ERROR_DLL_NOT_FOUND);
                                throw WUIF::WUIF_exception(TEXT("Failed to get handle for kernelbase.dll"));
                            }
                            //check for Win10 1511 November Update
                            if (GetProcAddress(kernelbaselib, "GetSystemWow64Directory2W"))
                            {
                                WUIF::DebugPrint(TEXT("Windows 10 OS version is %i"), 1511);
                                osver = WUIF::OSVersion::WIN10_1511;
                            }
                            else
                            {
                                //check for Win10 initial release
                                if (GetProcAddress(kernelbaselib, "GetIntegratedDisplaySize"))
                                {
                                    WUIF::DebugPrint(TEXT("Windows 10 OS version is initial release"));
                                    osver = WUIF::OSVersion::WIN10;
                                }
                                else
                                {
                                    //check for Win 8.1
                                    if (GetProcAddress(lib, "QueryProtectedPolicy"))
                                    {
                                        WUIF::DebugPrint(TEXT("Windows 8.1 detected"));
                                        osver = WUIF::OSVersion::WIN8_1;
                                    }
                                    else
                                    {
                                        //check for Win 8
                                        if (GetProcAddress(lib, "CreateFile2"))
                                        {
                                            WUIF::DebugPrint(TEXT("Windows 8 detected"));
                                            osver = WUIF::OSVersion::WIN8;
                                        }
                                        else
                                        {
                                            WUIF::DebugPrint(TEXT("Windows 7 detected"));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (osver < minversion)
        {
            /*Application is attempting to run on an Windows OS version that does not meat the minimum requirement, throw an error with
            the required minimum version in the message.*/
            SetLastError(WE_WRONGOSFORAPP);
            TCHAR* minTxt = nullptr;
            switch (minversion)
            {
                case WUIF::OSVersion::WIN7:
                {
                    minTxt = TEXT("7");
                }
                case WUIF::OSVersion::WIN8:
                {
                    minTxt = TEXT("8");
                }
                break;
                case WUIF::OSVersion::WIN8_1:
                {
                    minTxt = TEXT("8.1");
                }
                break;
                case WUIF::OSVersion::WIN10:
                {
                    minTxt = TEXT("10");
                }
                break;
                case WUIF::OSVersion::WIN10_1511:
                {
                    minTxt = TEXT("10_1511 November Update");
                }
                break;
                case WUIF::OSVersion::WIN10_1607:
                {
                    minTxt = TEXT("10_1607 Anniversary Update");
                }
                break;
                case WUIF::OSVersion::WIN10_1703:
                {
                    minTxt = TEXT("10_1703 Creators Update");
                 }
                break;
                case WUIF::OSVersion::WIN10_1709:
                {
                    minTxt = TEXT("10_1709 Fall Creators Update");
                 }
                break;
                case WUIF::OSVersion::WIN10_1803:
                {
                    minTxt = TEXT("10_1803 April 2018 Update");
                }
                break;
                case WUIF::OSVersion::WIN10_1809:
                {
                    minTxt = TEXT("10_1809 October 2018 Update");
                }
            }
            LPCTSTR pszFormat = TEXT("%s %s");
            TCHAR* pszTxt = TEXT("This application does not meet the minimum Windows OS version requirement of:");
            size_t cchDest = 78 + 29;
            TCHAR pszDest[78 + 29];
            StringCchPrintf(pszDest, cchDest, pszFormat, pszTxt, minTxt);
            throw WUIF::WUIF_exception(pszDest);
        }
        if ((osver >= WUIF::OSVersion::WIN10) && (WUIF::App::GFXflags & WUIF::FLAGS::D3D12))
        {
            WUIF::d3d12libAPI = new WUIF::D3D12libAPI;
        }
        else
        {
            if ((WUIF::App::GFXflags & WUIF::FLAGS::D3D12) && (!(WUIF::App::GFXflags & WUIF::FLAGS::D3D11))) //D3D12 only flag setting
            {
                //if we are D3D12 only and not on Win 10 must throw an error
                SetLastError(WE_D3D12_NOT_FOUND);
                throw WUIF::WUIF_exception(TEXT("This application requires Direct3D 12, D3D12.dll not found!"));
            }
            //Remove D3D12 flag if set, as D3D12 isn't available.
            //NB: it's quicker to call changeconst than do a "if d3d12 is set" type of expression
            const WUIF::FLAGS::GFX_Flags flagtempval = (WUIF::App::GFXflags & (~WUIF::FLAGS::D3D12));
            WUIF::changeconst(&const_cast<WUIF::FLAGS::GFX_Flags&>(WUIF::App::GFXflags), &flagtempval);
        }
        //update osver with the determined version of the running Windows OS
        WUIF::changeconst(&const_cast<WUIF::OSVersion&>(WUIF::App::winversion), &osver);
        PrintExit(TEXT("::OSCheck"));
        return;
    } //end OSCheck
}
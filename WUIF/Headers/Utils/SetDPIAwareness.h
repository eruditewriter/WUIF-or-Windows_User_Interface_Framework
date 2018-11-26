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
#include <ShellScalingApi.h> //needed for PROCESS_DPI_AWARENESS, PROCESS_PER_MONITOR_DPI_AWARE, MONITOR_DPI_TYPE, MDT_EFFECTIVE_DPI
#include "WUIF_Const.h"
#include "WUIF_Error.h"
#include "Utils/dllhelper.h"
#include "Application/Application.h"

namespace
{
    //DPI Awareness typedefs needed since we are compiling for Win7 and up compatibility they are defined out in the .H files
                                     STDAPI GetProcessDpiAwareness             (HANDLE, PROCESS_DPI_AWARENESS*);
                                     STDAPI SetProcessDpiAwareness             (PROCESS_DPI_AWARENESS);
    WINUSERAPI BOOL                  WINAPI IsValidDpiAwarenessContext         (DPI_AWARENESS_CONTEXT);
    WINUSERAPI BOOL                  WINAPI SetProcessDpiAwarenessContext      (DPI_AWARENESS_CONTEXT);
    WINUSERAPI DPI_AWARENESS         WINAPI GetAwarenessFromDpiAwarenessContext(DPI_AWARENESS_CONTEXT);
    WINUSERAPI BOOL                  WINAPI IsProcessDPIAware                  (VOID);
    WINUSERAPI BOOL                  WINAPI SetProcessDPIAware                 (VOID);
    WINUSERAPI DPI_AWARENESS_CONTEXT WINAPI GetThreadDpiAwarenessContext       (VOID);
    //WINUSERAPI BOOL                  WINAPI AreDpiAwarenessContextsEqual(DPI_AWARENESS_CONTEXT value, DPI_AWARENESS_CONTEXT value2);

    class HighDPIAPI
    {
        WUIF::ModuleHelper _user32dll{ TEXT("user32.dll"), WUIF::OSVersion::WIN7 };
        WUIF::DllHelper _shcoredll{ TEXT("shcore.dll"), WUIF::OSVersion::WIN8 }; //not available in Windows 7
    public:
        decltype(SetProcessDpiAwareness)              *SetProcessDpiAwareness              = _shcoredll.assign("SetProcessDpiAwareness", WUIF::OSVersion::WIN8_1);
        decltype(GetProcessDpiAwareness)              *GetProcessDpiAwareness              = _shcoredll.assign("GetProcessDpiAwareness", WUIF::OSVersion::WIN8_1);
        decltype(IsProcessDPIAware)                   *IsProcessDPIAware                   = _user32dll.assign("IsProcessDPIAware", WUIF::OSVersion::WIN7);
        decltype(SetProcessDPIAware)                  *SetProcessDPIAware                  = _user32dll.assign("SetProcessDPIAware", WUIF::OSVersion::WIN7);
        decltype(IsValidDpiAwarenessContext)          *IsValidDpiAwarenessContext          = _user32dll.assign("IsValidDpiAwarenessContext", WUIF::OSVersion::WIN10_1607);
        decltype(GetThreadDpiAwarenessContext)        *GetThreadDpiAwarenessContext        = _user32dll.assign("GetThreadDpiAwarenessContext", WUIF::OSVersion::WIN10_1607);
        decltype(SetProcessDpiAwarenessContext)       *SetProcessDpiAwarenessContext       = _user32dll.assign("SetProcessDpiAwarenessContext", WUIF::OSVersion::WIN10_1703);
        decltype(GetAwarenessFromDpiAwarenessContext) *GetAwarenessFromDpiAwarenessContext = _user32dll.assign("GetAwarenessFromDpiAwarenessContext", WUIF::OSVersion::WIN10_1607);
    };

    /*void SetDPIAwareness()
        Sets the DPI Awareness by the proper method for the OS Version. Attempts to set the highest dpi awareness Sets
        App::processdpiawareness and App::processdpiawarenesscontext (will be nullptr if not Win10 or supported). Using a manifest to set
        DPI awareness will supersede this function.*/
    void SetDPIAwareness()
    {
        PrintEnter(TEXT("::SetDPIAwareness"));
        //internal pointers for App::processdpiawareness and App::processdpiawarenesscontext
        PROCESS_DPI_AWARENESS _processdpiawareness = PROCESS_DPI_UNAWARE;
        DPI_AWARENESS_CONTEXT _processdpiawarenesscontext = DPI_AWARENESS_CONTEXT_UNAWARE;
        HighDPIAPI highDPIAPI;
        //get the current process wide DPI Awareness
        if (WUIF::App::winversion >= WUIF::OSVersion::WIN8_1)
        {
            HRESULT hr = highDPIAPI.GetProcessDpiAwareness(NULL, &_processdpiawareness);
            if (hr == S_OK)
            {
                //a return value of DPI_AWARENESS_UNAWARE could mean that Process DPI Awareness has not been set in manifest
                if (_processdpiawareness == DPI_AWARENESS_UNAWARE)
                {
                    if (WUIF::App::winversion >= WUIF::OSVersion::WIN10_1607)
                    {
                        WUIF::DebugPrint(TEXT("Windows Version >= 10.1607 using SetProcessDpiAwarenessContext"));
                        if (highDPIAPI.IsValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
                        {
                            WUIF::DebugPrint(TEXT("DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 is valid"));
                            if (highDPIAPI.SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
                            {
                                _processdpiawareness = PROCESS_PER_MONITOR_DPI_AWARE;
                                _processdpiawarenesscontext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
                                void *val = &_processdpiawareness;
                                WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
                                val = &_processdpiawarenesscontext;
                                WUIF::changeconst(&const_cast<DPI_AWARENESS_CONTEXT*&>(WUIF::App::processdpiawarenesscontext), &val);
                                WUIF::DebugPrint(TEXT("SetProcessDpiAwarenessContext successful with DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2"));
                            }
                            else
                            {
                                WUIF::DebugPrint(TEXT("SetProcessDpiAwarenessContext failed with %u"), GetLastError());
                            }
                        }
                        else
                        {
                            WUIF::DebugPrint(TEXT("DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE should be valid"));
                            if (highDPIAPI.SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
                            {
                                _processdpiawareness = PROCESS_PER_MONITOR_DPI_AWARE;
                                _processdpiawarenesscontext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
                                void *val = &_processdpiawareness;
                                WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
                                val = &_processdpiawarenesscontext;
                                WUIF::changeconst(&const_cast<DPI_AWARENESS_CONTEXT*&>(WUIF::App::processdpiawarenesscontext), &val);
                                WUIF::DebugPrint(TEXT("SetProcessDpiAwarenessContext successful with DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE"));
                            }
                            else
                            {
                                WUIF::DebugPrint(TEXT("SetProcessDpiAwarenessContext failed with %u"), GetLastError());
                            }
                        }

                    }
                    else
                    {
                        WUIF::DebugPrint(TEXT("Windows Version between 8.1 and 10.1607 using SetProcessDpiAwareness"));
                        //set dpi awareness for Win8.1
                        hr = highDPIAPI.SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
                        if (hr == S_OK)
                        {
                            _processdpiawareness = PROCESS_PER_MONITOR_DPI_AWARE;
                            void * val = &_processdpiawareness;
                            WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
                            WUIF::DebugPrint(TEXT("SetProcessDpiAwareness used with PROCESS_PER_MONITOR_DPI_AWARE"));
                        }
                    }
                }
                else
                {
                    void * val = &_processdpiawareness;
                    WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
                    if (WUIF::App::winversion >= WUIF::OSVersion::WIN10_1607)
                    {
                        _processdpiawarenesscontext = highDPIAPI.GetThreadDpiAwarenessContext();
                        val = &_processdpiawarenesscontext;
                        WUIF::changeconst(&const_cast<DPI_AWARENESS_CONTEXT*&>(WUIF::App::processdpiawarenesscontext), &val);
                    }
                    WUIF::DebugPrint(TEXT("DPI Awareness already set. Setting values to process context."));
                }
            }
            else
            {
                WUIF::DebugPrint(TEXT("Unable to get current DPI Awareness."));
            }
        }
        else
        {
            /*use Vista/Win7/Win8 SetProcessDPIAware and IsProcessDPIAware - these are not guaranteed to be in later operating systems so
            we shouldn't link statically*/
            if (highDPIAPI.IsProcessDPIAware())
            {
                WUIF::DebugPrint(TEXT("Process is DPI Aware already"));
                _processdpiawareness = PROCESS_SYSTEM_DPI_AWARE;
                void * val = &_processdpiawareness;
                WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
            }
            else
            {
                if (highDPIAPI.SetProcessDPIAware())
                {
                    WUIF::DebugPrint(TEXT("SetProcessDPIAware succeeded"));
                    _processdpiawareness = PROCESS_SYSTEM_DPI_AWARE;
                    void * val = &_processdpiawareness;
                    WUIF::changeconst(&const_cast<PROCESS_DPI_AWARENESS*&>(WUIF::App::processdpiawareness), &val);
                }
                else
                {
                    WUIF::DebugPrint(TEXT("Process is not DPI Aware already"));
                }
            }
        }
        PrintExit(TEXT("::SetDPIAwareness"));
        return;
    }
}
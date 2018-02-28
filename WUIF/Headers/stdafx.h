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

#include <WinSDKVer.h>
//Win7 Platform Update were included in Win8 SDK headers, so minimum version is Win8 (0x0602)
#define _WIN7_PLATFORM_UPDATE
#define _WIN32_WINNT  0x0602
#define WINVER        0x0602
#define NTDDI_VERSION 0x06020000
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN  //Exclude rarely-used stuff from Windows headers
#define NOMINMAX   		     //Macros min(a,b) and max(a,b)
#define NOMCX      		     //Modem Configuration Extensions
#define NOSERVICE  		     //All Service Controller routines, SERVICE_ equates, etc.
#define NOHELP 	   	         //Help engine interface.
#define NODRAWTEXT 		     //DrawText() and DT_*
#define NOCOMM 	   	         //COMM driver routines
#define STRICT
#include <windows.h>

/*_CRTDBG_MAP_ALLOC define is needed for debug heap manager and evaluating "new" and should come
before including crtdbg.h*/
#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>  //crtdbg.h is needed for _ASSERTE and for map_alloc
#ifdef _DEBUG
//CRT_NEW is the definition for the debug heap manager and evaluating "new"
#define CRT_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define CRT_NEW new
#endif //_DEBUG
#endif //_MSC_VER
#include <mutex>
#include "WUIF_Error.h"
#define WINVECLOCK   std::unique_lock<std::mutex> guard(WUIF::App::veclock);\
                     WUIF::App::vecready.wait(guard, WUIF::App::is_vecwritable);\
                     WUIF::App::vecwrite = false;
#define WINVECUNLOCK WUIF::App::vecwrite = true;\
                     guard.unlock();\
                     WUIF::App::vecready.notify_one();
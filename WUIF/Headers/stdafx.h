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
#define _WIN7_PLATFORM_UPDATE //Win7 Platform Update were included in Win8 SDK headers, so minimum version is Win8 (0x0602)
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
#include <tchar.h>
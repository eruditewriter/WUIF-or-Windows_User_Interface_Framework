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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#pragma once

#include "targetver.h"

// Windows Header Files:
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
#if defined(_MSC_VER) && defined(_DEBUG)
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>                                         //crtdbg.h is needed for _ASSERTE and for map_alloc
    #define CRT_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ ) //CRT_NEW is the definition for the debug heap manager and evaluating "new"
#else
    #define CRT_NEW new
#endif //_MSC_VER && _DEBUG
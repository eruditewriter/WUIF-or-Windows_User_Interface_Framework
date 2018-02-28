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
#ifdef _MSC_VER
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>  //crtdbg.h is needed for _ASSERTE and for map_alloc
    #ifdef _DEBUG
        //CRT_NEW is the definition for the debug heap manager and evaluating "new"
        #define CRT_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
        #else
        #define DBG_NEW new
    #endif //_DEBUG
#endif //_MSC_VER
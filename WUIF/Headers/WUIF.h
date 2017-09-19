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
// WUIF.h : Windows UI Framework general header file
//
#pragma once
#define NOMINMAX   		     //Macros min(a,b) and max(a,b)
#define NOMCX      		     //Modem Configuration Extensions
#define NOSERVICE  		     //All Service Controller routines, SERVICE_ equates, etc.
#define NOHELP 	   	         //Help engine interface.
#define NODRAWTEXT 		     //DrawText() and DT_*
#define NOCOMM 	   	         //COMM driver routines
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#ifdef _DEBUG
#define D3DCOMPILE_DEBUG 1   //tell the HLSL compiler to include debug information into the shader blob
//#define REFDRIVER          //uncomment to create a D3D 11 reference driver
#endif

#include <windows.h>
#include <tchar.h> //for _T macro
#include <mutex>
#include "WUIF_Error.h"
#include "BitField.h"



//#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")

namespace WUIF {

	namespace FLAGS {
		BIT_FIELD(1, long) WUIF_Flags;
		BIT_CONST(WUIF_Flags, WARP, 1);
		BIT_CONST(WUIF_Flags, D2D, 2);
		BIT_CONST(WUIF_Flags, D3D11, 3);
		BIT_CONST(WUIF_Flags, D3D12, 4);
	}
	//WIN10_1511 - November Update
	//WIN10_1607 - Anniversary Update
	//WIN10_1703 - Creators Update
	//WIN10_1709 - Fall Creators Update
	//WIN10_1803 - ???
	enum class OSVersion { WIN7, WIN8, WIN8_1, WIN10, WIN10_1511, WIN10_1607, WIN10_1703, WIN_10_1709, WIN10_1803 };

	//interface
	struct IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	class Application;
	extern Application *App;

	int Run(int);
}

extern int AppMain(int argc, LPWSTR *argv);
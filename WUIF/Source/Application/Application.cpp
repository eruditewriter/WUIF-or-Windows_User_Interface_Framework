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
#include "WUIF.h"
#include <VersionHelpers.h>  //needed for IsWindows8OrGreater definition
#include <wrl/client.h> //needed for ComPtr
#include "DirectX\DirectX.h"
#include "Application\Application.h"
#include "Window\Window.h"



using namespace WUIF;

Application::Application(const HINSTANCE &inst, const LPWSTR &cmdline, const int &cmdshow, const OSVersion &winver) :
	ExceptionHandler(nullptr),
	hInstance(&inst),
	lpCmdLine(&cmdline),
	nCmdShow(&cmdshow),
	winversion(&winver),
	mainWindow(nullptr),
	Windows(nullptr)
{
	mainWindow = new Window(this);
	Windows = mainWindow;
	//set the cmdshow variable to the nCmdShow passed into the application
	mainWindow->props.cmdshow(*nCmdShow);
}


Application::~Application()
{
	if (Windows != nullptr) //Windows is "nullptr" if all windows have been closed
	{
		Window *nextwin = nullptr;
		while (Windows != nullptr)
		{
			nextwin = Windows->Next;
			try {
				if (Windows = mainWindow)
				{
					mainWindow = nullptr;
				}
				delete Windows; //if there's a problem with the delete catch the exception and just continue. obviously a memory leak, but if we are in the destructor application is shutting down anyway
				Windows = nullptr;
			}
			catch (...) {
				ASSERT(false);
			}
			Windows = nextwin;
			nextwin = nullptr;
		}
	}
}
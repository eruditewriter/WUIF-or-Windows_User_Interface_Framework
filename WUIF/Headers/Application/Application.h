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
#include <shellscalingapi.h>
#include <mutex>

namespace WUIF {

	class Window;
	class DXResources;

	class Application
	{
	public:
		Application(const HINSTANCE &inst, const LPWSTR &cmdline, const int &cmdshow, const WUIF::OSVersion &winver);
		~Application();

		void(*ExceptionHandler)();

		const HINSTANCE             * const hInstance;
		const LPWSTR                * const lpCmdLine;
		const int                   * const nCmdShow;
		const OSVersion             * const winversion;  //which version of Windows OS app is running on

		static const WUIF::FLAGS::WUIF_Flags flags;

		Window *mainWindow;
		Window *Windows;

		//std::mutex paintmutex;

		//functions
	};



}
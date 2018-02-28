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
#include "stdafx.h"
#include "Application\Application.h"
#include "Window\Window.h"

using namespace WUIF;

//const int        App::nCmdShow        = NULL;
//const LPTSTR     App::lpCmdLine       = nullptr;
//const HINSTANCE  App::hInstance       = NULL;
//const OSVersion  App::winversion      = OSVersion::WIN7;
//const FLAGS::GFX_Flags  App::GFXflags = FLAGS::D3D11;



Window    *App::mainWindow = nullptr;
HINSTANCE  App::libD3D12   = NULL;
void(*App::ExceptionHandler)(void) = nullptr;
//std::vector<Window*> App::Windows;
std::unordered_map<int, WNDPROC> App::GWndProc_map;
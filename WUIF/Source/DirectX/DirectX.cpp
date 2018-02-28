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
#include "WUIF_Main.h"
#include "WUIF_Error.h"
#include "DirectX\DirectX.h"
#include "Application\Application.h"
#include "Window\Window.h"

using namespace WUIF;

DXResources::DXResources(Window *win) :
    useD3D12(false),
	DXGIres(this),
	d3d12res(this),
	d3d11res(this),
	d2dres(this),
	winparent(win),
    deviceNotify(nullptr)
{

}

void DXResources::HandleDeviceLost()
{
	//App::paintmutex.lock();
#if defined(_DEBUG)
	HRESULT reason = d3d11res.d3d11Device1->GetDeviceRemovedReason();
	wchar_t outString[100];
	size_t size = 100;
	swprintf_s(outString, size, L"Device removed! DXGI_ERROR code: 0x%X\n", reason);
	OutputDebugStringW(outString);
	DXGIres.dxgiInfoQueue.Reset();
	DXGIres.dxgiDebug.Reset();
	d3d11res.d3dInfoQueue.Reset();
	d3d11res.d3dDebug.Reset();
#endif
	DXGIres.dxgiSwapChain1 = nullptr;
	DXGIres.dxgiDevice.Reset();
	d3d11res.d3d11ImmediateContext.Reset();
	d3d11res.d3d11Device1.Reset();
	d3d12res.d3d11on12Device.Reset();
	d3d12res.d3dCommQueue.Reset();
	d3d12res.d3d12Device.Reset();

	if (deviceNotify != nullptr)
	{
		// Notify the renderers that device resources need to be released.
		// This ensures all references to the existing swap chain are released so that a new one can be created.
		deviceNotify->OnDeviceLost();
	}

	if (useD3D12)
	{
		//create D3D12 device
		WUIF::D3D12Resources::CreateStaticResources();
	}
	else
	{
		//create D3D11 device
		WUIF::D3D11Resources::CreateStaticResources();
	}
	if (App::GFXflags & WUIF::FLAGS::D2D)
	{
		WUIF::D2DResources::CreateStaticResources();
	}
	//recreate the swap chain and resources for each window
	Window *temp = App::Windows;
	do {
		temp->DXres->DXGIres.CreateSwapChain();
		if (useD3D12)
		{
			//temp->DXres->d3d12res.CreateDeviceResources();
		}
		else
		{
			temp->DXres->d3d11res.CreateDeviceResources();
		}
		if (App::GFXflags & WUIF::FLAGS::D2D)
		{
			temp->DXres->d2dres.CreateDeviceResources();
		}
		temp = App::Windows->Next;
	} while (App::Windows->Next != nullptr);
	if (deviceNotify != nullptr)
	{
		// Notify the renderers that resources can now be created again
		deviceNotify->OnDeviceRestored();
	}
	//App::paintmutex.unlock();
}
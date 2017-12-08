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
#include "GFX\GFX.h"
#include "GFX\D2D\D2D.h"
#include "GFX\DXGI\DXGI.h"
#include "Application\Application.h"
#include "Window\Window.h"

using namespace WUIF;
using namespace Microsoft::WRL;

ComPtr<ID2D1Factory1>       D2DResources::d2dFactory = nullptr;
ComPtr<ID2D1Device>         D2DResources::d2dDevice  = nullptr;
ComPtr<IDWriteFactory1>     D2DResources::dwFactory  = nullptr;
ComPtr<IWICImagingFactory2> D2DResources::wicFactory = nullptr;

D2DResources::D2DResources(Window &win) :
	d2dRenderTarget(nullptr),
	d2dDeviceContext(nullptr),
	window(win)
{

}

D2DResources::~D2DResources()
{
	ReleaseNonStaticResources();
}

void D2DResources::CreateStaticResources()
{
	D2D1_FACTORY_OPTIONS fo = {};
	//ZeroMemory(&fo, sizeof(D2D1_FACTORY_OPTIONS));
#ifdef _DEBUG // for debug build, enable debugging via SDK Layers with factory options debugLevel flag
	fo.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	//Initialize the D2D Factory
	ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, //threading model; default:D2D1_FACTORY_TYPE_SINGLE_THREADED
		__uuidof(ID2D1Factory),			                               //reference to the IID of d2dFactory
		&fo,							                               //factory options (used for debugging)
		&d2dFactory));					                               //returns the factory

	ThrowIfFailed(d2dFactory->CreateDevice(DXGIResources::dxgiDevice.Get(), &d2dDevice));

	//Initialize the DirectWrite Factory
	ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, //whether the factory will be shared or isolated; default: DWRITE_FACTORY_TYPE_SHARED
		__uuidof(IDWriteFactory1),			                      //reference to the IID of dwFactory
		&dwFactory));					                          //returns the factory

	ThrowIfFailed(CoCreateInstance(
		CLSID_WICImagingFactory2,	 //CLSID associated with the data and code that will be used to create the object
		nullptr,					 //If NULL, indicates that the object is not being created as part of an aggregate
		CLSCTX_INPROC_SERVER,		 //Context in which the code that manages the newly created object will run
		IID_PPV_ARGS(&wicFactory))); //returns the WIC Factory

	return;
}

void D2DResources::CreateDeviceResources()
{
	FLOAT dpi = static_cast<FLOAT>(window.getWindowDPI());
	// Create a Direct2D surface (bitmap) linked to the Direct3D texture back buffer via the DXGI back buffer
	D2D1_BITMAP_PROPERTIES1 bitmapProperties =
		D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(window.GFX->DXGI->dxgiSwapChainDesc1.Format, D2D1_ALPHA_MODE_IGNORE), dpi, dpi, NULL);

	ThrowIfFailed(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dDeviceContext));

	d2dDeviceContext->CreateBitmapFromDxgiSurface(window.GFX->DXGI->dxgiBackBuffer.Get(), &bitmapProperties,
		&d2dRenderTarget);

	// Set surface as render target in Direct2D device context
	d2dDeviceContext->SetTarget(d2dRenderTarget.Get());
	//set the D2D target to the system DPI
	d2dDeviceContext->SetDpi(dpi, dpi);
}

void D2DResources::ReleaseNonStaticResources()
{
	d2dRenderTarget.Reset();
	d2dDeviceContext.Reset();
}
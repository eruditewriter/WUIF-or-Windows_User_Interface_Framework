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
#include "GFX\GFX.h"
#include "Application\Application.h"
#include "Window\Window.h"

using namespace WUIF;
using namespace Microsoft::WRL;

ComPtr<ID2D1Factory1>       D2DResources::d2dFactory = nullptr;
ComPtr<ID2D1Device>         D2DResources::d2dDevice  = nullptr;
ComPtr<IDWriteFactory1>     D2DResources::dwFactory  = nullptr;
ComPtr<IWICImagingFactory2> D2DResources::wicFactory = nullptr;

D2D1_FACTORY_TYPE   D2DResources::d2d1factorytype = D2D1_FACTORY_TYPE_SINGLE_THREADED;
DWRITE_FACTORY_TYPE D2DResources::dwfactorytype   = DWRITE_FACTORY_TYPE_SHARED;

D2DResources::D2DResources(Window * const winptr) : D3D11Resources(winptr),
    d2dRenderTarget(nullptr),
    d2dDeviceContext(nullptr)
{
}

D2DResources::~D2DResources()
{
    ReleaseD2DNonStaticResources();
}

void D2DResources::CreateD2DStaticResources()
{
    DebugPrint(TEXT("CreateD2DStaticResources"));

    D2D1_FACTORY_OPTIONS fo = {};
    #ifdef _DEBUG // for debug build, enable debugging via SDK Layers with factory options debugLevel flag
    fo.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
    #endif

    //Initialize the D2D Factory
    ThrowIfFailed(D2D1CreateFactory(d2d1factorytype,                     //threading model; default:D2D1_FACTORY_TYPE_SINGLE_THREADED
        __uuidof(ID2D1Factory1),			                             //reference to the IID of d2dFactory
        &fo,							                                 //factory options (used for debugging)
        reinterpret_cast<void**>(d2dFactory.ReleaseAndGetAddressOf()))); //returns the factory
    DebugPrint(TEXT("D2D factory created"));

    //Create the D2D device
    ThrowIfFailed(d2dFactory->CreateDevice(dxgiDevice.Get(), d2dDevice.ReleaseAndGetAddressOf()));
    DebugPrint(TEXT("D2D device created"));

    //Initialize the DirectWrite Factory
    ThrowIfFailed(DWriteCreateFactory(dwfactorytype,                        //factory type; default: DWRITE_FACTORY_TYPE_SHARED
        __uuidof(IDWriteFactory1),			                                //reference to the IID of dwFactory
        reinterpret_cast<IUnknown**>(dwFactory.ReleaseAndGetAddressOf()))); //returns the factory
    DebugPrint(TEXT("DWrite factory created"));

    ThrowIfFailed(CoCreateInstance(
        CLSID_WICImagingFactory2,	 //CLSID associated with the data and code that will be used to create the object
        nullptr,					 //If NULL, indicates that the object is not being created as part of an aggregate
        CLSCTX_INPROC_SERVER,		 //Context in which the code that manages the newly created object will run
        IID_PPV_ARGS(wicFactory.ReleaseAndGetAddressOf()))); //returns the WIC Factory
    DebugPrint(TEXT("WIC factory created"));

    return;
}

void D2DResources::CreateD2DDeviceResources()
{
    FLOAT dpi = static_cast<FLOAT>(win->getWindowDPI());
    // Create a Direct2D surface (bitmap) linked to the Direct3D texture back buffer via the DXGI back buffer
    D2D1_BITMAP_PROPERTIES1 bitmapProperties =
        D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(win->dxgiSwapChainDesc1.Format, D2D1_ALPHA_MODE_IGNORE), dpi, dpi, NULL);

    ThrowIfFailed(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dDeviceContext));

    d2dDeviceContext->CreateBitmapFromDxgiSurface(win->dxgiBackBuffer.Get(), &bitmapProperties,
        &d2dRenderTarget);

    // Set surface as render target in Direct2D device context
    d2dDeviceContext->SetTarget(d2dRenderTarget.Get());
    //set the D2D target to the system DPI
    d2dDeviceContext->SetDpi(dpi, dpi);
}

void D2DResources::ReleaseD2DNonStaticResources()
{
    d2dRenderTarget.Reset();
    d2dDeviceContext.Reset();
}
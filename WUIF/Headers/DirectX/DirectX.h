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
#include <wrl/client.h> //needed for ComPtr
#include <unordered_map>
#include <wincodec.h>   //needed for IWICImagingFactory2 definition
#include <dxgi1_6.h>    //needed for DXGI resources
#include <d3d11_4.h>    //needed for D3D11 resources
#include <d3d11on12.h>  //needed for D2D resources when using D3D12
#ifdef _DEBUG
#define D3DCOMPILE_DEBUG 1   //tell the HLSL compiler to include debug information into the shader blob
#endif
#include <d3d12.h>      //needed for D3D12 resources
#include <d2d1_3.h>     //needed for D2D resources
#include <dwrite_3.h>   //needed for D2D/DWrite resources
#ifdef _DEBUG
#include <dxgidebug.h>  //needed for IDXGIDebug and IDXGIInfoQueue
#endif

namespace WUIF {

    //interface to notify D3D resources on device loss
    struct IDeviceNotify
    {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;
    };

    class DXResources;

    class D3D11Resources;
    typedef void(*FPTR)(D3D11Resources*);

    class DXGIResources
    {
    public:
        DXGIResources(DXResources*);
        ~DXGIResources();
        //global DXGI objects
        static Microsoft::WRL::ComPtr<IDXGIFactory2>   dxgiFactory; //CreateDXGIFactory2 needs a IDXGIFactory2 object
        static Microsoft::WRL::ComPtr<IDXGIAdapter1>   dxgiAdapter;
        static Microsoft::WRL::ComPtr<IDXGIDevice>     dxgiDevice;

        //non-global dxgi objects and variables
        Microsoft::WRL::ComPtr<IDXGISurface>    dxgiBackBuffer;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> dxgiSwapChain1;     //DXGI swap chain for this window
        DXGI_SWAP_CHAIN_DESC1   dxgiSwapChainDesc1;

#ifdef _DEBUG
        static Microsoft::WRL::ComPtr<IDXGIDebug>	   dxgiDebug;
        static Microsoft::WRL::ComPtr<IDXGIInfoQueue>  dxgiInfoQueue;
#endif

        bool  HDRsupport() { return _HDRsupport; }

        //functions
        static void GetDXGIAdapterandFactory();
        void CreateSwapChain();
        void DetectHDRSupport();
        void ReleaseNonStaticResources();

    private:
        DXResources *DXres;

        bool tearingsupport;
        bool _HDRsupport;  //if window is on a display that supports HDR

        //functions
        //void CheckTearingSupport();
    };

    class Window; //forward declaration for D3D11Resources::CreateDeviceResources

    class D3D11Resources
    {
    public:
        D3D11Resources(DXResources*);
       ~D3D11Resources();

        //variables
        static Microsoft::WRL::ComPtr<ID3D11Device1>	      d3d11Device1;	         //Direct3D 11 device
        static Microsoft::WRL::ComPtr<ID3D11DeviceContext1>   d3d11ImmediateContext; //Direct3D 11 device context

        // Direct3D rendering objects.
        Microsoft::WRL::ComPtr<ID3D11Texture2D>		          d3d11BackBuffer;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>        d3d11RenderTargetView;

        //the following variables are for D3D11 debug purposes
#ifdef _DEBUG
        static Microsoft::WRL::ComPtr<ID3D11Debug>            d3dDebug;
        static Microsoft::WRL::ComPtr<ID3D11InfoQueue>        d3dInfoQueue;
#endif

        //functions
        static void CreateStaticResources();
               void CreateDeviceResources();
               void ReleaseNonStaticResources();
               void RegisterResourceCreation(FPTR, unsigned int);
               void UnregisterResourceCreation(FPTR);


        DXResources *DXres;
    private:
        void CreateRenderTargets();
        std::unordered_multimap<unsigned int, FPTR> createresources;
    };

    class D3D12Resources
    {
    public:
        D3D12Resources(DXResources*);
        ~D3D12Resources();

        static Microsoft::WRL::ComPtr<ID3D12Device>       d3d12Device;
        static Microsoft::WRL::ComPtr<ID3D12CommandQueue> d3dCommQueue;     //Direct3D command queue (win >= 10)
        static Microsoft::WRL::ComPtr<ID3D11On12Device>   d3d11on12Device;  //Direct3311on12 device (win >=10)

        //functions
        static void CreateStaticResources();
        void ReleaseNonStaticResources();

    private:
        DXResources *DXres;
    };

    class D2DResources
    {
    public:
        D2DResources(DXResources*);
        ~D2DResources();

        //Direct2D objects
        static Microsoft::WRL::ComPtr<ID2D1Factory1> 	   d2dFactory;       //Direct2D factory access
        static Microsoft::WRL::ComPtr<ID2D1Device> 		   d2dDevice;        //Direct2D device
        Microsoft::WRL::ComPtr<ID2D1DeviceContext>  d2dDeviceContext; //Direct2d render target (actually a device context)
        Microsoft::WRL::ComPtr<ID2D1Bitmap1> 	   d2dRenderTarget;  //Direct2D target rendering bitmap
                                                                     //DirectWrite objects
        static Microsoft::WRL::ComPtr<IDWriteFactory1> 	   dwFactory; 	//DirectWrite factory access
        static Microsoft::WRL::ComPtr<IWICImagingFactory2> wicFactory;	//Windows Imaging Component (WIC) factory access

        //functions
        static void CreateStaticResources();
        void CreateDeviceResources();
        void ReleaseNonStaticResources();
    private:
        DXResources *DXres;
    };

    class Window;

    class DXResources
    {
    public:
        DXResources(Window*);

        bool               useD3D12;
        DXGIResources      DXGIres;
        D3D12Resources     d3d12res;
        D3D11Resources     d3d11res;
        D2DResources       d2dres;
        Window            *winparent;

        IDeviceNotify     *deviceNotify; //a mechanism for allowing user code to handle DirectX device loss

        //functions
        void HandleDeviceLost();
    };
}
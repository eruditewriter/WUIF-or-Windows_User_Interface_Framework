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
#include <dxgi1_6.h>    //needed for DXGI resources
#ifdef _DEBUG
#include <dxgidebug.h>  //needed for IDXGIDebug and IDXGIInfoQueue
#endif

namespace WUIF {

    class Window;

    class DXGIResources
    {
    public:
         DXGIResources(Window&);
        ~DXGIResources();

        //global DXGI objects
        static Microsoft::WRL::ComPtr<IDXGIFactory2>  dxgiFactory; //CreateDXGIFactory2 needs a IDXGIFactory2 object
        static Microsoft::WRL::ComPtr<IDXGIAdapter1>  dxgiAdapter;
        static Microsoft::WRL::ComPtr<IDXGIDevice>    dxgiDevice;
        #ifdef _DEBUG
        static Microsoft::WRL::ComPtr<IDXGIDebug>	  dxgiDebug;
        static Microsoft::WRL::ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
        #endif

        //non-global dxgi objects and variables
        Microsoft::WRL::ComPtr<IDXGISurface>          dxgiBackBuffer;
        Microsoft::WRL::ComPtr<IDXGISwapChain1>       dxgiSwapChain1;     //DXGI swap chain for this window
        DXGI_SWAP_CHAIN_DESC1                         dxgiSwapChainDesc1;


        bool  HDRsupport() { return _HDRsupport; }

        //functions
        static void GetDXGIAdapterandFactory();
        void CreateSwapChain();
        void DetectHDRSupport();

        Window &window;
    private:
        bool tearingsupport;
        bool _HDRsupport;  //if window is on a display that supports HDR

        //functions
        //void CheckTearingSupport();
    };

}

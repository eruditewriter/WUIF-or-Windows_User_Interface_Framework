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
#include <wrl/client.h>  //needed for ComPtr
#include <d3d11_4.h>     //needed for D3D11 resources
#include <unordered_map>
#include "GFX/DXGI/DXGI.h"

namespace WUIF {
    class Window;
    class D3D11Resources;

    typedef void(*FPTR)(D3D11Resources*);

    class D3D11Resources : public DXGIResources
    {
    public:
        D3D11Resources(Window * const);
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

        static UINT d3d11createDeviceFlags; //device creation flags used by D3D11CreateDevice;

        //functions
        static void CreateD3D11StaticResources();
        void CreateD3D11DeviceResources();
        void ReleaseD3D11NonStaticResources();
        void RegisterD3D11ResourceCreation(FPTR, unsigned int);
        void UnregisterD3D11ResourceCreation(FPTR);

        //Window &window;
    protected:
        void CreateD3D11RenderTargets();
        std::unordered_multimap<unsigned int, FPTR> createresources;
    };
}
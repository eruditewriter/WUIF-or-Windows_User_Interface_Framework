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
#include <d3d12.h>      //needed for D3D12 resources
#include <d3d11on12.h>  //needed for D2D resources when using D3D12
#include "Utils/dllhelper.h"
#include "GFX/D2D/D2D.h"

namespace WUIF {
    class D3D12libAPI
    {
        DllHelper _d3d12{ TEXT("D3D12.dll"), OSVersion::WIN10 };
        DllHelper _d3d11{ TEXT("D3D11.dll"), OSVersion::UNKNOWN };
    public:
        decltype(D3D12CreateDevice)      *D3D12CreateDevice      = _d3d12.assign("D3D12CreateDevice", OSVersion::WIN10);
        decltype(D3D11On12CreateDevice)  *D3D11On12CreateDevice  = _d3d11.assign("D3D11On12CreateDevice", OSVersion::WIN10); //on Win7 this is not available so cannot implicitly linked
        #if _DEBUG
        decltype(D3D12GetDebugInterface) *D3D12GetDebugInterface = _d3d12.assign("D3D12GetDebugInterface", OSVersion::WIN10);
        #endif
    };

    extern D3D12libAPI *d3d12libAPI;

    class Window;

    class D3D12Resources : public D2DResources
    {
    public:
        D3D12Resources(Window * const);
        ~D3D12Resources();

        static Microsoft::WRL::ComPtr<ID3D12Device>       d3d12Device;
        static Microsoft::WRL::ComPtr<ID3D12CommandQueue> d3dCommQueue;     //Direct3D command queue (win >= 10)
        static Microsoft::WRL::ComPtr<ID3D11On12Device>   d3d11on12Device;  //Direct3311on12 device  (win >= 10)

        //functions
        static void CreateD3D12StaticResources();
        void ReleaseD3D12NonStaticResources();

        //Window &window;
    protected:
    };
}
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

namespace WUIF {
    class Window;

    class D3D12Resources
    {
    public:
        D3D12Resources(Window&);
        ~D3D12Resources();

        static Microsoft::WRL::ComPtr<ID3D12Device>       d3d12Device;
        static Microsoft::WRL::ComPtr<ID3D12CommandQueue> d3dCommQueue;     //Direct3D command queue (win >= 10)
        static Microsoft::WRL::ComPtr<ID3D11On12Device>   d3d11on12Device;  //Direct3311on12 device (win >=10)

                                                                            //functions
        static void CreateStaticResources();
        void ReleaseNonStaticResources();

        Window &window;
    private:

    };
}

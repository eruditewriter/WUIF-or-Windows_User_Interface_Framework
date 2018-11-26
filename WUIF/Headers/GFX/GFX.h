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

#include "D3D/WUIF_D3D12.h"

namespace WUIF {
    //interface to notify D3D resources on device loss
    struct IDeviceNotify
    {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;
    };

    class Window;

    class GFXResources : public D3D12Resources
    {
    public:
        GFXResources(Window * const);

        //a mechanism for allowing user code to handle DirectX device loss
        IDeviceNotify  *deviceNotify;

        //functions
        void HandleDeviceLost();
    };
}

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
#include <d2d1_3.h>     //needed for D2D resources
#include <dwrite_3.h>   //needed for D2D/DWrite resources
#include <wincodec.h>   //needed for IWICImagingFactory2 definition

namespace WUIF {
    class Window;

    class D2DResources
    {
    public:
        D2DResources(Window&);
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

        Window &window;
    private:
        
    };
}

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
#include "WUIF.h"
#include "DirectX\DirectX.h"
#include "Application\Application.h"
#include "Window\Window.h"


using namespace Microsoft::WRL;
using namespace WUIF;


ComPtr<IDXGIFactory2>  DXGIResources::dxgiFactory = nullptr;
ComPtr<IDXGIAdapter1>  DXGIResources::dxgiAdapter = nullptr;
ComPtr<IDXGIDevice>    DXGIResources::dxgiDevice = nullptr;
#ifdef _DEBUG
ComPtr<IDXGIDebug>     DXGIResources::dxgiDebug = nullptr;
ComPtr<IDXGIInfoQueue> DXGIResources::dxgiInfoQueue = nullptr;
#endif

DXGIResources::DXGIResources(DXResources *dxres) :
	dxgiBackBuffer(nullptr),
	dxgiSwapChain1(nullptr),
	dxgiSwapChainDesc1({}),
	DXres(dxres)
{
	//NB: MS documenation states that DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL is not available on Win 7 but testing shows that it is available with Platform Update for Windows 7
	//assign default values to DXGI_SWAP_CHAIN_DESC1
	dxgiSwapChainDesc1.BufferCount = 2;					                  //value that describes the number of buffers in the swap chain - use double-buffering to minimize latency - required for flip effects.
	dxgiSwapChainDesc1.SampleDesc.Count = 1;			                          //number of multisamples per pixel - don't use multi-sampling - flip effects can't use multi-sampling
	dxgiSwapChainDesc1.Format = DXGI_FORMAT_B8G8R8A8_UNORM;             //DXGI_FORMAT - this is the most common swap chain format.
	dxgiSwapChainDesc1.AlphaMode = DXGI_ALPHA_MODE_IGNORE;                 //DXGI_ALPHA_MODE value that identifies the transparency behavior of the swap-chain back buffer - indicates to ignore the transparency behavior - mandatory for CreateSwapChainForHwnd
	dxgiSwapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;        //DXGI_USAGE value that describes the surface usage and CPU access options for the back buffer
	dxgiSwapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;       //DXGI_SWAP_EFFECT value that describes the presentation model - flip presentation model persists the contents of the back buffer
	dxgiSwapChainDesc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;     //It is recommended to always use the tearing flag when it is available// It is recommended to always use the tearing flag when it is available - this will be disabled if not supported
	dxgiSwapChainDesc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; //enable an application to switch modes by calling IDXGISwapChain::ResizeTarget. When switching from windowed to full-screen mode, the display mode (or monitor resolution) will be changed to match the dimensions of the application window.
																		//these values are already 0 on initialization - noted here for reference
																		//dxgiSwapChainDesc1.Width              = 0;		                   //resolution width - 0=match the size of the window
																		//dxgiSwapChainDesc1.Height             = 0;		                   //resolution height - 0=match the size of the window
																		//dxgiSwapChainDesc1.SampleDesc.Quality = 0;			               //image quality level - default sampler mode, with no anti-aliasing, has a count of 1 and a quality level of 0
																		//dxgiSwapChainDesc1.Stereo             = FALSE;				       //specifies whether the full-screen display mode or the swap-chain back buffer is stereo
																		//dxgiSwapChainDesc1.Scaling            = DXGI_SCALING_STRETCH;	       //DXGI_SCALING (0)-typed value that identifies resize behavior - only win7 scaling supported
}

DXGIResources::~DXGIResources()
{
	ReleaseNonStaticResources();
}

void DXGIResources::GetDXGIAdapterandFactory()
{
	//First, create the DXGI Factory if it doesn't already exist
	if (!dxgiFactory)
	{
#ifdef _DEBUG
		HINSTANCE lib = NULL;
		if (DXResources::usingD3D12)
		{
			//validate D3D12 capability
			lib = LoadLibrary(_T("D3D12.dll"));
			if (lib)
			{
				/*The D3D12 debug device is not enabled through a creation flag like it is in
				Direct3D 11. The debug device is only present if the Graphics Tools Windows 10
				optional feature is enabled.*/
				//function signature PFN_D3D12_GET_DEBUG_INTERFACE is provided as a typedef by d3d12.h for D3D12GetDebugInterface
				PFN_D3D12_GET_DEBUG_INTERFACE getdebuginterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(lib, "D3D12GetDebugInterface");
				if (getdebuginterface)
				{
					Microsoft::WRL::ComPtr<ID3D12Debug> debugController;   //Direct3D 12 debug controller
					if (SUCCEEDED(getdebuginterface(IID_PPV_ARGS(&debugController))))
					{
						debugController->EnableDebugLayer();
						debugController.Reset();
					}
					else
					{
						//not a critical failure if debug layer fails, but ASSERT for debugging
						ASSERT(false);
					}
				}
				FreeLibrary(lib);
				lib = NULL;
			}
			else
			{
				if (DXResources::usingD3D12)
				{
					ASSERT(false);  //this section should never be in release code - so assert here instead of throwing
				}
			}
		}

		/*In Windows 8, any DXGI factory created while DXGIDebug.dll was present on the system would load and use it.
		Starting in Windows 8.1, apps explicitly request that DXGIDebug.dll be loaded instead. Use CreateDXGIFactory2
		and specify the DXGI_CREATE_FACTORY_DEBUG flag to request DXGIDebug.dll; the DLL will be loaded if it is
		present on the system.*/
		if (*App->winversion > WUIF::OSVersion::WIN8)
		{
			lib = LoadLibrary(_T("Dxgi.dll"));
			if (lib)
			{
				/*CreateDXGIFactory2 is available on Win > 8.1 and is only needed to load the debug dll*/
				using CREATEDXGIFACTORY2 = HRESULT(WINAPI *)(UINT, REFIID, _Out_ void**);
				CREATEDXGIFACTORY2 createfactory2 = (CREATEDXGIFACTORY2)GetProcAddress(lib, "CreateDXGIFactory2");
				if (createfactory2)
				{
					ThrowIfFailed(createfactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)));
				}
				else
				{//if CreateFactory2 is inexplicably not available fallback to old method
					ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
				}
				FreeLibrary(lib);
				lib = NULL;
			}
			else
			{
				ASSERT(false);
			}
		}
		else
		{
#endif
			//Create DXGI Factory without debug so we can use CreateDXGIFactory1
			{
				ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
			}
#ifdef _DEBUG
		}
		//get the DXGI Debug and InfoQueue resources (MS says only available Win 8 or greater but have loaded on Win7 with Platform update and latest SDK)
		lib = LoadLibrary(_T("Dxgidebug.dll"));
		if (lib)
		{
			using LPDXGIGETDEBUGINTERFACE = HRESULT(WINAPI *)(REFIID, void**);
			LPDXGIGETDEBUGINTERFACE dxgigetdebuginterface = (LPDXGIGETDEBUGINTERFACE)GetProcAddress(lib, "DXGIGetDebugInterface");
			if (dxgigetdebuginterface)
			{
				dxgigetdebuginterface(IID_PPV_ARGS(&dxgiDebug));
				if (SUCCEEDED(dxgigetdebuginterface(IID_PPV_ARGS(&dxgiInfoQueue))))
				{
					dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
					dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
				}
			}
			FreeLibrary(lib);
			lib = NULL;
		}
#endif // _DEBUG
	}
	//factory exists - if we have an adapter already release and reenumerate
	if (dxgiAdapter)
	{
		dxgiAdapter.Reset();
	}
	ComPtr<IDXGIAdapter1> adapter;
	if (DXResources::usingD3D12)
	{
		//enumerate the hardware adapters until we find the first one that supports the minimum D3D feature level
		for (unsigned int adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf());
			++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			if (FAILED(adapter->GetDesc1(&desc)))
			{
				continue;
			}
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) // Don't select the Basic Render Driver adapter for now
			{
				continue;
			}

			//check to see if the adapter supports Direct3D 12 but don't create the actual device yet.
			HINSTANCE lib = LoadLibrary(_T("D3D12.dll"));
			//function signature PFN_D3D12_CREATE_DEVICE is provided as a typedef by d3d12.h for D3D12CreateDevice
			PFN_D3D12_CREATE_DEVICE created3d12device = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(lib, "D3D12CreateDevice");
			if (created3d12device)
			{
				if (SUCCEEDED(created3d12device(
					adapter.Get(),
					DXResources::minDXFL,
					_uuidof(ID3D12Device),
					nullptr)))
				{
					FreeLibrary(lib);
					dxgiAdapter = adapter;
					adapter.Reset();
					return;
				}

			}
			else
			{
				ASSERT(false); //D3D12CreateDevice should have loaded
			}
		}
	}
	//we didn't find a D3D12 hardware adapter, try, if allowed, to fallback to a warp device
	if (DXResources::usingD3D12)
	{
		if (App->flags & FLAGS::WARP)
		{
			ComPtr<IDXGIFactory4> dxgiFactory4;
			if (SUCCEEDED(dxgiFactory.As(&dxgiFactory4)))
			{
				if (SUCCEEDED(dxgiFactory4->EnumWarpAdapter(IID_PPV_ARGS(&adapter))))
				{
					dxgiAdapter = adapter;
					dxgiFactory4.Reset();
					adapter.Reset();
					return;
				}
			}
			dxgiFactory4.Reset();
		}
		//no capable WARP adapter
		if ((App->flags & FLAGS::D3D12) && (!(App->flags & FLAGS::D3D11))) //if D3D12 only throw an error
		{
			throw WUIF_exception(_T("This application requires Direct3D 12 and there is not an available adapter!"));
		}
		else
		{
			DXResources::usingD3D12 = false;
		}
	}

	//no D3D12 adapter available or D3D11 only, we've checked and faulted if D3D12 only, so try D3D11
	bool renderdriver = false;
	int  renderindex = 0;
	D3D_FEATURE_LEVEL returnedFeatureLevel;
	//on less than Win 10 if we include feature levels for 12 we will get an E_INVALIDARG return on D3D11CreateDevice
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	for (unsigned int adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf());
		++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		if (FAILED(adapter->GetDesc1(&desc)))
		{
			continue;
		}
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) // Don't select the Basic Render Driver adapter for now
		{
			if (renderdriver) //already found a basic render driver so ignore this one
			{
				continue;
			}
			else
			{
				renderdriver = true;	    //mark we have a basic render driver in case no actual hardware is present (i.e. virtual machine)
				renderindex = adapterIndex; //mark the index we found it at
				continue;
			}
		}

		//Set ppDevice and ppImmediateContext to NULL to determine which feature level is supported by looking at pFeatureLevel.
		/*If the Direct3D 11.1 runtime is present on the computer and pFeatureLevels is set to NULL, this function won't create a
		D3D_FEATURE_LEVEL_11_1 device. To create a D3D_FEATURE_LEVEL_11_1 device, you must explicitly provide a D3D_FEATURE_LEVEL
		array that includes D3D_FEATURE_LEVEL_11_1. If you provide a D3D_FEATURE_LEVEL array that contains D3D_FEATURE_LEVEL_11_1
		on a computer that doesn't have the Direct3D 11.1 runtime installed, this function immediately fails with E_INVALIDARG*/
		HRESULT hr = (D3D11CreateDevice(
			adapter.Get(),
			D3D_DRIVER_TYPE_UNKNOWN,	//must use D3D_DRIVER_TYPE_UNKNOWN if pAdapter is not null
			NULL,
			D3D11_CREATE_DEVICE_SINGLETHREADED,
			featureLevels,
			_countof(featureLevels),
			D3D11_SDK_VERSION,
			nullptr,
			&returnedFeatureLevel,
			nullptr));
		if (SUCCEEDED(hr))
		{
			if (returnedFeatureLevel >= DXResources::minDXFL)
			{
				dxgiAdapter = adapter;
				adapter.Reset();
				return;
			}
		}
		else
		{ //for 11.0 only systems we will get an E_INVALIDARG return
			if ((hr == E_INVALIDARG) && (DXResources::minDXFL == D3D_FEATURE_LEVEL_11_0))
			{
				HRESULT hr = D3D11CreateDevice(
					adapter.Get(),
					D3D_DRIVER_TYPE_UNKNOWN,	//must use D3D_DRIVER_TYPE_UNKNOWN if we pAdapter  is not null
					NULL,
					D3D11_CREATE_DEVICE_SINGLETHREADED,
					&featureLevels[1],
					_countof(featureLevels) - 1,
					D3D11_SDK_VERSION,
					nullptr,
					&returnedFeatureLevel,
					nullptr);
				if (SUCCEEDED(hr))
				{
					if (returnedFeatureLevel == DXResources::minDXFL)
					{
						dxgiAdapter = adapter;
						adapter.Reset();
						return;
					}
				}
			}
		}
	}
	//no suitable hardware adapters found, check if we found a Basic Render Driver and use that
	if ((renderdriver) && (App->flags & FLAGS::WARP))
	{
		dxgiFactory->EnumAdapters1(renderindex, adapter.ReleaseAndGetAddressOf());
		HRESULT hr = D3D11CreateDevice(
			adapter.Get(),
			D3D_DRIVER_TYPE_UNKNOWN,
			NULL,
			D3D11_CREATE_DEVICE_SINGLETHREADED,
			featureLevels,
			_countof(featureLevels),
			D3D11_SDK_VERSION,
			nullptr,
			&returnedFeatureLevel,
			nullptr);
		if (SUCCEEDED(hr))
		{
			if (returnedFeatureLevel >= DXResources::minDXFL)
			{
				dxgiAdapter = adapter;
				adapter.Reset();
				return;
			}
		}
		else
		{
			if ((hr == E_INVALIDARG) && (DXResources::minDXFL == D3D_FEATURE_LEVEL_11_0))
			{
				HRESULT hr = D3D11CreateDevice(
					adapter.Get(),
					D3D_DRIVER_TYPE_UNKNOWN,	//must use D3D_DRIVER_TYPE_UNKNOWN if pAdapter is not null
					NULL,
					D3D11_CREATE_DEVICE_SINGLETHREADED,
					&featureLevels[1],
					_countof(featureLevels) - 1,
					D3D11_SDK_VERSION,
					nullptr,
					&returnedFeatureLevel,
					nullptr);
				if (SUCCEEDED(hr))
				{
					if (returnedFeatureLevel == DXResources::minDXFL)
					{
						dxgiAdapter = adapter;
						adapter.Reset();
						return;
					}
				}
			}
			else
			{
				adapter.Reset();
				throw WUIF_exception(_T("Unable to find a suitable graphics adapter to run application"));
			}
		}
	}
	else
	{
		adapter.Reset();
		throw WUIF_exception(_T("Unable to find a suitable graphics adapter to run application!"));
	}
}

void DXGIResources::CreateSwapChain()
{
	if (dxgiSwapChain1)
	{
		// If the swap chain already exists, resize it.
		// Clear the previous window size specific context.
		ID3D11RenderTargetView* nullViews[] = { nullptr };
		DXres->d3d11res.d3d11ImmediateContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
		DXres->d3d11res.ReleaseNonStaticResources();
		DXres->d3d11res.d3d11RenderTargetView.Reset();
		DXres->d3d11res.d3d11BackBuffer.Reset();
		if (App->flags & WUIF::FLAGS::D2D)
		{
			if (DXres->d2dres.d2dDeviceContext)
			{
				DXres->d2dres.d2dDeviceContext->SetTarget(nullptr);
			}
			DXres->d2dres.d2dRenderTarget.Reset();
		}
		dxgiBackBuffer.Reset();
		DXres->d3d11res.d3d11ImmediateContext->Flush();
		DXres->d3d11res.d3d11ImmediateContext->ClearState();

		//Perform checks for flag values that are not supported in older OS
		if (*App->winversion < OSVersion::WIN10)
		{
			if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED)
			{
				throw WUIF_exception(_T("SwapChain Flag requires Windows version 10 or higher!"));
			}
			if (*App->winversion < OSVersion::WIN8_1)
			{
				if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
				{
					throw WUIF_exception(_T("SwapChain Flag requires Windows version 8.1 or higher!"));
				}
				if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER)
				{
					throw WUIF_exception(_T("SwapChain Flag requires Windows version 8.1 or higher!"));
				}
				if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO)
				{
					throw WUIF_exception(_T("SwapChain Flag requires Windows version 8.1 or higher!"));
				}
				if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO)
				{
					throw WUIF_exception(_T("SwapChain Flag requires Windows version 8.1 or higher!"));
				}

				if (*App->winversion == OSVersion::WIN7)
				{
					if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT)
					{
						throw WUIF_exception(_T("SwapChain Flag requires Windows version 8 or higher!"));
					}
					if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER)
					{
						throw WUIF_exception(_T("SwapChain Flag requires Windows version 8 or higher!"));
					}
					if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY)
					{
						throw WUIF_exception(_T("SwapChain Flag requires Windows version 8 or higher!"));
					}
				}
			}
		}
		if (DXres->winparent->props.allowfsexclusive())
		{
			dxgiSwapChainDesc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; //must be present if we allow fullscreen exclusive mode
		}
		HRESULT hr = dxgiSwapChain1->ResizeBuffers(
			dxgiSwapChainDesc1.BufferCount,         //number of buffers in the swap chain (0 = preserve existing number of buffers)
			DXres->winparent->props.actualwidth(),  //new width of the back buffer
			DXres->winparent->props.actualheight(),	//new height of the back buffer
			dxgiSwapChainDesc1.Format,              //DXGI_FORMAT value for the new format of the back buffer (0 = keep existing format)
			dxgiSwapChainDesc1.Flags);	            //DXGI_SWAP_CHAIN_FLAG values specifying options for swap-chain behavior

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			DXres->HandleDeviceLost();

			// Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
			// and correctly set up the new device.
			return;
		}
		else
		{
			ThrowIfFailed(hr);
		}
	}
	else
	{
		//Perform checks for values that are not supported in older OS and if possible substitute a fallback
		if (*App->winversion < OSVersion::WIN10)
		{
			if (dxgiSwapChainDesc1.Scaling == DXGI_SCALING_ASPECT_RATIO_STRETCH)
			{
				//DXGI_SCALING_ASPECT_RATIO_STRETCH is only supported on Win10
				//we perform a substitution to equivalent legacy Win32 DXGI_SCALING_STRETCH
				dxgiSwapChainDesc1.Scaling = DXGI_SCALING_STRETCH;
			}
			if (dxgiSwapChainDesc1.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD)
			{
				//only supported in >=Win10
				throw WUIF_exception(_T("SwapChain Swap Effect requires Windows version 10 or higher!"));
			}
			if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED)
			{
				throw WUIF_exception(_T("SwapChain Flag requires Windows version 10 or higher!"));
			}
			if (*App->winversion < OSVersion::WIN8_1)
			{
				if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
				{
					throw WUIF_exception(_T("SwapChain Flag requires Windows version 8.1 or higher!"));
				}
				if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER)
				{
					throw WUIF_exception(_T("SwapChain Flag requires Windows version 8.1 or higher!"));
				}
				if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO)
				{
					throw WUIF_exception(_T("SwapChain Flag requires Windows version 8.1 or higher!"));
				}
				if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO)
				{
					throw WUIF_exception(_T("SwapChain Flag requires Windows version 8.1 or higher!"));
				}
				if (*App->winversion == OSVersion::WIN7)
				{
					//following formats are not supported on Win7
					switch (dxgiSwapChainDesc1.Format)
					{
					case DXGI_FORMAT_B5G6R5_UNORM:
					case DXGI_FORMAT_B5G5R5A1_UNORM:
					case DXGI_FORMAT_AYUV:
					case DXGI_FORMAT_Y410:
					case DXGI_FORMAT_Y416:
					case DXGI_FORMAT_NV12:
					case DXGI_FORMAT_P010:
					case DXGI_FORMAT_P016:
					case DXGI_FORMAT_420_OPAQUE:
					case DXGI_FORMAT_YUY2:
					case DXGI_FORMAT_Y210:
					case DXGI_FORMAT_Y216:
					case DXGI_FORMAT_NV11:
					case DXGI_FORMAT_AI44:
					case DXGI_FORMAT_IA44:
					case DXGI_FORMAT_P8:
					case DXGI_FORMAT_A8P8:
					case DXGI_FORMAT_B4G4R4A4_UNORM:
					{
						throw WUIF_exception(_T("SwapChain Format requires Windows version 8 or higher!"));
					}
					break;
					default:
						break;
					}
				}
			}
		}
		//set the description width/height to the scaled values
		dxgiSwapChainDesc1.Width = DXres->winparent->props.actualwidth();
		dxgiSwapChainDesc1.Height = DXres->winparent->props.actualheight();
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fs = {};
		fs.Windowed = TRUE;
		//validate if tearing support is available
		if (dxgiSwapChainDesc1.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
		{
			BOOL allowTearing = FALSE;
			ComPtr<IDXGIFactory5> factory5;
			HRESULT hr = dxgiFactory.As(&factory5);
			if (SUCCEEDED(hr))
			{
				hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
			}
			if (FAILED(hr))
			{
				dxgiSwapChainDesc1.Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
#ifdef _DEBUG
				OutputDebugStringA("WARNING: Variable refresh rate displays not supported/n");
#endif
			}
		}
		if (DXres->winparent->props.allowfsexclusive())
		{
			dxgiSwapChainDesc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; //must be present if we allow fullscreen exclusive mode
		}
		// Create DXGI swap chain targeting a window handle
		HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(
			DXres->d3d11res.d3d11Device1.Get(), //D3D11 - this is a pointer to the Direct3D device for the swap chain
												//D3D12 - this is a pointer to a direct command queue
			DXres->winparent->hWnd,             //The HWND handle that is associated with the swap chain
			&dxgiSwapChainDesc1,                //A pointer to a DXGI_SWAP_CHAIN_DESC1 structure for the swap-chain description
			&fs,	//recommendation is to create a Windowed swap chain and then allow a change to full-screen
					//Set it to NULL to create a windowed swap chain.
			NULL,   //A pointer to the IDXGIOutput interface for the output to restrict content to
			&dxgiSwapChain1); //A pointer to a variable that receives a pointer to the IDXGISwapChain1 interface for the swap chain
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			DXres->HandleDeviceLost();

			// Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
			// and correctly set up the new device.
			return;
		}
		else
		{
			ThrowIfFailed(hr);
		}
	}

	/*we'll handle ALT-ENTER for fullscreen toggle
	Applications that want to handle mode changes or Alt+Enter themselves should call MakeWindowAssociation
	with the DXGI_MWA_NO_WINDOW_CHANGES flag after swap chain creation.*/
	//if (!DXres->winparent->props.allowfsexclusive())
	{
		ThrowIfFailed(dxgiFactory->MakeWindowAssociation(DXres->winparent->hWnd, DXGI_MWA_NO_ALT_ENTER));
	}
	if ((*App->winversion >= OSVersion::WIN10) && (DXres->winparent->enableHDR))
	{
		DetectHDRSupport();
	}
	ComPtr<IDXGIDevice1> dxgiDevice1;
	dxgiDevice.As(&dxgiDevice1);
	// Ensure that DXGI doesn't queue more than one frame at a time.
	dxgiDevice1->SetMaximumFrameLatency(1);
	dxgiDevice1.Reset();
	// Get the back buffer as an IDXGISurface (Direct2D doesn't accept an ID3D11Texture2D directly as a render target)
	ThrowIfFailed(dxgiSwapChain1->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer)));
}

void DXGIResources::DetectHDRSupport()
{
	static bool checkSupport = true;
	// Output information is cached on the DXGI Factory. If it is stale we need to create
	// a new factory and re-enumerate the displays
	if (!dxgiFactory->IsCurrent())
	{
		dxgiFactory.Reset();
		GetDXGIAdapterandFactory();
	}
	// Get information about the display we are presenting to.
	ComPtr<IDXGIOutput> output;
	ComPtr<IDXGIOutput6> output6;
	ThrowIfFailed(dxgiSwapChain1->GetContainingOutput(&output));
	if (SUCCEEDED(output.As(&output6)))
	{
		DXGI_OUTPUT_DESC1 outputDesc;
		ThrowIfFailed(output6->GetDesc1(&outputDesc));
		_HDRsupport = (outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
		//DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R16G16B16A16_FLOAT
		DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		switch (dxgiSwapChainDesc1.Format)
		{
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			// The application creates the HDR10 signal.
			colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
			break;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			// The system creates the HDR10 signal; application uses linear values.
			colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
			break;
		default:
			break;
		}
		ComPtr<IDXGISwapChain3> swapChain3;
		if (SUCCEEDED(dxgiSwapChain1.As(&swapChain3)))
		{
			UINT colorSpaceSupport = 0;
			if (SUCCEEDED(swapChain3->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport))
				&& (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
			{
				ThrowIfFailed(swapChain3->SetColorSpace1(colorSpace));
			}
			swapChain3.Reset();
		}
	}
}

void DXGIResources::ReleaseNonStaticResources()
{
	dxgiBackBuffer.Reset();
	dxgiSwapChain1.Reset();
}
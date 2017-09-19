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
#include <math.h> //needed for floorf
#include "DirectX\DirectX.h"
#include "Application\Application.h"
#include "Window\Window.h"

namespace {
	long exceptionraised = 0;
}

using namespace WUIF;

#if defined(_M_IX86) //if compiling for x86
LRESULT CALLBACK Window::_WndProc(Window* pThis, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
#elif defined(_M_AMD64) //if compiling for x64
LRESULT CALLBACK Window::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, Window* pThis)
#endif
{
	if (InterlockedDecrement(&exceptionraised) < 0)
	{
        InterlockedIncrement(&exceptionraised); //bring back to 0
		bool handled = false;
		//exceptions are not propagated in WndProc
		try
		{
			if (!pThis->WndProc_map.empty())
			{
				auto proc = pThis->WndProc_map.count(message);
				if (proc == 1)
				{
					//App->paintmutex.lock();
					handled = (WndProc)(pThis->WndProc_map[message])(hWnd, message, wParam, lParam);
					//App->paintmutex.unlock();
				}
			}
			if (!handled)
			{
				switch (message)
				{
				case WM_GETMINMAXINFO:
				{
					MINMAXINFO* mminfo = reinterpret_cast<MINMAXINFO *>(lParam);
					//if the property is 0, treat it as requesting default, so do not change the mminfo value
					if (pThis->props.minwidth() != 0)
					{
						mminfo->ptMinTrackSize.x = pThis->props.minwidth(); //cant' make the window smaller than min width
					}
					if (pThis->props.minheight() != 0)
					{
						mminfo->ptMinTrackSize.y = pThis->props.minheight(); //cant' make the window smaller than min height
					}
					if (pThis->props.maxwidth() != 0)
					{
						mminfo->ptMaxTrackSize.x = pThis->props.maxwidth(); //can't make window bigger than max width
					}
					if (pThis->props.maxheight() != 0)
					{
						mminfo->ptMaxTrackSize.y = pThis->props.maxheight(); //can't make window bigger than max height
					}
					handled = true;
				}
				break;
				case WM_NCCREATE:
				{
					pThis->hWnd = hWnd; //set the hWnd for retrieval by other functions without needing to pass

					if (*App->winversion >= OSVersion::WIN10)
					{
						/*Enable per-monitor DPI scaling for caption, menu, and top-level scroll bars for top-level windows that are
						running as per-monitor DPI aware (This will have no effect if the thread's DPI context is not per-monitor-DPI-aware).
						This API should be called while processing WM_NCCREATE.*/
						HINSTANCE lib = LoadLibrary(_T("user32.dll"));
						if (lib)
						{
							using ENABLENONCLIENTDPISCALING = BOOL(WINAPI *)(HWND);
							ENABLENONCLIENTDPISCALING nonclientscaling = (ENABLENONCLIENTDPISCALING)GetProcAddress(lib, "EnableNonClientDpiScaling");
							if (nonclientscaling)
							{
								nonclientscaling(hWnd);
							}
							FreeLibrary(lib);
						}
					}
				}
				break;
				case WM_CREATE:
				{
					RECT rcWindow = {};
					//get the window rectangle
					GetWindowRect(hWnd, &rcWindow);

					//if we are using CW_USEDEFAULT set the value to the rcWindow value
					if (pThis->props._left == CW_USEDEFAULT)
					{
						pThis->props._left = static_cast<int>(rcWindow.left);
					}
					if (pThis->props._top == CW_USEDEFAULT)
					{
						pThis->props._top = static_cast<int>(rcWindow.top);
					}
					if (pThis->props._width == CW_USEDEFAULT)
					{
						pThis->props._width = static_cast<int>(rcWindow.right - rcWindow.left);
					}
					if (pThis->props._height == CW_USEDEFAULT)
					{
						pThis->props._height = static_cast<int>(rcWindow.bottom - rcWindow.top);
					}

					//update our _prev variables to be the same as the initial size variables
					pThis->props._prevleft = pThis->props._left;
					pThis->props._prevtop = pThis->props._top;
					pThis->props._prevwidth = pThis->props._width;
					pThis->props._prevheight = pThis->props._height;

					//calculate the size width and height should be based on scaling
					UINT dpi = pThis->getWindowDPI();
					pThis->props._actualwidth = pThis->Scale(pThis->props._width);
					pThis->props._actualheight = pThis->Scale(pThis->props._height);

					//setup D3D dependent resources
					pThis->DXres->DXGIres.CreateSwapChain();
					//setup D3D dependent resources
					if (pThis->DXres->usingD3D12)
					{
						//pThis->DXres->d3d12res.CreateD12Resources();
					}
					else //use D3D11
					{
						pThis->DXres->d3d11res.CreateDeviceResources();
					}
					if (App->flags & WUIF::FLAGS::D2D)
					{
						pThis->DXres->d2dres.CreateDeviceResources();
					}

					//if we have scaling resize the window to account for DPI. In this case, we resize the window for the DPI manually
					if ((pThis->props._actualwidth != pThis->props._width) || (pThis->props._actualheight != pThis->props._height))
					{
						SetWindowPos(hWnd, HWND_TOP, pThis->props._left, pThis->props._top, pThis->props._actualwidth,
							pThis->props._actualheight, SWP_NOZORDER | SWP_NOACTIVATE);
					}

					handled = true;
				}
				break;
				case WM_SYSKEYDOWN:
				{
					if ((wParam == VK_RETURN) && (lParam & (1 << 29)))
					{
						//if ((HIWORD(lParam) & KF_ALTDOWN))
						{
							//if (!pThis->DXres->winparent->props.allowfsexclusive())
							{
								pThis->ToggleFullScreen();
								handled = true;
							}
						}
					}
				}
				break;
				case WM_PAINT:
				{
					/*
					//setup the DirectX resources for the window "painting"
					pThis->DXres->DXGIres.CreateSwapChain();
					//setup D3D dependent resources
					if (pThis->DXres->usingD3D12)
					{
					//pThis->DXres->d3d12res.CreateD12Resources();
					}
					else //use D3D11
					{
					pThis->DXres->d3d11res.CreateDeviceResources();
					}
					if (App->flags & WUIF::FLAGS::D2D)
					{
					pThis->DXres->d2dres.CreateDeviceResources();
					}
					*/
					//complete the paint process but do nothing
					PAINTSTRUCT ps;
					//App->paintmutex.lock();
					HDC hdc = BeginPaint(hWnd, &ps);
					pThis->Present();
					EndPaint(hWnd, &ps);
					//App->paintmutex.unlock();
					handled = true;
				}
				break;
				case WM_WINDOWPOSCHANGED:
				{
					/*By default, the DefWindowProc function sends the WM_SIZE and WM_MOVE messages to the
					window. The WM_SIZE and WM_MOVE messages are not sent if an application handles the
					WM_WINDOWPOSCHANGED message without calling DefWindowProc. It is more efficient to
					perform any move or size change processing during the WM_WINDOWPOSCHANGED message
					without calling */
					WINDOWPOS *newpos = reinterpret_cast<WINDOWPOS*>(lParam);
					pThis->props._left = newpos->x;
					pThis->props._top = newpos->y;
					bool minimized = newpos->x < 0 ? true : false; //less than zero means window is minimizing
																   //don't update if no change to width or height or if the window is minimizing
					if ((pThis->props._actualwidth != newpos->cx || pThis->props._actualheight != newpos->cy) &&
						!minimized)
					{
						//update the property position - don't update the previous position, that should occur explicitly
						pThis->props._actualwidth = newpos->cx;
						pThis->props._actualheight = newpos->cy;
						if (pThis->_fullscreen) //in fullscreen non-scaled and scaled are equal
						{
							pThis->props._width = newpos->cx;
							pThis->props._height = newpos->cy;
						}
						else
						{
							pThis->getWindowDPI();
							pThis->props._width = MulDiv(newpos->cx, 100, pThis->scaleFactor); //width without scaling
							pThis->props._height = MulDiv(newpos->cy, 100, pThis->scaleFactor); //height without scaling
						}
						//App->paintmutex.lock();
						pThis->DXres->DXGIres.CreateSwapChain();
						//setup D3D dependent resources
						if (pThis->DXres->usingD3D12)
						{
							//pThis->DXres->d3d12res.CreateD12Resources();
						}
						else //use D3D11
						{
							pThis->DXres->d3d11res.CreateDeviceResources();
						}
						if (App->flags & WUIF::FLAGS::D2D)
						{
							pThis->DXres->d2dres.CreateDeviceResources();
						}
						//App->paintmutex.unlock();
					}
					handled = true;
				}
				break;
				case WM_DISPLAYCHANGE:
				{
					InvalidateRect(hWnd, nullptr, false);
					handled = true;
				}
				break;
				case WM_DPICHANGED:
				{
					// This message tells the program that most of its window is on a monitor with a new DPI. The wParam contains
					// the new DPI, and the lParam contains a rect which defines the window rectangle scaled to the new DPI.
					pThis->scaleFactor = MulDiv(LOWORD(wParam), 100, 96);
					if (pThis->DXres->d2dres.d2dDeviceContext)
					{
						//App->paintmutex.lock();
						pThis->DXres->d2dres.d2dDeviceContext->SetDpi(LOWORD(wParam), LOWORD(wParam));
						//App->paintmutex.unlock();
					}
					// Get the window rectangle scaled for the new DPI, retrieved from the lParam
					LPRECT lprcNewScale = reinterpret_cast<LPRECT>(lParam);
					pThis->props._left = lprcNewScale->left;
					pThis->props._top = lprcNewScale->top;
					pThis->props._actualwidth = lprcNewScale->right - lprcNewScale->left;
					pThis->props._width = MulDiv(lprcNewScale->right - lprcNewScale->left, 100, pThis->scaleFactor); //width without scaling
					pThis->props._actualheight = lprcNewScale->bottom - lprcNewScale->top;
					pThis->props._height = MulDiv(lprcNewScale->bottom - lprcNewScale->top, 100, pThis->scaleFactor);//height without scaling
																													 // For the new DPI: resize the window
					SetWindowPos(hWnd, HWND_TOP, lprcNewScale->left, lprcNewScale->top, (lprcNewScale->right - lprcNewScale->left),
						(lprcNewScale->bottom - lprcNewScale->top), SWP_NOZORDER | SWP_NOACTIVATE);
					RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
					handled = true;
				}
				break;
				case WM_DESTROY:
				{
					if (pThis->_fullscreen)
					{
						if (pThis->props._allowfsexclusive)
						{
							pThis->DXres->DXGIres.dxgiSwapChain1->SetFullscreenState(FALSE, NULL);
							pThis->_fullscreen = false;
						}
					}

					if (App->mainWindow == pThis)
					{
						PostQuitMessage(0);
					}
					handled = true;
				}
				break;
				default:
				{
					handled = false;
				}
				}
			}
		}
		catch (const WUIF_exception &e)
		{
			InterlockedIncrement(&exceptionraised);
			if (WUIF::App != nullptr)
			{
				if (WUIF::App->ExceptionHandler != nullptr)
				{
					try {
						WUIF::App->ExceptionHandler();
						delete WUIF::App;
						WUIF::App = nullptr;
					}
					catch (...)
					{
						if (WUIF::App != nullptr)
						{
							delete WUIF::App;
							WUIF::App = nullptr;
						}
					}
				}
			}
			try
			{
				LPVOID lpMsgBuf;
				LPVOID lpDisplayBuf;
				LPTSTR lpszMessage = e.WUIFWhat();
                TCHAR* pszTxt = _T("An unrecoverable critical error was encountered and the application is now terminating!\n\n%s\n\nReported error is %d: %s");
				DWORD err = GetLastError();
				FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					err,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    reinterpret_cast<LPTSTR>(&lpMsgBuf),
					0, NULL);
                lpDisplayBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlen(reinterpret_cast<LPCTSTR>(lpMsgBuf))
                    + lstrlen(reinterpret_cast<LPCTSTR>(lpszMessage)) + 118) * sizeof(TCHAR));
                if (lpDisplayBuf)
                {
                    StringCchPrintf(reinterpret_cast<LPTSTR>(lpDisplayBuf),
                        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                        pszTxt,
                        lpszMessage, err, lpMsgBuf);
                    MessageBox(NULL, reinterpret_cast<LPCTSTR>(lpDisplayBuf), _T("Critical Error"), (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
                }
                else //LocalAlloc failed - print fallback message
                {
                    MessageBox(NULL, _T("Critical Error! Unable to retrieve error message."), _T("Critical Error"), (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
                }
                LocalFree(lpMsgBuf);
                HeapFree(GetProcessHeap(), NULL, lpDisplayBuf);
				PostQuitMessage(WE_WNDPROC_EXCEPTION);
			}
			catch (...) {}
		}
		catch (...) //catch all other unhandled exceptions
		{
			InterlockedIncrement(&exceptionraised);
			if (WUIF::App != nullptr)
			{
				if (WUIF::App->ExceptionHandler != nullptr)
				{
					try {
						WUIF::App->ExceptionHandler();
						delete WUIF::App;
						WUIF::App = nullptr;
					}
					catch (...)
					{
						if (WUIF::App != nullptr)
						{
							delete WUIF::App;
							WUIF::App = nullptr;
						}
					}
				}
			}
			try {
				LPVOID  lpMsgBuf;
				LPVOID  lpDisplayBuf;
				LPTSTR  lpszMessage = _T("Critical Unhandled Exception in WindowProc!");
				DWORD   err    = GetLastError();
                LPCTSTR pszTxt = _T("An unrecoverable critical error was encountered and the application is now terminating!\n\n%s\n\nReported error is %d: %s");
				FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					err,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					reinterpret_cast<LPTSTR>(&lpMsgBuf),
					0, NULL);
                lpDisplayBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlen(reinterpret_cast<LPCTSTR>(lpMsgBuf))
                    + lstrlen(reinterpret_cast<LPCTSTR>(lpszMessage)) + 118) * sizeof(TCHAR));
                if (lpDisplayBuf)
                {
                    StringCchPrintf(reinterpret_cast<LPTSTR>(lpDisplayBuf),
                        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                        pszTxt,
                        lpszMessage, err, lpMsgBuf);
                    MessageBox(NULL, reinterpret_cast<LPCTSTR>(lpDisplayBuf), _T("Critical Error"), (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
                }
                else //LocalAlloc failed - print fallback message
                {
                    MessageBox(NULL, _T("Critical Error! Unable to retrieve error message."), _T("Critical Error"), (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
                }
				LocalFree(lpMsgBuf);
                HeapFree(GetProcessHeap(), NULL, lpDisplayBuf);
				PostQuitMessage(WE_WNDPROC_EXCEPTION);
			}
			catch (...) {}
		}
		if (!handled)
		{
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}
	return 0;
}


/*
bool Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
return 0;
}
*/

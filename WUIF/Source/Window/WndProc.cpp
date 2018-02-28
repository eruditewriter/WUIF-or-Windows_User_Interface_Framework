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
#include <math.h> //needed for floorf
#include <strsafe.h> //needed for StringCchPrintf
#include "GFX\GFX.h"
#include "Application\Application.h"
#include "Window\Window.h"

namespace {
    long exceptionraised = 0;
}

using namespace WUIF;

#if defined(_M_IX86)    //if compiling for x86
LRESULT CALLBACK Window::_WndProc(_In_ Window* pThis, _In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM  lParam)
#elif defined(_M_AMD64) //if compiling for x64
LRESULT CALLBACK Window::_WndProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_ Window* pThis)
#endif
{
    LRESULT retval = 0;
    if (InterlockedDecrement(&exceptionraised) < 0)
    {
        InterlockedIncrement(&exceptionraised); //bring back to 0
        bool handled = false;
        //exceptions are not propagated in WndProc
        try
        {
            if (!App::GWndProc_map.empty())
            {
                auto proc = App::GWndProc_map.count(message);
                if (proc == 1)
                {
                    handled = (WNDPROC)(App::GWndProc_map[message])(hWnd, message, wParam, lParam);
                }
            }
            if (!pThis->WndProc_map.empty())
            {
                auto proc = pThis->WndProc_map.count(message);
                if (proc == 1)
                {
                    //App::paintmutex.lock();
                    handled = (WndProc)(pThis->WndProc_map[message])(hWnd, message, wParam, lParam, pThis);
                    //App::paintmutex.unlock();
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
                    if (pThis->minwidth() != 0)
                    {
                        mminfo->ptMinTrackSize.x = pThis->minwidth(); //cant' make the window smaller than min width
                    }
                    if (pThis->minheight() != 0)
                    {
                        mminfo->ptMinTrackSize.y = pThis->minheight(); //cant' make the window smaller than min height
                    }
                    if (pThis->maxwidth() != 0)
                    {
                        mminfo->ptMaxTrackSize.x = pThis->maxwidth(); //can't make window bigger than max width
                    }
                    if (pThis->maxheight() != 0)
                    {
                        mminfo->ptMaxTrackSize.y = pThis->maxheight(); //can't make window bigger than max height
                    }
                    handled = true;
                }
                break;
                case WM_NCCREATE:
                {
                    pThis->hWnd = hWnd; //set the hWnd for retrieval by other functions without needing to pass
                    if ((App::winversion >= OSVersion::WIN10_1607) && (pThis->_threaddpiawarenesscontext == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
                    {
                        /*Enable per-monitor DPI scaling for caption, menu, and top-level scroll
                        bars for top-level windows that are running as per-monitor DPI aware (This
                        will have no effect if the thread's DPI context is not
                        per-monitor-DPI-aware). This API should be called while processing
                        WM_NCCREATE.*/
                        HMODULE lib = GetModuleHandle(TEXT("user32.dll"));
                        if (lib)
                        {
                            using PFN_ENABLE_NON_CLIENT_DPI_SCALING = BOOL(WINAPI *)(HWND);
                            PFN_ENABLE_NON_CLIENT_DPI_SCALING pfnenablenonclientscaling = (PFN_ENABLE_NON_CLIENT_DPI_SCALING)GetProcAddress(lib, "EnableNonClientDpiScaling");
                            if (pfnenablenonclientscaling)
                            {
                                pfnenablenonclientscaling(hWnd);
                            }
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
                    if (pThis->left() == CW_USEDEFAULT)
                    {
                        pThis->left(static_cast<int>(rcWindow.left));
                    }
                    if (pThis->top() == CW_USEDEFAULT)
                    {
                        pThis->top(static_cast<int>(rcWindow.top));
                    }
                    if (pThis->width() == CW_USEDEFAULT)
                    {
                        pThis->width(static_cast<int>(rcWindow.right - rcWindow.left));
                    }
                    if (pThis->height() == CW_USEDEFAULT)
                    {
                        pThis->height(static_cast<int>(rcWindow.bottom - rcWindow.top));
                    }

                    //update our _prev variables to be the same as the initial size variables
                    pThis->_prevleft = pThis->_left;
                    pThis->_prevtop = pThis->_top;
                    pThis->_prevwidth = pThis->_width;
                    pThis->_prevheight = pThis->_height;

                    //calculate the size width and height should be based on scaling
                    pThis->getWindowDPI();
                    pThis->_actualwidth = pThis->Scale(pThis->width());
                    pThis->_actualheight = pThis->Scale(pThis->height());

                    //setup D3D dependent resources
                    pThis->CreateSwapChain();
                    //setup D3D dependent resources
                    if (WUIF::App::GFXflags & WUIF::FLAGS::D3D12)
                    {
                        //pThis->GFX->CreateD12Resources();
                    }
                    else //use D3D11
                    {
                        pThis->CreateD3D11DeviceResources();
                    }
                    if (App::GFXflags & WUIF::FLAGS::D2D)
                    {
                        pThis->CreateD2DDeviceResources();
                    }

                    //if we have scaling resize the window to account for DPI. In this case, we resize the window for the DPI manually
                    if ((pThis->_actualwidth != pThis->_width) || (pThis->_actualheight != pThis->_height))
                    {
                        SetWindowPos(hWnd, HWND_TOP, pThis->_left, pThis->_top, pThis->_actualwidth,
                            pThis->_actualheight, SWP_NOZORDER | SWP_NOACTIVATE);
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
                            //if (!pThis->DXres->winparent->allowfsexclusive())
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
                    if (pThis->DXres->useD3D12)
                    {
                    //pThis->DXres->d3d12res.CreateD12Resources();
                    }
                    else //use D3D11
                    {
                    pThis->DXres->d3d11res.CreateDeviceResources();
                    }
                    if (App::flags & WUIF::FLAGS::D2D)
                    {
                    pThis->DXres->d2dres.CreateDeviceResources();
                    }
                    */
                    //complete the paint process but do nothing
                    PAINTSTRUCT ps;
                    //App::paintmutex.lock();
                    //suppress warnings about hdc being declared but never used
                    #pragma warning(suppress: 4189)
                    HDC hdc = BeginPaint(hWnd, &ps);
                    pThis->Present();
                    EndPaint(hWnd, &ps);
                    //App::paintmutex.unlock();
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
                    pThis->_left = newpos->x;
                    pThis->_top = newpos->y;
                    bool minimized = newpos->x < 0 ? true : false; //less than zero means window is minimizing
                                                                   //don't update if no change to width or height or if the window is minimizing
                    if ((pThis->_actualwidth != newpos->cx || pThis->_actualheight != newpos->cy) &&
                        !minimized)
                    {
                        //update the property position - don't update the previous position, that should occur explicitly
                        pThis->_actualwidth = newpos->cx;
                        pThis->_actualheight = newpos->cy;
                        if (pThis->_fullscreen) //in fullscreen non-scaled and scaled are equal
                        {
                            pThis->_width = newpos->cx;
                            pThis->_height = newpos->cy;
                        }
                        else
                        {
                            pThis->getWindowDPI();
                            pThis->_width = MulDiv(newpos->cx, 100, pThis->scaleFactor); //width without scaling
                            pThis->_height = MulDiv(newpos->cy, 100, pThis->scaleFactor); //height without scaling
                        }
                        //App::paintmutex.lock();
                        pThis->CreateSwapChain();
                        //setup D3D dependent resources
                        if (App::GFXflags & FLAGS::D3D12)
                        {
                            //pThis->GFX->D3D12->CreateD12Resources();
                        }
                        else //use D3D11
                        {
                            pThis->CreateD3D11DeviceResources();
                        }
                        if (App::GFXflags & FLAGS::D2D)
                        {
                            pThis->CreateD2DDeviceResources();
                        }
                        //App::paintmutex.unlock();
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
                    //only per monitor aware need to respond to WM_DPICHANGED
                    if (((App::processdpiawarenesscontext) && ((*App::processdpiawarenesscontext == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ||
                        (*App::processdpiawarenesscontext == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))) ||
                        ((App::processdpiawareness) && (*App::processdpiawareness == PROCESS_PER_MONITOR_DPI_AWARE)))
                    {
                        // This message tells the program that most of its window is on a monitor with a new DPI. The wParam contains
                        // the new DPI, and the lParam contains a rect which defines the window rectangle scaled to the new DPI.
                        pThis->scaleFactor = MulDiv(LOWORD(wParam), 100, 96);
                        if (pThis->d2dDeviceContext)
                        {
                            //App::paintmutex.lock();
                            pThis->d2dDeviceContext->SetDpi(LOWORD(wParam), LOWORD(wParam));
                            //App::paintmutex.unlock();
                        }
                        // Get the window rectangle scaled for the new DPI, retrieved from the lParam
                        LPRECT lprcNewScale = reinterpret_cast<LPRECT>(lParam);
                        pThis->_left = lprcNewScale->left;
                        pThis->_top = lprcNewScale->top;
                        pThis->_actualwidth = lprcNewScale->right - lprcNewScale->left;
                        pThis->_width = MulDiv(lprcNewScale->right - lprcNewScale->left, 100, pThis->scaleFactor); //width without scaling
                        pThis->_actualheight = lprcNewScale->bottom - lprcNewScale->top;
                        pThis->_height = MulDiv(lprcNewScale->bottom - lprcNewScale->top, 100, pThis->scaleFactor);//height without scaling
                                                                                                                         // For the new DPI: resize the window
                        SetWindowPos(hWnd, HWND_TOP, lprcNewScale->left, lprcNewScale->top, (lprcNewScale->right - lprcNewScale->left),
                            (lprcNewScale->bottom - lprcNewScale->top), SWP_NOZORDER | SWP_NOACTIVATE);
                        RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
                        handled = true;
                    }
                }
                break;
                case WM_DESTROY:
                {
                    //leave full-screen exclusive mode before destroying the window
                    if (pThis->_fullscreen)
                    {
                        if (pThis->_allowfsexclusive)
                        {
                            pThis->dxgiSwapChain1->SetFullscreenState(FALSE, NULL);
                            pThis->_fullscreen = false;
                        }
                    }
                    pThis->initialized = false;
                }
                break;
                case WM_NCDESTROY:
                {
                    /*window has been destroyed, while destroying the non-client area delete the
                    window. We save cWndProc as we process the delete before calling the class
                    WndProc. If this is the mainWindow also call PostQuitMessage to signal the
                    application is now shutting down.*/
                    WNDPROC tempcwndproc = NULL;
                    if (pThis->cWndProc)
                    {
                        tempcwndproc = pThis->cWndProc;
                    }
                    delete pThis;
                    if (App::mainWindow == pThis)
                    {
                        PostQuitMessage(0);
                        //handled = true;
                        return retval;
                    }
                    if (tempcwndproc)
                    {
                        return CallWindowProc(tempcwndproc, hWnd, message, wParam, lParam);
                    }
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
                break;
                case WM_USER+1: //For changing the window's Dpi Awareness Context
                {
                    HMODULE lib = GetModuleHandle(TEXT("user32.dll"));
                    if (lib)
                    {
                        using PFN_SET_THREAD_DPI_AWARENESS_CONTEXT = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
                        PFN_SET_THREAD_DPI_AWARENESS_CONTEXT pfnsetthreaddpiawarenesscontext = (PFN_SET_THREAD_DPI_AWARENESS_CONTEXT)GetProcAddress(lib, "SetThreadDpiAwarenessContext");
                        if (pfnsetthreaddpiawarenesscontext)
                        {
                            retval = reinterpret_cast<LRESULT>(pfnsetthreaddpiawarenesscontext(pThis->_threaddpiawarenesscontext));
                        }
                    }
                    handled = true;
                }
                break;
                default:
                {
                    //handled = false;
                }
                }
            }
            if (pThis->cWndProc)
            {
                return CallWindowProc(pThis->cWndProc, hWnd, message, wParam, lParam);
            }
            if (!handled)
            {
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        catch (const WUIF_exception &e)
        {
            InterlockedIncrement(&exceptionraised);
            try
            {
                LPVOID lpMsgBuf;
                LPVOID lpDisplayBuf;
                LPTSTR lpszMessage = e.WUIFWhat();
                TCHAR* pszTxt = TEXT("An unrecoverable critical error was encountered and the application is now terminating!\n\n%s\n\nReported error is %d: %s");
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
                    MessageBox(NULL, reinterpret_cast<LPCTSTR>(lpDisplayBuf), TEXT("Critical Error"), (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
                }
                else //LocalAlloc failed - print fallback message
                {
                    MessageBox(NULL, TEXT("Critical Error! Unable to retrieve error message."), TEXT("Critical Error"), (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
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
            try {
                LPVOID  lpMsgBuf;
                LPVOID  lpDisplayBuf;
                LPTSTR  lpszMessage = TEXT("Critical Unhandled Exception in WindowProc!");
                DWORD   err = GetLastError();
                LPCTSTR pszTxt = TEXT("An unrecoverable critical error was encountered and the application is now terminating!\n\n%s\n\nReported error is %d: %s");
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
                    MessageBox(NULL, reinterpret_cast<LPCTSTR>(lpDisplayBuf), TEXT("Critical Error"), (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
                }
                else //LocalAlloc failed - print fallback message
                {
                    MessageBox(NULL, TEXT("Critical Error! Unable to retrieve error message."), TEXT("Critical Error"), (MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND));
                }
                LocalFree(lpMsgBuf);
                HeapFree(GetProcessHeap(), NULL, lpDisplayBuf);
                PostQuitMessage(WE_WNDPROC_EXCEPTION);
            }
            catch (...) {}
        }

    }
    return retval;
}

/*
bool Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
return 0;
}
*/
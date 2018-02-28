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

#include "Window\WindowProperties.h"
#include "Window\Window.h"
#include "Application\Application.h"
#include "GFX\GFX.h"

using namespace WUIF;

WindowProperties::WindowProperties(Window *const winptr) :
    win(winptr),
    initialized(false),
    scaleFactor(100),
    _icon(NULL),
    _iconsm(NULL),
    _classcursor(NULL),
    _windowname(nullptr),
    _height(CW_USEDEFAULT),
    _width(CW_USEDEFAULT),
    _prevheight(0),
    _prevwidth(0),
    _actualheight(0),
    _actualwidth(0),
    _minheight(0),
    _minwidth(0),
    _maxheight(0),
    _maxwidth(0),
    _top(CW_USEDEFAULT),
    _left(CW_USEDEFAULT),
    _prevtop(0),
    _prevleft(0),
    _exstyle(WS_EX_OVERLAPPEDWINDOW),
    _style(WS_OVERLAPPEDWINDOW | WS_VISIBLE),
    _prevexstyle(NULL),
    _prevstyle(NULL),
    //_cstyle(CS_HREDRAW | CS_VREDRAW),
    //_classname(nullptr),

    _menu(NULL),
    //_menuname(nullptr),
    _background(),
    _threaddpiawarenesscontext(NULL),
    _allowfsexclusive(false),
    _cmdshow(SW_SHOWNORMAL),
    _fullscreen(false)
{
    _icon = LoadIcon(NULL, IDI_APPLICATION);
    _classcursor = LoadCursor(NULL, IDC_ARROW);
    _iconsm = reinterpret_cast<HICON>(LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));

    _background[0] = 0.000000000f;
    _background[1] = 0.000000000f;
    _background[2] = 0.000000000f;
    _background[3] = 1.000000000f; //black
    if (App::winversion >= OSVersion::WIN10_1607)
    {
        HMODULE lib = GetModuleHandle(TEXT("user32.dll"));
        if (lib)
        {
            using PFN_GET_THREAD_DPI_CONTEXT = DPI_AWARENESS_CONTEXT(WINAPI*)(void);
            PFN_GET_THREAD_DPI_CONTEXT pfngetthreaddpiawarenesscontext = (PFN_GET_THREAD_DPI_CONTEXT)GetProcAddress(lib, "GetThreadDpiAwarenessContext");
            if (pfngetthreaddpiawarenesscontext)
            {
                _threaddpiawarenesscontext = pfngetthreaddpiawarenesscontext();
            }
        }
    }
}

WindowProperties::~WindowProperties()
{
    if (_windowname != nullptr)
    {
        delete[] _windowname;
        _windowname = nullptr;
    }
    if (_menu)
    {
        DestroyMenu(_menu);
        _menu = NULL;
    }
    /*if (_menuname != nullptr)
    {
        if (!IS_INTRESOURCE(_menuname))
        {
            delete _menuname;
        }
        _menuname = nullptr;
    }*/
}

void WindowProperties::height(_In_ const int v)
{
    //(v > 1 ? v : 1) = don't allow 0 size value
    if (initialized)
    {
        _prevheight = _height;
        _height = (v > 1 ? v : 1);
        _actualheight = Scale(_height);
        SetWindowPos(win->hWnd, 0, 0, 0, _actualwidth, _actualheight, (SWP_NOMOVE | SWP_NOZORDER)); //ignore hWndInsertAfter and X, Y
    }
    else
    {
        _height = (v > 1 ? v : 1);
    }
}

void WindowProperties::width(_In_ const int v)
{
    //(v > 1 ? v : 1) = don't allow 0 size value
    if (initialized)
    {
        _prevwidth = _width;
        _width = (v > 1 ? v : 1);
        _actualwidth = Scale(_width);
        SetWindowPos(win->hWnd, 0, 0, 0, _actualwidth, _actualheight, (SWP_NOMOVE | SWP_NOZORDER)); //ignore hWndInsertAfter and X, Y
    }
    else
    {
        _width = (v > 1 ? v : 1);
    }
}

void WindowProperties::top(_In_ const int v)
{
    if (initialized)
    {
        _prevtop = _top;
        _top = v;
        SetWindowPos(win->hWnd, 0, _left, _top, 0, 0, (SWP_NOSIZE | SWP_NOZORDER)); //ignore hWndInsertAfter and cx, cy
    }
    else
    {
        _top = v;
    }
}
void WindowProperties::left(_In_ const int v)
{
    if (initialized)
    {
        _prevleft = _left;
        _left = v;
        SetWindowPos(win->hWnd, 0, _left, _top, 0, 0, (SWP_NOSIZE | SWP_NOZORDER)); //ignore hWndInsertAfter and cx, cy
    }
    else
    {
        _left = v;
    }
}

void WindowProperties::exstyle(_In_ const DWORD v)
{
    if (initialized)
    {
        /*If the function succeeds, the return value is the previous value of the specified offset.
        If the function fails, the return value is zero. To get extended error information, call GetLastError.
        If the previous value is zero and the function succeeds, the return value is zero, but the function
        does not clear the last error information. To determine success or failure, clear the last error
        information by calling SetLastError with 0, then call SetWindowLongPtr. Function failure will be
        indicated by a return value of zero and a GetLastError result that is nonzero.*/
        SetLastError(0);
        LONG_PTR ret = SetWindowLongPtr(win->hWnd, GWL_EXSTYLE, v);
        if (ret == 0)
        { //check for failure
            if (GetLastError())
            {
                //failure do not set new value
                return;
            }
        }
        _prevexstyle = _exstyle;
        _exstyle = v;
        /*Certain window data is cached, so changes you make using SetWindowLongPtr will not take
        effect until you call the SetWindowPos function.*/
        SetWindowPos(win->hWnd, 0, 0, 0, 0, 0, (SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)); //no move, resize, or zorder changes
    }
    else
    {
        _exstyle = v;
    }
}

void WindowProperties::style(_In_ const DWORD v)
{
    if (initialized)
    {
        /*If the function succeeds, the return value is the previous value of the specified offset.
        If the function fails, the return value is zero. To get extended error information, call GetLastError.
        If the previous value is zero and the function succeeds, the return value is zero, but the function
        does not clear the last error information. To determine success or failure, clear the last error
        information by calling SetLastError with 0, then call SetWindowLongPtr. Function failure will be
        indicated by a return value of zero and a GetLastError result that is nonzero.*/
        SetLastError(0);
        LONG_PTR ret = SetWindowLongPtr(win->hWnd, GWL_STYLE, v);
        if (ret == 0)
        { //check for failure
            if (GetLastError())
            {
                //failure do not set new value
                return;
            }
        }
        _prevstyle = _style;
        _style = v;
        /*Certain window data is cached, so changes you make using SetWindowLongPtr will not take
        effect until you call the SetWindowPos function.*/
        SetWindowPos(win->hWnd, 0, 0, 0, 0, 0, (SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)); //no move, resize, or zorder changes
    }
    else
    {
        _style = v;
    }
}

/*HRESULT WindowProperties::windowname(_In_ LPCTSTR v)
Takes a string and if the window is initialized will attempt to set the window text to the string
and then update _windowname with the new string value.

LPCTSTR v - source string, must be null-terminated

Return value
HRESULT
  S_FALSE if 'v' is NULL or SetWindowText fails, otherwise HRESULT of StringCchCopyEx
*/
HRESULT WindowProperties::windowname(_In_ LPCTSTR v)
{
    if (v == nullptr)
    {
        return S_FALSE;
    }
    if (initialized)
    {
        if (!SetWindowText(win->hWnd, v))
        {
            return S_FALSE;
        }
    }
    if (_windowname != nullptr)
    {
        delete[] _windowname;
    }
    size_t length = 0;
    if (FAILED(StringCchLength(v, STRSAFE_MAX_CCH - 1, &length)))
    {
        length = STRSAFE_MAX_CCH - 1; //account for last character must be null-terminator

    }
    _windowname = CRT_NEW TCHAR[length + 1];
    //copy 'v' to '_windowname' - if a failure _windowname will be empty string
    return StringCchCopyEx(_windowname, length + 1, v, nullptr, nullptr, STRSAFE_NULL_ON_FAILURE);
}

bool WindowProperties::menu(_In_ LPCTSTR v)
{
    if (v == NULL)
        return false;
    HMENU newmenu = LoadMenu(App::hInstance, v);
    if ( newmenu == 0)
    {//failure do not set new value
        return false;
    }
    if (initialized)
    {
        if (!SetMenu(win->hWnd, newmenu))
        {
            DestroyMenu(newmenu);
            return false;
        }
        //clear any cached data
        SetWindowPos(win->hWnd, 0, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)); //no move, resize, or zorder changes
    }
    if (_menu != NULL)
        DestroyMenu(_menu);
    _menu = newmenu;
    return true;
}
void WindowProperties::icon(_In_ const HICON v)
{
    /*You can override the large or small class icon for a particular window by using the
    WM_SETICON message*/
    if (initialized)
    {
        //must set ICON_SMALL first and then ICON_BIG or ICON_BIG will fail
        SendMessage(win->hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(_iconsm));
        SetLastError(0);
        SendMessage(win->hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(v));
        DWORD err = GetLastError();
        if (err == ERROR_SUCCESS)
        {
            _icon = v;
        }
    }
    else
    {
        _icon = v;
    }
}
void WindowProperties::iconsm(_In_ const HICON v)
{
    /*You can override the large or small class icon for a particular window by using the
    WM_SETICON message*/
    if (initialized)
    {
        SetLastError(0);
        //must pass both ICON_BIG and ICON_SMALL or both will change
        SendMessage(win->hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(v));
        //SetLastError(0);
        //SendMessage(win->hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(_icon));
        DWORD err = GetLastError();
        if (err == ERROR_SUCCESS)
        {
            _iconsm = v;
        }
    }
    else
    {
        _iconsm = v;
    }
}

void WindowProperties::classcursor(_In_ const HCURSOR v)
{
    if (initialized)
    {
        SetLastError(0);
        ULONG_PTR ret = SetClassLongPtr(win->hWnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(v));
        if (ret == 0)
        { //0 may still indicate a failure, run GetLastError
            if (GetLastError())
            {
                //failure do not set new value
                return;
            }
        }
        SetWindowPos(win->hWnd, 0, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)); //no move, resize, or zorder changes
        _classcursor = v;
    }
    else
    {
        _classcursor = v;
    }
}

void WindowProperties::background(_In_ const FLOAT v[4])
{
    _background[0] = v[0];
    _background[1] = v[1];
    _background[2] = v[2];
    _background[3] = v[3];
}

/*DPI_AWARENESS_CONTEXT WindowProperties::threaddpiawarenesscontext(_In_ const DPI_AWARENESS_CONTEXT v)
For Windows 10 versions 1607 and greater
This function will, if the window is not initialized, set '_threaddpiawarenesscontext' equal to 'v'
and if the window is initialized it will send a window message (WM_USER+1) to tell the window to
change it's dpi awarness context. WM_USER+1 is handled by _WndProc and will return the results of
a call to SetThreadDpiAwarenessContext cast to LRESULT. We must use SendMessage as we need to
call SetThreadDpiAwarenessContext from the window's thread.

For other versions of Windows this function does nothing and returns NULL.
*/
DPI_AWARENESS_CONTEXT WindowProperties::threaddpiawarenesscontext(_In_ const DPI_AWARENESS_CONTEXT v)
{
    LRESULT retval = NULL;
    //only update if OSVersion is greater than WIN10 1607 (Anniversary Update)
    if (WUIF::App::winversion >= WUIF::OSVersion::WIN10_1607)
    {
        //only update if the window hasn't been created
        if (initialized)
        {
             retval = SendMessage(win->hWnd, WM_USER+1, reinterpret_cast<WPARAM>(v), NULL);
             if (retval)
             {
                 _threaddpiawarenesscontext = v;
             }
        }
        else
        {
            _threaddpiawarenesscontext = v;
            return v;
        }
    }
    return reinterpret_cast<DPI_AWARENESS_CONTEXT>(retval);
}
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

WindowProperties::WindowProperties() noexcept :
    //win(winptr),
    initialized(false),
    scaleFactor(100),
    _classatom(NULL),
    _icon(NULL),
    _iconsm(NULL),
    _classcursor(NULL),
    _hWnd(NULL),
    _hWndParent(NULL),
    _exstyle(WS_EX_OVERLAPPEDWINDOW),
    _windowname(nullptr),
    _style(WS_OVERLAPPEDWINDOW),
    _left(CW_USEDEFAULT),
    _top(CW_USEDEFAULT),
    _width(CW_USEDEFAULT),
    _height(CW_USEDEFAULT),
    _menu(NULL),
    _prevexstyle(NULL),
    _prevstyle(NULL),
    _prevleft(0),
    _prevtop(0),
    _prevwidth(0),
    _prevheight(0),
    _actualwidth(0),
    _actualheight(0),
    _minwidth(0),
    _maxwidth(0),
    _minheight(0),
    _maxheight(0),

    _background(),
    _threaddpiawarenesscontext(NULL),
    _allowfsexclusive(false),
    _cmdshow(SW_SHOWNORMAL),
    _fullscreen(false) 
{
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
    //if (_windowname != nullptr)
    //{
        delete[] _windowname;
        _windowname = nullptr;
    //}
    if (_menu)
    {
        DestroyMenu(_menu);
        _menu = NULL;
    }
}

/*bool WindowProperties::classatom(_In_ const ATOM v)
sets the _classatom variable if the window has not been initialized

ATOM v - atom returned by RegisterClass

return value
bool - true if not initialized and _classatom updated
*/
bool WindowProperties::classatom(_In_ const ATOM v)
{
    if (!initialized)
    {
        _classatom = v;
        return true;
    }
    return false;
}

/*DWORD WindowProperties::icon(_In_ const HICON v)
Takes the an icon handle and applies it to the "big" icon for the window. To update the icon a
WM_SETICON message must be sent to the window.

HICON v - handle to the "big" icon

Return value
System Error value (DWORD)
*/
DWORD WindowProperties::icon(_In_ const HICON v)
{
    /*You can override the large or small class icon for a particular window by using the
    WM_SETICON message*/
    if (initialized)
    {
        //must set ICON_SMALL first and then ICON_BIG or ICON_BIG will fail
        SendMessage(_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(_iconsm));
        //clear any error message - not concerned if previous SendMessage failed
        SetLastError(0);
        SendMessage(_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(v));
        DWORD err = GetLastError();
        if (err == ERROR_SUCCESS)
        {
            _icon = v;
        }
        return err;
    }
    else
    {
        _icon = v;
        return ERROR_SUCCESS;
    }
}

/*DWORD WindowProperties::iconsm(_In_ const HICON v)
Takes the an icon handle and applies it to the "small" icon for the window. To update the icon a
WM_SETICON message must be sent to the window.

HICON v - handle to the "small" icon

Return value
System Error value (DWORD)
*/
DWORD WindowProperties::iconsm(_In_ const HICON v)
{
    /*You can override the large or small class icon for a particular window by using the
    WM_SETICON message*/
    if (initialized)
    {
        SetLastError(0);
        SendMessage(_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(v));
        DWORD err = GetLastError();
        if (err == ERROR_SUCCESS)
        {
            _iconsm = v;
        }
        return err;
    }
    else
    {
        _iconsm = v;
        return ERROR_SUCCESS;
    }
}

/*DWORD WindowProperties::iconsm(_In_ const HCURSOR v)
Updates the class cursor. This will update the cursor for the WHOLE class, not just this window!

HCURSOR v - handle to the cursor

Return value
System Error value (DWORD)
*/
DWORD WindowProperties::classcursor(_In_ const HCURSOR v)
{
    if (initialized)
    {
        SetLastError(0);
        ULONG_PTR ret = SetClassLongPtr(_hWnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(v));
        if (ret == 0)
        { //0 may still indicate a failure, run GetLastError
            DWORD err = GetLastError();
            if (err != ERROR_SUCCESS)
            {
                //failure do not set new value
                return err;
            }
        }
        //call SetWindowPos with no move, resize, or zorder changes to ensure update of cursor
        SetWindowPos(_hWnd, 0, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER));
        _classcursor = v;
        return ERROR_SUCCESS;
    }
    else
    {
        _classcursor = v;
        return ERROR_SUCCESS;
    }
}

/*DWORD WindowProperties::hWndParent(_In_ const HWND v)
Update the window's parent. If initialized this calls SetParent

HWND v - handle to the new parent window

Return value
System Error value (DWORD)
*/
DWORD WindowProperties::hWndParent(_In_ const HWND v)
{
    if (initialized)
    {
        if (_hWndParent == v)
        {
            //same - do nothing
            return ERROR_SUCCESS;
        }
        if (_hWndParent == NULL) //currently top-level
        {
            /*if hWndNewParent is not NULL and the window was previously a child of the desktop,
            you should clear the WS_POPUP style and set the WS_CHILD style before calling
            SetParent.*/
            SetLastError(0);
            DWORD newstyle = _style & ~WS_POPUP;
            newstyle |= WS_CHILD;
            DWORD err = style(newstyle);
            if (err)
                return err;
        }
        if (SetParent(_hWnd, v))
        {
            if ((_hWndParent) && (v == NULL)) //child window is now top-window
            {
                _hWndParent = v;
                SetLastError(0);
                /*if hWndNewParent is NULL, you should also clear the WS_CHILD bit and set the
                WS_POPUP style after calling SetParent.*/
                DWORD newstyle = _style & ~WS_CHILD;
                newstyle |= WS_POPUP;
                DWORD err = style(newstyle);
                if (err)
                    return err;
                /*When you change the parent of a window, you should synchronize the UISTATE of
                both windows.*/
                SendMessage(_hWnd,
                            WM_CHANGEUISTATE,
                            MAKEWPARAM(UIS_INITIALIZE, UISF_HIDEACCEL | UISF_HIDEFOCUS),
                            0);
                return GetLastError();
            }
            else
            {
                _hWndParent = v;
                /*When you change the parent of a window, you should synchronize the UISTATE of
                both windows. See https://blogs.msdn.microsoft.com/oldnewthing/20171124-00/?p=97456
                for explanation of below*/
                UINT parentstate = LOWORD(SendMessage(v, WM_QUERYUISTATE, 0, 0));
                UINT currentstate = LOWORD(SendMessage(_hWnd, WM_QUERYUISTATE, 0, 0));
                UINT missingstate = parentstate & ~currentstate;
                if (missingstate)
                {
                    SendMessage(_hWnd, WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, missingstate), 0);
                }
                UINT extrastate = currentstate & ~parentstate;
                if (extrastate)
                {
                    SendMessage(_hWnd, WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR, extrastate), 0);
                }
                return GetLastError();
            }
        }
        else
        {
            return GetLastError();
        }

    }
    else
    {
        _hWndParent = v;
        return ERROR_SUCCESS;
    }
}

/*DWORD WindowProperties::exstyle(_In_ const DWORD v)
Update the window's extended style.

DWORD v - the new extended window style

Return value
System Error value (DWORD)
*/
DWORD WindowProperties::exstyle(_In_ const DWORD v)
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
        if (!SetWindowLongPtr(_hWnd, GWL_EXSTYLE, v))
        { //check for failure
            DWORD err = GetLastError();
            if (err)
            {
                //failure do not set new value
                return err;
            }
        }
        _prevexstyle = _exstyle;
        _exstyle = v;
        /*Certain window data is cached, so changes you make using SetWindowLongPtr will not take
        effect until you call the SetWindowPos function.*/
        SetWindowPos(_hWnd, 0, 0, 0, 0, 0, (SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)); //no move, resize, or zorder changes
        return ERROR_SUCCESS;
    }
    else
    {
        _exstyle = v;
        return ERROR_SUCCESS;
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
        return E_POINTER;
    }
    if (initialized)
    {
        if (!SetWindowText(_hWnd, v))
        {
            return E_FAIL;
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

/*DWORD WindowProperties::style(_In_ const DWORD v)
Update the window's style.

DWORD v - the new window style

Return value
System Error value (DWORD)
*/
DWORD WindowProperties::style(_In_ const DWORD v)
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
        if (!SetWindowLongPtr(_hWnd, GWL_STYLE, v))
        { //check for failure
            DWORD err = GetLastError();
            if (err)
            {
                //failure do not set new value
                return err;
            }
        }
        _prevstyle = _style;
        _style = v;
        /*Certain window data is cached, so changes you make using SetWindowLongPtr will not take
        effect until you call the SetWindowPos function.*/
        SetWindowPos(_hWnd, 0, 0, 0, 0, 0, (SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)); //no move, resize, or zorder changes
        return ERROR_SUCCESS;
    }
    else
    {
        _style = v;
        return ERROR_SUCCESS;
    }
}

/*BOOL WindowProperties::left(_In_ const int v)
Update the window's x co-ordinate.

int v - the new x position

Return value
BOOL if window position updated
*/
BOOL WindowProperties::left(_In_ const int v)
{
    if (initialized)
    {
        //save our current x position and set _prevleft if SetWindowPos succeeds
        int oldposx = _left;
        if (SetWindowPos(_hWnd, 0, v, _top, 0, 0, (SWP_NOSIZE | SWP_NOZORDER))) //ignore hWndInsertAfter and cx, cy
        {
            //update our previous x position
            _prevleft = oldposx;
            return TRUE;
        }
        return FALSE;
    }
    else
    {
        _left = v;
        return TRUE;
    }
}

/*BOOL WindowProperties::top(_In_ const int v)
Update the window's y co-ordinate.

int v - the new y position

Return value
BOOL if window position updated
*/
BOOL WindowProperties::top(_In_ const int v)
{
    if (initialized)
    {
        //save our current y position and set _prevtop if SetWindowPos succeeds
        int oldposy = _top;
        if (SetWindowPos(_hWnd, 0, _left, _top, 0, 0, (SWP_NOSIZE | SWP_NOZORDER))) //ignore hWndInsertAfter and cx, cy
        {
            //update our previous y position
            _prevtop = oldposy;
            return TRUE;
        }
        return FALSE;
    }
    else
    {
        _top = v;
        return TRUE;
    }
}

/*BOOL WindowProperties::width(_In_ const int v)
Update the window's width.

int v - the new width

Return value
BOOL if window width updated
*/
BOOL WindowProperties::width(_In_ const int v)
{
    if (initialized)
    {
        int oldwidth = _width;
        //(v > 0 ? v : 1) = don't allow 0 size value
        int newwidth = Scale((v > 0) ? v : 1);
        if (SetWindowPos(_hWnd, 0, 0, 0, newwidth, _actualheight, (SWP_NOMOVE | SWP_NOZORDER))) //ignore hWndInsertAfter and X, Y
        {
            _prevwidth = oldwidth;
            _actualwidth = newwidth;
            return TRUE;
        }
        return FALSE;
    }
    else
    {
        _width = (v > 1?v:1);
        return TRUE;
    }
}

BOOL WindowProperties::height(_In_ const int v)
{
    if (initialized)
    {
        _prevheight = _height;
        //(v > 1 ? v : 1) = don't allow 0 size value
        _height = (v > 1 ? v : 1);
        _actualheight = Scale(_height);
       return SetWindowPos(_hWnd, 0, 0, 0, _actualwidth, _actualheight, (SWP_NOMOVE | SWP_NOZORDER)); //ignore hWndInsertAfter and X, Y
    }
    else
    {
        _height = (v > 1 ? v : 1);
    }
    return TRUE;
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
        if (!SetMenu(_hWnd, newmenu))
        {
            DestroyMenu(newmenu);
            return false;
        }
        //clear any cached data
        SetWindowPos(_hWnd, 0, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)); //no move, resize, or zorder changes
    }
    if (_menu != NULL)
        DestroyMenu(_menu);
    _menu = newmenu;
    return true;
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
        if (initialized)
        {
             retval = SendMessage(_hWnd, WM_USER+1, reinterpret_cast<WPARAM>(v), NULL);
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
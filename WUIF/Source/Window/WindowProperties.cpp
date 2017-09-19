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
#include <string> //needed for int to string conversion
#include "Window\Window.h"

using namespace WUIF;

WindowProperties::WindowProperties(Window *parent_window) :
	parent(parent_window),
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
	_cstyle(CS_HREDRAW | CS_VREDRAW),
	_windowname(nullptr),
	_menu(NULL),
	_menuname(nullptr),
	_icon(NULL),
	_iconsm(NULL),
	_cursor(NULL),
	_background(NULL),
	_dpiawareness(NULL),
	_allowfsexclusive(false),
	_cmdshow(SW_SHOWNORMAL)
{
	std::wstring s = L"Window" + std::to_wstring(parent->instance);
	_windowname = s.c_str();
	_icon = LoadIcon(nullptr, IDI_APPLICATION);
	_cursor = LoadCursor(nullptr, IDC_ARROW);
	_background = (HBRUSH)(COLOR_WINDOW + 1);
	HINSTANCE lib = LoadLibrary(_T("user32.dll"));
	if (lib)
	{
		using GETTHREADDPICONTEXT = DPI_AWARENESS_CONTEXT(WINAPI*)(void);
		GETTHREADDPICONTEXT getthreaddpiawarenesscontext = (GETTHREADDPICONTEXT)GetProcAddress(lib, "GetThreadDpiAwarenessContext");
		if (getthreaddpiawarenesscontext)
		{
			_dpiawareness = getthreaddpiawarenesscontext();
		}
		FreeLibrary(lib);
	}
}

void WindowProperties::height(int v)
{
	//(v > 1 ? v : 1) = don't allow 0 size value
	if (parent->initialized)
	{
		_prevheight = _height;
		_height = (v > 1 ? v : 1);
		_actualheight = parent->Scale(_height);
		SetWindowPos(parent->hWnd, 0, 0, 0, _actualwidth, _actualheight, (SWP_NOMOVE | SWP_NOZORDER)); //ignore hWndInsertAfter and X, Y
	}
	else
	{
		_height = (v > 1 ? v : 1);
	}

}

void WindowProperties::width(int v)
{
	//(v > 1 ? v : 1) = don't allow 0 size value
	if (parent->initialized)
	{
		_prevwidth = _width;
		_width = (v > 1 ? v : 1);
		_actualwidth = parent->Scale(_width);
		SetWindowPos(parent->hWnd, 0, 0, 0, _actualwidth, _actualheight, (SWP_NOMOVE | SWP_NOZORDER)); //ignore hWndInsertAfter and X, Y
	}
	else
	{
		_width = (v > 1 ? v : 1);
	}
}



void WindowProperties::top(int v)
{
	if (parent->initialized)
	{
		_prevtop = _top;
		_top = v;
		SetWindowPos(parent->hWnd, 0, _left, _top, 0, 0, (SWP_NOSIZE | SWP_NOZORDER)); //ignore hWndInsertAfter and cx, cy
	}
	else
	{
		_top = v;
	}
}
void WindowProperties::left(int v)
{
	if (parent->initialized)
	{
		_prevleft = _left;
		_left = v;
		SetWindowPos(parent->hWnd, 0, _left, _top, 0, 0, (SWP_NOSIZE | SWP_NOZORDER)); //ignore hWndInsertAfter and cx, cy
	}
	else
	{
		_left = v;
	}
}
void WindowProperties::exstyle(DWORD v)
{
	if (parent->initialized)
	{
		/*If the function succeeds, the return value is the previous value of the specified offset.
		If the function fails, the return value is zero. To get extended error information, call GetLastError.
		If the previous value is zero and the function succeeds, the return value is zero, but the function
		does not clear the last error information. To determine success or failure, clear the last error
		information by calling SetLastError with 0, then call SetWindowLongPtr. Function failure will be
		indicated by a return value of zero and a GetLastError result that is nonzero.*/
		SetLastError(0);
		LONG_PTR ret = SetWindowLongPtr(parent->hWnd, GWL_EXSTYLE, v);
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
		SetWindowPos(parent->hWnd, 0, 0, 0, 0, 0, (SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)); //no move, resize, or zorder changes
	}
	else
	{
		_exstyle = v;
	}
}
void WindowProperties::style(DWORD v)
{
	if (parent->initialized)
	{
		/*If the function succeeds, the return value is the previous value of the specified offset.
		If the function fails, the return value is zero. To get extended error information, call GetLastError.
		If the previous value is zero and the function succeeds, the return value is zero, but the function
		does not clear the last error information. To determine success or failure, clear the last error
		information by calling SetLastError with 0, then call SetWindowLongPtr. Function failure will be
		indicated by a return value of zero and a GetLastError result that is nonzero.*/
		SetLastError(0);
		LONG_PTR ret = SetWindowLongPtr(parent->hWnd, GWL_STYLE, v);
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
		SetWindowPos(parent->hWnd, 0, 0, 0, 0, 0, (SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)); //no move, resize, or zorder changes
	}
	else
	{
		_style = v;
	}
}
void WindowProperties::classstyle(UINT v)
{
	if (parent->initialized)
	{
		SetLastError(0);
		ULONG_PTR ret = SetClassLongPtr(parent->hWnd, GCL_STYLE, v);
		if (ret == 0)
		{ //check for failure
			if (GetLastError())
			{
				//failure do not set new value
				return;
			}
		}
		_cstyle = v;
		SetWindowPos(parent->hWnd, 0, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)); //no move, resize, or zorder changes
	}
	else
	{
		_cstyle = v;
	}
}
void WindowProperties::windowname(LPCTSTR v)
{
	if (parent->initialized)
	{
		if (SetWindowText(parent->hWnd, v))
		{
			_windowname = v;
		}
	}
	else
	{
		_windowname = v;
	}
}
void WindowProperties::menu(HMENU v) { _menu = v; }
void WindowProperties::menuname(LPCTSTR v)
{
	if (parent->initialized)
	{
		SetLastError(0);
		ULONG_PTR ret = SetClassLongPtr(parent->hWnd, GCLP_MENUNAME, *v);
		if (ret == 0)
		{ //check for failure
			if (GetLastError())
			{
				//failure do not set new value
				return;
			}
		}
		SetWindowPos(parent->hWnd, 0, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)); //no move, resize, or zorder changes
		_menuname = v;
	}
	else
	{
		_menuname = v;
	}
}
void WindowProperties::icon(HICON v)
{
	if (parent->initialized)
	{
		SetLastError(0);
		ULONG_PTR ret = SetClassLongPtr(parent->hWnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(v));
		if (ret == 0)
		{ //check for failure
			if (GetLastError())
			{
				//failure do not set new value
				return;
			}
		}
		SetWindowPos(parent->hWnd, 0, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)); //no move, resize, or zorder changes
		_icon = v;
	}
	else
	{
		_icon = v;
	}
}
void WindowProperties::iconsm(HICON v)
{
	if (parent->initialized)
	{
		SetLastError(0);
		ULONG_PTR ret = SetClassLongPtr(parent->hWnd, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(v));
		if (ret == 0)
		{ //check for failure
			if (GetLastError())
			{
				//failure do not set new value
				return;
			}
		}
		SetWindowPos(parent->hWnd, 0, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)); //no move, resize, or zorder changes
		_iconsm = v;
	}
	else
	{
		_iconsm = v;
	}
}
void WindowProperties::cursor(HCURSOR v)
{
	if (parent->initialized)
	{
		SetLastError(0);
		ULONG_PTR ret = SetClassLongPtr(parent->hWnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(v));
		if (ret == 0)
		{ //check for failure
			if (GetLastError())
			{
				//failure do not set new value
				return;
			}
		}
		SetWindowPos(parent->hWnd, 0, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)); //no move, resize, or zorder changes
		_cursor = v;
	}
	else
	{
		_cursor = v;
	}
}
void WindowProperties::background(HBRUSH v)
{
	if (parent->initialized)
	{
		SetLastError(0);
		ULONG_PTR ret = SetClassLongPtr(parent->hWnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(v));
		if (ret == 0)
		{ //check for failure
			if (GetLastError())
			{
				//failure do not set new value
				return;
			}
		}
		SetWindowPos(parent->hWnd, 0, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)); //no move, resize, or zorder changes
		_background = v;
	}
	else
	{
		_background = v;
	}
}

void WindowProperties::dpiawareness(DPI_AWARENESS_CONTEXT v)
{
	//only update if the window hasn't been created
	if (!parent->initialized)
	{
		_dpiawareness = v;
	}
}
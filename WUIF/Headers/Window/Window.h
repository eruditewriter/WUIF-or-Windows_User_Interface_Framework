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
#include <unordered_map>
#include <forward_list>
#include <wrl/client.h> //needed for ComPtr
#include <dxgi1_5.h>    //needed for DXGI resources
#include "WndProcThunk.h"

namespace WUIF {

	class DXResources;
	class Application;
	class Window;
	class WinStruct;

	class WindowProperties {
		friend Window;
		Window * const parent;

		int _height;
		int _width;
		int _prevheight;
		int _prevwidth;
		int _actualheight;
		int _actualwidth;
		long _minheight;
		long _minwidth;
		long _maxheight;
		long _maxwidth;
		int _top;
		int _left;
		int _prevtop;
		int _prevleft;
		DWORD _exstyle;
		DWORD _style;
		DWORD _prevexstyle;
		DWORD _prevstyle;
		UINT _cstyle; //class style - same as style initially
		LPCTSTR _windowname;
		HMENU _menu;
		LPCTSTR _menuname;
		HICON _icon;
		HICON _iconsm;
		HCURSOR _cursor;
		HBRUSH _background;
		DPI_AWARENESS_CONTEXT _dpiawareness;
		bool _allowfsexclusive;
		int _cmdshow;

	public:
		WindowProperties(Window*);

		int height() const { return _height; }
		int width() const { return _width; }
		int actualheight() const { return _actualheight; }
		int actualwidth() const { return _actualwidth; }
		long minheight() const { return _minheight; }
		long minwidth() const { return _minwidth; }
		long maxheight() const { return _maxheight; }
		long maxwidth() const { return _maxwidth; }
		int top() const { return _top; }
		int left() const { return _left; }
		DWORD exstyle() { return _exstyle; }
		DWORD style() const { return _style; }
		UINT classtyle() const { return _cstyle; }
		LPCTSTR windowname() const { return _windowname; }
		HMENU menu() const { return _menu; }
		LPCTSTR menuname() const { return _menuname; }
		HICON icon() const { return _icon; }
		HICON iconsm() const { return _iconsm; }
		HCURSOR cursor() const { return _cursor; }
		HBRUSH background() const { return _background; }
		DPI_AWARENESS_CONTEXT dpiawareness() const { return _dpiawareness; }
		bool allowfsexclusive() const { return _allowfsexclusive; }
		int cmdshow() const { return _cmdshow; }

		void minheight(long v) { _minheight = v; }
		void minwidth(long v) { _minwidth = v; }
		void maxheight(long v) { _maxheight = v; }
		void maxwidth(long v) { _maxwidth = v; }
		void allowfsexclusive(bool v) { _allowfsexclusive = v; }
		void cmdshow(int v) { _cmdshow = v; }

		void height(int v);
		void width(int v);
		void top(int v);
		void left(int v);
		void exstyle(DWORD v);
		void windowname(LPCTSTR v);
		void style(DWORD v);
		void classstyle(UINT v);
		void menu(HMENU v);
		void menuname(LPCTSTR v);
		void icon(HICON v);
		void iconsm(HICON v);
		void cursor(HCURSOR v);
		void background(HBRUSH v);
		void dpiawareness(DPI_AWARENESS_CONTEXT v);
	};

	typedef void(*winptr)(Window*);

	class Window
	{
		friend WindowProperties;
	public:
		Window();
		Window(Application*);
		~Window();

		ATOM wndclassatom;

		//class for window creation
		WindowProperties props;
		HWND		     hWnd;
		HWND             hWndParent;
		DXResources      *DXres;

		bool             enableHDR;

		//user WindowProcedure function
		typedef bool(*WndProc)(HWND, UINT, WPARAM, LPARAM);
		std::unordered_map<int, WndProc> WndProc_map;

		//next window in App.Windows
		Window* Next;

		//functions
		WNDCLASSEX wndclass(); //returns class object registered with window
		void       DisplayWindow();
		void       Present();
		void       ToggleFullScreen();
		UINT       getWindowDPI();
		int        Scale(int x) { return (_fullscreen ? x : MulDiv(x, scaleFactor, 100)); };
		bool       fullscreen() { return _fullscreen; }
		std::forward_list<winptr> drawroutines;

	private:
		long             instance;    //number of window instances created
		bool             initialized; //whether the window has been registered and created
		wndprocThunk*    thunk;	     //thunk for overloading WndProc with pointer to class object
		UINT             scaleFactor; //dpi scaling factor
		bool             _fullscreen; //is window "fullscreen"
		bool             standby;     //if window is occluded we won't present any frames

									  //functions
#if defined(_M_IX86)    //if compiling for x86
		static LRESULT CALLBACK _WndProc(Window*, HWND, UINT, WPARAM, LPARAM);
#elif defined(_M_AMD64) //if compiling for x64
		static LRESULT CALLBACK _WndProc(HWND, UINT, WPARAM, LPARAM, Window*);
#endif
		WNDPROC          pWndProc() //returns a pointer to the window's WndProc thunk
		{
			return (thunk ? thunk->GetThunkAddress() : throw WUIF_exception(TEXT("Invalid thunk pointer!")));
		};
	};

}
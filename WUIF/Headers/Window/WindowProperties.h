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

namespace WUIF {
    class Window;

    class WindowProperties {
    protected:
        //variables
        //A window's *this
        //Window * const win;

        //general variables
        bool    initialized; //whether the window has been registered and created
        UINT    scaleFactor; //dpi scaling factor

        //class properties
        ATOM    _classatom; //class for window creation
        HICON   _icon;
        HICON   _iconsm;
        HCURSOR _classcursor;

        //window properties
        //CreateWindowEx
        HWND    _hWnd;
        HWND    _hWndParent;
        DWORD   _exstyle;
        LPTSTR  _windowname;
        DWORD   _style;
        int     _left;
        int     _top;
        int     _width;
        int     _height;
        HMENU   _menu;

        //internal window properties
        DWORD   _prevexstyle;
        DWORD   _prevstyle;
        int     _prevleft;
        int     _prevtop;
        int     _prevwidth;
        int     _prevheight;
        int     _actualwidth;
        int     _actualheight;
        long    _minwidth;
        long    _maxwidth;
        long    _minheight;
        long    _maxheight;


        FLOAT   _background[4];
        DPI_AWARENESS_CONTEXT _threaddpiawarenesscontext;
        bool    _allowfsexclusive;
        int     _cmdshow;
        bool    _fullscreen; //is window "fullscreen"

    public:
        WindowProperties() noexcept;
       ~WindowProperties();

        //functions
        inline bool    isInitialized() const { return initialized; }
        inline int     Scale(int x)    const { return (_fullscreen ? x : MulDiv(x, scaleFactor, 100)); }

        //class "getter" functions
        inline ATOM    classatom()     const { return _classatom; }
        inline HICON   icon()          const { return _icon; }
        inline HICON   iconsm()        const { return _iconsm; }
        inline HCURSOR classcursor()   const { return _classcursor; }

        //window "getter" functions
        inline HWND    hWnd()          const { return _hWnd; }
        inline HWND    hWndParent()    const { return _hWndParent; }
        inline DWORD   exstyle()       const { return _exstyle; }
        inline LPCTSTR windowname()    const { return _windowname; }
        inline DWORD   style()         const { return _style; }
        inline int     left()          const { return _left; }
        inline int     top()           const { return _top; }
        inline int     width()         const { return _width; }
        inline int     height()        const { return _height; }
        inline HMENU   menu()          const { return _menu; }

        inline int     actualwidth()   const { return _actualwidth; }
        inline int     actualheight()  const { return _actualheight; }
        inline long    minwidth()      const { return _minwidth; }
        inline long    maxwidth()      const { return _maxwidth; }
        inline long    minheight()     const { return _minheight; }
        inline long    maxheight()     const { return _maxheight; }


        inline bool allowfsexclusive() const { return _allowfsexclusive; }
        inline int cmdshow()           const { return _cmdshow; }
        inline bool fullscreen()       const { return _fullscreen; }

        inline const FLOAT*            background() const { return _background; }
        inline DPI_AWARENESS_CONTEXT   threaddpiawarenesscontext() const { return _threaddpiawarenesscontext; }


        //class "setter" functions
        bool  classatom  (_In_ const ATOM v);
        DWORD icon       (_In_ const HICON v);
        DWORD iconsm     (_In_ const HICON v);
        DWORD classcursor(_In_ const HCURSOR v);

        //window "setter" functions
        DWORD   hWndParent(_In_ const HWND v);
        DWORD   exstyle   (_In_ const DWORD v);
        HRESULT windowname(_In_ LPCTSTR v);
        DWORD   style     (_In_ const DWORD v);
        BOOL    left      (_In_ const int v);
        BOOL    top       (_In_ const int v);
        BOOL    width     (_In_ const int v);
        BOOL    height    (_In_ const int v);

        inline void minheight(const long v) { _minheight = v; }
        inline void minwidth(const long v)  { _minwidth  = v; }
        inline void maxheight(const long v) { _maxheight = v; }
        inline void maxwidth(const long v)  { _maxwidth  = v; }
        inline void cmdshow(const int v)    { _cmdshow   = v; }
        //void menu(const HMENU v)     { _menu      = v; }
        inline void allowfsexclusive(const bool v) { _allowfsexclusive = v; }


        //void classstyle(_In_ const UINT v);
        //void menuname(_In_opt_ LPCTSTR v);
        bool menu(_In_ LPCTSTR v);

        void background(_In_ const FLOAT v[4]);
        DPI_AWARENESS_CONTEXT threaddpiawarenesscontext(_In_ const DPI_AWARENESS_CONTEXT v);
    };
}
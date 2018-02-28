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
        Window * const win;
        //long    instance;    //number of window instances created
        bool    initialized; //whether the window has been registered and created
        UINT    scaleFactor; //dpi scaling factor

        //class properties
        HICON   _icon;
        HICON   _iconsm;
        HCURSOR _classcursor;

        //window properties
        LPTSTR  _windowname;
        int     _height;
        int     _width;
        int     _prevheight;
        int     _prevwidth;
        int     _actualheight;
        int     _actualwidth;
        long    _minheight;
        long    _minwidth;
        long    _maxheight;
        long    _maxwidth;
        int     _top;
        int     _left;
        int     _prevtop;
        int     _prevleft;
        DWORD   _exstyle;
        DWORD   _style;
        DWORD   _prevexstyle;
        DWORD   _prevstyle;
        HMENU   _menu;

        FLOAT   _background[4];
        DPI_AWARENESS_CONTEXT _threaddpiawarenesscontext;
        bool    _allowfsexclusive;
        int     _cmdshow;
        bool    _fullscreen; //is window "fullscreen"

    public:
        WindowProperties(Window* const);
        ~WindowProperties();

        bool isInitialized()    const { return initialized; }
        int height()            const { return _height; }
        int width()             const { return _width; }
        int actualheight()      const { return _actualheight; }
        int actualwidth()       const { return _actualwidth; }
        long minheight()        const { return _minheight; }
        long minwidth()         const { return _minwidth; }
        long maxheight()        const { return _maxheight; }
        long maxwidth()         const { return _maxwidth; }
        int top()               const { return _top; }
        int left()              const { return _left; }
        DWORD exstyle()         const { return _exstyle; }
        DWORD style()           const { return _style; }
        //UINT classtyle()        const { return _cstyle; }
        LPCTSTR windowname()    const { return _windowname; }
        HMENU menu()            const { return _menu; }
        //LPCTSTR menuname()      const { return _menuname; }
        HICON icon()            const { return _icon; }
        HICON iconsm()          const { return _iconsm; }
        HCURSOR classcursor()   const { return _classcursor; }
        bool allowfsexclusive() const { return _allowfsexclusive; }
        int cmdshow()           const { return _cmdshow; }
        bool fullscreen()       const { return _fullscreen; }

        const FLOAT*            background() const { return _background; }
        DPI_AWARENESS_CONTEXT   threaddpiawarenesscontext() const { return _threaddpiawarenesscontext; }

        int  Scale(int x) { return (_fullscreen ? x : MulDiv(x, scaleFactor, 100)); }

        void minheight(const long v) { _minheight = v; }
        void minwidth(const long v)  { _minwidth  = v; }
        void maxheight(const long v) { _maxheight = v; }
        void maxwidth(const long v)  { _maxwidth  = v; }
        void cmdshow(const int v)    { _cmdshow   = v; }
        //void menu(const HMENU v)     { _menu      = v; }
        void allowfsexclusive(const bool v) { _allowfsexclusive = v; }

        void height(_In_ const int v);
        void width(_In_ const int v);
        void top(_In_ const int v);
        void left(_In_ const int v);
        void exstyle(_In_ const DWORD v);
        HRESULT windowname(_In_ LPCTSTR v);
        void style(_In_ const DWORD v);
        //void classstyle(_In_ const UINT v);
        //void menuname(_In_opt_ LPCTSTR v);
        bool menu(_In_ LPCTSTR v);
        void icon(_In_ const HICON v);
        void iconsm(_In_ const HICON v);
        void classcursor(_In_ const HCURSOR v);
        void background(_In_ const FLOAT v[4]);
        DPI_AWARENESS_CONTEXT threaddpiawarenesscontext(_In_ const DPI_AWARENESS_CONTEXT v);
    };
}
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
        friend Window;
        Window * const parent;

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
        UINT    _cstyle; //class style - same as style initially
        LPCTSTR _windowname;
        HMENU   _menu;
        LPCTSTR _menuname;
        HICON   _icon;
        HICON   _iconsm;
        HCURSOR _cursor;
        FLOAT   _background[4];
        DPI_AWARENESS_CONTEXT _dpiawareness;
        bool _allowfsexclusive;
        int  _cmdshow;

    public:
        WindowProperties(Window*);

        int height() const { return _height; }
        int width() const  { return _width; }
        int actualheight() const { return _actualheight; }
        int actualwidth()  const { return _actualwidth; }
        long minheight()   const { return _minheight; }
        long minwidth()    const { return _minwidth; }
        long maxheight()   const { return _maxheight; }
        long maxwidth()    const { return _maxwidth; }
        int top()  const { return _top; }
        int left() const { return _left; }
        DWORD exstyle()  { return _exstyle; }
        DWORD style()    const { return _style; }
        UINT classtyle() const { return _cstyle; }
        LPCTSTR windowname() const { return _windowname; }
        HMENU menu() const { return _menu; }
        LPCTSTR menuname() const { return _menuname; }
        HICON icon()     const { return _icon; }
        HICON iconsm()   const { return _iconsm; }
        HCURSOR cursor() const { return _cursor; }
        const FLOAT* background() const { return _background; }
        DPI_AWARENESS_CONTEXT dpiawareness() const { return _dpiawareness; }
        bool allowfsexclusive() const { return _allowfsexclusive; }
        int cmdshow() const { return _cmdshow; }

        void minheight(const long v) { _minheight = v; }
        void minwidth( const long v) { _minwidth = v; }
        void maxheight(const long v) { _maxheight = v; }
        void maxwidth( const long v) { _maxwidth = v; }
        void allowfsexclusive(const bool v) { _allowfsexclusive = v; }
        void cmdshow(const int v) { _cmdshow = v; }
        void menu(const HMENU v) { _menu = v; }

        void height(const int v);
        void width( const int v);
        void top( const int v);
        void left(const int v);
        void exstyle(const DWORD v);
        void windowname(const LPCTSTR v);
        void style(const DWORD v);
        void classstyle(const UINT v);
        void menuname(const LPCTSTR v);
        void icon(const HICON v);
        void iconsm(const HICON v);
        void cursor(const HCURSOR v);
        void background(const FLOAT v[4]);
        void dpiawareness(const DPI_AWARENESS_CONTEXT v);
    };
}
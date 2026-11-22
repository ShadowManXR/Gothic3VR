#pragma once
#include <windows.h>
#include <commctrl.h>
#include <algorithm>
#include <cstdint>

namespace diagnostic {
// CP 1.75 handles WM_SIZE by resizing the graphics device. Skip that message
// only during our own window resize so the swap chain retains its render size.
inline LRESULT CALLBACK MirrorResizeMessage(HWND window,UINT message,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
    if(message==WM_SIZE)return 0;
    return DefSubclassProc(window,message,w,l);
}

inline bool ResizeMirrorWindow(HWND window,int width,int height){
    RECT frame{},client{};
    MONITORINFO monitor{sizeof(monitor)};
    if(!GetWindowRect(window,&frame)||!GetClientRect(window,&client)||
       !GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor))return false;

    // Measure the actual window borders in the same DPI coordinate space as
    // SetWindowPos. Keep the requested aspect ratio if the monitor is smaller.
    const int borderWidth=(frame.right-frame.left)-(client.right-client.left);
    const int borderHeight=(frame.bottom-frame.top)-(client.bottom-client.top);
    const auto& work=monitor.rcWork;
    const int maxWidth=work.right-work.left-borderWidth;
    const int maxHeight=work.bottom-work.top-borderHeight;
    if(width<=0||height<=0||maxWidth<=0||maxHeight<=0)return false;
    if(width>maxWidth||height>maxHeight){
        if(int64_t(width)*maxHeight>int64_t(height)*maxWidth){
            height=std::max(1,MulDiv(height,maxWidth,width));width=maxWidth;
        }else{
            width=std::max(1,MulDiv(width,maxHeight,height));height=maxHeight;
        }
    }

    const int outerWidth=width+borderWidth,outerHeight=height+borderHeight;
    const int x=std::clamp(frame.left,work.left,work.right-outerWidth);
    const int y=std::clamp(frame.top,work.top,work.bottom-outerHeight);
    if(frame.left==x&&frame.top==y&&client.right==width&&client.bottom==height)return true;
    if(!SetWindowSubclass(window,MirrorResizeMessage,1,0))return false;
    const bool resized=SetWindowPos(window,nullptr,x,y,outerWidth,outerHeight,SWP_NOZORDER|SWP_NOACTIVATE)!=FALSE;
    RemoveWindowSubclass(window,MirrorResizeMessage,1);
    return resized;
}
}

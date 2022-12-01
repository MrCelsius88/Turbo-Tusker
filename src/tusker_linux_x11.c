// NOTE(Cel): CRT for now... :)
#include <stdlib.h>
#include <stdio.h>
// NOTE(Cel): But maybe hold on to this one...
#include <stdint.h>

#include <sys/mman.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* 
- TODO(Cel)
- The big TODO list.

[ ] Avoid Linking With The CRT.
 
*/

// Special thanks to this absolute chad:
// https://handmade.network/forums/articles/t/2834-tutorial_a_tour_through_xlib_and_related_technologies

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

#define internal static
#define global static
#define persist static

typedef struct
{
    Display* xDisplay;
    Window xWindow;
    XVisualInfo xVisualInfo;
    int xScreen;
    int xRootWindow;
} LinuxDisplayInfo;

global int g_gameRunning = 1;

internal void
LinuxSetSizeHint(LinuxDisplayInfo* displayInfo, int minWidth, int maxWidth, int minHeight, int maxHeight)
{
    XSizeHints hints = {0};
    if (minWidth > 0 && minHeight > 0) hints.flags |= PMinSize;
    if (maxWidth > 0 && maxHeight > 0) hints.flags |= PMaxSize;
    
    hints.min_width = minWidth;
    hints.max_width = maxWidth;
    hints.min_height = minHeight;
    hints.max_height = maxHeight;
    
    XSetWMNormalHints(displayInfo->xDisplay, displayInfo->xWindow, &hints);
}

internal int
LinuxDisplayInit(LinuxDisplayInfo* displayInfo)
{
    displayInfo->xDisplay = XOpenDisplay(0);
    if (!displayInfo->xDisplay) 
    {
        fprintf(stderr, "Unable to open X11 display!\n");
        return 1;
    }
    displayInfo->xScreen = DefaultScreen(displayInfo->xDisplay);
    displayInfo->xRootWindow = DefaultRootWindow(displayInfo->xDisplay);
    
    int screenBitDepth = 24;
    if (!XMatchVisualInfo(displayInfo->xDisplay, displayInfo->xScreen, screenBitDepth, TrueColor, &displayInfo->xVisualInfo))
    {
        fprintf(stderr, "This screen type is not supported!\n");
        return 1;
    }
    
    return 0;
}

internal int
LinuxCreateWindow(LinuxDisplayInfo* displayInfo, const char* title, int posX, int posY, int width, int height, int minWidth, int maxWidth, int minHeight, int maxHeight)
{
    XSetWindowAttributes xWindowAttributes = {0};
    xWindowAttributes.background_pixel = 0;
    xWindowAttributes.colormap = XCreateColormap(displayInfo->xDisplay, displayInfo->xRootWindow, displayInfo->xVisualInfo.visual, AllocNone);
    xWindowAttributes.event_mask = StructureNotifyMask;
    u32 attributeMask = CWBackPixel | CWColormap | CWEventMask;
    
    displayInfo->xWindow = XCreateWindow(displayInfo->xDisplay, displayInfo->xRootWindow, posX, posY, width, height, 0, displayInfo->xVisualInfo.depth, InputOutput, displayInfo->xVisualInfo.visual, attributeMask, &xWindowAttributes);
    if (!displayInfo->xWindow) 
    {
        fprintf(stderr, "Unable to create window!\n");
        return 1;
    }
    
    XStoreName(displayInfo->xDisplay, displayInfo->xWindow, title);
    LinuxSetSizeHint(displayInfo, minWidth, maxWidth, minHeight, maxHeight);
    XMapWindow(displayInfo->xDisplay, displayInfo->xWindow);
    
    XFlush(displayInfo->xDisplay);
    return 0;
}

internal void
LinuxDoEvents(LinuxDisplayInfo* displayInfo, Atom wmDelete)
{
    XEvent event;
    if (XPending(displayInfo->xDisplay) > 0)
    {
        XNextEvent(displayInfo->xDisplay, &event);
        switch (event.type)
        {
            // NOTE(Cel): Run this when WM_DELETE_WINDOW resolution fails.
            case DestroyNotify:
            {
                XDestroyWindowEvent* destroyEvent = (XDestroyWindowEvent*)&event;
                if (destroyEvent->window == displayInfo->xWindow)
                {
                    g_gameRunning = 0;
                }
            } break;
            
            
            case ClientMessage:
            {
                if (event.xclient.data.l[0] == wmDelete)
                {
                    g_gameRunning = 0;
                }
            } break;
            
        }
    }
}

int main(int argc, char** argv)
{
    int width = 800, height = 600;
    LinuxDisplayInfo displayInfo = {0};
    if (LinuxDisplayInit(&displayInfo) != 0)
    {
        fprintf(stderr, "Error creating display info!\n");
        return 1;
    }
    
    if (LinuxCreateWindow(&displayInfo, "Turbo Tusker", 0, 0, width, height, 400, 0, 300, 0) != 0)
    {
        fprintf(stderr, "Error creating window!\n");
        return 1;
    }
    
    Atom wmDelete = XInternAtom(displayInfo.xDisplay, "WM_DELETE_WINDOW", False);
    if (!XSetWMProtocols(displayInfo.xDisplay, displayInfo.xWindow, &wmDelete, 1))
    {
        fprintf(stderr, "Unable to register WM_DELETE_WINDOW"); 
    }
    while (g_gameRunning)
    {
        LinuxDoEvents(&displayInfo, wmDelete);
    }
    
    return 0;
}
#include <stdlib.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

// Special thanks to this absolute chad:
// https://handmade.network/forums/articles/t/2834-tutorial_a_tour_through_xlib_and_related_technologies

#define internal static
#define global static
#define persist static

internal void
LinuxSetSizeHint(Display* display, Window window, int minWidth, int maxWidth, int minHeight, int maxHeight)
{
    XSizeHints hints = {0};
    if (minWidth > 0 && minHeight > 0) hints.flags |= PMinSize;
    if (maxWidth > 0 && maxHeight > 0) hints.flags |= PMaxSize;
    
    hints.min_width = minWidth;
    hints.max_width = maxWidth;
    hints.min_height = minHeight;
    hints.max_height = maxHeight;
    
    XSetWMNormalHints(display, window, &hints);
}

internal Status
LinuxToggleWindowMaximize(Display* display, Window window)
{
    XClientMessageEvent event = {0};
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wmMaxH = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    Atom wmMaxV = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    
    if (wmState == None) return 0;
    
    event.type = ClientMessage;
    event.format = 32;
    event.window = window;
    event.message_type = wmState;
    event.data.l[0] = 2; // NOTE(Cel): _NET_WM_STATE_TOGGLE 2 according to spec: https://specifications.freedesktop.org/wm-spec/1.3/
    event.data.l[1] = wmMaxH;
    event.data.l[2] = wmMaxV;
    event.data.l[3] = 1;
    
    return XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, (XEvent*)&event);
}

int main(int argc, char** args)
{
    int width = 800, height = 600;
    Display* display = XOpenDisplay(0);
    if (!display) { fprintf(stderr, "Unable to open X display!\n"); exit(1); }
    int root = DefaultRootWindow(display);
    int screen = DefaultScreen(display);
    
    int screenBitDepth = 24;
    XVisualInfo visualInfo  = {0};
    if (!XMatchVisualInfo(display, screen, screenBitDepth, TrueColor, &visualInfo)) { fprintf(stderr, "Unable to match visual info!\n"); exit(1); }
    
    XSetWindowAttributes windowAttributes = {0};
    // NOTE(Cel): The attributes we set in windowAttributes also needs to be OR'd in to the attributeMask
    windowAttributes.background_pixel = 0;
    windowAttributes.colormap = XCreateColormap(display, root, visualInfo.visual, AllocNone);
    windowAttributes.event_mask = StructureNotifyMask;
    unsigned long attributeMask = CWBackPixel | CWColormap | CWEventMask;
    
    Window window = XCreateWindow(display, root, 0, 0, width, height, 0, visualInfo.depth, InputOutput, visualInfo.visual, attributeMask, &windowAttributes);
    if (!window) { fprintf(stderr, "Unable to create window!\n"); exit(1); }
    
    XStoreName(display, window, "Cavern");
    LinuxSetSizeHint(display, window, 400, 0, 300, 0);
    XMapWindow(display, window);
    
    XFlush(display);
    
    Atom wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", False);
    if (!XSetWMProtocols(display, window, &wmDelete, 1)) { fprintf(stderr, "Unable to register WM_DELETE_WINDOW"); }
    int gameRunning = 1;
    while (gameRunning)
    {
        XEvent event = {0};
        while (XPending(display) > 0)
        {
            XNextEvent(display, &event);
            switch (event.type)
            {
                case DestroyNotify:
                {
                    XDestroyWindowEvent* destroyEvent = (XDestroyWindowEvent*)&event;
                    if (destroyEvent->window == window)
                    {
                        gameRunning = 0;
                    }
                } break;
                
                case ClientMessage:
                {
                    if (event.xclient.data.l[0] == wmDelete)
                    {
                        XDestroyWindow(display, window);
                        gameRunning = 0;
                    }
                } break;
            }
        }
    }
    
    return 0;
}


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
    int width, height;
} LinuxWindowDimension;

typedef struct
{
    // NOTE(Cel): Pixels are 32-bits wide
    // Memory Order: 0x BB GG RR XX
    // Little Endian: 0x XX RR GG BB
    int pixelBitCount;
    int pixelByteCount;
    int bufferSize;
} LinuxPixmapData;

typedef struct
{
    XImage* handle;
    int width, height;
    LinuxPixmapData data;
} LinuxOffscreenBuffer;

global LinuxOffscreenBuffer g_backbuffer;

internal void
RenderGoofyGradient(LinuxOffscreenBuffer* buffer, int xOffset, int yOffset)
{
    int pitch = buffer->width * 4;
    u8* row = (u8*)buffer->handle->data;
    for (int y = 0; y < buffer->height; ++y)
    {
        u32* pixel = (u32*)row;
        for (int x = 0; x < buffer->width; ++x)
        {
            u8 red = 0;
            u8 green = (u8)(y + yOffset);
            u8 blue = (u8)(x + xOffset);
            
            *pixel++ = red << 16 | green << 8 | blue;
        }
        row += pitch;
    }
}

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

internal LinuxWindowDimension
LinuxGetWindowDimension(Display* display, Window window)
{
    LinuxWindowDimension out;
    XGetGeometry(display, window, 0, 0, 0, &out.width, &out.height, 0, 0);
    
    return out;
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

internal void
LinuxResizePixmapSection(LinuxOffscreenBuffer* buffer, Display* display, Window window, int screen, XVisualInfo visualInfo, int width, int height)
{
    // NOTE(Cel): We only need to run this if we call
    // this function more than once!
    if (buffer->handle)
    {
        munmap(buffer->handle->data, buffer->data.bufferSize);
        buffer->handle->data = NULL;
        
        XDestroyImage(buffer->handle); // NOTE(Cel): This also frees image memory!! So we need to free pixmapMemory first, and make it NULL along with the pixmapHandle pointer to the data.
    }
    
    buffer->width = width;
    buffer->height = height;
    buffer->data.pixelBitCount = 32;
    buffer->data.pixelByteCount = buffer->data.pixelBitCount / 8;
    buffer->data.bufferSize = (buffer->width * buffer->height) * buffer->data.pixelByteCount;
    
    void* pixmapMemory = mmap(0, buffer->data.bufferSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    
    buffer->handle = XCreateImage(display, visualInfo.visual, visualInfo.depth, ZPixmap, 0, pixmapMemory, buffer->width, buffer->height, buffer->data.pixelBitCount, 0);
}

internal void
LinuxDisplayBuffer(LinuxOffscreenBuffer* buffer, Display* display, Window window, GC graphicsContext, int x, int y, int width, int height)
{
    if (buffer->handle)
    {
        XPutImage(display, window, graphicsContext, buffer->handle, 0, 0, y, x, width, height);
    }
}

int main(int argc, char** args)
{
    int width = 1280, height = 720;
    Display* display = XOpenDisplay(0);
    if (!display) { fprintf(stderr, "Unable to open X display!\n"); exit(1); }
    int root = DefaultRootWindow(display);
    int screen = DefaultScreen(display);
    
    int screenBitDepth = 24;
    XVisualInfo visualInfo  = {0};
    if (!XMatchVisualInfo(display, screen, screenBitDepth, TrueColor, &visualInfo)) { fprintf(stderr, "Unable to match visual info!\n"); exit(1); }
    
    XSetWindowAttributes windowAttributes = {0};
    // NOTE(Cel): The attributes we set in windowAttributes also needs to be OR'd in to the attributeMask
    
    // TODO(Cel): Do we need these?
    // windowAttributes.bit_gravity = StaticGravity;
    // windowAttributes.background_pixel = 0;
    windowAttributes.colormap = XCreateColormap(display, root, visualInfo.visual, AllocNone);
    windowAttributes.event_mask = StructureNotifyMask;
    u32 attributeMask = CWColormap | CWEventMask;
    
    Window window = XCreateWindow(display, root, 0, 0, width, height, 0, visualInfo.depth, InputOutput, visualInfo.visual, attributeMask, &windowAttributes);
    if (!window) { fprintf(stderr, "Unable to create window!\n"); exit(1); }
    
    XStoreName(display, window, "Cavern");
    LinuxSetSizeHint(display, window, 400, 0, 300, 0);
    XMapWindow(display, window);
    
    XFlush(display);
    
    int xOffset = 0, yOffset = 0;
    LinuxResizePixmapSection(&g_backbuffer, display, window, screen, visualInfo, width, height);
    
    Atom wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", False);
    if (!XSetWMProtocols(display, window, &wmDelete, 1)) { fprintf(stderr, "Unable to register WM_DELETE_WINDOW"); }
    GC gfxContext = DefaultGC(display, screen);
    int gameRunning = 1;
    while (gameRunning)
    {
        /* EVENT LOOP */
        XEvent event;
        while (XPending(display) > 0)
        {
            XNextEvent(display, &event);
            switch (event.type)
            {
                case ConfigureNotify:
                {
                    XConfigureEvent* configureEvent = (XConfigureEvent*)&event;
                    if (configureEvent->window == window)
                    {
                        LinuxResizePixmapSection(&g_backbuffer, display, window, screen, visualInfo, configureEvent->width, configureEvent->height);
                    }
                } break;
                
                // NOTE(Cel): Run this when WM_DELETE_WINDOW resolution fails.
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
                        gameRunning = 0;
                    }
                } break;
            }
        }
        
        RenderGoofyGradient(&g_backbuffer, xOffset, yOffset);
        ++xOffset;
        // NOTE(Cel): Xlib does not have a stretchdibits equivielant, but this is not a big deal because we will be going to 
        // hardware accelerated rendering anyway.
        LinuxDisplayBuffer(&g_backbuffer, display, window, gfxContext, 0, 0, g_backbuffer.width, g_backbuffer.height);
    }
    
    return 0;
}


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

int main(void) {
    Display* display = NULL;
    int screen = 0;
    Window window = {0};
    GC gfxContext = {0};
    
    display = XOpenDisplay(0);
    screen = DefaultScreen(display);
    unsigned long black = BlackPixel(display, screen);
    unsigned long white = WhitePixel(display, screen);
    
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, 800, 600, 1, black, white);
    if(!window) { /* Window creation unsuccessful! TODO(Cel): Logging*/ }
    XSetStandardProperties(display, window, "Cavern", "Cavern", None, 0, 0, 0);
    XSelectInput(display, window, ExposureMask | StructureNotifyMask | ButtonPressMask | KeyPressMask);
    gfxContext = XCreateGC(display, window, 0, 0);
    
    XClearWindow(display, window);
    XMapRaised(display, window);
    
    XEvent event;
    KeySym key;
    char text[255];
    
    Atom wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDelete, 1);
    int running = 1;
    while (running)
    {
        XNextEvent(display, &event);
        
        switch (event.type)
        {
            case Expose:
            {
                XExposeEvent* exposeEvent = &event.xexpose;
                XColor xorange;
                XParseColor(display, DefaultColormap(display, screen), "orange", &xorange);
                XAllocColor(display, DefaultColormap(display, screen), &xorange);
                unsigned long orange = xorange.pixel;
                
                XSetForeground(display, gfxContext, orange);
                XFillRectangle(exposeEvent->display, exposeEvent->window, gfxContext, exposeEvent->x, exposeEvent->y, exposeEvent->width, exposeEvent->height);
            } break;
            
            case ConfigureNotify:
            {
                XConfigureEvent* configureEvent = &event.xconfigure;
                XResizeWindow(configureEvent->display, configureEvent->event, configureEvent->width, configureEvent->height);
            } break;
            
            case ClientMessage:
            {
                if (event.xclient.data.l[0] == wmDelete)
                    running = 0;
            } break;
        }
    }
    
    XFreeGC(display, gfxContext);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
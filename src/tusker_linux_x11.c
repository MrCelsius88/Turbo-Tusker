// NOTE(Cel): CRT for now... :)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// NOTE(Cel): But maybe hold on to these...
#include <stdint.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <GL/glx.h>

// Special thanks to these absolute chads for Xlib help:
// https://handmade.network/forums/articles/t/2834-tutorial_a_tour_through_xlib_and_related_technologies
// https://github.com/gamedevtech/X11OpenGLWindow

// Glx loading resources:
// https://registry.khronos.org/OpenGL/extensions/ARB/GLX_ARB_create_context.txt

// Gamepad input resources used:
// https://docs.kernel.org/input/input.html

/* TODO Known bugs: 
- Using wireless bluetooth controllers.
*/

#define internal static
#define global static
#define persist static

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

typedef struct
{
    Display* xDisplay;
    Window xWindow;
    XVisualInfo* xVisualInfo;
    int xScreen;
    int xRootWindow;
} LinuxDisplayInfo;

global bool g_gameRunning = true;

#define GLX_CREATE_CONTEXT_ATTRIBS_ARB(name) GLXContext name(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list)
typedef GLX_CREATE_CONTEXT_ATTRIBS_ARB(glx_create_context_attribs_arb);
GLX_CREATE_CONTEXT_ATTRIBS_ARB(glXCreateContextAttribsARBStub)
{
    return NULL;
}
global glx_create_context_attribs_arb* glXCreateContextAttribsARB_ = glXCreateContextAttribsARBStub;
#define glXCreateContextAttribsARB glXCreateContextAttribsARB_

internal void
LinuxLoadGlxFuncs(void)
{
    glXCreateContextAttribsARB = glXGetProcAddressARB("glXCreateContextAttribsARB");
    if (!glXCreateContextAttribsARB) { glXCreateContextAttribsARB = glXCreateContextAttribsARBStub; }
}

internal bool
LinuxCheckOpenGLExtension(const char* extList, const char* extension)
{
    const char *start, *where, *terminator;
    
    // NOTE(Cel): Extension names cannot have spaces
    where = strchr(extension, ' ');
    if (where || *extension == '\0')
        return false;
    
    for (start=extList;;)
    {
        where = strstr(start, extension);
        if (!where) break;
        
        terminator = where + strlen(extension);
        
        if (where == start || *(where - 1) == ' ')
        {
            if (*terminator == ' ' || *terminator == '\0')
                return true;
        }
        
        start = terminator;
    }
    
    return false;
}

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
    
    // TODO(Cel): We cant use this because of OpenGL
#if 0
    int screenBitDepth = 24;
    if (!XMatchVisualInfo(displayInfo->xDisplay, displayInfo->xScreen, screenBitDepth, TrueColor, &displayInfo->xVisualInfo))
    {
        fprintf(stderr, "This screen type is not supported!\n");
        return 1;
    }
#endif
    return 0;
}

internal int
LinuxCreateWindow(LinuxDisplayInfo* displayInfo, const char* title, int posX, int posY, int width, int height, int minWidth, int maxWidth, int minHeight, int maxHeight)
{
    XSetWindowAttributes xWindowAttributes = {0};
    xWindowAttributes.border_pixel = BlackPixel(displayInfo->xDisplay, displayInfo->xScreen);
    xWindowAttributes.background_pixel = BlackPixel(displayInfo->xDisplay, displayInfo->xScreen);
    xWindowAttributes.override_redirect = True;
    xWindowAttributes.colormap = XCreateColormap(displayInfo->xDisplay, displayInfo->xRootWindow, displayInfo->xVisualInfo->visual, AllocNone);
    xWindowAttributes.event_mask = ExposureMask | StructureNotifyMask;
    u32 attributeMask = CWBackPixel | CWColormap | CWBorderPixel | CWEventMask;
    
    displayInfo->xWindow = XCreateWindow(displayInfo->xDisplay, displayInfo->xRootWindow, posX, posY, width, height, 0, displayInfo->xVisualInfo->depth, InputOutput, displayInfo->xVisualInfo->visual, attributeMask, &xWindowAttributes);
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
                    g_gameRunning = false;
                }
            } break;
            
            case ConfigureNotify:
            {
                XConfigureEvent* configureEvent = (XConfigureEvent*)&event;
                if (configureEvent->window == displayInfo->xWindow)
                {
                    glViewport(0, 0, configureEvent->width, configureEvent->height);
                }
            } break;
            
            case ClientMessage:
            {
                if (event.xclient.data.l[0] == wmDelete)
                {
                    g_gameRunning = false;
                }
            } break;
            
        }
    }
}

internal int
LinuxInitOpenGLWindow(LinuxDisplayInfo* displayInfo)
{
    int majorGLX, minorGLX;
    glXQueryVersion(displayInfo->xDisplay, &majorGLX, &minorGLX);
    // TODO(Cel): Maybe bump this down to version 1.2...
    if (majorGLX <= 1 && minorGLX < 4)
    {
        fprintf(stderr, "A GLX Version of 1.4 or greater is required!\n");
        return 1;
    }
    else
    {
        fprintf(stderr, "GLX validated. Version: %d.%d\n", majorGLX, minorGLX);
    }
    
    int glxAttributes[] =
    {
        GLX_X_RENDERABLE,  True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE,      8,
        GLX_GREEN_SIZE,    8,
        GLX_BLUE_SIZE,     8,
        GLX_ALPHA_SIZE,    8,
        GLX_DEPTH_SIZE,    24,
        GLX_STENCIL_SIZE,  8,
        GLX_DOUBLEBUFFER,  True,
        None
    };
    
    int fbCount;
    GLXFBConfig* fbc = glXChooseFBConfig(displayInfo->xDisplay, displayInfo->xScreen, glxAttributes, &fbCount);
    if (fbc == 0)
    {
        fprintf(stderr, "Unable to retrieve framebuffer!\n");
        return 1;
    }
    else
    {
        fprintf(stderr, "Found %d matching framebuffers.\n", fbCount);
    }
    
    displayInfo->xVisualInfo = glXGetVisualFromFBConfig(displayInfo->xDisplay, fbc[0]);
    
    if (LinuxCreateWindow(displayInfo, "Turbo Tusker", 0, 0, 800, 600, 400, 0, 300, 0) != 0)
    {
        fprintf(stderr, "Error creating window!\n");
        return 1;
    }
    
    int contextAttributes[] =
    {
        // NOTE(Cel): Makes testing easier, switch back to ES when vs and fs are created.
        GLX_CONTEXT_MAJOR_VERSION_ARB, 2, // 3
        GLX_CONTEXT_MINOR_VERSION_ARB, 1, // 0
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, 
        //GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_ES_PROFILE_BIT_EXT,
        None
    };
    
    GLXContext glxContext = 0;
    glxContext = glXCreateContextAttribsARB(displayInfo->xDisplay, fbc[0], 0, true, contextAttributes);
    if (glxContext == NULL)
    {
        fprintf(stderr, "Unable to obtain glx context!\n");
        return 1;
    }
    
    XSync(displayInfo->xDisplay, False);
    
    if (!glXIsDirect(displayInfo->xDisplay, glxContext))
    {
        fprintf(stderr, "Unable to obtain direct rendering context!\n");
    }
    else
    {
        fprintf(stderr, "Obtained direct rending context.\n");
    }
    
    glXMakeCurrent(displayInfo->xDisplay, displayInfo->xWindow, glxContext);
    
    fprintf(stderr, "OpenGL successfully initialized!\n");
    fprintf(stderr, "GL Vendor: %s\n",           glGetString(GL_VENDOR));
    fprintf(stderr, "GL Renderer: %s\n",         glGetString(GL_RENDERER));
    fprintf(stderr, "GL Version: %s\n",          glGetString(GL_VERSION));
    fprintf(stderr, "GL Shading Language: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    return 0;
}

internal Atom
LinuxGetCloseMessage(LinuxDisplayInfo* displayInfo)
{
    Atom wmDelete = XInternAtom(displayInfo->xDisplay, "WM_DELETE_WINDOW", False);
    if (!XSetWMProtocols(displayInfo->xDisplay, displayInfo->xWindow, &wmDelete, 1))
    {
        fprintf(stderr, "Unable to register WM_DELETE_WINDOW"); 
    }
    
    return wmDelete;
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
    
    // NOTE(Cel): We want to do this before we call XMapWindow();
    // NOTE(Cel): LinuxInitOpenGL works different than Casey's Win32InitOpenGL
    // because GLX requires setting up window creating AND operating on window after it is created
    // the InitOpenGL function will ALSO create the window.
    LinuxLoadGlxFuncs();
    if (LinuxInitOpenGLWindow(&displayInfo) != 0)
    {
        fprintf(stderr, "Error initializing OpenGL!\n");
        return 1;
    }
    
    Atom wmDelete = LinuxGetCloseMessage(&displayInfo);
    
    int gamepad = open("/dev/input/by-id/usb-Microsoft_Controller_30394E4F30343234353133303133-event-joystick", O_RDONLY | O_NONBLOCK);
    int gamepadStickLeftX = 0;
    int gamepadStickLeftY = 0;
    bool gamepadButtonA = 0;
    bool gamepadButtonB = 0;
    
    persist f32 testValue = 0;
    
    while (g_gameRunning)
    {
        LinuxDoEvents(&displayInfo, wmDelete);
        
        struct input_event events[2]; // TODO(Cel): Change this to something smaller like 1, 2, or 4 at max.
        int bufferRead = read(gamepad, events, sizeof(events));
        // NOTE(Cel): Besides an error, -1 can also mean that there are no input events in the buffer.
        if (bufferRead != -1) 
        {
            int eventCount = bufferRead / sizeof(struct input_event);
            for (int eventIndex = 0; eventIndex < eventCount; ++eventIndex)
            {
                struct input_event* event = &events[eventIndex];
                switch (event->type)
                {
                    case EV_ABS: // Joystick movement
                    {
                        switch (event->code)
                        {
                            case ABS_X: { gamepadStickLeftX = event->value; } break;
                            case ABS_Y: { gamepadStickLeftY = event->value; } break;
                        }
                    } break;
                    
                    case EV_KEY:
                    {
                        switch (event->code)
                        {
                            case BTN_A: { gamepadButtonA = event->value; } break;
                            case BTN_B: { gamepadButtonB = event->value; } break;
                        }
                    } break;
                }
            }
        }
        
        if (gamepadButtonA != 0 || gamepadButtonB != 0)
            fprintf(stderr, "A: %d B: %d Left stick: %d, %d\n", gamepadButtonA, gamepadButtonB, gamepadStickLeftX, gamepadStickLeftY);
        
        if (gamepadButtonA)
        {
            testValue+=.01f;
        }
        else if (gamepadButtonB)
        {
            testValue-=.01f;
        }
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glBegin(GL_TRIANGLES);
        
        glColor3f(  1.0f,  testValue,  0.0f);
        glVertex3f(-0.5f, -0.5f,  0.0f);
        glColor3f(  0.0f,  testValue,  0.0f);
        glVertex3f( 0.5f, -0.5f,  0.0f);
        glColor3f(  0.0f,  testValue,  1.0f);
        glVertex3f( 0.0f,  0.5f,  0.0f);
        
        glEnd();
        
        glXSwapBuffers(displayInfo.xDisplay, displayInfo.xWindow);
        
    }
    
    return 0;
}
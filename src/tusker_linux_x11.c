/*
  TODO(Cel): The big platform layer TODO list.
  (Taken from Casey's TODO list.)

  - Saved game locations
  - Getting a handle to our own executable file
  - Asset loading path
  - Multithreading (launch a thread)
  - Raw Input (support for multiple keyboards)
  - Sleep/timeBeginPeriod
  - ClipCursor() (for multi-monitor support)
  - Fullscreen support
  - Get keyboard layout (for French keyboards, international WASD support)
*/

// TODO(Cel): I'm not gonna pretend like I know what im doing with audio.
// i'll throw in the towel here, but i'll be back >:) (prob when I have multithreading).

// None of this would be possible without Handmade Hero and
// https://yakvi.github.io/handmade-hero-notes/html/day7.html

// Special thanks to this absolute chad for Xlib help:
// https://handmade.network/forums/articles/t/2834-tutorial_a_tour_through_xlib_and_related_technologies

// Glx loading resources:
// https://registry.khronos.org/OpenGL/extensions/ARB/GLX_ARB_create_context.txt
// https://github.com/gamedevtech/X11OpenGLWindow

// Gamepad input resources used:
// https://docs.kernel.org/input/input.html
// https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
// https://github.com/MysteriousJ/Joystick-Input-Examples/blob/main/src/evdev.cpp
// https://handmade.network/forums/t/3673-modern_way_to_read_gamepad_input_with_c_on_linux
// https://docs.kernel.org/input/gamepad.html

// PNG loading:
// https://www.youtube.com/watch?v=lkEWbIUEuN0&list=PLEMXAbCVnmY5y4dSkKf297tJqwMxJOk7e

/* TODO Known bugs: 
- Using wireless bluetooth controllers.
*/

#define internal static
#define global static
#define persist static

#define LINUX_GAMEPAD_BUTTON_INDEX_OFFSET 0x130

#define LINUX_GAMEPAD_MAX_BUTTONS 5
#define LINUX_GAMEPAD_MAX_AXES 4
#define LINUX_MAX_GAMEPADS 4

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define LINUX_CLOCK CLOCK_MONOTONIC

#define M_PI 3.14159265358979323846
#define FPS 30

#include <stdint.h>
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

// NOTE(Cel): All this stuff is not linux-specific,
// so our game code can know about it.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <math.h> // Im so lazy lmao

// TODO(Cel): We will abstract renderer later
// and our game code should not know if
// OpenGL exists or not. So move this when
// that happens.
#include <GL/glx.h>

#include "tusker.c"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <linux/input.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

typedef struct
{
    Window xWindow;
    Display* xDisplay;
    XVisualInfo* xVisualInfo;
    int xScreen;
    int xRootWindow;
} LinuxDisplayInfo;

typedef struct
{
    s32 min, max;
    f32 value;
} LinuxGamepadAxis;

typedef struct
{
    bool connected;
    bool buttons[LINUX_GAMEPAD_MAX_BUTTONS];
    LinuxGamepadAxis axes[LINUX_GAMEPAD_MAX_AXES];
    char name[128];
    int file;
} LinuxGamepad;

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
    glXCreateContextAttribsARB = (glx_create_context_attribs_arb*)glXGetProcAddressARB("glXCreateContextAttribsARB");
    if (!glXCreateContextAttribsARB) { glXCreateContextAttribsARB = glXCreateContextAttribsARBStub; }
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

// NOTE(Cel): SDL2 source was rlly helpful here:
// https://github.com/zielmicha/SDL2/blob/master/src/timer/unix/SDL_systimer.c
internal u64
LinuxGetPerformaceCounter(void)
{
    u64 ticks;
    struct timespec currentTime;
    clock_gettime(LINUX_CLOCK, &currentTime);
    ticks = currentTime.tv_sec * 1000000000;
    ticks += currentTime.tv_nsec;
    return ticks;
}

internal u64
LinuxGetPerformaceFrequency(void)
{
    return 1000000000;
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
LinuxDisplayInit(LinuxDisplayInfo* displayInfo, bool createVisualInfo)
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
    if (createVisualInfo)
    {
        int screenBitDepth = 24;
        if (!XMatchVisualInfo(displayInfo->xDisplay, displayInfo->xScreen, screenBitDepth, TrueColor, displayInfo->xVisualInfo))
        {
            fprintf(stderr, "This screen type is not supported!\n");
            return 1;
        }
    }
    return 0;
}

internal void
LinuxOpenGamepads(LinuxGamepad* gamepads, int numGamepads)
{
    char event[32];
    int gamepadIndex = 0;
    for (int eventIndex = 0; eventIndex <= 256; ++eventIndex)
    {
        sprintf(event, "/dev/input/event%d", eventIndex);
        int file = open(event, O_RDWR | O_NONBLOCK);
        if (file != -1)
        {
            if (gamepads[gamepadIndex].file == file) continue;

            LinuxGamepad gamepad = {0};
            gamepad.connected = true;
            gamepad.file = file;
            // NOTE(Cel): Get controller name
            ioctl(file, EVIOCGNAME(sizeof(gamepad.name)), gamepad.name);

            // NOTE(Cel): Setup controller axis
            for (int i = 0; i < LINUX_GAMEPAD_MAX_AXES; ++i)
            {
                struct input_absinfo axisInfo;
                if (ioctl(file, EVIOCGABS(i), &axisInfo) != -1)
                {
                    gamepad.axes[i].min = axisInfo.minimum;
                    gamepad.axes[i].max = axisInfo.maximum;
                }
            }

            gamepads[gamepadIndex] = gamepad;
            if (gamepadIndex < numGamepads) gamepadIndex++;
        }
    }
}

internal void
LinuxGetGamepadInput(LinuxGamepad* gamepad)
{
    struct input_event event;
    while (read(gamepad->file, &event, sizeof(event)) > 0)
    {
        if (event.type == EV_KEY && event.code >= BTN_SOUTH && event.code <= BTN_WEST)
        {
            gamepad->buttons[event.code-LINUX_GAMEPAD_BUTTON_INDEX_OFFSET] = event.value;
        }
        if (event.type == EV_ABS && event.code < ABS_TOOL_WIDTH)
        {
            LinuxGamepadAxis* axis = &gamepad->axes[event.code];
            // TODO(Cel): Tweak this:
            f32 axisNormalized = (event.value - axis->min) / (f32)(axis->max - axis->min) * 2 - 1;
            axis->value = axisNormalized;
        }
    }
}

internal void
LinuxCloseGamepad(LinuxGamepad* gamepad)
{
    if (gamepad->connected)
    {
        close(gamepad->file);
        gamepad->connected = false;
    }
}

internal void
linuxCloseGamepads(LinuxGamepad* gamepads, int numGamepads)
{
    for (int i = 0; i < numGamepads; ++i)
    {
        LinuxCloseGamepad(&gamepads[i]);
    }
}

internal void
LinuxProcessGamepadDigitalButton(LinuxGamepad* gamepad, GameButtonState* oldState, u16 buttonCode, GameButtonState* newState)
{
    newState->endedDown = gamepad->buttons[buttonCode-LINUX_GAMEPAD_BUTTON_INDEX_OFFSET];
    newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;
}

internal void
LinuxSwapGameInput(GameInput** oldInput, GameInput** newInput)
{
    GameInput* temp = *newInput;
    *newInput = *oldInput;
    *oldInput = temp;
}

internal int
LinuxCreateWindow(LinuxDisplayInfo* displayInfo, const char* title, int posX, int posY, int width, int height, int minWidth, int maxWidth, int minHeight, int maxHeight)
{
    // Window Attributes
    XSetWindowAttributes xWindowAttributes = {0};
    xWindowAttributes.border_pixel = BlackPixel(displayInfo->xDisplay, displayInfo->xScreen);
    xWindowAttributes.background_pixel = BlackPixel(displayInfo->xDisplay, displayInfo->xScreen);
    xWindowAttributes.override_redirect = True;
    xWindowAttributes.colormap = XCreateColormap(displayInfo->xDisplay, displayInfo->xRootWindow, displayInfo->xVisualInfo->visual, AllocNone);
    xWindowAttributes.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask;
    u32 attributeMask = CWBackPixel | CWColormap | CWBorderPixel | CWEventMask;
    
    // Window Creation
    displayInfo->xWindow = XCreateWindow(displayInfo->xDisplay, displayInfo->xRootWindow, posX, posY, width, height, 0, displayInfo->xVisualInfo->depth, InputOutput, displayInfo->xVisualInfo->visual, attributeMask, &xWindowAttributes);
    if (!displayInfo->xWindow) 
    {
        fprintf(stderr, "Unable to create window!\n");
        return 1;
    }
    
    // Window Properties
    XStoreName(displayInfo->xDisplay, displayInfo->xWindow, title);
    LinuxSetSizeHint(displayInfo, minWidth, maxWidth, minHeight, maxHeight);
    
    // Display Window
    XMapWindow(displayInfo->xDisplay, displayInfo->xWindow);
    // Apply Changes
    XFlush(displayInfo->xDisplay);
    return 0;
}

internal int
LinuxInitOpenGLWindow(LinuxDisplayInfo* displayInfo, const char* title, int posX, int posY, int width, int height, int minWidth, int maxWidth, int minHeight, int maxHeight)
{
    LinuxLoadGlxFuncs();
    
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
    
    if (LinuxCreateWindow(displayInfo, title, posX, posY, width, height, minWidth, maxWidth, minHeight, maxHeight) != 0)
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
        fprintf(stderr, "Unable to obtain glx context! (likely glXCreateContextAttribsARB could not be loaded.)\n");
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

internal void
LinuxDoEvents(LinuxDisplayInfo* displayInfo, Atom wmDelete)
{
    XEvent event;
    if (XPending(displayInfo->xDisplay) > 0)
    {
        XNextEvent(displayInfo->xDisplay, &event);
        switch (event.type)
        {
            // TODO(Cel): Make this work, the way this is set up currently is sub-optimal. 
            // Might wanna check out this function: https://tronche.com/gui/x/xlib/input/XQueryKeymap.html
            case KeyPress: 
            case KeyRelease:
            {
                XKeyPressedEvent* keyEvent = (XKeyPressedEvent*)&event;
                
                if (keyEvent->keycode == XKeysymToKeycode(displayInfo->xDisplay, XK_W)) 
                {
                    
                }
                if (keyEvent->keycode == XKeysymToKeycode(displayInfo->xDisplay, XK_A)) 
                {
                    
                }
                if (keyEvent->keycode == XKeysymToKeycode(displayInfo->xDisplay, XK_S)) 
                {
                    
                }
                if (keyEvent->keycode == XKeysymToKeycode(displayInfo->xDisplay, XK_D)) 
                {
                    
                }
                if (keyEvent->keycode == XKeysymToKeycode(displayInfo->xDisplay, XK_Q)) 
                {
                    
                }
                if (keyEvent->keycode == XKeysymToKeycode(displayInfo->xDisplay, XK_E)) 
                {
                    
                }
                if (keyEvent->keycode == XKeysymToKeycode(displayInfo->xDisplay, XK_space)) 
                {
                    
                }
                
            } break;
            
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

int main(int argc, char** argv)
{
    int width = 800, height = 600;
    
    LinuxDisplayInfo displayInfo = {0};
    if (LinuxDisplayInit(&displayInfo, false) != 0)
    {
        fprintf(stderr, "Error creating display info!\n");
        return 1;
    }
    
    // NOTE(Cel): We want to do this before we call XMapWindow();
    // NOTE(Cel): LinuxInitOpenGL works different than Casey's Win32InitOpenGL
    // because GLX requires setting up window creating AND operating on window after it is created
    // the InitOpenGL function will ALSO create the window.
    if (LinuxInitOpenGLWindow(&displayInfo, "Turbo Tusker", 0, 0, width, height, 400, 0, 300, 0) != 0)
    {
        fprintf(stderr, "Error initializing OpenGL!\n");
        return 1;
    }
    Atom wmDelete = LinuxGetCloseMessage(&displayInfo);

    LinuxGamepad gamepads[LINUX_MAX_GAMEPADS] = {0};
    LinuxOpenGamepads(gamepads, LINUX_MAX_GAMEPADS);

    GameInput input[2] = {0};
    GameInput* oldInput = &input[0];
    GameInput* newInput = &input[1];
     int numControllers = LINUX_MAX_GAMEPADS;
    if (numControllers > ArrayCount(newInput->controllers)) numControllers = ArrayCount(newInput->controllers);

    u64 performaceFrequency = LinuxGetPerformaceFrequency();
    u64 lastCounter = LinuxGetPerformaceCounter();
    while (g_gameRunning)
    {
        LinuxDoEvents(&displayInfo, wmDelete);
        
        for (int i = 0; i < numControllers; ++i)
        {
            if (gamepads[i].connected)
            {
                LinuxGetGamepadInput(&gamepads[i]);

                GameControllerInput* oldController = &oldInput->controllers[i];
                GameControllerInput* newController = &newInput->controllers[i];

                // NOTE(Cel):
                LinuxProcessGamepadDigitalButton(&gamepads[i], &oldController->south, BTN_A, &newController->south);
                LinuxProcessGamepadDigitalButton(&gamepads[i], &oldController->east, BTN_B, &newController->east);
                LinuxProcessGamepadDigitalButton(&gamepads[i], &oldController->north, BTN_Y, &newController->north);
                LinuxProcessGamepadDigitalButton(&gamepads[i], &oldController->west, BTN_X, &newController->west);
                newController->startX = oldController->endX;
                newController->startX = oldController->endY;
                newController->minX = newController->maxX = newController->endX = gamepads[i].axes[0].value;
                newController->minY = newController->maxY = newController->endY = gamepads[i].axes[1].value;
                newController->isAnalog = true;

                #if 0
                fprintf(stderr, "%s: | ", gamepads[0].name);
                fprintf(stderr, "X1 Axis: %f", gamepads[0].axes[0].value);
                fprintf(stderr, "Y1 Axis: %f", gamepads[0].axes[1].value);
                fprintf(stderr, "A Button: %d | ", gamepads[0].buttons[0]);
                fprintf(stderr, "B Button: %d | ", gamepads[0].buttons[1]);
                fprintf(stderr, "X Button: %d | ", gamepads[0].buttons[3]); //NOTE(Cel:) What even is a 'c' button anyway?
                fprintf(stderr, "Y Button: %d | ", gamepads[0].buttons[4]);
                fprintf(stderr, "\n");
                #endif
            }
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        GameUpdateAndRender(newInput);
        glXSwapBuffers(displayInfo.xDisplay, displayInfo.xWindow);

        LinuxSwapGameInput(&oldInput, &newInput);

        u64 currentCounter = LinuxGetPerformaceCounter();

        u64 counterElapsed = currentCounter - lastCounter;
        f32 msPerFrame = ((f32)(1000.f * counterElapsed) / (f32)performaceFrequency);
        f32 framesPerSecond = (f32)performaceFrequency / (f32)counterElapsed;

        // fprintf(stderr, "ms/frame: %.02fms | f/s: %.02f\n", msPerFrame, framesPerSecond);

        lastCounter = currentCounter;
    }


    return 0;
}

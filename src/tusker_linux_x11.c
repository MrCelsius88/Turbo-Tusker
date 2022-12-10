// NOTE(Cel): CRT for now... :)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// NOTE(Cel): But maybe hold on to these...
#include <stdint.h>
#include <stdbool.h>

#include <math.h> // Im so lazy lmao

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

#include <alsa/asoundlib.h>

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

// Audio (ALSA) resources used:
// https://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html
// https://alexvia.com/post/003_alsa_playback/
// https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html
// https://github.com/AlexViaColl/alsa-playback/blob/main/main.c

// PNG loading:
// https://www.youtube.com/watch?v=lkEWbIUEuN0&list=PLEMXAbCVnmY5y4dSkKf297tJqwMxJOk7e

/* TODO Known bugs: 
- Using wireless bluetooth controllers.
*/

#define internal static
#define global static
#define persist static

#define M_PI 3.14159265358979323846
#define FPS 30

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
    Window xWindow;
    Display* xDisplay;
    XVisualInfo* xVisualInfo;
    int xScreen;
    int xRootWindow;
} LinuxDisplayInfo;

typedef struct
{
    snd_pcm_t* handle;
    int nChannels;
    int samplesPerSecond;
    int bytesPerSample;
    int bufferSizeFrames;
    int bufferSizeBytes;
} LinuxAudioData;

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

internal void
LinuxCheckAlsa(int retValue)
{
    if (retValue < 0)
    {
        fprintf(stderr, "ALSA Error: %s\n", snd_strerror(retValue));
        exit(1);
    }
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
LinuxInitAudio(LinuxAudioData* audioData)
{
    LinuxCheckAlsa(snd_pcm_open(&audioData->handle, "default", SND_PCM_STREAM_PLAYBACK, 0));
    
    snd_pcm_hw_params_t* hwParams;
    snd_pcm_hw_params_alloca(&hwParams);
    
    LinuxCheckAlsa(snd_pcm_hw_params_any(audioData->handle, hwParams));
    LinuxCheckAlsa(snd_pcm_hw_params_set_access(audioData->handle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED));
    LinuxCheckAlsa(snd_pcm_hw_params_set_format(audioData->handle, hwParams, SND_PCM_FORMAT_S16_LE));
    LinuxCheckAlsa(snd_pcm_hw_params_set_channels(audioData->handle, hwParams, audioData->nChannels));
    LinuxCheckAlsa(snd_pcm_hw_params_set_rate(audioData->handle, hwParams, audioData->samplesPerSecond, 0));
    LinuxCheckAlsa(snd_pcm_hw_params_set_buffer_size(audioData->handle, hwParams, audioData->bufferSizeFrames));
    LinuxCheckAlsa(snd_pcm_hw_params(audioData->handle, hwParams));
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
    if (LinuxInitOpenGLWindow(&displayInfo, "Turbo Tusker", 0, 0, 800, 600, 400, 0, 300, 0) != 0)
    {
        fprintf(stderr, "Error initializing OpenGL!\n");
        return 1;
    }
    Atom wmDelete = LinuxGetCloseMessage(&displayInfo);
    
    LinuxAudioData audioData = {0};
    audioData.nChannels = 2;
    audioData.samplesPerSecond = 48000;
    audioData.bytesPerSample = sizeof(s16) * audioData.nChannels;
    audioData.bufferSizeFrames = audioData.samplesPerSecond;
    audioData.bufferSizeBytes = audioData.bufferSizeFrames * audioData.bytesPerSample;
    LinuxInitAudio(&audioData);
    
    int gamepad = open("/dev/input/by-id/usb-Microsoft_Controller_30394E4F30343234353133303133-event-joystick", O_RDONLY | O_NONBLOCK);
    f32 gamepadStickLeftX = 0;
    f32 gamepadStickLeftY = 0;
    bool gamepadButtonA = 0;
    bool gamepadButtonB = 0;
    
    s16 audioBuffer[audioData.bufferSizeBytes];
    
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
        
        // NOTE(Cel): Audio Test
        // NOTE(Cel): The program will wait for snd_pcm_writei to finish, so in order to
        // continue program execution we can either run this from a seperate thread, 
        // or to avoid complexity, only write 1 frame worth of data.
        int toneHz = 256;
        int toneVolume = 3000;
        u32 runningSampleIndex = 0;
        s16* currentSample = audioBuffer;
        
        snd_pcm_sframes_t avalible, delay;
        snd_pcm_avail_delay(audioData.handle, &avalible, &delay);
        int samplesPerFrame = audioData.samplesPerSecond / 30.f;
        samplesPerFrame -= delay;
        
        if (samplesPerFrame > avalible)
        {
            samplesPerFrame = avalible;
        }
        
        for (int i = 0; i < audioData.bufferSizeFrames; ++i)
        {
            f32 t = 2.f * M_PI * (f32)runningSampleIndex / ((f32)audioData.samplesPerSecond / (f32)toneHz);
            f32 sineValue = sinf(t);
            s16 sampleValue = (s16)(sineValue * toneVolume);
            
            *currentSample++ = sampleValue;
            *currentSample++ = sampleValue;
            
            runningSampleIndex++;
        }
        
        snd_pcm_writei(audioData.handle, audioBuffer, samplesPerFrame);
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glBegin(GL_TRIANGLES);
        
        glColor3f(  1.f,  0.0f,  0.0f);
        glVertex3f(-0.5f, -0.5f,  0.0f);
        glColor3f(  0.0f,  1.f,  0.0f);
        glVertex3f( 0.5f, -0.5f,  0.0f);
        glColor3f(  0.0f,  0.0f,  1.f);
        glVertex3f( 0.0f,  0.5f,  0.0f);
        
        glEnd();
        
        glXSwapBuffers(displayInfo.xDisplay, displayInfo.xWindow);
    }
    snd_pcm_close(audioData.handle);
    return 0;
}
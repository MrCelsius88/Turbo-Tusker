#ifndef TUSKER_LINUX_X11_H_
#define TUSKER_LINUX_X11_H_

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

#endif // TUSKER_LINUX_X11_H_

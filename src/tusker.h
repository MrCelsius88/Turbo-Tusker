#ifndef TUSKER_H_
#define TUSKER_H_

/*
** NOTE(Cel):
** TUSKER_INTERNAL:
**  0 - Release build
**  1 - Development build
**
** TUSKER_SLOW
**  0 - Only fast code
**  1 - Slow code included
*/

#define TUSKER_INTERNAL 1 
#define TUSKER_SLOW 1

#if TUSKER_SLOW
# define ASSERT(expression) if (!(expression)) { *(int*)0 = 0; }
#else
# define ASSERT(expression)
#endif

#define KILOBYTES(value) ((value) * 1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define internal static
#define global static
#define persist static

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
#include <string.h> // TODO(Cel): Make my own c-string functions
#include <stdbool.h>

#include <math.h> // Im so lazy lmao

// TODO(Cel): We will abstract renderer later
// and our game code should not know if
// OpenGL exists or not. So move this when
// that happens.
#include <GL/glx.h>

// NOTE(Cel): Platform layer -> Game services

#if TUSKER_INTERNAL
typedef struct
{
    u32 fileSize;
    void* content;
} DebugFileData;

DebugFileData
DebugPlatformFileRead(const char* filename);

bool
DebugPlatformFileWrite(const char* filename, u32 dataSize, void* data);

void
DebugPlatformFileFreeMemory(DebugFileData* fileData);
#endif

// NOTE(Cel): Game -> Platform layer services

typedef struct
{
    s32 halfTransitionCount;
    bool endedDown;

} GameButtonState;

typedef struct
{
    bool isAnalog;

    f32 stickAverageX, stickAverageY;

    union
    {
        GameButtonState buttons[8];
        struct
        {
            GameButtonState stickSouth;
            GameButtonState stickEast;
            GameButtonState stickNorth;
            GameButtonState stickWest;

            GameButtonState buttonSouth;
            GameButtonState buttonEast;
            GameButtonState buttonNorth;
            GameButtonState buttonWest;
        };
    };

} GameControllerInput;
global const GameControllerInput g_zeroController = {0};

typedef struct
{
    GameControllerInput controllers[5];
} GameInput;

typedef struct
{
    ssize_t permanentMemorySize;
    void* permanentMemory; // NOTE(Cel): Always clear to zero on startup

    ssize_t transientMemorySize;
    void* transientMemory;

    bool initialized;
} GameMemory;

extern void
GameUpdateAndRender(GameMemory* gameMemory, GameInput* gameInput);

// NOTE(Cel): Platform-independent definitions
// with no relation to the platform layer

typedef struct
{
    f32 trianglePosX;
    f32 trianglePosY;
    f32 triangleColor;
} GameState;

#endif // TUSKER_H_

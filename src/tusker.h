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

#define M_PI 3.14159265358979323846
#define FPS 30

// NOTE(Cel): Platform layer -> Game services

#if TUSKER_INTERNAL
typedef struct
{
    u32 fileSize;
    void* content;
} DebugFileData;

internal DebugFileData
DebugPlatformFileRead(const char* filename);

internal bool
DebugPlatformFileWrite(const char* filename, u32 dataSize, void* data);

internal void
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

    f32 startX, startY;
    f32 minX, minY;
    f32 maxX, maxY;
    f32 endX, endY;

    union
    {
        GameButtonState buttons[4];
        struct
        {
            GameButtonState south;
            GameButtonState east;
            GameButtonState north;
            GameButtonState west;
        };
    };

} GameControllerInput;

typedef struct
{
    GameControllerInput controllers[4];
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

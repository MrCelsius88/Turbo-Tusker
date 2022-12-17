#ifndef TUSKER_H_
#define TUSKER_H_

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
    u64 permanentMemorySize;
    void* permanentMemory; // NOTE(Cel): Always clear to zero on startup

    u64 transientMemorySize;
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

#ifndef TUSKER_H_
#define TUSKER_H_

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

    // NOTE(Cel): These do not represent
    // the D PAD!!! Instead they are mapped to the
    // A B X Y buttons. So on an XBox controller UP
    // would be Y. And on a keyboard, UP is W.
    union
    {
        GameButtonState buttons[4];
        struct
        {
            GameButtonState up;
            GameButtonState down;
            GameButtonState left;
            GameButtonState right;
        };
    };

} GameControllerInput;

typedef struct
{
    GameControllerInput controllers[4];
} GameInput;

// NOTE(Cel): Platform layer -> Game services

// NOTE(Cel): Game -> Platform layer services
extern void
GameUpdateAndRender(GameInput* gameInput);

#endif // TUSKER_H_

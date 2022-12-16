#include "tusker.h"

extern void
GameUpdateAndRender(GameInput* gameInput)
{
    persist f32 trianglePosX;
    persist f32 trianglePosY;
    if (gameInput->controllers[0].right.endedDown)
    {
        trianglePosX += 0.01f;
    }

    if (gameInput->controllers[0].up.endedDown)
    {
        trianglePosY += 0.01f;
    }

    glBegin(GL_TRIANGLES);
    glColor3f(   1.f,  0.0f,  0.0f  );
    glVertex3f( -0.5f + trianglePosX, -0.5f + trianglePosY,  0.0f );
    glColor3f(   0.0f, 1.f,  0.0f  );
    glVertex3f(  0.5f + trianglePosX, -0.5f + trianglePosY,  0.0f );
    glColor3f(   0.0f,  0.0f,  1.f  );
    glVertex3f(  0.0f + trianglePosX,  0.5f + trianglePosY,  0.0f );
    glEnd();
}

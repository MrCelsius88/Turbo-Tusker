#include "tusker.h"

extern void
GameUpdateAndRender(GameInput* gameInput)
{
    persist f32 trianglePosX;
    persist f32 trianglePosY;

    if (gameInput->controllers[0].south.endedDown)
    {
        trianglePosY -= 0.01f;
    }
    else if (gameInput->controllers[0].east.endedDown)
    {
        trianglePosX += 0.01f;
    }
    else if (gameInput->controllers[0].north.endedDown)
    {
        trianglePosY += 0.01f;
    }
    else if (gameInput->controllers[0].west.endedDown)
    {
        trianglePosX -= 0.01f;
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

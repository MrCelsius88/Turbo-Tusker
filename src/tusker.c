#include "tusker.h"

extern void
GameUpdateAndRender(GameMemory* gameMemory, GameInput* gameInput)
{
    ASSERT(sizeof(GameState) <= gameMemory->permanentMemorySize);
    GameState* gameState = (GameState*)gameMemory->permanentMemory;

    DebugFileData fileData = DebugPlatformFileRead("/home/giannib/dev/TurboTusker/src/"__FILE__);
    if (fileData.content)
    {
        DebugPlatformFileWrite("/home/giannib/dev/TurboTusker/build/test.out", fileData.fileSize, fileData.content);
        DebugPlatformFileFreeMemory(&fileData);
    }

    if (!gameMemory->initialized)
    {
        gameState->trianglePosX = 0.f;
        gameState->trianglePosY = 0.f;
        gameState->triangleColor = 0.f;
        gameMemory->initialized = true;
    }

    GameControllerInput* input0 = &gameInput->controllers[0];
    if (input0->south.endedDown)
    {
        gameState->trianglePosY -= 0.01f;
    }
    if (input0->east.endedDown)
    {
        gameState->trianglePosX += 0.01f;
    }
    if (input0->north.endedDown)
    {
        gameState->trianglePosY += 0.01f;
    }
    if (input0->west.endedDown)
    {
        gameState->trianglePosX -= 0.01f;
    }

    if (input0->isAnalog)
    {
        gameState->triangleColor = input0->endX;
    }

    glBegin(GL_TRIANGLES);
    glColor3f(   gameState->triangleColor,  0.0f,  0.0f  );
    glVertex3f( -0.5f + gameState->trianglePosX, -0.5f + gameState->trianglePosY,  0.0f );
    glColor3f(   0.0f, gameState->triangleColor,  0.0f  );
    glVertex3f(  0.5f + gameState->trianglePosX, -0.5f + gameState->trianglePosY,  0.0f );
    glColor3f(   0.0f,  0.0f,  gameState->triangleColor  );
    glVertex3f(  0.0f + gameState->trianglePosX,  0.5f + gameState->trianglePosY,  0.0f );
    glEnd();
}

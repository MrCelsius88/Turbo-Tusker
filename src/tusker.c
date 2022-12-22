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

    for (int i = 0; i < ArrayCount(gameInput->controllers); ++i)
    {
        GameControllerInput* input = &gameInput->controllers[i];
        if (input->buttonSouth.endedDown)
        {
            gameState->trianglePosY -= 0.01f;
        }
        if (input->buttonEast.endedDown)
        {
            gameState->trianglePosX += 0.01f;
        }
        if (input->buttonNorth.endedDown)
        {
            gameState->trianglePosY += 0.01f;
        }
        if (input->buttonWest.endedDown)
        {
            gameState->trianglePosX -= 0.01f;
        }

        if (input->isAnalog)
        {
            gameState->triangleColor = input->stickAverageX;
        }
        else
        {
            if (input->stickWest.endedDown)
            {
                gameState->triangleColor = -1.f;
            }
            else if (input->stickEast.endedDown)
            {
                gameState->triangleColor = 1.f;
            }
        }
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

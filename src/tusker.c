#include "tusker.h"

internal void
RenderTriangleThing(void)
{
    glBegin(GL_TRIANGLES);
    glColor3f(   1.f,  0.0f,  0.0f  );
    glVertex3f( -0.5f, -0.5f,  0.0f );
    glColor3f(   0.0f,  1.f,  0.0f  );
    glVertex3f(  0.5f, -0.5f,  0.0f );
    glColor3f(   0.0f,  0.0f,  1.f  );
    glVertex3f(  0.0f,  0.5f,  0.0f );
    glEnd();
}

extern void
GameUpdateAndRender(void)
{
    RenderTriangleThing();
}

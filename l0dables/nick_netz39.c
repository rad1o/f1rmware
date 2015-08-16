#include <stdint.h>
#include <string.h>

#include <r0ketlib/display.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/render.h>
#include <r0ketlib/fonts/smallfonts.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/config.h>
#include <r0ketlib/render.h>
#include <r0ketlib/mesh.h>

#include "usetable.h"

#define M_PI 3.14159265358979323846	/* pi */

void ram(void)
{
    int dx=0;
    int dy=0;
    static uint32_t ctr=0;
    ctr++;

    int length = 0;

    setExtFont(GLOBAL(nickfont));
    // get length of user name before rendering
    length=DoString(0,0,GLOBAL(nickname));

    int frameRate = 100;
    float invFrameRate = 1.0 / frameRate;

    // set initial position and rotation of mesh
    float r[3] = { M_PI / 2, 0, 0 };
    int p[3] = { RESX/2, RESY*2/5, 0 };

    int verticeNumb = 0;
    int faceNumb = 0;

    // get number of vertices and number of faces of
    // the wanted mesh. Abort if file could not loaded
    if( !getMeshSizes( "netz39.obj",
                       &verticeNumb,
                       &faceNumb ) )
    {
        return;
    }

    // reserve the needed memory for the mesh
    float vertices[verticeNumb*3];
    int faces[faceNumb*3];

    // load mesh data from file to reserved memory buffers
    if( !getMesh( "netz39.obj",
                  vertices,
                  verticeNumb,
                  faces,
                  faceNumb ) )
    {
        return;
    }

    // main loop
    while (1)
    {
      // rotate
      r[2] += M_PI * 2 / 3 * invFrameRate;

      lcdClear();
      lcdFill(GLOBAL(nickbg));
      setTextColor(GLOBAL(nickbg),GLOBAL(nickfg));

      // print nickname in lower mid of display
      lcdSetCrsr(RESX / 2 - length / 2,  RESY * 0.9 - getFontHeight() / 2);
      lcdPrint(GLOBAL(nickname));

      // print mesh on screen
      DoMesh( vertices,
              verticeNumb,
              faces,
              faceNumb,
              r,
              p,
              100 );

      lcdDisplay();

      // Exit on any key
      int key = getInputRaw();
      if(key!= BTN_NONE)
          return;
      // sleep and process queue (e. g. meshnetwork)
      delayms_queue_plus(1000 * invFrameRate,0);
    }
  return;
}

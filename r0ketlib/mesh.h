#ifndef __MESH_H_
#define __MESH_H_

#include <stdint.h>

uint8_t getMeshSizes( char *fname, int* verticeNumb, int* facesNumb );
uint8_t getMesh( char *fname, float* vertices, int verticeNumb, int* faces, int facesNumb );

#endif

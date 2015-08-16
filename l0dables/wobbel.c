/*
 * Some basic 3d line vector plasma stuff. Not really optimized yet.
 ********************************************************************/

#include <math.h>
#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include "usetable.h"

// Background- and line color
#define COLOR_LINE 127
#define COLOR_BACKGROUND 0

// Screen size
#define SCREEN_WIDTH 130
#define SCREEN_HEIGHT 130

// Defines how big a cell is in model space
#define SCALE 1.5

// How many cells do we wanna have?
#define TESSELATION 14
#define NUM_VERTICES (TESSELATION * TESSELATION)

#define DEG2RAD(x) ((x * 3.1415f) / 180.0f)

#define VERTEX_BEHIND_CAMERA 0x8000

#define VERTEX_XY(x,y) (x + y * TESSELATION)

struct vertex2d {
    int x;
    int y;
};

struct vertex {
    float x;
    float y;
    float z;
    float w;
};

int frame;

struct vertex vertices[NUM_VERTICES];

/* Declarations **************************************************************/
void transform_vertex(struct vertex *in, struct vertex *out, float* matrix);
void init_matrix_projection(float *matrix, float near, float far, float aspect_ratio);
void init_matrix_projection_fov(float *matrix, float fov, float near, float far, float aspect_ratio);
void init_matrix_rotation_x(float *matrix, float rad);
void init_matrix_rotation_y(float *matrix, float rad);
void init_matrix_rotation_z(float *matrix, float rad);
void init_matrix_translation(float *matrix, float x, float y, float z);
void matrix_multiplication(float* a, float* b, float *result);
void init_vertices(struct vertex *v);
void line(int x0, int y0, int x1, int y1, uint8_t color);


void ram(void) {
	int i;
    frame = 0;

    struct vertex2d projected[NUM_VERTICES];

    float matrix_projection[16];
    init_matrix_projection_fov(matrix_projection, DEG2RAD(2), 0.1, 50, 1);

    while (true) {
        lcdFill(COLOR_BACKGROUND);

        float mat_a[16];
        float mat_b[16];
        float mat_c[16];
        float matrix_worldviewprojection[16];

        init_vertices(vertices);

        // Calculate worldviewprojection matrix by concatenating the
        // following operations:
        //  - rotate y axis
        //  - rotate x axis
        //  - translate
        //  - project
        float rad = (float)frame * 0.015f;
        init_matrix_rotation_y(mat_a, rad);
        init_matrix_rotation_x(mat_b, -rad*0.8);
        matrix_multiplication(mat_a, mat_b, mat_c);
        init_matrix_translation(mat_a, 0, -2, 11.5f);
        matrix_multiplication(mat_c, mat_a, mat_b);
        matrix_multiplication(mat_b, matrix_projection, matrix_worldviewprojection);

        // Transform vertices
        for (i=0; i<NUM_VERTICES; i++) {
            struct vertex transformed;

            transform_vertex(&vertices[i], &transformed, matrix_worldviewprojection);

            if (transformed.w > -0.0001)
                projected[i].x = VERTEX_BEHIND_CAMERA;
            else {
                projected[i].x = (int)(transformed.x / transformed.w) + SCREEN_WIDTH / 2;
                projected[i].y = (int)(transformed.y / transformed.w) + SCREEN_HEIGHT / 2;
            }
        }

        // Draw line grid
        struct vertex2d from, to;
        for (int x=0; x<TESSELATION; x++)
        for (int y=0; y<TESSELATION; y++)
        {
            if (x < (TESSELATION - 1)) {
                from = projected[x + y * TESSELATION];
                to = projected[x + y * TESSELATION + 1];
                line(from.x, from.y, to.x, to.y, COLOR_LINE);
            }

            if (y < (TESSELATION - 1)) {
                from = projected[x + y * TESSELATION];
                to = projected[x + y * TESSELATION + TESSELATION];
                line(from.x, from.y, to.x, to.y, COLOR_LINE);
            }
        }

        lcdDisplay();
        rad += 0.015;
        frame++;

        int key = getInputRaw();
        if (key == BTN_ENTER)
            break;
    }
}

// This is basically a gool ol' plasma effect
float wobbel(int i, int j) {
    float animation = ((float) frame) * 0.05f;

    float x = -1 + ((float)i * 2.0f) / (float)TESSELATION;
    float y = -1 + ((float)j * 2.0f) / (float)TESSELATION;

    float s1 = cos(animation * 0.3) * sin(x * 3- animation) * cos(y * 2);
    float s2 = sin(animation * 0.4) * (cos(x * 2) + sin(y * 3+ animation));

    float sine = (s1 + s2) * 4;

    return sine;
}

// Sets up the vertices
void init_vertices(struct vertex *v) {
    int i,j;
    int idx = 0;

    float offset = (float)(TESSELATION - 1) / 2.0f;

    for (i=0; i<TESSELATION; i++)
    for (j=0; j<TESSELATION; j++) {
        v->x = ((float)i - offset) * SCALE;
        v->y = wobbel(i, j);
        v->z = ((float)j - offset) * SCALE;
        v->w = 1;

        v++;
    }
}

/*
 * Rendering stuff
 *****************************************************************************/
int abs(int value) {
    if (value < 0)
        return -value;
    return value;
}

bool isInViewport(int x, int y) {
    return x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT;
}

void line(int x0, int y0, int x1, int y1, uint8_t color) {
    // Check if line is connected with a dot behind the
    // camera
    if (x0 == VERTEX_BEHIND_CAMERA || x1 == VERTEX_BEHIND_CAMERA)
        return;

    if (!isInViewport(x0, y0)) {
        if (!isInViewport(x1, y1))
            return;

        int dummy = x0;
        x0 = x1;
        x1 = dummy;
        dummy = y0;
        y0 = y1;
        y1 = dummy;
    }

    // Bresenham
    int dx = abs(x1 - x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0<y1 ? 1 : -1;
    int err = (dx>dy? dx : -dy) / 2, e2;

    while(true) {
        // Optimize me: For now, we just ignore pixels outside the screen. We might
        // want to clip those, though.
        if (!isInViewport(x0, y0))
            break;

        lcdSetPixel(x0, y0, color);

        if (x0==x1 && y0==y1)
            break;

        e2 = err;
        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

/*
 * Matrix handling
 ******************************************************************************/
// Matrix-Matrix multiplication
void matrix_multiplication(float *a, float* b, float *c) {

    for (int i=0; i<4; i++)
    for (int j=0; j<4; j++) {
        c[i + j * 4] = 0;
        for (int k=0; k<4; k++)
            c[i + j * 4] += a[i + k * 4] * b[k + j * 4];
    }
}

// Matrix-Vector multiplication
void transform_vertex(struct vertex *in, struct vertex *out, float* matrix) {
    out->x = in->x * matrix[0 + 0 * 4] + in->y * matrix[1 + 0 * 4] + in->z * matrix[2 + 0 * 4] + in->w * matrix[3 + 0 * 4];
    out->y = in->x * matrix[0 + 1 * 4] + in->y * matrix[1 + 1 * 4] + in->z * matrix[2 + 1 * 4] + in->w * matrix[3 + 1 * 4];
    out->z = in->x * matrix[0 + 2 * 4] + in->y * matrix[1 + 2 * 4] + in->z * matrix[2 + 2 * 4] + in->w * matrix[3 + 2 * 4];
    out->w = in->x * matrix[0 + 3 * 4] + in->y * matrix[1 + 3 * 4] + in->z * matrix[2 + 3 * 4] + in->w * matrix[3 + 3 * 4];
}

// Initializes a rotation matrix
void init_matrix_rotation_x(float *matrix, float rad) {
    *(matrix++) = 1;
    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 0;

    *(matrix++) = 0;
    *(matrix++) = cos(rad);
    *(matrix++) = -sin(rad);
    *(matrix++) = 0;

    *(matrix++) = 0;
    *(matrix++) = sin(rad);
    *(matrix++) = cos(rad);
    *(matrix++) = 0;

    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 1;
}

// Initializes a rotation matrix
void init_matrix_rotation_y(float *matrix, float rad) {
    *(matrix++) = cos(rad);
    *(matrix++) = 0;
    *(matrix++) = sin(rad);
    *(matrix++) = 0;

    *(matrix++) = 0;
    *(matrix++) = 1;
    *(matrix++) = 0;
    *(matrix++) = 0;

    *(matrix++) = -sin(rad);
    *(matrix++) = 0;
    *(matrix++) = cos(rad);
    *(matrix++) = 0;

    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 1;
}

// Initializes a rotation matrix
void init_matrix_rotation_z(float *matrix, float rad) {
    *(matrix++) = cos(rad);
    *(matrix++) = -sin(rad);
    *(matrix++) = 0;
    *(matrix++) = 0;

    *(matrix++) = sin(rad);
    *(matrix++) = cos(rad);
    *(matrix++) = 0;
    *(matrix++) = 0;

    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 1;
    *(matrix++) = 0;

    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 1;
}

// Initializes a translation matrix
void init_matrix_translation(float *matrix, float dx, float dy, float dz) {
    *(matrix++) = 1;
    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = dx;

    *(matrix++) = 0;
    *(matrix++) = 1;
    *(matrix++) = 0;
    *(matrix++) = dy;

    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 1;
    *(matrix++) = dz;

    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 1;
}

// Initializes a projeciton matrix
void init_matrix_projection_fov(float *matrix, float fov, float near, float far, float aspect_ratio) {
    float e = 1.0 / tan(fov / 2);

    *(matrix++) = e;
    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = 0;

    *(matrix++) = 0;
    *(matrix++) = e / aspect_ratio;;
    *(matrix++) = 0;
    *(matrix++) = 0;

    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = -((far + near) / (far - near));
    *(matrix++) = -((2 * far * near) / (far - near));

    *(matrix++) = 0;
    *(matrix++) = 0;
    *(matrix++) = -1;
    *(matrix++) = 0;
}

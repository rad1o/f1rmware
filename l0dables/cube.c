#include <stdlib.h>
#include <rad1olib/setup.h>
#include <rad1olib/draw.h>
#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include <math.h>

#include "usetable.h"

#define SIZE 130.0
#define DISTANCE 1600.0
#define CULLING
#define TRAIL_LENGTH 13
#define THICKNESS 4

typedef struct {
	float x;
	float y;
	float z;
} point3d_t;

typedef struct {
	float x;
	float y;
} point2d_t;


point3d_t rotate(point3d_t v, float r[], point3d_t* o);
point2d_t project(point3d_t v, float oz, point2d_t* o);
int* bfCull(float r[], float s, float oz, int* o);


void ram(void) {
	// getInputWaitRelease();
	
	int i;
	float distance = DISTANCE;
	float rx = 0;
	float ry = 0;         //  rotation around the 3 axes
	float rz = 0;
	float rotsincos[6];   // array for rotation calculations
	point3d_t v[8];       // 3d vertices (in float!)
	point2d_t p[8];       // rotated & projected vertices (in int!)
#ifdef CULLING
	int bf[] = {0,0,0,0,0,0}; // backface condition for each of the 6 faces
#endif
	int trailIndex = 0;
	point2d_t trail[TRAIL_LENGTH*8];
	for(int p=0; p<TRAIL_LENGTH*8; p++) {
		trail[p].x = -1;
		trail[p].y = -1;
	}
	float distT = 0.0;
	
	// Set up cube - put a vertex at each corner
	v[0].x = SIZE; v[0].y = SIZE; v[0].z = SIZE;
	v[1].x = SIZE; v[1].y = SIZE; v[1].z = -SIZE;
	v[2].x = SIZE; v[2].y = -SIZE; v[2].z = SIZE;
	v[3].x = SIZE; v[3].y = -SIZE; v[3].z = -SIZE;
	v[4].x = -SIZE; v[4].y = SIZE; v[4].z = SIZE;
	v[5].x = -SIZE; v[5].y = SIZE; v[5].z = -SIZE;
	v[6].x = -SIZE; v[6].y = -SIZE; v[6].z = SIZE;
	v[7].x = -SIZE; v[7].y = -SIZE; v[7].z = -SIZE;
	
	
	drawRectFill(0, 0, SIZE, SIZE, 0x00);
	// Frame
	while(1)
	{
		// Clear screen to black
		// drawRectFill(0, 0, SIZE, SIZE, 0x00);
		
		// Rotate arbitrarily
		rx += 0.05;
		ry += 0.0483;
		rz -= 0.062;
		
		// Calculate rotation matrix coefficients
		rotsincos[0] = sin(rx); rotsincos[1] = cos(rx);
		rotsincos[2] = sin(ry); rotsincos[3] = cos(ry);
		rotsincos[4] = sin(rz); rotsincos[5] = cos(rz);
		
#ifdef CULLING
		// Backface culling
		bfCull(rotsincos,SIZE,distance,bf);
#endif
		
		// Rotate and project every vertex
		for(int k=0; k<8; k++) {
			point3d_t orot;
			rotate(v[k], rotsincos, &orot);
			point2d_t oproj;
			project(orot, distance, &oproj);
			p[k] = oproj;
			trail[trailIndex*8+k] = oproj;
		}
		
		// animate distance
		distT = distT>2.0 ? (distT+0.1)-2.0 : distT+0.1;
		distance -= (distance-DISTANCE+sin(distT*3.141)*1000)*.1;
		
		// trail cleanup
		int trailIndexLast = (trailIndex+1)%TRAIL_LENGTH;
		drawLine(trail[trailIndexLast*8+0].x,trail[trailIndexLast*8+0].y,trail[trailIndexLast*8+1].x,trail[trailIndexLast*8+1].y,0b00000000, THICKNESS);
		drawLine(trail[trailIndexLast*8+1].x,trail[trailIndexLast*8+1].y,trail[trailIndexLast*8+3].x,trail[trailIndexLast*8+3].y,0b00000000, THICKNESS);
		drawLine(trail[trailIndexLast*8+2].x,trail[trailIndexLast*8+2].y,trail[trailIndexLast*8+3].x,trail[trailIndexLast*8+3].y,0b00000000, THICKNESS);
		drawLine(trail[trailIndexLast*8+0].x,trail[trailIndexLast*8+0].y,trail[trailIndexLast*8+2].x,trail[trailIndexLast*8+2].y,0b00000000, THICKNESS);
		drawLine(trail[trailIndexLast*8+4].x,trail[trailIndexLast*8+4].y,trail[trailIndexLast*8+5].x,trail[trailIndexLast*8+5].y,0b00000000, THICKNESS);
		drawLine(trail[trailIndexLast*8+5].x,trail[trailIndexLast*8+5].y,trail[trailIndexLast*8+7].x,trail[trailIndexLast*8+7].y,0b00000000, THICKNESS);
		drawLine(trail[trailIndexLast*8+6].x,trail[trailIndexLast*8+6].y,trail[trailIndexLast*8+7].x,trail[trailIndexLast*8+7].y,0b00000000, THICKNESS);
		drawLine(trail[trailIndexLast*8+6].x,trail[trailIndexLast*8+6].y,trail[trailIndexLast*8+4].x,trail[trailIndexLast*8+4].y,0b00000000, THICKNESS);
		drawLine(trail[trailIndexLast*8+0].x,trail[trailIndexLast*8+0].y,trail[trailIndexLast*8+4].x,trail[trailIndexLast*8+4].y,0b00000000, THICKNESS);
		drawLine(trail[trailIndexLast*8+1].x,trail[trailIndexLast*8+1].y,trail[trailIndexLast*8+5].x,trail[trailIndexLast*8+5].y,0b00000000, THICKNESS);
		drawLine(trail[trailIndexLast*8+2].x,trail[trailIndexLast*8+2].y,trail[trailIndexLast*8+6].x,trail[trailIndexLast*8+6].y,0b00000000, THICKNESS);
		drawLine(trail[trailIndexLast*8+3].x,trail[trailIndexLast*8+3].y,trail[trailIndexLast*8+7].x,trail[trailIndexLast*8+7].y,0b00000000, THICKNESS);
		
		// colored trail - render last frame's wireframe. we cheat a bit here by using the current frame's culling, but it's hardly noticeable. :)
		int trailIndexSecond = (trailIndex-1+TRAIL_LENGTH)%TRAIL_LENGTH;
		if(bf[0] || bf[2]){ drawLine(trail[trailIndexSecond*8+0].x,trail[trailIndexSecond*8+0].y,trail[trailIndexSecond*8+1].x,trail[trailIndexSecond*8+1].y,0b11110100/*0b01101000*/, THICKNESS); }
		if(bf[0] || bf[5]){ drawLine(trail[trailIndexSecond*8+1].x,trail[trailIndexSecond*8+1].y,trail[trailIndexSecond*8+3].x,trail[trailIndexSecond*8+3].y,0b11111000/*0b01101100*/, THICKNESS); }
		if(bf[0] || bf[3]){ drawLine(trail[trailIndexSecond*8+2].x,trail[trailIndexSecond*8+2].y,trail[trailIndexSecond*8+3].x,trail[trailIndexSecond*8+3].y,0b11111100/*0b01101100*/, THICKNESS); }
		if(bf[0] || bf[4]){ drawLine(trail[trailIndexSecond*8+0].x,trail[trailIndexSecond*8+0].y,trail[trailIndexSecond*8+2].x,trail[trailIndexSecond*8+2].y,0b11001000/*0b01100100*/, THICKNESS); }
		if(bf[1] || bf[2]){ drawLine(trail[trailIndexSecond*8+4].x,trail[trailIndexSecond*8+4].y,trail[trailIndexSecond*8+5].x,trail[trailIndexSecond*8+5].y,0b00011010/*0b00001101*/, THICKNESS); }
		if(bf[1] || bf[5]){ drawLine(trail[trailIndexSecond*8+5].x,trail[trailIndexSecond*8+5].y,trail[trailIndexSecond*8+7].x,trail[trailIndexSecond*8+7].y,0b00011010/*0b00001101*/, THICKNESS); }
		if(bf[1] || bf[3]){ drawLine(trail[trailIndexSecond*8+6].x,trail[trailIndexSecond*8+6].y,trail[trailIndexSecond*8+7].x,trail[trailIndexSecond*8+7].y,0b11111000/*0b01101100*/, THICKNESS); }
		if(bf[1] || bf[4]){ drawLine(trail[trailIndexSecond*8+6].x,trail[trailIndexSecond*8+6].y,trail[trailIndexSecond*8+4].x,trail[trailIndexSecond*8+4].y,0b11110100/*0b01101000*/, THICKNESS); }
		if(bf[2] || bf[4]){ drawLine(trail[trailIndexSecond*8+0].x,trail[trailIndexSecond*8+0].y,trail[trailIndexSecond*8+4].x,trail[trailIndexSecond*8+4].y,0b00011010/*0b00001101*/, THICKNESS); }
		if(bf[2] || bf[5]){ drawLine(trail[trailIndexSecond*8+1].x,trail[trailIndexSecond*8+1].y,trail[trailIndexSecond*8+5].x,trail[trailIndexSecond*8+5].y,0b11111100/*0b01101100*/, THICKNESS); }
		if(bf[3] || bf[4]){ drawLine(trail[trailIndexSecond*8+2].x,trail[trailIndexSecond*8+2].y,trail[trailIndexSecond*8+6].x,trail[trailIndexSecond*8+6].y,0b11001000/*0b01100100*/, THICKNESS); }
		if(bf[3] || bf[5]){ drawLine(trail[trailIndexSecond*8+3].x,trail[trailIndexSecond*8+3].y,trail[trailIndexSecond*8+7].x,trail[trailIndexSecond*8+7].y,0b00011010/*0b00001101*/, THICKNESS); }
		
		// vertices
		// With backface culling. Check if adjacent faces are visible - if at least one of them is, draw the line.
		if(bf[0] || bf[2]){ drawLine(p[0].x,p[0].y,p[1].x,p[1].y,0xFF/*0b11110100*/, THICKNESS); }
		if(bf[0] || bf[5]){ drawLine(p[1].x,p[1].y,p[3].x,p[3].y,0xFF/*0b11111000*/, THICKNESS); }
		if(bf[0] || bf[3]){ drawLine(p[2].x,p[2].y,p[3].x,p[3].y,0xFF/*0b11111100*/, THICKNESS); }
		if(bf[0] || bf[4]){ drawLine(p[0].x,p[0].y,p[2].x,p[2].y,0xFF/*0b11001000*/, THICKNESS); }
		if(bf[1] || bf[2]){ drawLine(p[4].x,p[4].y,p[5].x,p[5].y,0xFF/*0b00011010*/, THICKNESS); }
		if(bf[1] || bf[5]){ drawLine(p[5].x,p[5].y,p[7].x,p[7].y,0xFF/*0b00011010*/, THICKNESS); }
		if(bf[1] || bf[3]){ drawLine(p[6].x,p[6].y,p[7].x,p[7].y,0xFF/*0b11111000*/, THICKNESS); }
		if(bf[1] || bf[4]){ drawLine(p[6].x,p[6].y,p[4].x,p[4].y,0xFF/*0b11110100*/, THICKNESS); }
		if(bf[2] || bf[4]){ drawLine(p[0].x,p[0].y,p[4].x,p[4].y,0xFF/*0b00011010*/, THICKNESS); }
		if(bf[2] || bf[5]){ drawLine(p[1].x,p[1].y,p[5].x,p[5].y,0xFF/*0b11111100*/, THICKNESS); }
		if(bf[3] || bf[4]){ drawLine(p[2].x,p[2].y,p[6].x,p[6].y,0xFF/*0b11001000*/, THICKNESS); }
		if(bf[3] || bf[5]){ drawLine(p[3].x,p[3].y,p[7].x,p[7].y,0xFF/*0b00011010*/, THICKNESS); }
		
		trailIndex = trailIndexLast;
		
		delayms(10);
		lcdDisplay();
		
		int key = getInputRaw();
		if(key&BTN_LEFT) {
			return;
		}
		if(key&BTN_ENTER) {
			// enlarge on middle joystick press
			distance = DISTANCE*.5;
		}
	}
}

/**
 * Rotate a 3d point by the given rotations.
 * Rotations are precalculated each frame and
 * stored in an array, so the trigonometric
 * functions get calculated only per frame not
 * per vertex rotation.
 */
point3d_t rotate(point3d_t v, float r[], point3d_t* o) {
	float ttx, tty, ttz; // Temporary rotation values
	
	// Apply 3d rotation matrix
	tty  = v.y*r[5]+v.z*r[4];	ttz  = v.z*r[5]-v.y*r[4];
	o->z = ttz*r[3]+v.x*r[2];	ttx  = v.x*r[3]-ttz*r[2];
	o->x = ttx*r[1]+tty*r[0];	o->y = tty*r[1]-ttx*r[0];
	
	return *o;
}

/**
 * Project a point into 2d screen space
 * That means divide by z-distance
 */
point2d_t project(point3d_t v, float oz, point2d_t* o) {
	// Perspective distortion
	float iz = (SIZE*2)/(v.z+oz);
	
	// Project x & y (and cast to pixels (int))
	o->x = (int)(v.x*iz+(SIZE/2));
	o->y = (int)(v.y*iz+(SIZE/2));
	
	return *o;
}

#ifdef CULLING
/**
 * Okay, the intense stuff starts here.
 * This essentially does a dot product between the face normal (nv) and the
 * vector pointing from the face to the camera (cv). The product is equal to
 * the cosine of the angle between the vectors. Thus if the angle is 90° the
 * result will be 0. So we only need to check if:
 * cv*nv > 0 (the angle is <90°) -> the face points towards the camera.
 * Else it points away.
 * Faces pointing towards the camera should be visible, others should not.
 * 
 * Note that for a cube the face normals are pretty much known. With a more
 * irregular shape you would have to calculate the normals first!
 */
int* bfCull(float r[], float s, float oz, int* o) {
	float ttx, tty, ttz, length;
	float cv[3];
	
	/**
	 * Rotate x-face(s)
	 * This is a highly simplified rotation matrix (see above
	 * in rotate() for the full matrix) beause we know one
	 * coordinate to be 1 (or -1) and the other two to be 0.
	 */
	ttx = r[3]*r[1];  tty = r[3]*r[0];  ttz = r[2];
	
	// Build the cubeface-to-camera vector
	cv[0]=ttx*s; cv[1]=tty*s; cv[2]=ttz*s+oz;
	// Now normalize that vector (dot product for angle calculations only works with normalized vectors)
	// For that we calculate the length of the vector ...
	length = sqrt(cv[0]*cv[0]+cv[1]*cv[1]+cv[2]*cv[2]);
	// ... and divide the components by the length
	cv[0] /= length; cv[1] /= length; cv[2] /= length;
	// Calculate dot product, if above 0 then the face points towards the camera and is visible -> write 1 to output vector o
	o[0] = (cv[0]*-ttx+cv[1]*-tty+cv[2]*-ttz) > 0.0;
	
	// The same for the opposite side
	// The sweet thing is that we have a cube, so we can use the first rotated vector and just negate the components
	cv[0]=-ttx*s; cv[1]=-tty*s; cv[2]=(-ttz*s)+oz;
	length = sqrt(cv[0]*cv[0]+cv[1]*cv[1]+cv[2]*cv[2]); // however we have to recalculate the length
	cv[0] /= length; cv[1] /= length; cv[2] /= length;
	o[1] = (cv[0]*ttx+cv[1]*tty+cv[2]*ttz) > 0.0;
	
	
	// And everything again for the 2 y-faces: simplified rotate, normalize and dotproduct
	ttx = r[4]*r[2]*r[1]+r[5]*r[0];  tty = r[5]*r[1]-r[4]*r[2]*r[0];  ttz = -r[4]*r[3];
	
	cv[0]=ttx*s; cv[1]=tty*s; cv[2]=ttz*s+oz;
	length = sqrt(cv[0]*cv[0]+cv[1]*cv[1]+cv[2]*cv[2]);
	cv[0] /= length; cv[1] /= length; cv[2] /= length;
	o[2] = (cv[0]*-ttx+cv[1]*-tty+cv[2]*-ttz) > 0.0;
	
	cv[0]=-ttx*s; cv[1]=-tty*s; cv[2]=(-ttz*s)+oz;
	length = sqrt(cv[0]*cv[0]+cv[1]*cv[1]+cv[2]*cv[2]);
	cv[0] /= length; cv[1] /= length; cv[2] /= length;
	o[3] = (cv[0]*ttx+cv[1]*tty+cv[2]*ttz) > 0.0;
	
	
	// ... and once more for the 2 z-faces
	ttx = -r[5]*r[2]*r[1]+r[4]*r[0];  tty = r[4]*r[1]+r[5]*r[2]*r[0];  ttz = r[5]*r[3];
	
	cv[0]=ttx*s; cv[1]=tty*s; cv[2]=ttz*s+oz;
	length = sqrt(cv[0]*cv[0]+cv[1]*cv[1]+cv[2]*cv[2]);
	cv[0] /= length; cv[1] /= length; cv[2] /= length;
	o[4] = (cv[0]*-ttx+cv[1]*-tty+cv[2]*-ttz) > 0.0;
	
	cv[0]=-ttx*s; cv[1]=-tty*s; cv[2]=(-ttz*s)+oz;
	length = sqrt(cv[0]*cv[0]+cv[1]*cv[1]+cv[2]*cv[2]);
	cv[0] /= length; cv[1] /= length; cv[2] /= length;
	o[5] = (cv[0]*ttx+cv[1]*tty+cv[2]*ttz) > 0.0;
	
	return o;
}
#endif

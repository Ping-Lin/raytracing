#ifndef __RAYTRACING_H
#define __RAYTRACING_H

#include "objects.h"
#include <stdint.h>

/*void raytracing(uint8_t *pixels, color background_color,
                rectangular_node rectangulars, sphere_node spheres,
                light_node lights, const viewpoint *view,
                int width, int height, int threadNum, int threadIndex);*/
typedef struct RayInfo {
	uint8_t *pixels;
    light_node lights;
    rectangular_node rectangulars;
    sphere_node spheres;
    color background;
	const viewpoint *view;
	int threadNum;
	int threadIndex;
	int width;
	int height;
}RayInfo;

void* raytracing(void* arg);

#endif

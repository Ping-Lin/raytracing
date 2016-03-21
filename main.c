#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#include "primitives.h"
#include "raytracing.h"

#define OUT_FILENAME "out.ppm"

#define ROWS 512
#define COLS 512

static void write_to_ppm(FILE *outfile, uint8_t *pixels,
                         int width, int height)
{
    fprintf(outfile, "P6\n%d %d\n%d\n", width, height, 255);
    fwrite(pixels, 1, height * width * 3, outfile);
}

static double diff_in_second(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}

int main()
{
	int threadNum = 1;
	RayInfo *rayInfo = malloc(sizeof(RayInfo));
	pthread_t *threadIndex = malloc(sizeof(pthread_t) * threadNum);

    light_node lights = NULL;
    rectangular_node rectangulars = NULL;
    sphere_node spheres = NULL;
    color background = { 0.0, 0.1, 0.1 };

    struct timespec start, end;

#include "use-models.h"
    rayInfo->lights = lights;
    rayInfo->rectangulars = rectangulars;
    rayInfo->spheres = spheres;
    rayInfo->background[0] = background[0];
    rayInfo->background[1] = background[1];
    rayInfo->background[2] = background[2];
    rayInfo->view = &view;
    rayInfo->width = ROWS;
    rayInfo->height = COLS;


    /* allocate by the given resolution */
    rayInfo->pixels = malloc(sizeof(unsigned char) * ROWS * COLS * 3);
    if (!rayInfo->pixels) exit(-1);

    printf("# Rendering scene\n");
    /* do the ray tracing with the given geometry */
    clock_gettime(CLOCK_REALTIME, &start);

	rayInfo->threadNum = threadNum;
	for(int i = 0; i < threadNum; i++) {
		rayInfo->threadIndex = i;
		int err = pthread_create(&threadIndex[i], 0, raytracing, (void*)rayInfo);
		if(err) {
			printf("create thread error.\n");
			return -1;
		}

		//raytracing(pixels, background, rectangulars, spheres, lights, &view, ROWS, COLS, 2, 0);
		//raytracing(pixels, background, rectangulars, spheres, lights, &view, ROWS, COLS, 2, 1);
    }

	for(int i = 0; i < threadNum; i++) {
		void *returnVal;
		int err = pthread_join(threadIndex[i], &returnVal);
		if(err) {
			printf("join thread error.\n");
			return -1;
		}
	}


	clock_gettime(CLOCK_REALTIME, &end);
    {
        FILE *outfile = fopen(OUT_FILENAME, "wb");
        write_to_ppm(outfile, rayInfo->pixels, ROWS, COLS);
        fclose(outfile);
    }

    delete_rectangular_list(&rayInfo->rectangulars);
    delete_sphere_list(&rayInfo->spheres);
    delete_light_list(&rayInfo->lights);
    free(rayInfo->pixels);
	free(rayInfo);
    printf("Done!\n");
    printf("Execution time of raytracing() : %lf sec\n", diff_in_second(start, end));
    return 0;
}

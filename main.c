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

static void write_to_ppm(FILE *outfile, RayInfo *rayInfo,
                         int width, int height)
{
    fprintf(outfile, "P6\n%d %d\n%d\n", ROWS, COLS, 255);
    for(int k = 1; k < rayInfo[0].threadNum; k++) {
        for(int j = rayInfo[k].height * rayInfo[k].threadIndex / rayInfo[k].threadNum; j < rayInfo[k].height * (rayInfo[k].threadIndex+1)/rayInfo[k].threadNum; j++) {
            for(int i = 0; i < rayInfo[k].width; i++) {
                rayInfo[0].pixels[((i + (j * rayInfo[k].width)) * 3) + 0] = rayInfo[k].pixels[((i + (j * rayInfo[k].width)) * 3) + 0];
                rayInfo[0].pixels[((i + (j * rayInfo[k].width)) * 3) + 1] = rayInfo[k].pixels[((i + (j * rayInfo[k].width)) * 3) + 1];
                rayInfo[0].pixels[((i + (j * rayInfo[k].width)) * 3) + 2] = rayInfo[k].pixels[((i + (j * rayInfo[k].width)) * 3) + 2];
            }
        }
    }
    fwrite(rayInfo[0].pixels, 1, height * width * 3, outfile);
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

int main(int argc, char*argv[])
{
    int threadNum = 1;
    if(argc == 2)
        threadNum = atoi(argv[1]);
    else {
        printf("[Usage] ./raytracing thread_num\n");
        return -1;
    }
    RayInfo *rayInfo = malloc(sizeof(RayInfo) * threadNum);
    pthread_t *threadIndex = malloc(sizeof(pthread_t) * threadNum);

    light_node lights = NULL;
    rectangular_node rectangulars = NULL;
    sphere_node spheres = NULL;
    color background = { 0.0, 0.1, 0.1 };

    struct timespec start, end;

#include "use-models.h"
    for(int i = 0; i < threadNum; i++) {
        rayInfo[i].lights = lights;
        rayInfo[i].rectangulars = rectangulars;
        rayInfo[i].spheres = spheres;
        rayInfo[i].background[0] = background[0];
        rayInfo[i].background[1] = background[1];
        rayInfo[i].background[2] = background[2];
        rayInfo[i].view = &view;
        rayInfo[i].width = ROWS;
        rayInfo[i].height = COLS;

        /* allocate by the given resolution */
        rayInfo[i].pixels = malloc(sizeof(unsigned char) * ROWS * COLS * 3);
        if (!rayInfo[i].pixels) exit(-1);

        rayInfo[i].threadNum = threadNum;
    }




    printf("# Rendering scene\n");
    /* do the ray tracing with the given geometry */
    clock_gettime(CLOCK_REALTIME, &start);

    for(int i = 0; i < threadNum; i++) {
        rayInfo[i].threadIndex = i;
        int err = pthread_create(&threadIndex[i], 0, raytracing, (void*)&rayInfo[i]);
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
        write_to_ppm(outfile, rayInfo, ROWS, COLS);
        fclose(outfile);
    }

    delete_rectangular_list(&rayInfo[0].rectangulars);
    delete_sphere_list(&rayInfo[0].spheres);
    delete_light_list(&rayInfo[0].lights);
    free(rayInfo[0].pixels);
    free(rayInfo);

    printf("Done!\n");
    printf("Execution time of raytracing() : %lf sec\n", diff_in_second(start, end));
    return 0;
}

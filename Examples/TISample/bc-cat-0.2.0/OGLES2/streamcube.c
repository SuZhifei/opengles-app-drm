/*****************************************************************************
 * cmem.c
 *
 * bc-cat module unit test - BC_MEMORY_USERPTR feature
 * use cmem buffer to store YUV frames, and pass them to bc_cat.ko
 * run loadmodules.sh before run the app
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 *
 *   Neither the name of Texas Instruments Incorporated nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h>

#ifdef GLES_20
#include <math.h>
#endif

#include "bc_cat.h"
#include "common.h"
#include "drv_util.h"

#define FRAME_WIDTH     720
#define FRAME_HEIGHT    480
#define FRAME_SIZE      (FRAME_WIDTH * FRAME_HEIGHT * 2)
#define MAX_FRAMES      1
#define YUV_PIXEL_FMT   BC_PIX_FMT_UYVY
#define MAX_BUFFERS     3


typedef struct buf_info {
    char *start;
    unsigned long pa;
} buf_info_t;

void frame_cleanup(void);

static buf_info_t frame[MAX_FRAMES];
static char *yuv_data = NULL;
static unsigned long yuv_phys = 0;
static int   fr_idx = 0;
static int   bcdev_id = 0;
static tex_buffer_info_t buf_info;
struct drv_util *map_util;

int frame_init(bc_buf_params_t *p)
{
    int   ii;

    map_util = create_drv_util();
    if (map_util == NULL) {
        fprintf(stdout, "create_drv_util() failed\n");
        return -1;
    }

    yuv_data = map_util->share_buf_alloc(map_util, FRAME_SIZE * MAX_FRAMES);
    if (yuv_data == NULL) {
        fprintf(stdout, "share_buf_alloc() failed\n");
        destory_drv_util(map_util);
        return -1;
    }

    yuv_phys = map_util->ram_get_pa(map_util, yuv_data);
    if (!yuv_phys) {
        fprintf(stdout, "ram_get_pa() failed\n");
        frame_cleanup();
        return -1;
    }

    for (ii = 0; ii < MAX_FRAMES; ii++) {
        frame[ii].start = &yuv_data[ii * FRAME_SIZE];
        frame[ii].pa = yuv_phys + ii * FRAME_SIZE;
    }

    if (p) {
        p->count = MAX_FRAMES;
        p->width = FRAME_WIDTH;
        p->height = FRAME_HEIGHT;
        p->fourcc = YUV_PIXEL_FMT;
        p->type = BC_MEMORY_USERPTR;
    }

    return 0;
}

char *frame_get(bc_buf_ptr_t *p)
{
    char *va;
    va = frame[fr_idx].start;
    pattern_uyvy(fr_idx, va, FRAME_WIDTH, FRAME_HEIGHT);

    if (p) {
        p->index = fr_idx;
        p->pa = frame[fr_idx].pa;
    }

    fr_idx = (fr_idx + 1) % MAX_FRAMES;
    return va;
}

void frame_cleanup(void)
{
    if (yuv_data)
        map_util->share_buf_free(map_util, yuv_data);

    destory_drv_util(map_util);
}


#ifdef GLES_20
void drawCube(int bufferindex)
{
    static float rot_x = 0.0;
    static float rot_y = 0.0;
    float sx, cx, sy, cy;
    int tex_sampler;

    /* rotate the cube */
    sx = sin(rot_x);
    cx = cos(rot_x);
    sy = sin(rot_y);
    cy = cos(rot_y);

    modelview[0] = cy;
    modelview[1] = 0;
    modelview[2] = -sy;
    modelview[4] = sy * sy;
    modelview[5] = cx;
    modelview[6] = cy * sx;
    modelview[8] = sy * cx;
    modelview[9] = -sx;
    modelview[10] = cx * cy;

    glClearColor (0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program[0]);

    glUniformMatrix4fv(model_view_idx[0], 1, GL_FALSE, modelview);
    glUniformMatrix4fv(proj_idx[0], 1, GL_FALSE, projection);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 8);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 8);

    /* associate the stream texture */
    tex_sampler = glGetUniformLocation(program[1], "streamtexture");

    glUseProgram(program[1]);

    if (ptex_objs) {
        /* activate texture unit */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_STREAM_IMG, ptex_objs[bufferindex]);

        /* associate the sampler to a texture unit
         * 0 matches GL_TEXTURE0 */
        glUniform1i(tex_sampler, 0);

        glUniformMatrix4fv(model_view_idx[1], 1, GL_FALSE, modelview);
        glUniformMatrix4fv(proj_idx[1], 1, GL_FALSE, projection);

        glDrawArrays (GL_TRIANGLE_STRIP, 0, 8);
        glDrawArrays (GL_TRIANGLE_STRIP, 8, 8);
    }

    eglSwapBuffers(dpy, surface);

    rot_x += 0.01;
    rot_y += 0.01;
}
#else

void drawCube(int bufferindex)
{
    static GLfloat m_fAngleX = 0.0f;
    static GLfloat m_fAngleY = 0.0f;

    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_TEXTURE_STREAM_IMG);

    glPushMatrix();

    // Rotate the cube model
    glRotatef(m_fAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(m_fAngleY, 0.0f, 1.0f, 0.0f);

    glTexBindStreamIMG(bcdev_id, bufferindex);
/*    glBindTexture(GL_TEXTURE_STREAM_IMG, bufferindex);*/

    // Enable 3 types of data
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    // Set pointers to the arrays
    glVertexPointer(3, GL_FLOAT, 0, cube_vertices);
    glNormalPointer(GL_FLOAT, 0, cube_normals);
    glTexCoordPointer(2, GL_FLOAT, 0, cube_tex_coords);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 8);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 8);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    glPopMatrix();


    glDisable(GL_TEXTURE_STREAM_IMG);
    eglSwapBuffers(dpy, surface);

    m_fAngleX += 0.25f;
    m_fAngleY += 0.25f;
}
#endif

void usage(char *arg)
{
    printf("Usage:\n"
           "    %s [-b <id> | -p | -h]\n"
           "\t-b - bc device id [default: 0]\n"
           "\t-p - enable profiling\n"
           "\t-h - print this message\n\n", arg);
}

int main(int argc, char *argv[])
{
    int bcfd = -1;
    char bcdev_name[] = "/dev/bccatX";
    BCIO_package ioctl_var;
    bc_buf_params_t buf_param;
    bc_buf_ptr_t buf_pa;

    char *frame = NULL;
    int c, idx, ret = -1;
    char opts[] = "pb:h";

    struct timeval tvp, tv, tv0 = {0,0};
    unsigned long tdiff = 0;
    unsigned long fcount = 0;

    for (;;) {
        c = getopt_long(argc, argv, opts, (void *)NULL, &idx);

        if (-1 == c)
            break;

        switch (c) {
            case 0:
                break;
            case 'b':
                bcdev_id = atoi(optarg) % 10;
                break;
            case 'p':
                profiling = TRUE;
                printf("INFO: profiling enabled\n");
                break;
            default:
                usage(argv[0]);
                return 0;
        }
    }

    signal(SIGINT, signalHandler);

    if (frame_init(&buf_param))
        return -1;

    bcdev_name[strlen(bcdev_name)-1] = '0' + bcdev_id;

    if ((bcfd = open(bcdev_name, O_RDWR|O_NDELAY)) == -1) {
        printf("ERROR: open %s failed\n", bcdev_name);
        goto err_ret;
    }

    if (ioctl(bcfd, BCIOREQ_BUFFERS, &buf_param) != 0) {
        printf("ERROR: BCIOREQ_BUFFERS failed\n");
        goto err_ret;
    }

    if (ioctl(bcfd, BCIOGET_BUFFERCOUNT, &ioctl_var) != 0) {
        goto err_ret;
    }

    if (ioctl_var.output == 0) {
        printf("ERROR: no texture buffer available\n");
        goto err_ret;
    }

    /* for BC_MEMORY_USERPTR, BCIOSET_BUFFERPHYADDR must be called
     * before init IMG_extensions in initTexExt()*/
    for (idx = 0; idx < buf_param.count; idx++) {
        while ((frame = frame_get(&buf_pa)) == NULL) { }

        if (frame == (char *)-1)
            goto err_ret;

        if (ioctl(bcfd, BCIOSET_BUFFERPHYADDR, &buf_pa) != 0) {
            printf("ERROR: BCIOSET_BUFFERADDR[%d]: failed (0x%lx)\n",
                   buf_pa.index, buf_pa.pa);
            goto err_ret;
        }
    }

    if (initEGL(buf_param.count)) {
        printf("ERROR: init EGL failed\n");
        goto err_ret;
    }

    if ((ret = initTexExt(bcdev_id, &buf_info)) < 0) {
        printf("ERROR: initTexExt() failed [%d]\n", ret);
        goto err_ret;
    }

    if (buf_info.n > MAX_BUFFERS) {
        printf("ERROR: number of texture buffer exceeds the limit\n");
        goto err_ret;
    }

    ret = 0;

    if (profiling == TRUE) {
        gettimeofday(&tvp, NULL);
        tv0 = tvp;
    }

    while (!gQuit) {
#ifdef USE_SOLID_PATTERN
        usleep(1000 * 1000);
#endif
        frame = frame_get(&buf_pa);

        if (frame == (char *) -1)
            break;

        if (frame)
            idx = buf_pa.index;

        drawCube(idx);

        if (profiling == FALSE)
            continue;

        gettimeofday(&tv, NULL);

        fcount++;

        if (!(fcount % 60)) {
            tdiff = (unsigned long)(tv.tv_sec*1000 + tv.tv_usec/1000 -
                                tvp.tv_sec*1000 - tvp.tv_usec/1000);
            if (tdiff < 1800)   /*print fps every 2 sec*/
                continue;

            fprintf(stderr, "\rAvg FPS: %ld",
                    fcount / (tv.tv_sec - tv0.tv_sec));
            tvp = tv;
        }
    }
    printf("\n");

err_ret:
    if (bcfd > -1)
        close(bcfd);

    deInitEGL(buf_info.n);
    frame_cleanup();

    printf("done\n");
    return ret;
}

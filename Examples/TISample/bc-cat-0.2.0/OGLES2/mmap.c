/*****************************************************************************
 * mmap.c
 *
 * bc-cat module unit test - BC_MEMORY_MMAP feature
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
#include <stdio.h>
#include <stdlib.h>     /*malloc() & free()*/
#include <sys/ioctl.h>
#include <fcntl.h>
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

#define FRAME_WIDTH     720
#define FRAME_HEIGHT    480
#define FRAME_SIZE      (FRAME_WIDTH * FRAME_HEIGHT * 2)
#define MAX_FRAMES      3
#define YUV_PIXEL_FMT   BC_PIX_FMT_UYVY
#define MAX_BUFFERS     3

static char *frame[MAX_FRAMES];
static char *yuv_data = NULL;
static int   fr_idx = 0;
static int   bcdev_id = 0;
static tex_buffer_info_t buf_info;


int frame_init(bc_buf_params_t *p)
{
    int   ii;

    yuv_data = malloc(FRAME_SIZE * MAX_FRAMES);
    if (yuv_data == NULL) {
        fprintf(stdout, "no enough memory for input file\n");
        return -1;
    }

    for (ii = 0; ii < MAX_FRAMES; ii++) {
        frame[ii] = &yuv_data[ii * FRAME_SIZE];
    }

    if (p) {
        p->count = MAX_FRAMES;
        p->width = FRAME_WIDTH;
        p->height = FRAME_HEIGHT;
        p->fourcc = YUV_PIXEL_FMT;
        p->type = BC_MEMORY_MMAP;
    }

    return 0;
}

char *frame_get(bc_buf_ptr_t *p)
{
    char *va;
    va = frame[fr_idx];
    pattern_uyvy(fr_idx, va, FRAME_WIDTH, FRAME_HEIGHT);
    fr_idx = (fr_idx + 1) % MAX_FRAMES;

    if (p)
        p->pa = 0;

    return va;
}

void frame_cleanup(void)
{
    if (yuv_data)
        free(yuv_data);
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
           "    %s [-b <id> | -p | -w <width> | -t <height> | -h]\n"
           "\t-b - bc device id [default: 0]\n"
           "\t-p - enable profiling\n"
           "\t-w - width of texture buffer in pixel, multiple of 8 (for ES3.x)\n"
           "\t     or 32 (for ES2.x)\n"
           "\t-t - height of texture buffer in pixel\n"
           "\t-h - print this message\n\n", arg);
}

int main(int argc, char *argv[])
{
    int bcfd = -1;
    char bcdev_name[] = "/dev/bccatX";
    BCIO_package ioctl_var;
    bc_buf_params_t buf_param;
    bc_buf_ptr_t buf_pa;

    unsigned long buf_paddr[MAX_BUFFERS];
    char *buf_vaddr[MAX_BUFFERS] = { MAP_FAILED };
    char *frame = NULL;
    int buf_size = 0;
    int c, idx, ret = -1;
    char opts[] = "pw:t:b:h";

    int   ii;
    int   frame_w, frame_h;
    int   min_w = 0, min_h = 0;;
    int   cp_offset = 0;

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
            case 'w':
                min_w = atoi(optarg);
                break;
            case 't':
                min_h = atoi(optarg);
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

    frame_w = buf_param.width;
    frame_h = buf_param.height;

    if (min_w > 0 && !(min_w % 8))
        buf_param.width = min_w;

    if (min_h > 0)
        buf_param.height = min_h;

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

    if (initEGL(ioctl_var.output)) {
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

    /*FIXME calc stride instead of 2*/
    buf_size = buf_info.w * buf_info.h * 2;
    min_w    = buf_info.w < frame_w ? buf_info.w : frame_w;
    min_h    = buf_info.h < frame_h ? buf_info.h : frame_h;

    if (buf_info.h > frame_h)
        cp_offset = (buf_info.h - frame_h) * buf_info.w;

    if (buf_info.w > frame_w)
        cp_offset += buf_info.w - frame_w;

    if (buf_param.type == BC_MEMORY_MMAP) {
        for (idx = 0; idx < buf_info.n; idx++) {
            ioctl_var.input = idx;

            if (ioctl(bcfd, BCIOGET_BUFFERPHYADDR, &ioctl_var) != 0) {
                printf("ERROR: BCIOGET_BUFFERADDR failed\n");
                goto err_ret;
            }

            buf_paddr[idx] = ioctl_var.output;
            buf_vaddr[idx] = (char *)mmap(NULL, buf_size,
                              PROT_READ | PROT_WRITE, MAP_SHARED,
                              bcfd, buf_paddr[idx]);

            if (buf_vaddr[idx] == MAP_FAILED) {
                printf("ERROR: mmap failed\n");
                goto err_ret;
            }
        }
    }

    ret = 0;
    idx = 0;

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

        if (frame) {
            if (buf_param.type == BC_MEMORY_MMAP) {
                for (ii = 0; ii < min_h; ii++)
                      /*FIXME calc stride instead of 2*/
                    memcpy(buf_vaddr[idx] + buf_info.w * 2 * ii + cp_offset,
                           frame + frame_w * 2 * ii, min_w * 2);
            }
            else    /*buf_param.type == BC_MEMORY_USERPTR*/
                idx = buf_pa.index;
        }

        drawCube(idx);

        idx = (idx + 1) % buf_info.n;

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
    if (buf_param.type == BC_MEMORY_MMAP) {
        for (idx = 0; idx < buf_info.n; idx++) {
            if (buf_vaddr[idx] != MAP_FAILED)
                munmap(buf_vaddr[idx], buf_size);
        }
    }
    if (bcfd > -1)
        close(bcfd);

    deInitEGL(buf_info.n);
    frame_cleanup();

    printf("done\n");
    return ret;
}

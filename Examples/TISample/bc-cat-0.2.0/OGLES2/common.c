/*****************************************************************************
 * common.c
 *
 * bc-cat module unit test - common functions
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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/fb.h>
#include "common.h"

#define CUBE_V_LEN   1.8

#define BARS     8  /*number of color bars*/
#define STEPS   16  /*moving steps of a bar*/

#ifdef GLES_20
GLuint *ptex_objs;
GLint model_view_idx[2], proj_idx[2];

/* Shader program */
int program[2];

float projection[16] = {
    4.0f, 0.0f,  0.0f,  0.0f,
    0.0f, 4.0f,  0.0f,  0.0f,
    0.0f, 0.0f, -1.0f, -1.0f,
    0.0f, 0.0f, -1.0f,  0.0f
};

float modelview[16] = {
    1.0f, 0.0f,  0.0f, 0.0f,
    0.0f, 1.0f,  0.0f, 0.0f,
    0.0f, 0.0f,  1.0f, 0.0f,
    0.0f, 0.0f, -9.0f, 1.0f
};

/* Vertex shader */
static const char *vshader_src =
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "attribute vec4 vertex;\n"
    "attribute vec4 color;\n"
    "attribute vec3 normal;\n"
    "varying mediump vec4 v_color;\n"
    "attribute vec2 inputtexcoord;\n"
    "varying mediump vec2 texcoord;\n"
    "void main()\n"
    "{\n"
    "   vec4 eye_vertex = modelview*vertex;\n"
    "   gl_Position = projection*eye_vertex;\n"
    "   v_color = color;\n" "   texcoord = inputtexcoord;\n" "}";

/* Fragment shader */
static const char *fshader_2 =
    "#ifdef GL_IMG_texture_stream2\n"
    "#extension GL_IMG_texture_stream2 : enable\n"
    "#endif\n"
    "varying mediump vec2 texcoord;\n"
    "uniform samplerStreamIMG streamtexture;\n"
    "varying mediump vec4 v_color;\n"
    "void main(void)\n"
    "{\n" "    gl_FragColor = textureStreamIMG(streamtexture, texcoord);\n"
    //  "    gl_FragColor = v_color;\n"
    "}";

static const char *fshader_1 =
    "void main(void)\n"
    "{\n" "   gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);\n" "}";

/* shader object handles */
static int ver_shader, frag_shader[2];

static int setup_shaders(int bcdev_id, int num_bufs);
#endif

int gQuit = FALSE;
int profiling = FALSE;
PFNGLTEXBINDSTREAMIMGPROC glTexBindStreamIMG = NULL;

EGLDisplay dpy;
EGLSurface surface = EGL_NO_SURFACE;

static EGLContext context = EGL_NO_CONTEXT;

GLfloat cube_vertices[16][3] =
{   // x     y     z
    {-1.0, -1.0,  1.0}, // 1  left    First Strip
    {-1.0,  1.0,  1.0}, // 3
    {-1.0, -1.0, -1.0}, // 0
    {-1.0,  1.0, -1.0}, // 2
    { 1.0, -1.0, -1.0}, // 4  back
    { 1.0,  1.0, -1.0}, // 6
    { 1.0, -1.0,  1.0}, // 5  right
    { 1.0,  1.0,  1.0}, // 7

    { 1.0,  1.0, -1.0}, // 6  top     Second Strip
    {-1.0,  1.0, -1.0}, // 2
    { 1.0,  1.0,  1.0}, // 7
    {-1.0,  1.0,  1.0}, // 3
    { 1.0, -1.0,  1.0}, // 5  front
    {-1.0, -1.0,  1.0}, // 1
    { 1.0, -1.0, -1.0}, // 4  bottom
    {-1.0, -1.0, -1.0}  // 0
};

GLfloat cube_normals[16][3] =    // One normal per vertex.
{   // x     y     z
    {-0.5, -0.5,  0.5}, // 1  left          First Strip
    {-0.5,  0.5,  0.5}, // 3
    {-0.5, -0.5, -0.5}, // 0
    {-0.5,  0.5, -0.5}, // 2
    { 0.5, -0.5, -0.5}, // 4  back
    { 0.5,  0.5, -0.5}, // 6
    { 0.5, -0.5,  0.5}, // 5  right
    { 0.5,  0.5,  0.5}, // 7

    { 0.5,  0.5, -0.5}, // 6  top           Second Strip
    {-0.5,  0.5, -0.5}, // 2
    { 0.5,  0.5,  0.5}, // 7
    {-0.5,  0.5,  0.5}, // 3
    { 0.5, -0.5,  0.5}, // 5  front
    {-0.5, -0.5,  0.5}, // 1
    { 0.5, -0.5, -0.5}, // 4  bottom
    {-0.5, -0.5, -0.5}  // 0
};

GLfloat cube_tex_coords[16][2] =
{   // x   y
    {0.0, 1.0}, // 1  left                  First Strip
    {0.0, 0.0}, // 3
    {1.0, 1.0}, // 0
    {1.0, 0.0}, // 2
    {0.0, 1.0}, // 4  back
    {0.0, 0.0}, // 6
    {1.0, 1.0}, // 5  right
    {1.0, 0.0}, // 7

    {0.0, 1.0}, // 1  top                   Second Strip
    {1.0, 1.0}, // 3
    {0.0, 0.0}, // 0
    {1.0, 0.0}, // 2
    {0.0, 1.0}, // 4  front
    {1.0, 1.0}, // 6
    {0.0, 0.0}, // 5  bottom
    {1.0, 0.0}  // 7
};

int colors[BARS] = {
           /* y       |   v       |   y      | u */
            128 << 24 | 128 << 16 | 128 << 8 | 128, /* white  */
            162 << 24 | 142 << 16 | 162 << 8 |  44, /* yellow */
            131 << 24 |  44 << 16 | 131 << 8 | 156, /* cyan   */
            112 << 24 |  58 << 16 | 112 << 8 |  72, /* green  */
             84 << 24 | 198 << 16 |  84 << 8 | 184, /* mag    */
             65 << 24 | 212 << 16 |  65 << 8 | 100, /* red    */
             35 << 24 | 114 << 16 |  35 << 8 | 212, /* blue   */
             16 << 24 | 128 << 16 |  16 << 8 | 128  /* black  */

};

void pattern_uyvy(int fr_idx, void *p, int w, int h)
{
#ifdef USE_SOLID_PATTERN
    unsigned int pattern = 0;
    unsigned int* pBuff = (unsigned int*)p;

    switch (fr_idx & 0x3)
    {
        case 0 : pattern = 0x51f0515a; printf("Red\n"); break; // Red
        case 1 : pattern = 0x91219135; printf("Green\n"); break; // Green
        case 2 : pattern = 0x296d29f0; printf("Blue\n"); break; // Blue
        case 3 : pattern = 0xeb80eb80; printf("White\n");  break; // White
    }

    int r,c;
    for (r = 0; r < h; r++)
    {
        for (c = 0; c < w; c += 2)
        {
            *pBuff++ = pattern;
        }
    }

#else
    static int moves = 0;
	unsigned int *t = (unsigned int *)p;
	int ii, jj, kk, c0, c;

    (void)fr_idx;

    /*first partial strip*/
    c0 = colors[(moves/STEPS)%BARS];
    for (jj = 0; jj < h/BARS/STEPS * (STEPS - moves%STEPS); jj++) {
        for (ii = 0; ii < w; ii += 2)
            *t++ = c0;
    }

    /*all middle complete strips*/
    for (kk = 1; kk < BARS; kk++) {
        c = colors[(kk + moves/STEPS)%BARS];
        for (jj = 0; jj < h/BARS; jj++) {
            for (ii = 0; ii < w; ii += 2)
                *t++ = c;
        }
    }

    /*the remder partial strip*/
    for (jj = 0; jj < h/BARS/STEPS * (moves%STEPS); jj++) {
        for (ii = 0; ii < w; ii += 2)
            *t++ = c0;
    }
    moves++;
#endif
}

void signalHandler(int signum) { (void)signum; gQuit=TRUE; }

int get_disp_resolution(int *w, int *h)
{
    int fb_fd, ret = -1;
    struct fb_var_screeninfo vinfo;

    if ((fb_fd = open("/dev/fb0", O_RDONLY)) < 0) {
        printf("failed to open fb0 device\n");
        return ret;
    }

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("FBIOGET_VSCREENINFO");
        goto exit;
    }

    *w = vinfo.xres;
    *h = vinfo.yres;

    if (*w && *h)
        ret = 0;

exit:
    close(fb_fd);
    return ret;
}

#ifdef X11
Display* x11Display  = 0;
Window   x11Window   = 0;
Colormap x11Colormap = 0;

int initX11(int *w, int *h)
{
    long                 screen = 0;
    XVisualInfo          visual;
    Window               rootWin;
    XSetWindowAttributes wa;
    unsigned int         mask;
    int                  depth;
    int                  width;
    int                  height;

    if (!(x11Display = XOpenDisplay(":0"))) {
        printf("Error: Unable to open X display\n");
        return -1;
    }
    screen = XDefaultScreen(x11Display);

    width = DisplayWidth(x11Display, screen);
    height = DisplayHeight(x11Display, screen);
    rootWin = RootWindow(x11Display, screen);
    depth = DefaultDepth(x11Display, screen);

    if (!XMatchVisualInfo(x11Display, screen, depth, TrueColor, &visual)) {
        printf("Error: Unable to acquire visual\n");
        XCloseDisplay(x11Display);
        return -1;
    }
    x11Colormap = XCreateColormap(x11Display, rootWin,
                                  visual.visual, AllocNone);
    wa.colormap = x11Colormap;

    wa.event_mask = StructureNotifyMask | ExposureMask;
    mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;

    x11Window = XCreateWindow(x11Display, rootWin,
                              0, 0, width/4*3, height/4*3,
                              0, CopyFromParent, InputOutput,
                              CopyFromParent, mask, &wa);
    XMapWindow(x11Display, x11Window);
    XFlush(x11Display);

    if (w) *w = width;
    if (h) *h = height;

    return 0;
}

void deInitX11(void)
{
    if (x11Window)
        XDestroyWindow(x11Display, x11Window);
    if (x11Colormap)
        XFreeColormap(x11Display, x11Colormap);
    if (x11Display)
        XCloseDisplay(x11Display);
}

int doX11Events(void)
{
    int ii, w, h;
    float r;
    XEvent e;
    Atom wdw;

    wdw = XInternAtom(x11Display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(x11Display, x11Window, &wdw, 1);

    for (ii = 0; ii < XPending(x11Display); ii++) {
        XNextEvent(x11Display, &e);

        switch (e.type) {
        case ClientMessage:
            /* WM_DELETE_WINDOW - terminate */
            if ((Atom)e.xclient.data.l[0] == wdw)
                return 1;
            break;
        case ConfigureNotify:
            w = e.xconfigure.width;
            h = e.xconfigure.height;
            r = (float)w / h;

            glViewport(0, 0, w, h);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();

            if (w > h)
                glOrthof(-CUBE_V_LEN * r, CUBE_V_LEN * r, -CUBE_V_LEN,
                         CUBE_V_LEN, -CUBE_V_LEN * 2, CUBE_V_LEN * 2);
            else
                glOrthof(-CUBE_V_LEN, CUBE_V_LEN, -CUBE_V_LEN / r,
                         CUBE_V_LEN / r, -CUBE_V_LEN * 2, CUBE_V_LEN * 2);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            break;
        case Expose:
            /*must handle this, otherwise, background does not redraw*/
            break;
        default:
            break;
        }
    }
    return 0;
}
#endif

int initTexExt(int bcdev_id, tex_buffer_info_t *binfo)
{
    PFNGLGETTEXSTREAMDEVICEATTRIBUTEIVIMGPROC glGetTexAttrIMG = NULL;
    PFNGLGETTEXSTREAMDEVICENAMEIMGPROC glGetTexDeviceIMG = NULL;
    const GLubyte *glext;
    const GLubyte *dev_name;

    if (!binfo)
        return -5;

    if (!(glext = glGetString(GL_EXTENSIONS)))
        return -1;

#ifdef GLES_20
    if (!strstr((char *)glext, "GL_IMG_texture_stream2"))
#else
    if (!strstr((char *)glext, "GL_IMG_texture_stream"))
#endif
        return -2;

    glTexBindStreamIMG =
        (PFNGLTEXBINDSTREAMIMGPROC)eglGetProcAddress("glTexBindStreamIMG");
    glGetTexAttrIMG = (PFNGLGETTEXSTREAMDEVICEATTRIBUTEIVIMGPROC)
        eglGetProcAddress("glGetTexStreamDeviceAttributeivIMG");
    glGetTexDeviceIMG = (PFNGLGETTEXSTREAMDEVICENAMEIMGPROC)
        eglGetProcAddress("glGetTexStreamDeviceNameIMG");

    if (!glTexBindStreamIMG || !glGetTexAttrIMG || !glGetTexDeviceIMG)
        return -3;

    dev_name = glGetTexDeviceIMG(bcdev_id);
    if (!dev_name)
        return -4;

    glGetTexAttrIMG(bcdev_id, GL_TEXTURE_STREAM_DEVICE_NUM_BUFFERS_IMG, &binfo->n);
    glGetTexAttrIMG(bcdev_id, GL_TEXTURE_STREAM_DEVICE_WIDTH_IMG, &binfo->w);
    glGetTexAttrIMG(bcdev_id, GL_TEXTURE_STREAM_DEVICE_HEIGHT_IMG, &binfo->h);
    glGetTexAttrIMG(bcdev_id, GL_TEXTURE_STREAM_DEVICE_FORMAT_IMG, &binfo->fmt);

#ifdef GLES_20
    if (setup_shaders(bcdev_id, binfo->n))
        return -5;
#endif

    printf("\ndevice: %s num: %d, width: %d, height: %d, format: 0x%x\n",
        dev_name, binfo->n, binfo->w, binfo->h, binfo->fmt);

    return 0;
}

static void print_err(char *name)
{
    char *err_str[] = {
          "EGL_SUCCESS",
          "EGL_NOT_INITIALIZED",
          "EGL_BAD_ACCESS",
          "EGL_BAD_ALLOC",
          "EGL_BAD_ATTRIBUTE",
          "EGL_BAD_CONFIG",
          "EGL_BAD_CONTEXT",
          "EGL_BAD_CURRENT_SURFACE",
          "EGL_BAD_DISPLAY",
          "EGL_BAD_MATCH",
          "EGL_BAD_NATIVE_PIXMAP",
          "EGL_BAD_NATIVE_WINDOW",
          "EGL_BAD_PARAMETER",
          "EGL_BAD_SURFACE" };

    EGLint ecode = eglGetError();

    printf("'%s': egl error '%s' (0x%x)\n",
           name, err_str[ecode-EGL_SUCCESS], ecode);
}

void deInitEGL(int n_buf)
{
#ifdef GLES_20
    if (ptex_objs) {
        glDeleteTextures (n_buf, ptex_objs);
        glDisable(GL_TEXTURE_STREAM_IMG);
        free(ptex_objs);
        ptex_objs = NULL;
    }

    /* clean up shaders */
    glDeleteProgram(program[0]);
    glDeleteProgram(program[1]);
    glDeleteShader(ver_shader);
    glDeleteShader(frag_shader[0]);
    glDeleteShader(frag_shader[1]);
#else
    (void)n_buf;
#endif

    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (context != EGL_NO_CONTEXT)
        eglDestroyContext(dpy, context);
    if (surface != EGL_NO_SURFACE)
        eglDestroySurface(dpy, surface);
    eglTerminate(dpy);
}

int initEGL(int n_buf)
{
#ifdef GLES_20
    EGLint  context_attr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
#else
    typedef NativeDisplayType EGLNativeDisplayType;
    typedef NativeWindowType EGLNativeWindowType;
#endif

    EGLint            disp_w, disp_h;
    EGLNativeDisplayType disp_type;
    EGLNativeWindowType  window;
    EGLConfig         cfgs[2];
    EGLint            n_cfgs;
    EGLint            egl_attr[] = {
                         EGL_BUFFER_SIZE, EGL_DONT_CARE,
                         EGL_RED_SIZE,    8,
                         EGL_GREEN_SIZE,  8,
                         EGL_BLUE_SIZE,   8,
                         EGL_DEPTH_SIZE,  8,
#ifdef GLES_20
                         EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#endif
                         EGL_NONE };

#ifdef X11
    if (initX11(&disp_w, &disp_h))
        return -1;

    disp_type = (EGLNativeDisplayType)x11Display;
    window = (EGLNativeWindowType)x11Window;
#else
    if (get_disp_resolution(&disp_w, &disp_h)) {
        printf("ERROR: get display resolution failed\n");
        return -1;
    }

    disp_type = (EGLNativeDisplayType)EGL_DEFAULT_DISPLAY;
    window  = 0;
#endif

    dpy = eglGetDisplay(disp_type);

    if (eglInitialize(dpy, NULL, NULL) != EGL_TRUE) {
        print_err("eglInitialize");
        return -1;
    }

    if (eglGetConfigs(dpy, cfgs, 2, &n_cfgs) != EGL_TRUE) {
        print_err("eglGetConfigs");
        goto cleanup;
    }

    if (eglChooseConfig(dpy, egl_attr, cfgs, 2, &n_cfgs) != EGL_TRUE) {
        print_err("eglChooseConfig");
        goto cleanup;
    }

    surface = eglCreateWindowSurface(dpy, cfgs[0], window, NULL);
    if (surface == EGL_NO_SURFACE) {
        print_err("eglCreateWindowSurface");
        goto cleanup;
    }

#ifdef GLES_20
    context = eglCreateContext(dpy, cfgs[0], EGL_NO_CONTEXT, context_attr);
#else
    context = eglCreateContext(dpy, cfgs[0], EGL_NO_CONTEXT, NULL);
#endif
    if (context == EGL_NO_CONTEXT) {
        print_err("eglCreateContext");
        goto cleanup;
    }

    if (eglMakeCurrent(dpy, surface, surface, context) != EGL_TRUE) {
        print_err("eglMakeCurrent");
        goto cleanup;
    }

    /* 0 - do not sync with video frame */
    if (profiling == TRUE) {
        if (eglSwapInterval(dpy, 0) != EGL_TRUE) {
            print_err("eglSwapInterval");
            goto cleanup;
        }
    }

#ifndef GLES_20
    glShadeModel(GL_FLAT);
    glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFrontFace(GL_CW);
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);

    glEnable(GL_NORMALIZE);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrthof(-CUBE_V_LEN * disp_w / disp_h, CUBE_V_LEN * disp_w / disp_h,
             -CUBE_V_LEN, CUBE_V_LEN, -CUBE_V_LEN * 2, CUBE_V_LEN * 2);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif

    return 0;

cleanup:
    deInitEGL(n_buf);
#ifdef X11
    deInitX11();
#endif
    return -1;
}


#ifdef GLES_20
static int setup_shaders(int bcdev_id, int num_bufs)
{
    int status, idx;

    if (num_bufs <= 0)
        return -1;

    /* Initialize shaders */
    ver_shader     = glCreateShader(GL_VERTEX_SHADER);
    frag_shader[0] = glCreateShader(GL_FRAGMENT_SHADER);
    frag_shader[1] = glCreateShader(GL_FRAGMENT_SHADER);

    /* Attach and compile shaders */
    glShaderSource(ver_shader, 1, (const char **) &vshader_src, NULL);
    glCompileShader(ver_shader);

    glGetShaderiv(ver_shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buf[1024];

        glGetShaderInfoLog(ver_shader, sizeof (buf), NULL, buf);
        printf("ERROR: Vertex shader compilation failed, info log:\n%s", buf);
        return -1;
    }

    for (idx = 0; idx < 2; idx++) {
        if (idx == 0)
            glShaderSource(frag_shader[idx], 1, (const char **) &fshader_1, NULL);
        else
            glShaderSource(frag_shader[idx], 1, (const char **) &fshader_2, NULL);

        glCompileShader(frag_shader[idx]);

        glGetShaderiv(frag_shader[idx], GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            char buf[1024];

            glGetShaderInfoLog(frag_shader[idx], sizeof (buf), NULL, buf);
            printf("ERROR: Fragment shader compilation failed, info log:\n%s", buf);
            return -1;
        }

        program[idx] = glCreateProgram();

        /* Attach shader to the program */
        glAttachShader(program[idx], ver_shader);
        glAttachShader(program[idx], frag_shader[idx]);

        /* associating the program attributes */
        glBindAttribLocation(program[idx], 0, "vertex");
        glBindAttribLocation(program[idx], 1, "normal");
        glBindAttribLocation(program[idx], 2, "inputtexcoord");

        /* link the program */
        glLinkProgram(program[idx]);

        glGetProgramiv(program[idx], GL_LINK_STATUS, &status);
        if (status != GL_TRUE) {
            char buf[1024];

            glGetProgramInfoLog(program[idx], sizeof (buf), NULL, buf);
            printf("ERROR: Program linking failed, info log:\n%s", buf);
            return -1;
        }

        glValidateProgram(program[idx]);

        glGetProgramiv(program[idx], GL_VALIDATE_STATUS, &status);
        if (status != GL_TRUE) {
            char buf[1024];

            glGetProgramInfoLog(program[idx], sizeof (buf), NULL, buf);
            printf("ERROR: Program validation failed, info log:\n%s", buf);
            return -1;
        }

        /* pass the model view and projection matrix to shader */
        model_view_idx[idx] = glGetUniformLocation (program[idx], "modelview");
        proj_idx[idx] = glGetUniformLocation (program[idx], "projection");
    }

    /* assign the vertices and their texture co-ords */

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, cube_vertices);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, cube_normals);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, cube_tex_coords);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* allocate texture objects */
    ptex_objs = malloc(sizeof (GLuint) * num_bufs);
    if (!ptex_objs) {
        printf("ERROR: malloc failed in %s\n", __FUNCTION__);
        return -1;
    }
    glGenTextures(num_bufs, ptex_objs);

    /* activate texture unit */
    glActiveTexture(GL_TEXTURE0);

    /* associate buffers to textures */
    for (idx = 0; idx < num_bufs; idx++) {
      glBindTexture (GL_TEXTURE_STREAM_IMG, ptex_objs[idx]);

      /* specify filters */
      glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      // assign the buffer
      glTexBindStreamIMG(bcdev_id, idx);
    }

    return 0;
}
#endif

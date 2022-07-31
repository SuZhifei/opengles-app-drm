/*
 * Copyright (C) 2011 Texas Instruments
 * Author: Rob Clark <rob.clark@linaro.org>
 * Adopted for testing VIP capture by: Nikhil Devshatwar <nikhil.nd@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <pthread.h>
#include "util.h"

#define NBUF 6
#define CNT  500000
#define MAX_CAP 6
#define PIC_MAX_PATH 256

#define IMG_PATH "/media/sdcard0/snapshot_pic.yuyv"
#define INPUT_DEV_FILE "/dev/input/event1"

static bool b_need_snapshot;
pthread_mutex_t snapshot_status_lock;
pthread_cond_t snapshot_status_cond;

struct snapshot_data {
	void *vaddr;
	uint32_t size;
};

struct thr_data {
	struct display *disp;
	struct v4l2 *v4l2;
	uint32_t fourcc, width, height;
};

static void
usage(char *name)
{
	MSG("Usage: %s [OPTION]...", name);
	MSG("Test of buffer passing between v4l2 camera and display.");
	MSG("");
	MSG("viddec3test options:");
	MSG("\t-h, --help: Print this help and exit.");
	MSG("\t--multi <num>: Capture from <num> different devices.");
	MSG("\t\t\tEach device name and format would be parsed in the cmdline");
	MSG("");
	disp_usage();
	v4l2_usage();
}

static int get_file_index()
{
	unsigned int index = 0;
	char pic_path[PIC_MAX_PATH];
	sprintf(pic_path, "/media/sdcard0/snapshot_pic_%d.yuyv", index);
	while(!access(pic_path, F_OK)) {
		index++;
		sprintf(pic_path, "/media/sdcard0/snapshot_pic_%d.yuyv", index);
		if (index > 100) {
			MSG("Too many pic capture");
			break;
		}
	}

	MSG("get file index %d", index);

	return index;
}

static int save_yuv_pic(char *path, void *buf, unsigned int size)
{
       int ret = false;
       int count;
       FILE *fp;

       if (path == NULL || buf == NULL)
               goto end;

       MSG("cur pic path %s, size is %d\n", path, size);

       if ((fp = fopen(path, "wb+")) == NULL) {
               ERROR("file open fail\n");
               goto end;
       }

       count = fwrite(buf, size, 1, fp);
       MSG("fwrite %d\n", count);

       fclose(fp);

       system("sync");
       ret = true;

end:
       return ret;
}

void *
snapshot_loop(void *arg)
{
	int ret = 0;
	int input_dev_fd;
	bool need_snapshot;

	input_dev_fd = open_input_dev(INPUT_DEV_FILE);
	MSG("snapshot thread start");

	while(1) {
		/*  Get mouse trigger event */
		need_snapshot = snapshot_status_get(input_dev_fd);

		pthread_mutex_lock(&snapshot_status_lock);
		b_need_snapshot = need_snapshot;
		if(b_need_snapshot) {
			MSG("snapshot status %d", b_need_snapshot);
			pthread_cond_wait(&snapshot_status_cond, &snapshot_status_lock);
		}
		pthread_mutex_unlock(&snapshot_status_lock);
	}

	close_input_dev(input_dev_fd);
}

void *
capture_loop(void *arg)
{
	struct thr_data *data = (struct thr_data *)arg;
	struct display *disp = data->disp;
	struct v4l2 *v4l2 = data->v4l2;
	uint32_t fourcc = data->fourcc;
	uint32_t width = data->width, height = data->height;

	struct buffer **buffers, *capt;
	int ret, i;
	bool need_snapshot;

	char img_path[PIC_MAX_PATH];
	int pic_index = 0;

	struct snapshot_data snapshot_buf =
	{
		.vaddr = NULL,
		.size = 0,
	};

	buffers = disp_get_vid_buffers(disp, NBUF, fourcc, width, height);
	if (!buffers) {
		return NULL;
	}

	ret = v4l2_reqbufs(v4l2, buffers, NBUF);
	if (ret) {
		return NULL;
	}

	for (i = 0; i < NBUF; i++) {
		v4l2_qbuf(v4l2, buffers[i]);
	}

	ret = v4l2_streamon(v4l2);
	if (ret) {
		return NULL;
	}

	for (i = 1; i < CNT; i++) {

		capt = v4l2_dqbuf(v4l2);

		/* Get snapshot status */
		pthread_mutex_lock(&snapshot_status_lock);
		need_snapshot = b_need_snapshot;
		if(need_snapshot) {
			/* start snapshot */
			/* 1. Get buffer */
			snapshot_buf.vaddr = omap_bo_map(capt->bo[0]);
			snapshot_buf.size = omap_bo_size(capt->bo[0]);
			/* 2. Get file name */
			pic_index = get_file_index();
			sprintf(img_path, "/media/sdcard0/snapshot_pic_%d.yuyv", pic_index);
			/* 3. Save yuyv data to file */
			ret = save_yuv_pic(img_path, snapshot_buf.vaddr, snapshot_buf.size);
			if(ret == false)
				ERROR("save_yuv_pic failed");
			pthread_cond_signal(&snapshot_status_cond);
		}
		pthread_mutex_unlock(&snapshot_status_lock);

		ret = disp_post_vid_buffer(disp, capt,
			0, 0, width, height);
		if (ret) {
			ERROR("Post buffer failed");
			return NULL;
		}
		v4l2_qbuf(v4l2, capt);
	}
	v4l2_streamoff(v4l2);

	MSG("Ok!");
	return disp;
}

int
main(int argc, char **argv)
{
	struct display *disp;
	struct v4l2 *v4l2;
	pthread_t threads[MAX_CAP];
	pthread_t snapshot_thrd;
	struct thr_data tdata[MAX_CAP];
	void *result = NULL;
	int ret = 0, i, multi = 1, idx = 0;
	char c;

	b_need_snapshot = false;
	pthread_mutex_init(&snapshot_status_lock,NULL);

	MSG("Opening Display..");
	disp = disp_open(argc, argv);
	if (!disp) {
		usage(argv[0]);
		return 1;
	}

	disp->multiplanar = false;

	for (i = 1; i < argc; i++) {
		if (!argv[i])
			continue;

		if (!strcmp("--multi", argv[i])) {
			argv[i++] = NULL;
			sscanf(argv[i], "%d", &multi);
			argv[i] = NULL;
			if(multi < 1 || multi > MAX_CAP) {
				usage(argv[i]);
				return 1;
			}
		}
	}

	for (i = 0; i < multi; i++) {
		MSG("Opening V4L2..");
		v4l2 = v4l2_open(argc, argv, &tdata[i].fourcc,
			&tdata[i].width, &tdata[i].height);
		if (!v4l2) {
			usage(argv[0]);
			return 1;
		}
		tdata[i].disp = disp;
		tdata[i].v4l2 = v4l2;
	}

	if (check_args(argc, argv)) {
		/* remaining args.. print usage msg */
		usage(argv[0]);
		return 0;
	}

	ret = pthread_create(&snapshot_thrd, NULL, snapshot_loop, NULL);
	if(ret) {
		MSG("Failed creating snapshot thread");
	}

	for (i = 0; i < multi; i++) {
		ret = pthread_create(&threads[i], NULL, capture_loop, &tdata[i]);
		if(ret) {
			MSG("Failed creating thread");
		}
	}

	for (i = 0; i < multi; i++) {
		pthread_join(threads[i], &result);
	}

	disp_close(disp);

	return ret;
}

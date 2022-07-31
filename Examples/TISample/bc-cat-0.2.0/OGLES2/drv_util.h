#ifndef __DRV_UTIL_H__
#define __DRV_UTIL_H__

/* from zongmu */
#define PARAM_BUF_DATA_ALIGN_SHIFT	2
#define PARAM_BUF_DATA_ALIGN_SIZE	(1<<PARAM_BUF_DATA_ALIGN_SHIFT)
#define PARAM_BUF_DATA_ALIGN_MASK	(~(PARAM_BUF_DATA_ALIGN_SIZE - 1))

#define PARAM_BUF_DATA_ALIGN(size)   \
		((size + ~PARAM_BUF_DATA_ALIGN_MASK)&(PARAM_BUF_DATA_ALIGN_MASK))

#define PARAM_BUF_SIZE(in_size, out_size)  \
		(sizeof(struct ZM_SVC_CALL_PARAM) + PARAM_BUF_DATA_ALIGN(in_size) \
		+ PARAM_BUF_DATA_ALIGN(out_size))

struct ZM_IOCTL_PARAM {
	int svc_id;
	int ioctl;
	void *in_buf;
	int in_buf_size; /* in buffer size */
	void *out_buf;
	int out_buf_size; /* out buffer size */
	int out_size; /* data written into out buffer */
	int ret; /* return value */
	unsigned char data[0]; /* in/out buffer */
};

#define PARAM_BUF_IN_BUF(param_buf)		((param_buf)->in_buf)
#define PARAM_BUF_IN_BUF_SIZE(param_buf)	((param_buf)->in_buf_size)

#define PARAM_BUF_OUT_BUF(param_buf)		((param_buf)->out_buf)
#define PARAM_BUF_OUT_BUF_SIZE(param_buf)	((param_buf)->out_buf_size)

#define PARAM_BUF_OUT_SIZE(param_buf)		((param_buf)->out_size)
#define PARAM_BUF_RET(param_buf)		((param_buf)->ret)
#define PARAM_BUF_SVC_ID(param_buf)		((param_buf)->svc_id)
#define PARAM_BUF_IOCTL(param_buf)		((param_buf)->ioctl)

#define ZM_SVC_NR_MAX		32

#define ZM_SVC_NAME_MAX	48 /* inlude last null char. */

#define ZM_SVC_INVALID	-1

#define _is_valid_svc_id(id) ((id) >= 0 && (id) < ZM_SVC_NR_MAX)

struct ZM_GET_SVC_ID {
	char name[ZM_SVC_NAME_MAX];
	int id;
};

#define ZM_WAIT_TIMEOUT_INFINITY	0xffffffff
struct ZM_WAIT_SVC_READY {
	int id;
	int ready;
	unsigned timeout; /* ms */
};

struct ZM_BROADCAST_INFO {
	char name[ZM_SVC_NAME_MAX];
	int data_size;
	void *data;
};

struct ZM_SHARE_BUF_ALLOC {
	int size;
	void *buf;
};

enum ZM_FLUSH_CACHE_OP {
	FLUSH_CACHE_OP_INVALID = 0,
	FLUSH_CACHE_OP_CLEAN,
	FLUSH_CACHE_OP_FLUSH,
};

struct ZM_BUF_FLUSH {
	unsigned long size;
	unsigned long pa;
	int op;
};

struct ZM_NAMED_BUF {
	char name[ZM_SVC_NAME_MAX];
	int size;
	void *buf;
};

struct ZM_SHARE_BUF_ADDR_CONVERT {
	void *va;
	unsigned long pa;
};

#define ZM_APP_NR_MAX		16
struct ZM_APP_INFO {
	char name[ZM_SVC_NAME_MAX];
	int pid;
};

struct ZM_GET_APPS_INFO {
	int app_nr;
	struct ZM_APP_INFO apps[ZM_APP_NR_MAX];
};

struct ZM_GLOBAL_INFO {
	struct ZM_APP_INFO ui_proc;
	int show;
};

struct ZM_SHARE_BUF_RESERVE {
	int size;
	int num;
};

#include <linux/ioctl.h>
#define IOCTL_GET_ID	_IOWR('b', 1, struct ZM_GET_SVC_ID)
#define IOCTL_WAIT	_IOWR('b', 2, struct ZM_WAIT_SVC_READY)
#define IOCTL_REGISTER		_IOWR('b', 3, int)
#define IOCTL_DEREGISTER	_IOWR('b', 4, int)

#define IOCTL_SEND_REQUEST	_IOWR('b', 8, struct ZM_IOCTL_PARAM)
#define IOCTL_SEND_BROADCAST	_IOWR('b', 9, struct ZM_IOCTL_PARAM)

#define IOCTL_SEND_RESPONSE	_IOWR('b', 10, struct ZM_IOCTL_PARAM)

#define IOCTL_GET_REQUEST	_IOWR('b', 11, void *)

#define IOCTL_SHARE_BUF_ALLOC	_IOWR('b', 20, struct ZM_SHARE_BUF_ALLOC)
#define IOCTL_SHARE_BUF_FREE	_IOWR('b', 21, void *)

#define IOCTL_BUF_FLUSH	_IOWR('b', 22, struct ZM_BUF_FLUSH)

#define IOCTL_GET_NAMED_BUF	_IOWR('b', 23, struct ZM_NAMED_BUF)

#define IOCTL_RAM_SIZE			_IOWR('b', 24, int)
#define IOCTL_RAM_START_PA		_IOWR('b', 25, unsigned long)

#define IOCTL_SET_UI_SHOW		_IOWR('b', 29, int)
#define IOCTL_SET_UI_PROC		_IOWR('b', 30, struct ZM_APP_INFO)

#define IOCTL_GET_GLOBAL_INFO_BUF	_IOWR('b', 31, void *)

#define IOCTL_GET_APPS_INFO	_IOWR('b', 32, struct ZM_GET_APPS_INFO)
#define IOCTL_SHARE_BUF_RESERVE	_IOWR('b', 33, struct ZM_SHARE_BUF_RESERVE)

/* proc ioctl */
enum {
	PROC_MSG_IOCTL_BROADCAST = 0,
	PROC_MSG_IOCTL_PROC_EVENT
};

struct ZM_BROADCAST {
	char name[ZM_SVC_NAME_MAX];
	int data_size;
	char data[0];
};

enum {
	PROC_EVENT_START = 0,
	PROC_EVENT_STOP
};
struct ZM_PROC_EVENT {
	int event;
	char proc_name[ZM_SVC_NAME_MAX];
};

/* create by sv */
#define DEAD_FLAG  0xDEADBEEF

struct drv_util {
    /* members */
    int fd;
    void *va_base;
    void *va;
    unsigned long pa;
    unsigned int ram_size;
    unsigned int ram_base;

    /* method */
    void* (*share_buf_alloc)(struct drv_util *util, int size);
    void (*share_buf_free)(struct drv_util *util, void *buf);

    void* (*ram_get_va)(struct drv_util *util, unsigned long pa);
    unsigned long (*ram_get_pa)(struct drv_util *util, void *va);
};

struct drv_util* create_drv_util(void);
void destory_drv_util(struct drv_util *util);

#endif//__DRV_UTIL_H__

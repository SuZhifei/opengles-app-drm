#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "utils_common.h"
#include "drv_util.h"

#define UTIL_DEV    "/dev/zm_util"

static int util_open()
{
    int util_fd = -1;
	util_fd = open(UTIL_DEV, O_RDWR | O_CLOEXEC);
	if (util_fd < 0) {
		ERROR("can not open framebuffer device %s\n", UTIL_DEV);
	}

	return util_fd;
}

static unsigned long get_ram_size(int util_fd)
{
    unsigned long ram_size = DEAD_FLAG;

    /* Get DDR Size */
    if (ioctl(util_fd, IOCTL_RAM_SIZE, &ram_size)) {
		ERROR("IOCTL_RAM_SIZE failed\n");
		ram_size = DEAD_FLAG;
	}

    return ram_size;
}

static unsigned long get_ram_base(int util_fd)
{
    unsigned long ram_base = DEAD_FLAG;

    /* Get DDR physical address */
    if (ioctl(util_fd, IOCTL_RAM_START_PA, &ram_base)) {
		ERROR("IOCTL_RAM_SIZE failed\n");
		ram_base = DEAD_FLAG;
	}

    return ram_base;
}

static void* ram_map(int util_fd, unsigned long ram_base, unsigned long ram_size)
{
    void *ram_va = NULL;

    /* Map all DDR memory to user space */
    ram_va = (void *)mmap(0, ram_size,
                PROT_READ | PROT_WRITE, MAP_SHARED, util_fd, ram_base & ~(ram_size - 1));
    if (ram_va == MAP_FAILED) {
        ERROR_MSG("failed to map for util");
        ram_va = NULL;
        goto _exit;
    }

    DBG("ram get va 0x%08x", ram_va);
_exit:
    return ram_va;
}

static void ram_umap(void *ram_va, unsigned long ram_size)
{
    munmap(ram_va, ram_size);
}

static void* share_buf_alloc(struct drv_util *util, int size)
{
    struct ZM_SHARE_BUF_ALLOC buf_alloc;
    buf_alloc.size = size;
    void *buf = NULL;

    if (util == NULL) {
        ERROR("util is NULL, share_buf_alloc failed");
        goto _exit;
    }

    /* Get continuous physical buffer */
    if (ioctl(util->fd, IOCTL_SHARE_BUF_ALLOC, &buf_alloc)) {
		ERROR("IOCTL_SHARE_BUF_ALLOC failed\n");
        goto _exit;
	}
    buf = buf_alloc.buf;

_exit:
    return buf;
}

static void share_buf_free(struct drv_util *util, void *buf)
{
    if (util == NULL || buf == NULL) {
        ERROR("util or buf is NULL, share_buf_free failed");
        return;
    }

    if (ioctl(util->fd, IOCTL_SHARE_BUF_FREE, buf)) {
		ERROR("IOCTL_SHARE_BUF_FREE failed\n");
        return;
	}
}

static void* ram_get_va(struct drv_util *util, unsigned long pa)
{
    void *va = NULL;

    if (util == NULL) {
        ERROR("util is NULL, ram_get_va failed");
        goto _exit;
    }

    /* formula: vitrual_addr = vma->vm_start + phys_addr - phys_offset */
    va = util->va_base + pa - util->ram_base;
    DBG("ram_get_va 0x%x", va);

_exit:
    return va;
}

static unsigned long ram_get_pa(struct drv_util *util, void *va)
{
    unsigned long pa = DEAD_FLAG;
    if (util == NULL || va == NULL) {
        ERROR("util or va is NULL, ram_get_pa failed");
        return DEAD_FLAG;
    }

    /* formula: phys_addr = virtual_addr - vma->vm_start + phys_offset */
    pa = va - util->va_base + util->ram_base;
    DBG("ram_get_pa 0x%x", pa);
    return pa;
}

struct drv_util* create_drv_util(void)
{
    int ret = false;
    struct drv_util* util = (struct drv_util *)malloc(sizeof(struct drv_util));

    if (util == NULL) {
        ERROR("malloc drv_util failed");
        goto _exit;
    }

    util->fd = util_open();
    if (util->fd == -1) {
        ERROR("util driver open failed");
        goto _exit;
    }

    util->ram_size = get_ram_size(util->fd);
    if (util->ram_size == DEAD_FLAG) {
        ERROR("util get_ram_size failed");
        goto _exit;
    }

    util->ram_base = get_ram_base(util->fd);
    if (util->ram_base == DEAD_FLAG) {
        ERROR("util get_ram_base failed");
        goto _exit;
    }

    util->va_base = ram_map(util->fd, util->ram_base, util->ram_size);
    if (util->va_base == NULL) {
        ERROR("util ram_map failed");
        goto _exit;
    }

    util->share_buf_alloc = share_buf_alloc;
    util->share_buf_free = share_buf_free;
    util->ram_get_pa = ram_get_pa;
    util->ram_get_va = ram_get_va;

    DBG("create_drv_util successful !!");
    ret = true;

_exit:
    if (!ret && util != NULL) {
        free(util);
        util = NULL;
    }
    return util;
}

void destory_drv_util(struct drv_util *util)
{
    if (util != NULL) {
        ram_umap(util->va_base, util->ram_size);
        close(util->fd);
        free(util);
    }
}

#ifndef __SHADOW_RGA_H__
#define __SHADOW_RGA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "minigui.h"

#if ENABLE_RGA
#include <rga/RgaApi.h>
#endif

void *shadow_rga_g_bo_ptr(void);
int shadow_rga_init(int size);
void shadow_rga_exit(void);
void shadow_rga_refresh(int fd, int offset, int src_w, int src_h,
                        int dst_w, int dst_h, int rotate);
int yuv_draw(char *src_ptr, int src_fd, int format, int src_w, int src_h);
MG_EXPORT void GUIAPI shadow_rga_get_user_fd(int *fd, int *screen_w, int *screen_h);
MG_EXPORT void GUIAPI shadow_rga_switch(void *src_ptr, int src_fd, int src_fmt, int src_w, int src_h);

#ifdef __cplusplus
}
#endif

#endif

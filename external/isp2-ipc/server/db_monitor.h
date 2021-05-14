#if CONFIG_DBSERVER

#ifndef __DB_MONITOR_H
#define __DB_MONITOR_H

#include <errno.h>
#include <gdbus.h>
#include <glib.h>
#include <linux/kernel.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "call_fun_ipc.h"
#include "config.h"
#include "mediactl/mediactl.h"
#include "json-c/json.h"
#include "rkaiq/uAPI/rk_aiq_user_api_imgproc.h"
#include "rkaiq/uAPI/rk_aiq_user_api_sysctl.h"
#include "dbserver.h"
#include "../utils/log.h"

#ifdef __cplusplus
extern "C" {
#endif

void database_init(void);
char *dbserver_image_hdr_mode_get(void);
void dbserver_image_blc_get(char *hdr_mode, int *hdr_level);
void dbserver_image_enhancement_get(char *nr_mode, char *fec_mode, char *dehaze_mode,
            int *denoise_level, int *spatial_level, int *temporal_level, int *dehaze_level);
void dbserver_image_adjustment_get(int *brightness, int *contrast, int *saturation, int *sharpness, int *hue);
void dbserver_image_exposure_get(char *exposure_time, int *exposure_gain);
void dbserver_image_white_balance_get(char *white_balance_style, int *red_gain, int *blue_gain);
void dbserver_image_video_adjustment_get(char *frequency_mode);
void dbserver_image_night_to_day_get(rk_aiq_cpsl_cfg_t *cpsl_cfg);

void blc_hdr_level_set(int level);
void nr_mode_set(char *mode);
void fec_mode_set(char *mode);
void dehaze_mode_set(char *mode);
void exposure_time_set(char *time);
void exposure_gain_set(int gain);
void manual_white_balance_set();
void white_balance_style_set(char *style);
void frequency_mode_set(char *mode);
void night_to_day_param_set(void);

void night_to_day_param_cap_set_db(void);

void brightness_set(int level);
void contrast_set(int level);
void saturation_set(int level);
void sharpness_set(int level);

#ifdef __cplusplus
}
#endif

#endif

#endif

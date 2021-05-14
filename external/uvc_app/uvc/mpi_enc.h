/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __MPI_ENC_H__
#define __MPI_ENC_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#include "vld.h"
#endif

#define MODULE_TAG "mpi_enc"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rockchip/rk_mpi.h>

//#include "mpp_env.h"
//#include "mpp_mem.h"
//#include "printf.h"
//#include "mpp_time.h"
#include "mpp_common.h"

//#include "utils.h"
#define BIT(n)  (1<<(n))

#define MAX_FILE_NAME_LENGTH        256
#define RK_MPP_VERSION_DEFAULT 1
#define RK_MPP_USE_FULL_RANGE 0
#define RK_MPP_H264_FORCE_IDR_COUNT 5
#define RK_MPP_H264_FORCE_IDR_PERIOD 5 //must >=1
#define RK_MPP_ENC_TEST_NATIVE 0
#define RK_MPP_USE_ZERO_COPY 1

#if RK_MPP_USE_ZERO_COPY
#define RK_MPP_USE_DESTORY_BUFF_THREAD
#if !RK_MPP_ENC_TEST_NATIVE
#define RK_MPP_USE_UVC_VIDEO_BUFFER
#endif
extern struct uvc_encode uvc_enc;
#endif
#define RK_MPP_DYNAMIC_DEBUG_ON 1 //release version can set to 0
#define RK_MPP_RANGE_DEBUG_ON 1 //release version can set to 0

#define RK_MPP_DYNAMIC_DEBUG_OUT_CHECK "/tmp/uvc_enc_out"
#define RK_MPP_DYNAMIC_DEBUG_IN_CHECK "/tmp/uvc_enc_in" //open it will lower the fps
#define RK_MPP_RANGE_DEBUG_IN_CHECK "/tmp/uvc_range_in"
#define RK_MPP_OUT_LEN_DEBUG_CHECK "/tmp/uvc_out_len"
#define RK_MPP_CLOSE_FRM_LOSS_DEBUG_CHECK "/tmp/uvc_frm_loss"

#define RK_MPP_ENABLE_UVC_H265 "/tmp/use_encodec_h265"

#define RK_MPP_DEBUG_OUT_FILE "/data/uvc_enc_out.bin"
#define RK_MPP_DEBUG_IN_FILE "/data/uvc_enc_in.bin"

#define RK_MPP_ENC_CFG_ORIGINAL_PATH "/etc/mpp_enc_cfg.conf"
#define RK_MPP_ENC_CFG_MODIFY_PATH "/data/mpp_enc_cfg.conf"
//#define DEBUG_OUTPUT 1
#if RK_MPP_ENC_TEST_NATIVE
extern struct uvc_encode uvc_enc;
extern int uvc_encode_init(struct uvc_encode *e, int width, int height, int fcc, int h265);
#define TEST_ENC_TPYE V4L2_PIX_FMT_H264 //V4L2_PIX_FMT_MJPEG
#endif
#define MPP_ENC_CFG_MIN_FPS 1
#define MPP_ENC_CFG_MAX_FPS 60

#define MPP_ENC_CFG_MIN_BPS 2 * 1000
#define MPP_ENC_CFG_MAX_BPS 98 * 1000 * 1000
#define MPP_ENC_MJPEG_CFG_MAX_BPS 150 * 1024 * 1024
#define MPP_ENC_CFG_H264_DEFAULT_PROFILE 100
#define MPP_ENC_CFG_H264_DEFAULT_LEVEL 40
#define MPP_ENC_MJPEG_FRC_USE_MPP 0

//**********for simple frc************//
#define MJPEG_FRC_BPS_MAX 150*1024*1024
#define MJPEG_FRC_BPS_MIN 100*1024*1024
#define MJPEG_FRC_BPS_PER_STEP 1*1024*1024

#define MJPEG_FRC_QUANT_MAX 10
#define MJPEG_FRC_QUANT_MIN 7

#define MJPEG_FRC_QFACTOR_MAX 99
#define MJPEG_FRC_QFACTOR_MIN 80

#define MPP_FRC_WAIT_TIME_MS 2
#define MPP_FRC_WAIT_TIME_US (MPP_FRC_WAIT_TIME_MS * 1000)
#define MPP_FRC_WAIT_COUNT_MIN (18 / MPP_FRC_WAIT_TIME_MS) // in high resolution, the frame rate is not enough to lower this

#define MPP_FRC_UP_FRM_SET_INIT 1000
#define MPP_FRC_UP_FRM_SET_MIN  MPP_FRC_UP_FRM_SET_INIT
#define MPP_FRC_UP_FRM_SET_MAX 0x40000000

#define MPP_FRC_WAIT_COUNT_OFFSET 5 // bit rate control is too low to increase this

//***************ROI*********************//
#define MPP_ENC_ROI_ENABLE 1
#define UPALIGNTO(value, align) ((value + align - 1) & (~(align - 1)))
#define UPALIGNTO16(value) UPALIGNTO(value, 16)
#define VALUE_SCOPE_CHECK(X, MIN, MAX) assert((X >= MIN) && (X <= MAX))

typedef struct  {
    uint16_t x;            /**< horizontal position of top left corner */
    uint16_t y;            /**< vertical position of top left corner */
    uint16_t w;            /**< width of ROI rectangle */
    uint16_t h;            /**< height of ROI rectangle */
    uint16_t intra;        /**< flag of forced intra macroblock */
    int16_t quality;      /**<  qp of macroblock */
    uint16_t qp_area_idx;  /**< qp min max area select*/
    uint8_t  area_map_en;  /**< enable area map */
    uint8_t  abs_qp_en;    /**< absolute qp enable flag*/
} EncROIRegion;


enum SIMPLE_FRC_MODE
{
    FRC_OFF = 0,
    FRC_ONLY_LOW,
    FRC_BOTH_UP_LOW,
};

//**********************//

typedef struct
{
    MppCodingType   type;
    RK_U32          width;
    RK_U32          height;
    MppFrameFormat  format;
    RK_U32          debug;
    RK_U32          num_frames;

    RK_U32          have_output;
} MpiEncTestCmd;
typedef struct
{
    RK_U32 init;
    RK_U32 max;
    RK_U32 min;
    RK_U32 step;
    RK_U32 max_i_qp;
    RK_U32 min_i_qp;

} MpiEncQqCfg;
/********************do not change the order below*******************/
#define MPP_ENC_CFG_CHANGE_BIT(x) (1 << x)
typedef struct
{
    RK_U32 change;
    RK_U32 fbc;
    RK_U32 split_mode;
    RK_U32 split_arg;
    RK_U32 force_idr_count;
    RK_U32 force_idr_period;
    RK_U32 frc_fps;
    enum SIMPLE_FRC_MODE frc_mode;
    RK_U32 enc_time;
    RK_U32 try_count;
} MpiEncCommonCfg;

typedef struct
{
    RK_U32 change;
    RK_U32 frc_quant; //
    RK_S32 frc_qfactor;  //
    RK_U32 frc_bps;

    RK_U32 quant; //bit0      1- 10 default:7
    MppFrameColorRange range; // bit1   full:MPP_FRAME_RANGE_JPEG  limit:MPP_FRAME_RANGE_MPEG;
    RK_U32 qfactor; // bit2   0-99  priprity option this. set 0 is close this and set use quant
    RK_S32 qfactor_min;  // 3
    RK_S32 qfactor_max;  // 4
    RK_U32 gop;  // 5
    MppEncRcMode rc_mode; // 6
    RK_U32 bps; // 7
    RK_U32 framerate; // 8
} MpiEncMjpegCfg;

typedef struct
{
    RK_U32 change;
    RK_U32 gop; //0
    MppEncRcMode rc_mode;
    RK_U32 framerate; ////simple to set.
    MppFrameColorRange range; // full: MPP_FRAME_RANGE_JPEG  limit:MPP_FRAME_RANGE_MPEG;
    MppEncHeaderMode head_each_idr;
    MppEncSeiMode sei;//5
    MpiEncQqCfg qp;//6-9
    RK_U32 profile;//10
    RK_U32 cabac_en;
    RK_U32 cabac_idc;
    RK_U32 trans_8x8;//
    RK_U32 level;
    RK_U32 bps;//15
} MpiEncH264Cfg;

typedef struct
{
    RK_U32 change;
    RK_U32 gop; //0
    MppEncRcMode rc_mode;
    RK_U32 framerate;
    MppFrameColorRange range; //full: MPP_FRAME_RANGE_JPEG  limit:MPP_FRAME_RANGE_MPEG;
    MppEncHeaderMode head_each_idr;
    bool sei;//5
    MpiEncQqCfg qp;//6-11
    RK_U32 bps;//12
} MpiEncH265Cfg;

/***************************o not change the order above**************************************/
typedef struct DestoryNode
{
    MppFrame destory_frame;
    MppPacket destory_packet;
    MppBuffer destory_buf;
    MppBuffer destory_pkt_buf_out;
    bool unfinished;
    RK_U32 count;
} MppDestoryInfo;

typedef struct
{
    // global flow control flag
    RK_U32 frm_eos;
    RK_U32 pkt_eos;
    RK_U32 frame_count;
    RK_U64 stream_size;
#if RK_MPP_RANGE_DEBUG_ON
#define RANGE_PATH_MAX_LEN 128
    FILE *fp_range_path;
    FILE *fp_range_file;
    char *range_path;

#endif
    // src and dst
    FILE *fp_input;
    FILE *fp_output;

    // base flow context
    MppCtx ctx;
    MppApi *mpi;
    RK_U32 use_legacy_cfg;
    MppEncCfg cfg;
    MppEncPrepCfg prep_cfg;
    MppEncRcCfg rc_cfg;
    MppEncCodecCfg codec_cfg;
    MppEncSliceSplit split_cfg;
    MppEncOSDPltCfg osd_plt_cfg;
    MppEncOSDPlt     osd_plt;
    MppEncOSDData    osd_data;
#if MPP_ENC_ROI_ENABLE
    RK_U32 roi_enable;
    RK_U32 roi_number;
    MppEncROIRegion *roi_region;
    MppEncROICfg     roi_cfg;
#endif
    // input / output
    MppBuffer frm_buf;
    MppEncSeiMode sei_mode;
    MppEncHeaderMode header_mode;
    MppBuffer osd_idx_buf;

    // paramter for resource malloc
    RK_U32 width;
    RK_U32 height;
    RK_U32 hor_stride;
    RK_U32 ver_stride;
    MppFrameFormat fmt;
    MppCodingType type;
    RK_S32 num_frames;
    RK_S32 loop_times;

    // resources
    size_t header_size;
    size_t frame_size;
    /* NOTE: packet buffer may overflow */
    size_t packet_size;
    /*
    * osd idx size range from 16x16 bytes(pixels) to hor_stride*ver_stride(bytes).
    * for general use, 1/8 Y buffer is enough.
    */
    size_t osd_idx_size;
    RK_U32 plt_table[8];

    RK_U32 osd_enable;
    RK_U32 osd_mode;
    RK_U32 split_mode;
    RK_U32 split_arg;

    RK_U32 user_data_enable;

    // rate control runtime parameter
    RK_S32 gop;
    RK_S32 fps_in_flex;
    RK_S32 fps_in_den;
    RK_S32 fps_in_num;
    RK_S32 fps_out_flex;
    RK_S32 fps_out_den;
    RK_S32 fps_out_num;
    RK_S32 bps;
    RK_U32 gop_mode;

    MppFrame frame;
    MppPacket packet;
    void *enc_data;
    size_t enc_len;
    RK_U32 enc_version;
    RK_U32 h2645_frm_count;
#if RK_MPP_USE_ZERO_COPY
    MppBufferGroup pkt_grp;
    MppBuffer pkt_buf;
#endif
    MpiEncCommonCfg common_cfg;
    MpiEncMjpegCfg mjpeg_cfg;
    MpiEncH264Cfg h264_cfg;
    MpiEncH265Cfg h265_cfg;
    pthread_t check_cfg_change_hd;

    int cfg_notify_fd;
    int cfg_notify_wd;
#ifdef RK_MPP_USE_DESTORY_BUFF_THREAD
    MppDestoryInfo destory_info;

    pthread_t destory_buf_hd;
    sigset_t sig;
    pthread_cond_t cond;
    pthread_mutex_t cond_mutex;

    RK_U32 loss_frm;
    RK_U32 continuous_frm;
    RK_U32 frc_up_frm_set;
    bool set_frc_low;
//    pthread_mutex_t destory_mutex;
#endif
} MpiEncTestData;

typedef struct MPP_ENC_INFO {
    int fd;
    size_t size;
    RK_U32 pts;
} MPP_ENC_INFO_DEF;

MPP_RET mpi_enc_test_init(MpiEncTestCmd *cmd, MpiEncTestData **data);
MPP_RET mpi_enc_test_run(MpiEncTestData **data, MPP_ENC_INFO_DEF *info);
MPP_RET mpi_enc_test_deinit(MpiEncTestData **data);
void mpi_enc_cmd_config(MpiEncTestCmd *cmd, int width, int height, int fcc, int h265);
void mpi_enc_cmd_config_mjpg(MpiEncTestCmd *cmd, int width, int height);
void mpi_enc_cmd_config_h264(MpiEncTestCmd *cmd, int width, int height);
void mpi_enc_set_format(MppFrameFormat format);
int mpi_enc_get_h264_extra(MpiEncTestData *p, void *buffer, size_t *size);
RK_S32 mpi_get_env_u32(const char *name, RK_U32 *value, RK_U32 default_value);
MPP_RET mpp_enc_cfg_set_s32(MppEncCfg cfg, const char *name, RK_S32 val);
MPP_RET mpp_enc_cfg_set_u32(MppEncCfg cfg, const char *name, RK_U32 val);
MPP_RET mpp_enc_cfg_set_s64(MppEncCfg cfg, const char *name, RK_S64 val);
MPP_RET mpp_enc_cfg_set_u64(MppEncCfg cfg, const char *name, RK_U64 val);
MPP_RET mpp_enc_cfg_set_ptr(MppEncCfg cfg, const char *name, void *val);
MPP_RET mpp_enc_cfg_get_s32(MppEncCfg cfg, const char *name, RK_S32 *val);
MPP_RET mpp_enc_cfg_get_u32(MppEncCfg cfg, const char *name, RK_U32 *val);
MPP_RET mpp_enc_cfg_get_s64(MppEncCfg cfg, const char *name, RK_S64 *val);
MPP_RET mpp_enc_cfg_get_u64(MppEncCfg cfg, const char *name, RK_U64 *val);

#ifdef RK_MPP_USE_UVC_VIDEO_BUFFER
struct uvc_buffer *uvc_buffer_write_get(int id);
void uvc_buffer_read_set(int id, struct uvc_buffer *buf);
void uvc_user_lock();
void uvc_user_unlock();
struct uvc_buffer *uvc_buffer_write_get_nolock(int id);
void uvc_buffer_read_set_nolock(int id, struct uvc_buffer *buf);

#endif
#ifdef __cplusplus
}
#endif

#endif

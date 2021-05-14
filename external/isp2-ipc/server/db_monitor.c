#if CONFIG_DBSERVER

#include "db_monitor.h"

extern char *aiq_NR_mode;
extern char *aiq_FEC_mode;
extern rk_aiq_sys_ctx_t *db_aiq_ctx;

static DBusConnection *connection = 0;

#define DBSERVER  "rockchip.dbserver"
#define DBSERVER_PATH      "/"

#define DBSERVER_MEDIA_INTERFACE  DBSERVER ".media"

#define TABLE_IMAGE_SCENARIO "image_scenario"
#define TABLE_IMAGE_ADJUSTMENT "image_adjustment"
#define TABLE_IMAGE_EXPOSURE "image_exposure"
#define TABLE_IMAGE_NIGHT_TO_DAY "image_night_to_day"
#define TABLE_IMAGE_BLC "image_blc"
#define TABLE_IMAGE_WHITE_BLANCE "image_white_blance"
#define TABLE_IMAGE_ENHANCEMENT "image_enhancement"
#define TABLE_IMAGE_VIDEO_ADJUSTMEN "image_video_adjustment"

static void DataChanged(char *json_str)
{
    LOG_INFO("DataChanged, json is %s\n", json_str);
    json_object *j_cfg;
    json_object *j_key = 0;
    json_object *j_data = 0;
    char *table = 0;

    j_cfg = json_tokener_parse(json_str);
    table = (char *)json_object_get_string(json_object_object_get(j_cfg, "table"));
    j_key = json_object_object_get(j_cfg, "key");
    j_data = json_object_object_get(j_cfg, "data");
    char *cmd = (char *)json_object_get_string(json_object_object_get(j_cfg, "cmd"));

    if (g_str_equal(table, TABLE_IMAGE_BLC)) {
        if (!g_str_equal(cmd, "Update"))
            return;
        json_object *sHDR = json_object_object_get(j_data, "sHDR");
        json_object *iHDRLevel = json_object_object_get(j_data, "iHDRLevel");
        if (sHDR) {
            char *HDR_mode = (char *)json_object_get_string(sHDR);
            LOG_INFO("%s, HDR_mode is %s\n", __func__, HDR_mode);
            // if (!strcmp(HDR_mode, "close"))
            //     rk_aiq_uapi_setHDRMode(db_aiq_ctx, OP_AUTO);
            // else
            //     rk_aiq_uapi_setHDRMode(db_aiq_ctx, OP_MANUAL);
        }
        if (iHDRLevel) {
            int level = json_object_get_int(iHDRLevel);
            blc_hdr_level_set(level);
        }
    } else if (g_str_equal(table, TABLE_IMAGE_ENHANCEMENT)) {
        if (!g_str_equal(cmd, "Update"))
            return;
        json_object *sNoiseReduceMode = json_object_object_get(j_data, "sNoiseReduceMode");
        json_object *iDenoiseLevel = json_object_object_get(j_data, "iDenoiseLevel");
        json_object *iSpatialDenoiseLevel = json_object_object_get(j_data, "iSpatialDenoiseLevel");
        json_object *iTemporalDenoiseLevel = json_object_object_get(j_data, "iTemporalDenoiseLevel");
        json_object *sFEC = json_object_object_get(j_data, "sFEC");
        json_object *sDehaze = json_object_object_get(j_data, "sDehaze");
        json_object *iDehazeLevel = json_object_object_get(j_data, "iDehazeLevel");
        if (sNoiseReduceMode) {
            char *NR_mode = (char *)json_object_get_string(sNoiseReduceMode);
            nr_mode_set(NR_mode);
        }
        if (iDenoiseLevel) {
            int level = json_object_get_int(iDenoiseLevel);
            rk_aiq_uapi_setANRStrth(db_aiq_ctx, level);
        }
        if (iSpatialDenoiseLevel) {
            int level = json_object_get_int(iSpatialDenoiseLevel);
            rk_aiq_uapi_setMTNRStrth(db_aiq_ctx, true, level);
        }
        if (iTemporalDenoiseLevel) {
            int level = json_object_get_int(iTemporalDenoiseLevel);
            rk_aiq_uapi_setMSpaNRStrth(db_aiq_ctx, true, level);
        }
        if (sFEC) {
            char *FEC_mode = (char *)json_object_get_string(sFEC);
            fec_mode_set(FEC_mode);
        }
        if (sDehaze) {
            char *Dehaze_mode = (char *)json_object_get_string(sDehaze);
            dehaze_mode_set(Dehaze_mode);
        }
        if (iDehazeLevel) {
            int level = json_object_get_int(iDehazeLevel);
            rk_aiq_uapi_setMDhzStrth(db_aiq_ctx, true, level);
        }
    } else if (g_str_equal(table, TABLE_IMAGE_ADJUSTMENT)) {
        if (!g_str_equal(cmd, "Update"))
            return;
        json_object *iBrightness = json_object_object_get(j_data, "iBrightness");
        json_object *iContrast = json_object_object_get(j_data, "iContrast");
        json_object *iSaturation = json_object_object_get(j_data, "iSaturation");
        json_object *iSharpness = json_object_object_get(j_data, "iSharpness");
        json_object *iHue = json_object_object_get(j_data, "iHue");
        if (iBrightness) {
            int brightness = json_object_get_int(iBrightness);
            brightness_set(brightness);
        }
        if (iContrast) {
            int contrast = json_object_get_int(iContrast);
            contrast_set(contrast);
        }
        if (iSaturation) {
            int saturation = json_object_get_int(iSaturation);
            saturation_set(saturation);
        }
        if (iSharpness) {
            int sharpness = json_object_get_int(iSharpness);
            sharpness_set(sharpness);
        }
        if (iHue) {
            int hue = json_object_get_int(iHue);
            hue_set(hue);
        }
    } else if (g_str_equal(table, TABLE_IMAGE_EXPOSURE)) {
        if (!g_str_equal(cmd, "Update"))
            return;
        json_object *sExposureTime = json_object_object_get(j_data, "sExposureTime");
        json_object *iExposureGain = json_object_object_get(j_data, "iExposureGain");
        if (sExposureTime) {
            char *time = (char *)json_object_get_string(sExposureTime);
            exposure_time_set(time);
        }
        if (iExposureGain) {
            int gain = json_object_get_int(iExposureGain);
            exposure_gain_set(gain);
        }
    } else if (g_str_equal(table, TABLE_IMAGE_WHITE_BLANCE)) {
        if (!g_str_equal(cmd, "Update"))
            return;
        json_object *sWhiteBalanceStyle = json_object_object_get(j_data, "sWhiteBlanceStyle");
        json_object *iWhiteBalanceRed = json_object_object_get(j_data, "iWhiteBalanceRed");
        json_object *iWhiteBalanceBlue = json_object_object_get(j_data, "iWhiteBalanceBlue");
        if (sWhiteBalanceStyle) {
            char *style = (char *)json_object_get_string(sWhiteBalanceStyle);
            white_balance_style_set(style);
        }
        if ((iWhiteBalanceRed) || (iWhiteBalanceBlue))
            manual_white_balance_set();
    } else if (g_str_equal(table, TABLE_IMAGE_VIDEO_ADJUSTMEN)) {
        if (!g_str_equal(cmd, "Update"))
            return;
        json_object *sPowerLineFrequencyMode = json_object_object_get(j_data, "sPowerLineFrequencyMode");
        if (sPowerLineFrequencyMode) {
            char *frequency_mode = (char *)json_object_get_string(sPowerLineFrequencyMode);
            frequency_mode_set(frequency_mode);
        }
    } else if (g_str_equal(table, TABLE_IMAGE_NIGHT_TO_DAY)) {
        if (!g_str_equal(cmd, "Update"))
            return;
        night_to_day_param_set();
    }

    json_object_put(j_cfg);
}

static DBusHandlerResult database_monitor_changed(
    DBusConnection *connection,
    DBusMessage *message, void *user_data)
{
    LOG_INFO("database_monitor_changed\n");
    bool *enabled = user_data;
    DBusMessageIter iter;
    DBusHandlerResult handled;

    handled = DBUS_HANDLER_RESULT_HANDLED;
    if (dbus_message_is_signal(message, DBSERVER_MEDIA_INTERFACE,
                               "DataChanged")) {
        char *json_str;

        dbus_message_iter_init(message, &iter);
        dbus_message_iter_get_basic(&iter, &json_str);
        DataChanged(json_str);

        return handled;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

char *dbserver_image_hdr_mode_get(void)
{
    char *json_str = NULL;
    json_str = dbserver_media_get(TABLE_IMAGE_BLC);
    // LOG_INFO("%s, json_str is %s\n", __func__, json_str);
    if (json_str == NULL) {
        LOG_INFO("image blc table is null\n");
        return NULL;
    }
    json_object *j_cfg = json_tokener_parse(json_str);
    json_object *j_data_array = json_object_object_get(j_cfg, "jData");
    json_object *j_data = json_object_array_get_idx(j_data_array, 0);
    char *HDR_mode = (char *)json_object_get_string(json_object_object_get(j_data, "sHDR"));
    json_object_put(j_cfg);
    free(json_str);
    return HDR_mode;
}

void dbserver_image_blc_get(char *hdr_mode, int *hdr_level)
{
    char *json_str = NULL;
    json_str = dbserver_media_get(TABLE_IMAGE_BLC);
    LOG_INFO("%s, json_str is %s\n", __func__, json_str);
    if (json_str == NULL) {
        LOG_INFO("image blc table is null\n");
        return;
    }
    json_object *j_cfg = json_tokener_parse(json_str);
    json_object *j_data_array = json_object_object_get(j_cfg, "jData");
    json_object *j_data = json_object_array_get_idx(j_data_array, 0);
    if (hdr_mode) {
        char *hdr = (char *)json_object_get_string(json_object_object_get(j_data, "sHDR"));
        strncpy(hdr_mode, hdr, strlen(hdr));
        hdr_mode[strlen(hdr)] = '\0';
    }
    if (hdr_level)
        *hdr_level = (int)json_object_get_int(json_object_object_get(j_data, "iHDRLevel"));
    json_object_put(j_cfg);
    free(json_str);
}

void dbserver_image_enhancement_get(char *nr_mode, char *fec_mode, char *dehaze_mode,
    int *denoise_level, int *spatial_level, int *temporal_level, int *dehaze_level)
{
    char *json_str = NULL;
    json_str = dbserver_media_get(TABLE_IMAGE_ENHANCEMENT);
    LOG_INFO("%s, json_str is %s\n", __func__, json_str);
    if (json_str == NULL) {
        LOG_INFO("image enhancement table is null\n");
        return;
    }
    json_object *j_cfg = json_tokener_parse(json_str);
    json_object *j_data_array = json_object_object_get(j_cfg, "jData");
    json_object *j_data = json_object_array_get_idx(j_data_array, 0);
    if (nr_mode) {
        char *nr = (char *)json_object_get_string(json_object_object_get(j_data, "sNoiseReduceMode"));
        strncpy(nr_mode, nr, strlen(nr));
        nr_mode[strlen(nr)] = '\0';
    }
    if (fec_mode) {
        char *fec = (char *)json_object_get_string(json_object_object_get(j_data, "sFEC"));
        strncpy(fec_mode, fec, strlen(fec));
        fec_mode[strlen(fec)] = '\0';
    }
    if (dehaze_mode) {
        char *dehaze = (char *)json_object_get_string(json_object_object_get(j_data, "sDehaze"));
        strncpy(dehaze_mode, dehaze, strlen(dehaze));
        dehaze_mode[strlen(dehaze)] = '\0';
    }
    if (denoise_level)
        *denoise_level = (int)json_object_get_int(json_object_object_get(j_data, "iDenoiseLevel"));
    if (spatial_level)
        *spatial_level = (int)json_object_get_int(json_object_object_get(j_data, "iSpatialDenoiseLevel"));
    if (temporal_level)
        *temporal_level = (int)json_object_get_int(json_object_object_get(j_data, "iTemporalDenoiseLevel"));
    if (dehaze_level)
        *dehaze_level = (int)json_object_get_int(json_object_object_get(j_data, "iDehazeLevel"));
    json_object_put(j_cfg);
    free(json_str);
}

void dbserver_image_adjustment_get(int *brightness, int *contrast, int *saturation, int *sharpness, int *hue)
{
    char *json_str = NULL;
    json_str = dbserver_media_get(TABLE_IMAGE_ADJUSTMENT);
    LOG_INFO("%s, json_str is %s\n", __func__, json_str);
    if (json_str == NULL) {
        LOG_INFO("image adjustment table is null\n");
        return;
    }
    json_object *j_cfg = json_tokener_parse(json_str);
    json_object *j_data_array = json_object_object_get(j_cfg, "jData");
    json_object *j_data = json_object_array_get_idx(j_data_array, 0);
    *brightness = (int)json_object_get_int(json_object_object_get(j_data, "iBrightness"));
    *contrast = (int)json_object_get_int(json_object_object_get(j_data, "iContrast"));
    *saturation = (int)json_object_get_int(json_object_object_get(j_data, "iSaturation"));
    *sharpness = (int)json_object_get_int(json_object_object_get(j_data, "iSharpness"));
    *hue = (int)json_object_get_int(json_object_object_get(j_data, "iHue"));
    json_object_put(j_cfg);
    free(json_str);
}

void dbserver_image_exposure_get(char *exposure_time, int *exposure_gain)
{
    char *json_str = NULL;
    json_str = dbserver_media_get(TABLE_IMAGE_EXPOSURE);
    LOG_INFO("%s, json_str is %s\n", __func__, json_str);
    if (json_str == NULL) {
        LOG_INFO("image exposure table is null\n");
        return;
    }
    json_object *j_cfg = json_tokener_parse(json_str);
    json_object *j_data_array = json_object_object_get(j_cfg, "jData");
    json_object *j_data = json_object_array_get_idx(j_data_array, 0);
    char *time = (char *)json_object_get_string(json_object_object_get(j_data, "sExposureTime"));
    strncpy(exposure_time, time, strlen(time));
    exposure_time[strlen(time)] = '\0';
    *exposure_gain = (int)json_object_get_int(json_object_object_get(j_data, "iExposureGain"));
    json_object_put(j_cfg);
    free(json_str);
}

void dbserver_image_white_balance_get(char *white_balance_style, int *red_gain, int *blue_gain)
{
    char *json_str = NULL;
    json_str = dbserver_media_get(TABLE_IMAGE_WHITE_BLANCE);
    LOG_INFO("%s, json_str is %s\n", __func__, json_str);
    if (json_str == NULL) {
        LOG_INFO("image white balance table is null\n");
        return;
    }
    json_object *j_cfg = json_tokener_parse(json_str);
    json_object *j_data_array = json_object_object_get(j_cfg, "jData");
    json_object *j_data = json_object_array_get_idx(j_data_array, 0);
    if (white_balance_style) {
        char *style = (char *)json_object_get_string(json_object_object_get(j_data, "sWhiteBlanceStyle"));
        strncpy(white_balance_style, style, strlen(style));
        white_balance_style[strlen(style)] = '\0';
    }
    if (red_gain)
        *red_gain = (int)json_object_get_int(json_object_object_get(j_data, "iWhiteBalanceRed"));
    if (blue_gain)
        *blue_gain = (int)json_object_get_int(json_object_object_get(j_data, "iWhiteBalanceBlue"));
    json_object_put(j_cfg);
    free(json_str);
}

void dbserver_image_video_adjustment_get(char *frequency_mode)
{
    char *json_str = NULL;
    json_str = dbserver_media_get(TABLE_IMAGE_VIDEO_ADJUSTMEN);
    LOG_INFO("%s, json_str is %s\n", __func__, json_str);
    if (json_str == NULL) {
        LOG_INFO("image video adjustment table is null\n");
        return;
    }
    json_object *j_cfg = json_tokener_parse(json_str);
    json_object *j_data_array = json_object_object_get(j_cfg, "jData");
    json_object *j_data = json_object_array_get_idx(j_data_array, 0);
    char *frequency = (char *)json_object_get_string(json_object_object_get(j_data, "sPowerLineFrequencyMode"));
    strncpy(frequency_mode, frequency, strlen(frequency));
    frequency_mode[strlen(frequency)] = '\0';
    json_object_put(j_cfg);
    free(json_str);
}

void dbserver_image_night_to_day_get(rk_aiq_cpsl_cfg_t *cpsl_cfg)
{
    char *json_str = NULL;
    json_str = dbserver_media_get(TABLE_IMAGE_NIGHT_TO_DAY);
    LOG_INFO("%s, json_str is %s\n", __func__, json_str);
    if (json_str == NULL) {
        LOG_INFO("image video adjustment table is null\n");
        return;
    }
    json_object *j_cfg = json_tokener_parse(json_str);
    json_object *j_data_array = json_object_object_get(j_cfg, "jData");
    json_object *j_data = json_object_array_get_idx(j_data_array, 0);
    char *mode = (char *)json_object_get_string(json_object_object_get(j_data, "sNightToDay"));
    if (!strcmp(mode, "auto")) {
        cpsl_cfg->mode = RK_AIQ_OP_MODE_AUTO;
        cpsl_cfg->gray_on = false;
        cpsl_cfg->u.a.sensitivity = (float)json_object_get_int(json_object_object_get(j_data, "iNightToDayFilterLevel"));
        cpsl_cfg->u.a.sw_interval = (uint32_t)json_object_get_int(json_object_object_get(j_data, "iNightToDayFilterTime"));
    } else if (!strcmp(mode, "day")) {
        cpsl_cfg->mode = RK_AIQ_OP_MODE_MANUAL;
        cpsl_cfg->gray_on = false;
        cpsl_cfg->u.m.on = 1;
        // set iLightBrightness=0 to force switch to day mode
        cpsl_cfg->u.m.strength_led = (float)0;
    } else if (!strcmp(mode, "night")) {
        cpsl_cfg->mode = RK_AIQ_OP_MODE_MANUAL;
        cpsl_cfg->gray_on = true;
        cpsl_cfg->u.m.on = 1;
        cpsl_cfg->u.m.strength_led = (float)json_object_get_int(json_object_object_get(j_data, "iLightBrightness"));
    } else {
        LOG_INFO("Not currently supported\n");
        cpsl_cfg->mode = RK_AIQ_OP_MODE_INVALID;
    }
    char *fill_light_mode = (char *)json_object_get_string(json_object_object_get(j_data, "sFillLightMode"));
    if (!strcmp(fill_light_mode, "LED"))
        cpsl_cfg->lght_src = RK_AIQ_CPSLS_LED;
    else if (!strcmp(fill_light_mode, "IR"))
        cpsl_cfg->lght_src = RK_AIQ_CPSLS_IR;
    else if (!strcmp(fill_light_mode, "MIX"))
        cpsl_cfg->lght_src = RK_AIQ_CPSLS_MIX;
    json_object_put(j_cfg);
    free(json_str);
}

void blc_hdr_level_set(int level)
{
    LOG_INFO("%s, level is %d\n", __func__, level);
    if (level)
        rk_aiq_uapi_setMHDRStrth(db_aiq_ctx, true, level);
    else
        rk_aiq_uapi_setMHDRStrth(db_aiq_ctx, true, 1);
}

void nr_mode_set(char *mode)
{
    LOG_INFO("%s, mode is %s\n", __func__, mode);
    if (!strcmp(mode,"close"))
        rk_aiq_uapi_sysctl_setModuleCtl(db_aiq_ctx, RK_MODULE_TNR, false);
    else
        rk_aiq_uapi_sysctl_setModuleCtl(db_aiq_ctx, RK_MODULE_TNR, true);

    if (!strcmp(mode, "general")) {
        int denoise_level = 0;
        dbserver_image_enhancement_get(NULL, NULL, NULL, &denoise_level, NULL, NULL, NULL);
        rk_aiq_uapi_setANRStrth(db_aiq_ctx, denoise_level);
    } else if (!strcmp(mode, "advanced")){
        int spatial_level = 0;
        int temporal_level = 0;
        dbserver_image_enhancement_get(NULL, NULL, NULL, NULL, &spatial_level, &temporal_level, NULL);
        rk_aiq_uapi_setMTNRStrth(db_aiq_ctx, true, spatial_level);  // [0,100]
        rk_aiq_uapi_setMSpaNRStrth(db_aiq_ctx, true, temporal_level); // [0,100]
    }
}

void fec_mode_set(char *mode)
{
    LOG_INFO("%s, mode is %s\n", __func__, mode);
    if (!strcmp(mode,"close"))
        rk_aiq_uapi_sysctl_setModuleCtl(db_aiq_ctx, RK_MODULE_FEC, false);
    else
        rk_aiq_uapi_sysctl_setModuleCtl(db_aiq_ctx, RK_MODULE_FEC, true);
}

void dehaze_mode_set(char *mode)
{
#if 0
    LOG_INFO("%s, mode is %s\n", __func__, mode);
    if (!strcmp(mode,"close")) {
        rk_aiq_uapi_sysctl_setModuleCtl(db_aiq_ctx, RK_MODULE_DHAZ, true);
        rk_aiq_uapi_setDhzMode(db_aiq_ctx, OP_MANUAL);
        rk_aiq_uapi_setMDhzStrth(db_aiq_ctx, true, 0); // [0,10]
        // dynamic switch is not supported, and contrast cannot be set after close
        // rk_aiq_uapi_sysctl_setModuleCtl(db_aiq_ctx, RK_MODULE_DHAZ, false);
    } else {
        rk_aiq_uapi_sysctl_setModuleCtl(db_aiq_ctx, RK_MODULE_DHAZ, true);
        if (!strcmp(mode,"auto")) {
            rk_aiq_uapi_setDhzMode(db_aiq_ctx, OP_AUTO);
        } else {
            rk_aiq_uapi_setDhzMode(db_aiq_ctx, OP_MANUAL);
            int dehaze_level = 0;
            dbserver_image_enhancement_get(NULL, NULL, NULL, NULL, NULL, NULL, &dehaze_level);
            rk_aiq_uapi_setMDhzStrth(db_aiq_ctx, true, dehaze_level); // [0,10]
        }
    }
#endif
}

void exposure_time_set(char *time)
{
    LOG_INFO("%s, time is %s\n", __func__, time);
    paRange_t range;
    range.min = 1e-5; // TODO: obtained from capability
    float numerator, denominator;
    if (strcmp(time, "1")) {
        sscanf(time, "%f/%f", &numerator, &denominator);
        range.max = numerator / denominator;
    } else {
        sscanf(time, "%f", &range.max);
    }
    LOG_INFO("%s, min is %f, max is %f\n", __func__, range.min, range.max);
    rk_aiq_uapi_setExpTimeRange(db_aiq_ctx, &range);
}

void exposure_gain_set(int gain)
{
    paRange_t range;
    range.min = 1; // TODO: obtained from capability
    range.max = 1;
    if (gain)
        range.max = (float)gain;
    LOG_INFO("%s, min is %f, max is %f\n", __func__, range.min, range.max);
    rk_aiq_uapi_setExpGainRange(db_aiq_ctx, &range);
}

void manual_white_balance_set()
{
    int rg_level = 50;
    int bg_level = 50;
    dbserver_image_white_balance_get(NULL, &rg_level, &bg_level);
    LOG_INFO("%s, rg_level is %d, bg_level is %d\n", __func__, rg_level, bg_level);
    rk_aiq_wb_gain_t gain;
    float ratio_rg, ratio_bg;
    ratio_rg = rg_level / 100.0f;
    ratio_bg = bg_level / 100.0f;
    gain.rgain = 1.0f + ratio_rg * 3.0f; // [0, 100]->[1.0, 4.0]
    gain.bgain = 1.0f + ratio_bg * 3.0f; // [0, 100]->[1.0, 4.0]
    gain.grgain = 1.0f;
    gain.gbgain = 1.0f;
    rk_aiq_uapi_setMWBGain(db_aiq_ctx, &gain);
}

void white_balance_style_set(char *style)
{
    LOG_INFO("%s, style is %s\n", __func__, style);
    if (!strcmp(style, "lockingWhiteBalance")) {
        rk_aiq_uapi_lockAWB(db_aiq_ctx);
    } else {
        rk_aiq_uapi_unlockAWB(db_aiq_ctx);
    }

    if (!strcmp(style, "manualWhiteBalance")) {
        rk_aiq_uapi_setWBMode(db_aiq_ctx, OP_MANUAL);
        manual_white_balance_set();
    } else if (!strcmp(style, "autoWhiteBalance")) {
        rk_aiq_uapi_setWBMode(db_aiq_ctx, OP_AUTO);
    } else if (!strcmp(style, "fluorescentLamp")) {
        rk_aiq_uapi_setMWBScene(db_aiq_ctx, RK_AIQ_WBCT_DAYLIGHT);
    } else if (!strcmp(style, "incandescent")) {
        rk_aiq_uapi_setMWBScene(db_aiq_ctx, RK_AIQ_WBCT_INCANDESCENT);
    } else if (!strcmp(style, "warmLight")) {
        rk_aiq_uapi_setMWBScene(db_aiq_ctx, RK_AIQ_WBCT_WARM_FLUORESCENT);
    } else if (!strcmp(style, "naturalLight")) {
        rk_aiq_uapi_setMWBCT(db_aiq_ctx, 5500);
    }
}

void frequency_mode_set(char *mode)
{
    LOG_INFO("%s, mode is %s\n", __func__, mode);
    if (!strcmp(mode,"PAL(50HZ)"))
        rk_aiq_uapi_setExpPwrLineFreqMode(db_aiq_ctx, EXP_PWR_LINE_FREQ_50HZ);
    else
        rk_aiq_uapi_setExpPwrLineFreqMode(db_aiq_ctx, EXP_PWR_LINE_FREQ_60HZ);
}

void night_to_day_param_cap_set_db(void)
{
    rk_aiq_cpsl_cap_t compensate_light_cap;
    memset(&compensate_light_cap, 0, sizeof(rk_aiq_cpsl_cap_t));
    /* Write the supported capability set to the database */
    rk_aiq_uapi_sysctl_queryCpsLtCap(db_aiq_ctx, &compensate_light_cap);
    json_object *j_cfg = json_object_new_array();
    for (int i = 0; i < RK_AIQ_CPSLS_MAX; i++) {
        int lght_src = compensate_light_cap.supported_lght_src[i];
        if ((!lght_src) || (lght_src >= RK_AIQ_CPSLS_MAX))
            continue;
        if (lght_src == RK_AIQ_CPSLS_LED)
            json_object_array_add(j_cfg, json_object_new_string("LED"));
        else if (lght_src == RK_AIQ_CPSLS_IR)
            json_object_array_add(j_cfg, json_object_new_string("IR"));
        else if (lght_src == RK_AIQ_CPSLS_MIX)
            json_object_array_add(j_cfg, json_object_new_string("MIX"));
    }
    char *support_list = (char *)json_object_get_string(j_cfg);
    LOG_INFO("lght src support list is %s\n", support_list);
    struct StaticLocation st_lo = {
        .cap_name = "image_night_to_day",
        .target_key = "sFillLightMode"
    };
    dbserver_set_static_cap_option(st_lo, support_list);
    json_object_put(j_cfg);
}

void night_to_day_param_set(void)
{
    LOG_INFO("%s\n", __func__);
    rk_aiq_cpsl_cfg_t compensate_light_cfg;
    memset(&compensate_light_cfg, 0, sizeof(rk_aiq_cpsl_cfg_t));
    dbserver_image_night_to_day_get(&compensate_light_cfg);
    if (compensate_light_cfg.mode != RK_AIQ_OP_MODE_INVALID) {
        int ret = rk_aiq_uapi_sysctl_setCpsLtCfg(db_aiq_ctx, &compensate_light_cfg);
        LOG_INFO("%s, ret is %d\n", __func__, ret);
    }
}

void brightness_set(int level) {
    rk_aiq_uapi_setBrightness(db_aiq_ctx, (int)(level*2.55)); // [0, 100]->[0, 255]
}

void contrast_set(int level) {
    rk_aiq_uapi_setContrast(db_aiq_ctx, (int)(level*2.55)); // [0, 100]->[0, 255]
}

void saturation_set(int level) {
    rk_aiq_uapi_setSaturation(db_aiq_ctx, (int)(level*2.55)); // [0, 100]->[0, 255]
}

void sharpness_set(int level) {
    rk_aiq_uapi_setSharpness(db_aiq_ctx, level); // [0, 100]
}

void hue_set(int level) {
    rk_aiq_uapi_setHue(db_aiq_ctx, (int)(level*2.55)); // [0, 100]->[0, 255]
}

void database_init(void)
{
    LOG_INFO("database_init\n");
    DBusError err;

    dbus_error_init(&err);
    connection = g_dbus_setup_bus(DBUS_BUS_SYSTEM, NULL, &err);

    dbus_connection_add_filter(connection,
                               database_monitor_changed, NULL, NULL);
    dbus_bus_add_match(connection,
                       "type='signal',interface='rockchip.dbserver.media'", &err);
    if (dbus_error_is_set(&err)) {
        LOG_ERROR("Error: %s\n", err.message);
        return;
    }

    LOG_INFO("database_init over\n");
}

#endif

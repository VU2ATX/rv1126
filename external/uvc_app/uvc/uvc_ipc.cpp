// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sys/prctl.h>
#include <sys/time.h>


#include "uvc_ipc.h"
#include "uvc_log.h"
#include "drm.h"
#include "mpi_enc.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "uvc_ipc"

namespace ShmControl
{
struct UVC_IPC_INFO uvc_ipc_info;

// TODO: (lxh) add handle here for use instead global handle
extern "C" struct UVC_IPC_INFO *uvc_ipc_get_uvc_info_hd()
{
    return &uvc_ipc_info;
}

extern "C" void uvc_ipc_init()
{
    if (uvc_ipc_info.shm_control == NULL)
    {
        uvc_ipc_info.shm_control = new ShmUVCController();
//       uvc_ipc_info.shm_control->startRecvMessage();
        uvc_ipc_info.start = true;
        LOG_INFO("uvc_ipc_init first ok\n");
    }
    else
        LOG_INFO("uvc_ipc_init yet\n");
}

extern "C" void uvc_ipc_reinit()
{

    if (uvc_ipc_info.shm_control)
        delete uvc_ipc_info.shm_control;
    LOG_INFO("uvc_ipc_reinit ok\n");
}

extern "C" void uvc_ipc_event(enum UVC_IPC_EVENT event, void *data)
{
//    LOG_INFO("enter uvc_ipc_event event:%s\n", shm_meg_name[event - 1].c_str());
    pthread_mutex_lock(&uvc_ipc_info.mutex);
    if (uvc_ipc_info.start == false) //now only start onece
    {
        uvc_ipc_init();
    }
    switch (event)
    {
    case UVC_IPC_EVENT_START:
        uvc_ipc_info.stop = false;
        uvc_ipc_info.shm_control->clearRecvMessage(); // clear the last data
        uvc_ipc_info.shm_control->startRecvMessage();
        uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_START, data);
        if (uvc_ipc_info.history[0] == true)
        {
            LOG_INFO("history eptz:%d\n", uvc_ipc_info.enable_eptz);
            uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_ENABLE_ETPTZ, (void *)&uvc_ipc_info.enable_eptz);
            uvc_ipc_info.history[0] = false;
            uvc_ipc_info.history[2] = false;
            uvc_ipc_info.history[3] = false;
        } else {
            if (uvc_ipc_info.history[2] == true)
            {
                LOG_INFO("history eptz_pan:%d\n", uvc_ipc_info.zoom);
                uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_SET_ZOOM, (void *)&uvc_ipc_info.zoom);
                uvc_ipc_info.history[2] = false;
            }
            if (uvc_ipc_info.history[3] == true)
            {
                LOG_INFO("history eptz_tilt:%d\n", uvc_ipc_info.zoom);
                uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_SET_ZOOM, (void *)&uvc_ipc_info.zoom);
                uvc_ipc_info.history[3] = false;
            }
        }
        if (uvc_ipc_info.history[1] == true)
        {
            LOG_INFO("history zoom:%d\n", uvc_ipc_info.zoom);
            uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_SET_ZOOM, (void *)&uvc_ipc_info.zoom);
            uvc_ipc_info.history[1] = false;
        }


        break;
    case UVC_IPC_EVENT_STOP:
        if (!uvc_ipc_info.stop)
        {
            uvc_ipc_info.shm_control->stopRecvMessage(); //clear the recv data
            uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_STOP, data);
            uvc_ipc_info.shm_control->ShmUVCDrmRelease();
            uvc_ipc_info.stop = true;
        }
        else
        {
            LOG_INFO("UVC_IPC_EVENT_STOP yet!\n");
        }
//        uvc_ipc_reinit();
        break;
    case UVC_IPC_EVENT_ENABLE_ETPTZ:
#if UVC_STREAM_OFF_NOT_SEND_SETTING
        if (uvc_ipc_info.stop)
        {
            int *val = (int *)data;
            uvc_ipc_info.enable_eptz = *val;
            uvc_ipc_info.history[0] = true;
            LOG_INFO("eptz can't set %d now, UVC_IPC_EVENT_STOP yet! wait for next start effect!\n",
                      uvc_ipc_info.enable_eptz);
        }
        else
#endif
            uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_ENABLE_ETPTZ, data);
        break;
    case UVC_IPC_EVENT_SET_ZOOM:
#if UVC_STREAM_OFF_NOT_SEND_SETTING
        if (uvc_ipc_info.stop)
        {
            int *val = (int *)data;
            uvc_ipc_info.zoom = *val;
            uvc_ipc_info.history[1] = true;
            LOG_INFO("zoom can't set %d now, UVC_IPC_EVENT_STOP yet! wait for next start effect!\n",
                      uvc_ipc_info.zoom);
        }
        else
#endif
            uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_SET_ZOOM, data);
        break;
    case UVC_IPC_EVENT_RET_TRANSPORT_BUF:
        uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_TRANSPORT_BUF, data);
        break;
    case UVC_IPC_EVENT_CONFIG_CAMERA:
        uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_CONFIG_CAMERA, data);
        break;
    case MSG_UVC_SET_EPTZ_PAN:
#if UVC_STREAM_OFF_NOT_SEND_SETTING
        if (uvc_ipc_info.stop)
        {
            int *val = (int *)data;
            uvc_ipc_info.eptz_pan = *val;
            uvc_ipc_info.history[0] = false;
            uvc_ipc_info.history[2] = true;
            LOG_INFO("eptz_pan can't set %d now, UVC_IPC_EVENT_STOP yet! wait for next start effect!\n",
                      uvc_ipc_info.eptz_pan);
        }
        else
#endif
            uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_SET_EPTZ_PAN, data);
        break;
    case MSG_UVC_SET_EPTZ_TILT:
#if UVC_STREAM_OFF_NOT_SEND_SETTING
        if (uvc_ipc_info.stop)
        {
            int *val = (int *)data;
            uvc_ipc_info.eptz_tilt = *val;
            uvc_ipc_info.history[0] = false;
            uvc_ipc_info.history[3] = true;
            LOG_INFO("eptz_tilt can't set %d now, UVC_IPC_EVENT_STOP yet! wait for next start effect!\n",
                      uvc_ipc_info.eptz_tilt);
        }
        else
#endif
            uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_SET_EPTZ_TILT, data);
        break;
    default:
        LOG_INFO("no support such uvc_ipc_event event:%d\n", event);
        break;
    }
    pthread_mutex_unlock(&uvc_ipc_info.mutex);
    if (event != UVC_IPC_EVENT_RET_TRANSPORT_BUF)
        LOG_INFO("exit uvc_ipc_event event:%s\n", shm_meg_name[event - 1].c_str());

}

extern "C" void uvc_ipc_reconnect(void)//(struct UVC_IPC_INFO *info)
{
    if (uvc_ipc_info.shm_control && uvc_ipc_info.stop == false) {
        LOG_INFO("aiserver died, uvc reconnect to aiserver and send last info\n");
        uvc_ipc_info.shm_control->stopRecvMessage(); //clear the recv data
        uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_STOP, NULL);
        uvc_ipc_info.shm_control->ShmUVCDrmRelease();
        uvc_ipc_info.shm_control->startRecvMessage();
        uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_CONFIG_CAMERA, &uvc_ipc_info.camera);
        uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_START, NULL);
    } else {
        LOG_INFO("aiserver died, uvc status is stop,no need do anything\n");
    }
}

void ProcessRecvUVCMessage(void *opaque)
{
    prctl(PR_SET_NAME, "uvc_ipc_recv_msg_thread");
    ShmUVCController *controller = (ShmUVCController *)opaque;
    controller->recvUVCMessageLoop();
}

#if UVC_IPC_DYNAMIC_DEBUG_ON
void ProcessDebug(void *opaque)
{
    prctl(PR_SET_NAME, "uvc_ipc_debug_thread");
    ShmUVCController *controller = (ShmUVCController *)opaque;
    controller->uvcIPCDebugLoop();
}
#endif


ShmUVCController::ShmUVCController()
{
    memset(&uvc_ipc_info, 0, sizeof(uvc_ipc_info));
    pthread_mutex_init(&uvc_ipc_info.mutex, NULL);
    uvc_ipc_info.stop = true;
#if UVC_SENDBUFF_USE_INIT_ALLOC
    MediaBufferInfo *send_bufferinfo = new MediaBufferInfo;
    send_message.set_allocated_buffer_info(send_bufferinfo);
#endif

#if UVC_IPC_DYNAMIC_DEBUG_ON
    debugLooping = true;
    debugThread = new std::thread(ProcessDebug, this);
#endif
    initialize();
}

ShmUVCController::~ShmUVCController()
{
    if (drmFd >= 0)
    {
        drm_close(drmFd);
        drmFd = -1;
    }
#if UVC_IPC_DYNAMIC_DEBUG_ON
    debugLooping = false;
    if (debugThread)
    {
        debugThread->join();
        delete debugThread;
        debugThread = nullptr;
    }
#endif
    pthread_mutex_destroy(&uvc_ipc_info.mutex);
    uvc_ipc_reinit();
}

void ShmUVCController::ShmUVCDrmRelease()
{
    int free_count = 0;
    for (std::map<int, DrmBuffMap>::iterator iter = drm_info.begin(); iter != drm_info.end(); ++iter)
    {
        ++free_count;
        if (iter->second.buf)
            drm_unmap_buffer(iter->second.buf, iter->second.size);
        if (iter->second.buf_fd)
            close(iter->second.buf_fd);
        if (iter->second.handle)
            drm_free(drmFd, iter->second.handle);
        LOG_INFO("drm_free count:%d\n", free_count);
    }
    drm_info.clear();
}

void ShmUVCController::initialize()
{
    recvLooping = false;
    recvThread = NULL;
#ifdef USE_RK_AISERVER
    shmc::SetLogHandler(shmc::kDebug, [](shmc::LogLevel lv, const char *s)
    {
        printf("[%d] %s\n", lv, s);
    });

    shmWriteQueue.InitForWrite(kShmUVCWriteKey, kQueueBufSize);
    //assert(shmWriteReadQueue.InitForWrite(kShmUVCReadKey, kQueueBufSize));
    shmWriteReadQueue.InitForWrite(kShmUVCReadKey, kQueueBufSize);
    shmReadQueue.InitForRead(kShmUVCReadKey);
    LOG_INFO("ShmUVCController init\n");
#endif
    drmFd = drm_open();
}

void ShmUVCController::startRecvMessage()
{
    LOG_INFO("start recv message in\n");
#if UVC_IPC_DYNAMIC_DEBUG_ON
    gettimeofday(&ipc_enter_time, NULL);
    gettimeofday(&isp_enter_time, NULL);
#endif
    recvLooping = true;
    norecv_err_count = 0;
    norecv_count = 0;
    abandon_count = 0;
    recvThread = new std::thread(ProcessRecvUVCMessage, this);
    LOG_INFO("start recv message ok\n");
}

void ShmUVCController::clearRecvMessage()
{
    int ret = 0;
    int length = 0;
    std::string msg;

    std::lock_guard<std::mutex> lock(rQueueMtx);
    do
    {
        ret = shmReadQueue.Pop(&msg);
        length = msg.length();
        LOG_INFO("clear recv message(ret=%d, length=%d)\n", ret, length);
//      if (ret)
//          handleUVCMessage(msg); //test
        msg.clear();
    }
    while (ret && length > 0);
    //queue_r_.Reset();
}


void ShmUVCController::stopRecvMessage()
{
    LOG_INFO("stop recv message enter\n");
    recvLooping = false;
    if (recvThread)
    {
        recvThread->join();
        delete recvThread;
        recvThread = nullptr;
    }

    LOG_INFO("stop recv message ok\n");
    clearRecvMessage();
    //queue_r_.Reset();
}

#if UVC_IPC_DYNAMIC_DEBUG_ON
void ShmUVCController::uvcIPCDebugLoop()
{
    LOG_INFO("enter\n");
    bool eptz = false;
    int set_eptz = 0;
    while (debugLooping)
    {
        sleep(1);
#if UVC_IPC_DYNAMIC_DEBUG_ON
        if (!access(UVC_IPC_DYNAMIC_DEBUG_STATE, 0))
        {
            LOG_INFO("send state:%d, recv state:%d\n", send_state, recv_state);
        }
        if (!access(UVC_IPC_DYNAMIC_DEBUG_EPTZ, 0) && eptz == false)
        {
            LOG_INFO("oepn eptz\n");
            eptz = true;
            set_eptz = 1;
            uvc_ipc_event(UVC_IPC_EVENT_ENABLE_ETPTZ, (void *)&set_eptz);
        }
        else if (access(UVC_IPC_DYNAMIC_DEBUG_EPTZ, 0) && eptz == true)
        {
            LOG_INFO("close eptz\n");
            eptz = false;
            set_eptz = 0;
            uvc_ipc_event(UVC_IPC_EVENT_ENABLE_ETPTZ, (void *)&set_eptz);
        }
#endif
    }
    LOG_INFO("exit \n");
}
#endif

void ShmUVCController::recvUVCMessageLoop()
{
    int ret = 0;
    LOG_INFO("recv uvc message thread enter\n");
    while (recvLooping)
    {
        std::string msg;
        std::lock_guard<std::mutex> lock(rQueueMtx);

        ret = shmReadQueue.Pop(&msg);
        if (ret)
        {
            //LOG_INFO("recv uvc message = %s \n", msg.c_str());
            handleUVCMessage(msg);
            msg.clear();
            norecv_count = 0;
        }
        else
        {
            usleep(1 * 1000);
            ++ norecv_count;
            if (norecv_count >= 3000) // 3s not recv data have error
            {
                ++ norecv_err_count;
                LOG_ERROR("%d ms not recv data, count up to %d, maybe isp err(uvc not get raw). stat:%d,%d\n",
                           norecv_count * 1, norecv_err_count,
                           send_state, recv_state);
                if (norecv_err_count == 1)
                {
                    system("touch /tmp/uvc_ipc_buffer"); // for check uvc_ipc_buffer
                    LOG_INFO("test: touch /tmp/uvc_ipc_buffer ok!\n");
                }
                norecv_count = 0;
            }
        }
    }
    LOG_INFO("recv uvc message thread end\n");
}

void ShmUVCController::handleUVCMessage(std::string &msg)
{
    int msgType = 0;
    UVCMessage message;

    message.ParseFromString(msg);
    msgType = message.msg_type();
    if (msgType == MSG_UVC_TRANSPORT_BUF)  // UVC Buffer
    {
        MediaBufferInfo bufferInfo = message.buffer_info();
        recvUVCBuffer(&bufferInfo);
        //LOG_INFO("recv uvc buffer message\n");
    }
    else
    {
        LOG_INFO("recv uvc unknown message\n");
    }
}

void ShmUVCController::recvUVCBuffer(MediaBufferInfo *bufferInfo)
{
    int ret = 0;
    int buf_fd;
    unsigned int handle;
    int drm_size;
    char *buf = NULL;
    int32_t name = bufferInfo->id();
    int32_t buf_size = bufferInfo->size();
    int64_t privData = bufferInfo->priv_data();
    int64_t pts = bufferInfo->pts();
    struct SendBufferInfo send_info;
    //std::map<int, DrmBuffMap> temp_map;
    struct DrmBuffMap drm_map;
    int map_count;
    bool has_map = false;
    send_info.id = name;
    send_info.size = bufferInfo->size();
    send_info.fd = bufferInfo->fd();
    send_info.handle = bufferInfo->handle();
    send_info.data = bufferInfo->data();
    send_info.priv_data = privData;
    int count = 0;
    struct MPP_ENC_INFO enc_info;
#if UVC_DYNAMIC_DEBUG_USE_TIME
    if (!access(UVC_DYNAMIC_DEBUG_USE_TIME_CHECK, 0))
    {
        int64_t use_time_us, now_time_us;
        struct timespec now_tm = {0, 0};
        clock_gettime(CLOCK_MONOTONIC, &now_tm);
        now_time_us = now_tm.tv_sec * 1000000LL + now_tm.tv_nsec / 1000; // us
        use_time_us = now_time_us - pts;
        //pts = now_time_us;//for test
        LOG_INFO("isp->aiserver->ipc latency time:%lld us, %lld ms\n", use_time_us, use_time_us / 1000);
    }
#endif

#if UVC_IPC_DYNAMIC_DEBUG_ON
    recv_state = UVC_IPC_STATE_RECV_ENTER;
#endif
#if UVC_IPC_DYNAMIC_DEBUG_ON
    if (!access(UVC_IPC_DYNAMIC_DEBUG_FPS, 0))
    {
        ++frm;
        if (frm == 100) {
            struct timeval now_time;
            gettimeofday(&now_time, NULL);
            int64_t use_timems = (now_time.tv_sec * 1000 + now_time.tv_usec / 1000) -
                                  (ipc_enter_time.tv_sec * 1000 + ipc_enter_time.tv_usec / 1000);
            ipc_enter_time.tv_sec = now_time.tv_sec;
            ipc_enter_time.tv_usec = now_time.tv_usec;
            fps = (1000.0 * frm) / use_timems;
            frm = 0;
            LOG_INFO("ipc recv fps:%0.1f\n", fps);
        }
    }
#endif

    for (std::map<int, DrmBuffMap>::iterator iter = drm_info.begin(); iter != drm_info.end(); ++iter)
    {
        count ++;
        if (name == iter->second.id)
        {
            has_map = true;

#if RK_MPP_DYNAMIC_DEBUG_ON
            if (!access(RK_MPP_DYNAMIC_DEBUG_IN_CHECK, 0) || yuv_encode)
            {
                if (!iter->second.buf)
                {
                    iter->second.buf = (char *)drm_map_buffer(drmFd, iter->second.handle, iter->second.size);
                }
            }
#endif
            buf = iter->second.buf;
            buf_fd = iter->second.buf_fd;
            break;
        }
    }
//  LOG_INFO("count=%d drm_info.size()=%d\n",count, drm_info.size());

    if (has_map == false)
    {
        drm_get_info_from_name(drmFd, name, &handle, &drm_size);
        ret = drm_handle_to_fd(drmFd, handle, &buf_fd, 0);
        if (ret)
        {
            LOG_INFO("drm_handle_to_fd fail\n");
            return;
        }
#if RK_MPP_DYNAMIC_DEBUG_ON
        if (!access(RK_MPP_DYNAMIC_DEBUG_IN_CHECK, 0) || yuv_encode)
        {
            buf = (char *)drm_map_buffer(drmFd, handle, buf_size);
        }
#endif
        drm_map.buf = buf;
        drm_map.id = name;
        drm_map.size = buf_size;
        drm_map.handle = handle;
        drm_map.buf_fd = buf_fd;
        map_count = drm_info.size();
        drm_info.insert(std::map<int, DrmBuffMap>::value_type(map_count, drm_map));
        LOG_INFO("new drm_map count=%d name=%d,buf_fd=%d buf_size=%d drm_size=%d priv=0x%llx\n",
                 map_count, name, buf_fd, buf_size, drm_size, privData);
    }
#if UVC_IPC_DYNAMIC_DEBUG_ON
    recv_state = UVC_IPC_STATE_RECV_ENC_ENTER;
#endif
    enc_info.fd = buf_fd;
    enc_info.size = nv12_size;
    enc_info.pts = pts;
#if RK_MPP_DYNAMIC_DEBUG_ON
    if (!access(UVC_IPC_DYNAMIC_DEBUG_ISP_FPS, 0))
    {
        ++isp_frm;
        if (isp_frm == 100) {
            struct timeval now_time;
            gettimeofday(&now_time, NULL);
            int64_t use_timems = (now_time.tv_sec * 1000 + now_time.tv_usec / 1000) -
                                  (isp_enter_time.tv_sec * 1000 + isp_enter_time.tv_usec / 1000);
            isp_enter_time.tv_sec = now_time.tv_sec;
            isp_enter_time.tv_usec = now_time.tv_usec;
            fps = (1000.0 * isp_frm) / use_timems;
            isp_frm = 0;
            LOG_INFO("isp recv fps:%0.1f\n", fps);
        }
    }
    else
#endif
    {
#if UVC_ABANDON_FRM_COUNT
        if (abandon_count < UVC_ABANDON_FRM_COUNT)
        {
            ++ abandon_count;
            LOG_INFO("abandon frm, count=%d\n", abandon_count);
        }
        else
#endif
        {
            uvc_read_camera_buffer(buf, &enc_info,
                                   NULL, 0);
        }
    }
#if UVC_IPC_DYNAMIC_DEBUG_ON
    recv_state = UVC_IPC_STATE_RECV_ENC_EXIT;
#endif
    //uvc_ipc_event(UVC_IPC_EVENT_RET_TRANSPORT_BUF, (void *)&send_info); //not call uvc_ipc_event here,it will be dead lock
    uvc_ipc_info.shm_control->sendUVCBuffer(MSG_UVC_TRANSPORT_BUF, (void *)&send_info);
#if UVC_IPC_DYNAMIC_DEBUG_ON
    recv_state = UVC_IPC_STATE_RECV_EXIT;
#endif
}

void ShmUVCController::sendUVCBuffer(enum ShmUVCMessageType event, void *data)
{
    std::string sendbuf;
#if UVC_IPC_DYNAMIC_DEBUG_ON
    send_state = UVC_IPC_STATE_SEND_ENTER;
#endif
    std::lock_guard<std::mutex> lock(wQueueMtx);
#if UVC_IPC_DYNAMIC_DEBUG_ON
    send_state = UVC_IPC_STATE_SEND_ENTER_UNLOCK;
#endif
    switch (event)
    {
    case MSG_UVC_START:
    {
        UVCMessage message;
        message.set_msg_type(event);
        message.set_msg_name("uvcbuffer");
        message.SerializeToString(&sendbuf);
        message.ParseFromString(sendbuf);


    }
    break;
    case MSG_UVC_STOP:
	{
        UVCMessage message;
        message.set_msg_type(event);
        message.set_msg_name("uvcbuffer");
        message.SerializeToString(&sendbuf);
        message.ParseFromString(sendbuf);
    }
        break;
    case MSG_UVC_ENABLE_ETPTZ:
    {
        UVCMessage message;
        MethodParams *method_param = new MethodParams;
        int *enable;
        if (data == NULL)
            *enable = 0;
        else
            enable = (int *)data;
        method_param->set_i32_p(*enable);
        message.set_allocated_method_params(method_param);
        message.set_msg_type(event);
        message.set_msg_name("uvcbuffer");
        message.SerializeToString(&sendbuf);
        message.ParseFromString(sendbuf);
        LOG_INFO("send uvc eptz:%d \n", *enable);
        uvc_ipc_info.enable_eptz = *enable;
    }
    break;
    case MSG_UVC_SET_ZOOM:
    {
        UVCMessage message;
        MethodParams *method_param = new MethodParams;
        int *int_zoom = (int *)data;
        float zoom = (float)(*int_zoom / 10.0);
        //MethodParams method_param =
        method_param->set_flo_p(zoom);
        message.set_allocated_method_params(method_param);
        message.set_msg_type(event);
        message.set_msg_name("uvcbuffer");
        message.SerializeToString(&sendbuf);
        message.ParseFromString(sendbuf);
        LOG_INFO("send uvc zoom:%f \n", zoom);
        uvc_ipc_info.zoom = *int_zoom;
    }
    break;
    case MSG_UVC_TRANSPORT_BUF:
    {
#if UVC_SENDBUFF_USE_INIT_ALLOC
        MediaBufferInfo *bufferInfo = &const_cast<MediaBufferInfo &>(send_message.buffer_info());
        struct SendBufferInfo *info = (struct SendBufferInfo *)data;
        bufferInfo->set_id(info->id);
        bufferInfo->set_size(info->size);
        bufferInfo->set_fd(info->fd);
        bufferInfo->set_handle(info->handle);
        bufferInfo->set_data(info->data);
        bufferInfo->set_priv_data(info->priv_data);

        send_message.set_msg_type(event);
        send_message.set_msg_name("uvcbuffer");
        send_message.SerializeToString(&sendbuf);
        send_message.ParseFromString(sendbuf);
#else
        UVCMessage message;
        MediaBufferInfo *bufferInfo = new MediaBufferInfo;
        struct SendBufferInfo *info = (struct SendBufferInfo *)data;
        bufferInfo->set_id(info->id);
        bufferInfo->set_size(info->size);
        bufferInfo->set_fd(info->fd);
        bufferInfo->set_handle(info->handle);
        bufferInfo->set_data(info->data);
        bufferInfo->set_priv_data(info->priv_data);

        message.set_allocated_buffer_info(bufferInfo);
        message.set_msg_type(event);
        message.set_msg_name("uvcbuffer");
        message.SerializeToString(&sendbuf);
        message.ParseFromString(sendbuf);
#endif
    }
    break;
    case MSG_UVC_CONFIG_CAMERA:
    {
        UVCMessage message;
        StreamInfo *streamInfo = new StreamInfo;
        struct CAMERA_INFO *info = (struct CAMERA_INFO *)data;
        width = info->width;
        height = info->height;
        nv12_size = width * height * 3 / 2;
        streamInfo->set_width(info->width);
        streamInfo->set_height(info->height);
        streamInfo->set_vir_width(info->vir_width);
        streamInfo->set_vir_height(info->vir_height);
        streamInfo->set_buf_size(info->buf_size);
        streamInfo->set_range(info->range);
        yuv_encode = info->yuv_encode;
        message.set_allocated_stream_info(streamInfo);
        message.set_msg_type(event);
        message.set_msg_name("uvcbuffer");
        message.SerializeToString(&sendbuf);
        message.ParseFromString(sendbuf);
        LOG_INFO("send uvc config camera. \
width:%d,height:%d,vir_width=%d,vir_height=%d,buf_size=%d,range=%d,yuv=%d\n",
                 info->width, info->height, info->vir_width, info->vir_height,
                 info->buf_size, info->range, yuv_encode);
        memcpy(&uvc_ipc_info.camera, info, sizeof(struct CAMERA_INFO));
    }
    break;
    case MSG_UVC_SET_EPTZ_PAN:
    {
        UVCMessage message;
        MethodParams *method_param = new MethodParams;
        int *pan;
        if (data == NULL)
            *pan = 0;
        else
            pan = (int *)data;
        method_param->set_i32_p(*pan);
        message.set_allocated_method_params(method_param);
        message.set_msg_type(event);
        message.set_msg_name("uvcbuffer");
        message.SerializeToString(&sendbuf);
        message.ParseFromString(sendbuf);
        LOG_INFO("send uvc eptz pan:%d \n", *pan);
        uvc_ipc_info.eptz_pan = *pan;
    }
    break;
    case MSG_UVC_SET_EPTZ_TILT:
    {
        UVCMessage message;
        MethodParams *method_param = new MethodParams;
        int *tilt;
        if (data == NULL)
            *tilt = 0;
        else
            tilt = (int *)data;
        method_param->set_i32_p(*tilt);
        message.set_allocated_method_params(method_param);
        message.set_msg_type(event);
        message.set_msg_name("uvcbuffer");
        message.SerializeToString(&sendbuf);
        message.ParseFromString(sendbuf);
        LOG_INFO("send uvc eptz tilt:%d \n", *tilt);
        uvc_ipc_info.eptz_tilt = *tilt;
    }
    break;
    default :
        break;
    }
#if UVC_IPC_DYNAMIC_DEBUG_ON
    send_state = UVC_IPC_STATE_SEND_READY;
#endif
    shmWriteQueue.Push(sendbuf);
#if UVC_IPC_DYNAMIC_DEBUG_ON
    send_state = UVC_IPC_STATE_SEND_EXIT;
#endif
}

} // namespace ShmControl


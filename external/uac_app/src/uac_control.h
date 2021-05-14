/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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
 *
 */

enum UACStreamType {
    // our device record datas from usb, pc/remote->our device
    UAC_STREAM_RECORD = 0,
    // play datas to usb, our device->pc/remote
    UAC_STREAM_PLAYBACK,
    UAC_STREAM_MAX
};

int uac_start(int type);
void uac_stop(int type);
void uac_set_sample_rate(int type, int samplerate);

int uac_control_create();
void uac_control_destory();

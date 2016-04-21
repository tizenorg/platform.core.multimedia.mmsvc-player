/*
* Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef __TIZEN_MEDIA_MUSE_PLAYER_H__
#define	__TIZEN_MEDIA_MUSE_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <muse_core_ipc.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TIZEN_N_PLAYER"

#define MUSE_PLAYER_HEAD_GAP(api) ((api)/(1000)+(1))*(1000)

/**
 * @brief Auto-generated enumeration for the muse player APIs.
 * @details muse_player_api.def file which is a list of player API enums
 *          will be created during build time automatically like below.
 *          MUSE_PLAYER_API_CREATE,
 *          MUSE_PLAYER_API_DESTROY,
 *          ...
 * @see ../api.list
 * @see ../make_api.py
 */
typedef enum {
#include "muse_player_api.def"
	MUSE_PLAYER_API_MAX
} muse_player_api_e;

typedef enum {
	MUSE_PLAYER_CB_EVENT = MUSE_PLAYER_HEAD_GAP(MUSE_PLAYER_API_MAX),
	MUSE_PLAYER_CB_MAX
} muse_player_cb_e;

/**
 * @brief Enumeration for the muse player events.
 */
typedef enum {
	MUSE_PLAYER_EVENT_TYPE_PREPARE,
	MUSE_PLAYER_EVENT_TYPE_COMPLETE,
	MUSE_PLAYER_EVENT_TYPE_INTERRUPT,
	MUSE_PLAYER_EVENT_TYPE_ERROR,
	MUSE_PLAYER_EVENT_TYPE_BUFFERING,
	MUSE_PLAYER_EVENT_TYPE_SUBTITLE,
	MUSE_PLAYER_EVENT_TYPE_CAPTURE,
	MUSE_PLAYER_EVENT_TYPE_SEEK,
	MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME,
	MUSE_PLAYER_EVENT_TYPE_AUDIO_FRAME,
	MUSE_PLAYER_EVENT_TYPE_VIDEO_FRAME_RENDER_ERROR,
	MUSE_PLAYER_EVENT_TYPE_PD,
	MUSE_PLAYER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT,
	MUSE_PLAYER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT_PRESET,
	MUSE_PLAYER_EVENT_TYPE_MISSED_PLUGIN,
#ifdef _PLAYER_FOR_PRODUCT
	MUSE_PLAYER_EVENT_TYPE_IMAGE_BUFFER,
	MUSE_PLAYER_EVENT_TYPE_SELECTED_SUBTITLE_LANGUAGE,
#endif
	MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS,
	MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS,
	MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS_WITH_INFO,
	MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS_WITH_INFO,
	MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK,
	MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK,
	MUSE_PLAYER_EVENT_TYPE_AUDIO_STREAM_CHANGED,
	MUSE_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED,
	MUSE_PLAYER_EVENT_TYPE_NUM
}muse_player_event_e;

#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_MEDIA_MUSE_PLAYER_H__ */

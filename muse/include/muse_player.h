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

#include "muse_core_ipc.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TIZEN_N_PLAYER"

#define MUSE_PLAYER_HEAD_GAP(api) ((api)/(1000)+(1))*(1000)

typedef enum {
	MUSE_PLAYER_API_CREATE = API_CREATE,
	MUSE_PLAYER_API_DESTROY = API_DESTROY,
	MUSE_PLAYER_API_PREPARE,
	MUSE_PLAYER_API_PREPARE_ASYNC,
	MUSE_PLAYER_API_UNPREPARE,
	MUSE_PLAYER_API_SET_URI,
	MUSE_PLAYER_API_START,
	MUSE_PLAYER_API_STOP,
	MUSE_PLAYER_API_PAUSE,
	MUSE_PLAYER_API_SET_MEMORY_BUFFER,
	MUSE_PLAYER_API_DEINIT_MEMORY_BUFFER,
	MUSE_PLAYER_API_GET_STATE,
	MUSE_PLAYER_API_SET_VOLUME,
	MUSE_PLAYER_API_GET_VOLUME,
	MUSE_PLAYER_API_SET_SOUND_TYPE,
	MUSE_PLAYER_API_SET_AUDIO_POLICY_INFO,
	MUSE_PLAYER_API_SET_AUDIO_LATENCY_MODE,
	MUSE_PLAYER_API_GET_AUDIO_LATENCY_MODE,
	MUSE_PLAYER_API_SET_PLAY_POSITION,
	MUSE_PLAYER_API_GET_PLAY_POSITION,
	MUSE_PLAYER_API_SET_MUTE,
	MUSE_PLAYER_API_IS_MUTED,
	MUSE_PLAYER_API_SET_LOOPING,
	MUSE_PLAYER_API_IS_LOOPING,
	MUSE_PLAYER_API_GET_DURATION,
	MUSE_PLAYER_API_SET_DISPLAY,
	MUSE_PLAYER_API_SET_DISPLAY_MODE,
	MUSE_PLAYER_API_GET_DISPLAY_MODE,
	MUSE_PLAYER_API_SET_PLAYBACK_RATE,
	MUSE_PLAYER_API_SET_DISPLAY_ROTATION,
	MUSE_PLAYER_API_GET_DISPLAY_ROTATION,
	MUSE_PLAYER_API_SET_DISPLAY_VISIBLE,
	MUSE_PLAYER_API_IS_DISPLAY_VISIBLE,
	MUSE_PLAYER_API_RESIZE_VIDEO_RENDER_RECT,
	MUSE_PLAYER_API_GET_CONTENT_INFO,
	MUSE_PLAYER_API_GET_CODEC_INFO,
	MUSE_PLAYER_API_GET_AUDIO_STREAM_INFO,
	MUSE_PLAYER_API_GET_VIDEO_STREAM_INFO,
	MUSE_PLAYER_API_GET_VIDEO_SIZE,
	MUSE_PLAYER_API_GET_ALBUM_ART,
	MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BANDS_COUNT,
	MUSE_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_ALL_BANDS,
	MUSE_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_BAND_LEVEL,
	MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_LEVEL,
	MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_LEVEL_RANGE,
	MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY,
	MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY_RANGE,
	MUSE_PLAYER_API_AUDIO_EFFECT_EQUALIZER_CLEAR,
	MUSE_PLAYER_API_AUDIO_EFFECT_EQUALIZER_IS_AVAILABLE,
	MUSE_PLAYER_API_SET_PROGRESSIVE_DOWNLOAD_PATH,
	MUSE_PLAYER_API_GET_PROGRESSIVE_DOWNLOAD_STATUS,
	MUSE_PLAYER_API_CAPTURE_VIDEO,
	MUSE_PLAYER_API_SET_STREAMING_COOKIE,
	MUSE_PLAYER_API_SET_STREAMING_USER_AGENT,
	MUSE_PLAYER_API_GET_STREAMING_DOWNLOAD_PROGRESS,
	MUSE_PLAYER_API_SET_SUBTITLE_PATH,
	MUSE_PLAYER_API_SET_SUBTITLE_POSITION_OFFSET,
	MUSE_PLAYER_API_PUSH_MEDIA_STREAM,
	MUSE_PLAYER_API_SET_MEDIA_STREAM_INFO,
	MUSE_PLAYER_API_SET_CALLBACK,
	MUSE_PLAYER_API_MEDIA_PACKET_FINALIZE_CB,
	MUSE_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MAX_SIZE,
	MUSE_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MAX_SIZE,
	MUSE_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD,
	MUSE_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD,
	MUSE_PLAYER_API_GET_TRACK_COUNT,
	MUSE_PLAYER_API_GET_CURRENT_TRACK,
	MUSE_PLAYER_API_SELECT_TRACK,
	MUSE_PLAYER_API_GET_TRACK_LANGUAGE_CODE,
	MUSE_PLAYER_API_SET_PCM_EXTRACTION_MODE,
	MUSE_PLAYER_API_SET_PCM_SPEC,
	MUSE_PLAYER_API_SET_STREAMING_PLAYBACK_RATE,
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

#endif //__TIZEN_MEDIA_MUSE_PLAYER_H__

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlog.h>

#include <mm.h>
#include <mm_player.h>
#include <mm_player_internal.h>
#include <mm_types.h>
#include "muse_player.h"
#include "legacy_player.h"
#include "legacy_player_internal.h"
#include "legacy_player_private.h"

/*
* Internal Macros
*/
#define PLAYER_SET_CALLBACK(event_type, handle, callback, user_data) \
do { \
	PLAYER_INSTANCE_CHECK(handle); \
	PLAYER_NULL_ARG_CHECK(callback); \
	handle->user_cb[event_type] = callback; \
	handle->user_data[event_type] = user_data; \
	LOGI("[%s] Event type : %d ", __FUNCTION__, event_type); \
} while (0) \

bool __audio_stream_callback_ex(MMPlayerAudioStreamDataType *stream, void *user_data)
{
	player_s *handle = (player_s *)user_data;

	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return TRUE;
	}

	if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_AUDIO_FRAME])
		((player_audio_pcm_extraction_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_AUDIO_FRAME])((player_audio_raw_data_s *)stream, handle->user_data[MUSE_PLAYER_EVENT_TYPE_AUDIO_FRAME]);
	return TRUE;
}

int legacy_player_set_pcm_extraction_mode(player_h player, bool sync, player_audio_pcm_extraction_cb callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	player_s *handle = (player_s *)player;
	int ret = MM_ERROR_NONE;

	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	ret = mm_player_set_attribute(handle->mm_handle, NULL, "pcm_extraction", TRUE, "pcm_extraction_start_msec", 0, "pcm_extraction_end_msec", 0, NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	ret = mm_player_set_audio_stream_callback_ex(handle->mm_handle, sync, __audio_stream_callback_ex, (void *)handle);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	PLAYER_SET_CALLBACK(MUSE_PLAYER_EVENT_TYPE_AUDIO_FRAME, handle, callback, user_data);
	return PLAYER_ERROR_NONE;
}

int legacy_player_set_pcm_spec(player_h player, const char *format, int samplerate, int channel)
{
	PLAYER_INSTANCE_CHECK(player);

	player_s *handle = (player_s *)player;
	int ret = MM_ERROR_NONE;

	LOGE("[%s] legacy_player_set_pcm_spec %s %d %d", __FUNCTION__, format, samplerate, channel);
	ret = mm_player_set_attribute(handle->mm_handle, NULL, "pcm_audioformat", format, strlen(format), NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	ret = mm_player_set_pcm_spec(handle->mm_handle, samplerate, channel);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	return PLAYER_ERROR_NONE;
}

int legacy_player_set_streaming_playback_rate(player_h player, float rate)
{
	LOGI("[%s] rate : %0.1f", __FUNCTION__, rate);
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *) player;

	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	int ret = mm_player_set_play_speed(handle->mm_handle, rate, TRUE);

	switch (ret) {
	case MM_ERROR_NONE:
	case MM_ERROR_PLAYER_NO_OP:
		ret = PLAYER_ERROR_NONE;
		break;
	case MM_ERROR_NOT_SUPPORT_API:
	case MM_ERROR_PLAYER_SEEK:
		LOGE("[%s] PLAYER_ERROR_INVALID_OPERATION(0x%08x) : seek error", __FUNCTION__, PLAYER_ERROR_INVALID_OPERATION);
		ret = PLAYER_ERROR_INVALID_OPERATION;
		break;
	default:
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	}
	return ret;
}

static bool __media_stream_buffer_status_callback_ex(player_stream_type_e type, player_media_stream_buffer_status_e status, unsigned long long bytes, void *user_data)
{
	player_s *handle = (player_s *)user_data;
	muse_player_event_e event_type;

	if (type == PLAYER_STREAM_TYPE_AUDIO)
		event_type = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS_WITH_INFO;
	else if (type == PLAYER_STREAM_TYPE_VIDEO)
		event_type = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS_WITH_INFO;
	else
		return FALSE;

	LOGE("[%s] event type %d, status %d, bytes %llu", __FUNCTION__, event_type, status, bytes);

	if (handle->user_cb[event_type]) {
		((player_media_stream_buffer_status_cb_ex)handle->user_cb[event_type])(status, bytes, handle->user_data[event_type]);
	} else {
		LOGE("[%s][type:%d] buffer status cb was not set.", __FUNCTION__, type);
		return FALSE;
	}

	return TRUE;
}


int legacy_player_set_media_stream_buffer_status_cb_ex(player_h player, player_stream_type_e type, player_media_stream_buffer_status_cb_ex callback, void *user_data)
{
	int ret;
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	player_s *handle = (player_s *)player;
	muse_player_event_e event_type;

	if (handle->state != PLAYER_STATE_IDLE) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
	/* the type can be expaned with default and text. */
	if ((type != PLAYER_STREAM_TYPE_VIDEO) && (type != PLAYER_STREAM_TYPE_AUDIO)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_PARAMETER(type : %d)", __FUNCTION__, type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	ret = mm_player_set_media_stream_buffer_status_callback(handle->mm_handle, type, (mm_player_media_stream_buffer_status_callback)__media_stream_buffer_status_callback_ex, (void *)handle);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	if (type == PLAYER_STREAM_TYPE_VIDEO)
		event_type = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS_WITH_INFO;
	else
		event_type = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS_WITH_INFO;

	LOGI("[%s] Event type : %d ", __FUNCTION__, type);

	handle->user_cb[event_type] = callback;
	handle->user_data[event_type] = user_data;

	return PLAYER_ERROR_NONE;
}

int legacy_player_unset_media_stream_buffer_status_cb_ex(player_h player, player_stream_type_e type)
{
	int ret;
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	muse_player_event_e event_type;

	if (type == PLAYER_STREAM_TYPE_VIDEO)
		event_type = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS_WITH_INFO;
	else if (type == PLAYER_STREAM_TYPE_AUDIO)
		event_type = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS_WITH_INFO;
	else
		return PLAYER_ERROR_INVALID_PARAMETER;

	handle->user_cb[event_type] = NULL;
	handle->user_data[event_type] = NULL;

	LOGI("[%s] Event type : %d ", __FUNCTION__, type);

	ret = mm_player_set_media_stream_buffer_status_callback(handle->mm_handle, type, NULL, NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_set_media_stream_dynamic_resolution(player_h player, bool drc)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	int ret = mm_player_set_media_stream_dynamic_resolution(player, drc);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

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
#include <glib.h>
#include <muse_core.h>
#include <muse_core_msg_json.h>
#include <muse_core_ipc.h>
#include <mm_error.h>
#include <dlog.h>
#include <Evas.h>
#include "player2_private.h"
#include "player_msg_private.h"
#include "player_internal.h"

int player_set_pcm_extraction_mode(player_h player, bool sync, player_audio_pcm_extraction_cb callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_PCM_EXTRACTION_MODE;
	player_cli_s *pc = (player_cli_s *)player;
	char *ret_buf = NULL;
	_player_event_e event = _PLAYER_EVENT_TYPE_AUDIO_FRAME;

	LOGD("ENTER");

	player_msg_send1(api, pc, ret_buf, ret, INT, sync);

	if (ret == PLAYER_ERROR_NONE) {
		pc->cb_info->user_cb[event] = callback;
		pc->cb_info->user_data[event] = user_data;
		LOGI("Event type : %d ", event);
	}

	g_free(ret_buf);
	return ret;
}

int player_set_pcm_spec(player_h player, const char *format, int samplerate, int channel)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_PCM_SPEC;
	player_cli_s *pc = (player_cli_s *)player;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send3(api, pc, ret_buf, ret, STRING, format, INT, samplerate, INT, channel);

	g_free(ret_buf);
	return ret;
}

int player_set_streaming_playback_rate(player_h player, float rate)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_STREAMING_PLAYBACK_RATE;
	player_cli_s *pc = (player_cli_s *)player;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, pc, ret_buf, ret, DOUBLE, rate);
	g_free(ret_buf);
	return ret;
}

int player_set_media_stream_buffer_status_cb_ex(player_h player, player_stream_type_e stream_type, player_media_stream_buffer_status_cb_ex callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *)player;
	muse_player_api_e api = MUSE_PLAYER_API_SET_CALLBACK;
	char *ret_buf = NULL;
	_player_event_e type;
	int set = 1;

	LOGD("ENTER");

	if (stream_type == PLAYER_STREAM_TYPE_VIDEO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS_WITH_INFO;
	else if (stream_type == PLAYER_STREAM_TYPE_AUDIO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS_WITH_INFO;
	else {
		LOGE("PLAYER_ERROR_INVALID_PARAMETER(type : %d)", stream_type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	player_msg_send2(api, pc, ret_buf, ret, INT, type, INT, set);

	if (ret == PLAYER_ERROR_NONE) {
		pc->cb_info->user_cb[type] = callback;
		pc->cb_info->user_data[type] = user_data;
		LOGI("Event type : %d ", type);
	}

	g_free(ret_buf);
	return ret;
}

static void set_null_user_cb(callback_cb_info_s *cb_info, _player_event_e event)
{
	if (cb_info && event < _PLAYER_EVENT_TYPE_NUM) {
		cb_info->user_cb[event] = NULL;
		cb_info->user_data[event] = NULL;
	}
}

static void set_null_user_cb_lock(callback_cb_info_s *cb_info, _player_event_e event)
{
	bool lock = g_thread_self() != cb_info->event_queue.thread;

	if (lock)
		g_mutex_lock(&cb_info->event_queue.mutex);

	set_null_user_cb(cb_info, event);

	if (lock)
		g_mutex_unlock(&cb_info->event_queue.mutex);
}

int player_unset_media_stream_buffer_status_cb_ex(player_h player, player_stream_type_e stream_type)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *)player;
	muse_player_api_e api = MUSE_PLAYER_API_SET_CALLBACK;
	char *ret_buf = NULL;
	_player_event_e type;
	int set = 0;

	LOGD("ENTER");

	if (stream_type == PLAYER_STREAM_TYPE_VIDEO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS_WITH_INFO;
	else if (stream_type == PLAYER_STREAM_TYPE_AUDIO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS_WITH_INFO;
	else {
		LOGE("PLAYER_ERROR_INVALID_PARAMETER(type : %d)", stream_type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	set_null_user_cb_lock(pc->cb_info, type);

	player_msg_send2(api, pc, ret_buf, ret, INT, type, INT, set);

	g_free(ret_buf);
	return ret;
}

static void __evas_resize_cb (void *data, Evas *e, Evas_Object *eo, void *event_info)
{

	player_cli_s *pc = (player_cli_s *)data;
	wl_win_msg_type wl_win;
	char *wl_win_msg = (char *)&wl_win;
	char *ret_buf = NULL;
	muse_player_api_e api = MUSE_PLAYER_API_RESIZE_VIDEO_RENDER_RECT;
	int ret = PLAYER_ERROR_NONE;
	LOGD("ret =%d",ret);

	evas_object_geometry_get(eo, &wl_win.wl_window_x, &wl_win.wl_window_y, &wl_win.wl_window_width, &wl_win.wl_window_height);
	LOGD("get window rectangle: x(%d) y(%d) width(%d) height(%d)",
			wl_win.wl_window_x, wl_win.wl_window_y, wl_win.wl_window_width, wl_win.wl_window_height);

	wl_win.type = 0; /*init  but not use */
	wl_win.wl_surface_id = 0; /*init  but not use */

	player_msg_send_array(api, pc, ret_buf, ret, wl_win_msg, sizeof(wl_win_msg_type), sizeof(char));

	g_free(ret_buf);
	return;

}

static void __evas_del_cb (void *data, Evas *e, Evas_Object *eo, void *event_info)
{

	player_cli_s *pc = (player_cli_s *)data;

	evas_object_event_callback_del (eo, EVAS_CALLBACK_RESIZE, __evas_resize_cb);
	evas_object_event_callback_del (eo, EVAS_CALLBACK_DEL, __evas_del_cb);

	LOGD("evas callback del %p", eo);
	pc->have_evas_callback = FALSE;

	return;
}

int player_set_evas_object_cb(player_h player, Evas_Object * eo)
{

	PLAYER_INSTANCE_CHECK(player);
	return_val_if_fail(eo != NULL, MM_ERROR_INVALID_ARGUMENT);

	player_cli_s *pc = (player_cli_s *)player;

	if(pc->have_evas_callback && pc->eo == eo) {
		LOGW("evas object had callback already %p", pc->eo);
		return MM_ERROR_UNKNOWN;
	}
	pc->eo = eo;
	evas_object_event_callback_add (eo, EVAS_CALLBACK_RESIZE, __evas_resize_cb, player);
	evas_object_event_callback_add (eo, EVAS_CALLBACK_DEL, __evas_del_cb, player);
	LOGD("evas callback add %p", pc->eo);
	pc->have_evas_callback = TRUE;

	return MM_ERROR_NONE;
}

int player_unset_evas_object_cb(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	player_cli_s *pc = (player_cli_s *)player;
	return_val_if_fail(pc->eo != NULL, MM_ERROR_INVALID_ARGUMENT);

	evas_object_event_callback_del (pc->eo, EVAS_CALLBACK_RESIZE, __evas_resize_cb);
	evas_object_event_callback_del (pc->eo, EVAS_CALLBACK_DEL, __evas_del_cb);
	LOGD("evas callback del %p", pc->eo);
	pc->eo = NULL;
	pc->have_evas_callback = FALSE;

	return MM_ERROR_NONE;
}

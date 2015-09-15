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
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <glib.h>

#include "tbm_bufmgr.h"
#include "tbm_surface.h"
#include "tbm_surface_internal.h"
#include "media_packet.h"
#include "mm_types.h"
#include "mm_debug.h"
#include "mm_error.h"
#include "mm_player.h"
#include "mmsvc_core.h"
#include "mmsvc_core_ipc.h"
#include "player2_private.h"
#include "player_msg_private.h"

#define STREAM_PATH_LENGTH 32
#define STREAM_PATH_BASE "/tmp/mused_gst.%d"

static tbm_bufmgr bufmgr;
__thread media_format_h audio_format = NULL;
__thread media_format_h video_format = NULL;

typedef struct {
	GThread *thread;
	GQueue *queue;
	GMutex mutex;
	GCond cond;
	gint running;
}data_thread_info_t;

typedef struct {
	intptr_t handle;
	uint64_t pts;
	uint64_t size;
	media_format_mimetype_e mimetype;
}push_data_q_t;

typedef struct {
	player_h player;
	Client client;
}prepare_data_t;

/*
* define for lagacy API for mused
*/
#ifdef HAVE_WAYLAND
extern int player_set_display_wl_for_mused(player_h player, player_display_type_e type, intptr_t surface,
		int x, int y, int w, int h);
#else
extern int player_set_display_for_mused(player_h player, player_display_type_e type, unsigned int xhandle);
#endif
extern int player_set_audio_policy_info_for_mused(player_h player,
	char *stream_type, int stream_index);

int player_set_shm_stream_path_for_mused (player_h player, const char *stream_path);
int player_get_raw_video_caps(player_h player, char **caps);
/*
* Internal Implementation
*/
static int _push_media_stream(intptr_t handle, player_push_media_msg_type *push_media, char *buf);
static gpointer _player_push_media_stream_handler(gpointer param);

static void __player_callback(_player_event_e ev, Client client)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;

	LOGD("ENTER");

	player_msg_event(api, ev, client);
}

static void _prepare_async_cb(void *user_data)
{
	_player_event_e ev = _PLAYER_EVENT_TYPE_PREPARE;
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	prepare_data_t *prepare_data = (prepare_data_t *)user_data;
	Client client;
	player_h player;
	char *caps = NULL;

	if(!prepare_data) {
		LOGE("user data of callback is NULL");
		return;
	}
	client = prepare_data->client;
	player = prepare_data->player;
	g_free(prepare_data);

	if(player_get_raw_video_caps(player, &caps) == PLAYER_ERROR_NONE) {
		if(caps) {
			player_msg_event1(api, ev, client, STRING, caps);
			g_free(caps);
			return;
		}
	}
	player_msg_event(api, ev, client);
}

static void _seek_complate_cb(void *user_data)
{
	_player_event_e ev = _PLAYER_EVENT_TYPE_SEEK;
	__player_callback(ev, (Client)user_data);
}

static void _buffering_cb(int percent, void *user_data)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_BUFFERING;
	Client client = (Client)user_data;

	LOGD("ENTER");

	player_msg_event1(api, ev, client, INT, percent);

}

static void _completed_cb(void *user_data)
{
	_player_event_e ev = _PLAYER_EVENT_TYPE_COMPLETE;
	__player_callback(ev, (Client)user_data);
}

static void _interrupted_cb(player_interrupted_code_e code, void *user_data)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_INTERRUPT;
	Client client = (Client)user_data;

	LOGD("ENTER");

	player_msg_event1(api, ev, client, INT, code);
}

static void _error_cb(int code, void *user_data)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_ERROR;
	Client client = (Client)user_data;

	LOGD("ENTER");

	player_msg_event1(api, ev, client, INT, code);
}

static void _subtitle_updated_cb(unsigned long duration, char *text, void *user_data)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_SUBTITLE;
	Client client = (Client)user_data;

	LOGD("ENTER");

	player_msg_event2(api, ev, client, INT, duration, STRING, text);
}

static void _capture_video_cb(unsigned char *data, int width, int height,
		unsigned int size, void *user_data)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_CAPTURE;
	Client client = (Client)user_data;
	tbm_bo bo;
	tbm_bo_handle thandle;
	tbm_key key;
	char checker = 1;
	unsigned int expired = 0x0fffffff;

	LOGD("ENTER");

	bo = tbm_bo_alloc (bufmgr, size+1, TBM_BO_DEFAULT);
	if(!bo) {
		LOGE("TBM get error : tbm_bo_alloc return NULL");
		return;
	}
	thandle = tbm_bo_map (bo, TBM_DEVICE_CPU, TBM_OPTION_WRITE);
	if(thandle.ptr == NULL)
	{
		LOGE("TBM get error : handle pointer is NULL");
		goto capture_event_exit1;
	}
	memcpy(thandle.ptr, data, size);
	key = tbm_bo_export(bo);
	if(key == 0)
	{
		LOGE("TBM get error : export key is 0");
		checker = 0;
		goto capture_event_exit2;
	}
	/* mark to write */
	*((char *)thandle.ptr+size) = 1;

	player_msg_event4(api, ev, client, INT, width, INT, height,
			INT, size, INT, key);

capture_event_exit2:
	tbm_bo_unmap(bo);

	while(checker && expired--) {
		thandle = tbm_bo_map (bo, TBM_DEVICE_CPU, TBM_OPTION_READ);
		checker = *((char *)thandle.ptr+size);
		tbm_bo_unmap(bo);
	}

capture_event_exit1:
	tbm_bo_unref(bo);
}

static void _pd_msg_cb(player_pd_message_type_e type, void *user_data)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_PD;
	Client client = (Client)user_data;

	LOGD("ENTER");

	player_msg_event1(api, ev, client, INT, type);
}

static void _media_packet_video_decoded_cb(media_packet_h pkt, void *user_data)
{
	int ret;
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME;
	Client client = (Client)user_data;
	tbm_surface_h suf;
	tbm_bo bo[4];
	int bo_num;
	tbm_key key[4] = {0,};
	tbm_surface_info_s sinfo;
	int i;
	char *surface_info = (char *)&sinfo;
	int surface_info_size = sizeof(tbm_surface_info_s);
	intptr_t packet = (intptr_t)pkt;
	media_format_mimetype_e mimetype = MEDIA_FORMAT_NV12;
	media_format_h fmt;

	memset(&sinfo, 0, sizeof(tbm_surface_info_s));

	ret = media_packet_get_tbm_surface(pkt, &suf);
	if(ret != MEDIA_PACKET_ERROR_NONE){
		LOGE("get tbm surface error %d", ret);
		return;
	}

	bo_num = tbm_surface_internal_get_num_bos(suf);
	for(i = 0; i < bo_num; i++) {
		bo[i] = tbm_surface_internal_get_bo(suf, i);
		if(bo[i])
			key[i] = tbm_bo_export(bo[i]);
		else
			LOGE("bo_num is %d, bo[%d] is NULL", bo_num, i);
	};

	ret = tbm_surface_get_info(suf, &sinfo);
	if(ret != TBM_SURFACE_ERROR_NONE){
		LOGE("tbm_surface_get_info error %d", ret);
		return;
	}
	media_packet_get_format(pkt, &fmt);
	media_format_get_video_info(fmt, &mimetype, NULL, NULL, NULL, NULL);
	player_msg_event6_array(api, ev, client,
			INT, key[0],
			INT, key[1],
			INT, key[2],
			INT, key[3],
			POINTER, packet,
			INT, mimetype,
			surface_info, surface_info_size, sizeof(char));
}

static void _video_stream_changed_cb(
		int width, int height, int fps, int bit_rate, void *user_data)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED;
	Client client = (Client)user_data;

	player_msg_event4(api, ev, client,
			INT, width, INT, height, INT, fps, INT, bit_rate);
}

static void _media_stream_audio_buffer_status_cb(
		player_media_stream_buffer_status_e status, void *user_data)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS;
	Client client = (Client)user_data;

	player_msg_event1(api, ev, client, INT, status);
}

static void _media_stream_video_buffer_status_cb(
		player_media_stream_buffer_status_e status, void *user_data)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS;
	Client client = (Client)user_data;

	player_msg_event1(api, ev, client, INT, status);
}

static void _media_stream_audio_seek_cb(
		unsigned long long offset, void *user_data)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK;
	Client client = (Client)user_data;

#if 0
	unsigned upper_offset = (unsigned)((offset >> 32) & 0xffffffff);
	unsigned lower_offset = (unsigned)(offset & 0xffffffff);

	player_msg_event2(api, ev, client,
			INT, upper_offset, INT, lower_offset);
#else
	player_msg_event1(api, ev, client,
			INT64, offset);
#endif

}

static void _media_stream_video_seek_cb(
		unsigned long long offset, void *user_data)
{
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK;
	Client client = (Client)user_data;

#if 0
	unsigned upper_offset = (unsigned)((offset >> 32) & 0xffffffff);
	unsigned lower_offset = (unsigned)(offset & 0xffffffff);

	player_msg_event2(api, ev, client,
			INT, upper_offset, INT, lower_offset);
#else
	player_msg_event1(api, ev, client,
			INT64, offset);
#endif
}

static void _set_completed_cb(player_h player, void *client, bool set)
{
	if(set)
		player_set_completed_cb(player, _completed_cb, client);
	else
		player_unset_completed_cb(player);
}

static void _set_interrupted_cb(player_h player, void *client, bool set)
{
	if(set)
		player_set_interrupted_cb(player, _interrupted_cb, client);
	else
		player_unset_interrupted_cb(player);
}

static void _set_error_cb(player_h player, void *client, bool set)
{
	if(set)
		player_set_error_cb(player, _error_cb, client);
	else
		player_unset_error_cb(player);
}

static void _set_subtitle_cb(player_h player, void *client, bool set)
{
	if(set)
		player_set_subtitle_updated_cb(player, _subtitle_updated_cb, client);
	else
		player_unset_subtitle_updated_cb(player);
}

static void _set_buffering_cb(player_h player, void *client, bool set)
{
	if(set)
		player_set_buffering_cb(player, _buffering_cb, client);
	else
		player_unset_buffering_cb(player);
}

static void _set_pd_msg_cb(player_h player, void *client, bool set)
{
	if(set)
		player_set_progressive_download_message_cb(player, _pd_msg_cb, client);
	else
		player_unset_progressive_download_message_cb(player);
}

static void _set_media_packet_video_frame_cb(player_h player,
		void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	Client client = (Client)data;

	if(set){
		ret = player_set_media_packet_video_frame_decoded_cb(
				player, _media_packet_video_decoded_cb, client);
	} else {
		ret = player_unset_media_packet_video_frame_decoded_cb(player);
	}

	player_msg_return(api, ret, client);
}

static void _set_video_stream_changed_cb(player_h player,
		void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	Client client = (Client)data;

	if(set){
		ret = player_set_video_stream_changed_cb(
				player, _video_stream_changed_cb, client);
	} else {
		ret = player_unset_video_stream_changed_cb(player);
	}

	player_msg_return(api, ret, client);
}

static void _set_media_stream_audio_seek_cb(player_h player,
		void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	Client client = (Client)data;

	if(set){
		ret = player_set_media_stream_seek_cb(
				player, PLAYER_STREAM_TYPE_AUDIO,
				_media_stream_audio_seek_cb, client);
	} else {
		ret = player_unset_media_stream_seek_cb(
				player, PLAYER_STREAM_TYPE_AUDIO);
	}

	player_msg_return(api, ret, client);
}

static void _set_media_stream_video_seek_cb(player_h player,
		void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	Client client = (Client)data;

	if(set){
		ret = player_set_media_stream_seek_cb(
				player, PLAYER_STREAM_TYPE_VIDEO,
				_media_stream_video_seek_cb, client);
	} else {
		ret = player_unset_media_stream_seek_cb(
				player, PLAYER_STREAM_TYPE_VIDEO);
	}

	player_msg_return(api, ret, client);
}

static void _set_media_stream_audio_buffer_cb(player_h player,
		void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	Client client = (Client)data;

	if(set){
		ret = player_set_media_stream_buffer_status_cb(
				player, PLAYER_STREAM_TYPE_AUDIO,
				_media_stream_audio_buffer_status_cb, client);
	} else {
		ret = player_unset_media_stream_buffer_status_cb(
				player, PLAYER_STREAM_TYPE_AUDIO);
	}

	player_msg_return(api, ret, client);
}

static void _set_media_stream_video_buffer_cb(player_h player,
		void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	Client client = (Client)data;

	if(set){
		ret = player_set_media_stream_buffer_status_cb(
				player, PLAYER_STREAM_TYPE_VIDEO,
				_media_stream_video_buffer_status_cb, client);
	} else {
		ret = player_unset_media_stream_buffer_status_cb(
				player, PLAYER_STREAM_TYPE_VIDEO);
	}

	player_msg_return(api, ret, client);
}

static void (*set_callback_func[_PLAYER_EVENT_TYPE_NUM])
					(player_h player, void *user_data, bool set) = {
	NULL, /* _PLAYER_EVENT_TYPE_PREPARE */
	_set_completed_cb, /* _PLAYER_EVENT_TYPE_COMPLETE */
	_set_interrupted_cb, /* _PLAYER_EVENT_TYPE_INTERRUPT */
	_set_error_cb, /* _PLAYER_EVENT_TYPE_ERROR */
	_set_buffering_cb, /* _PLAYER_EVENT_TYPE_BUFFERING */
	_set_subtitle_cb, /* _PLAYER_EVENT_TYPE_SUBTITLE */
	NULL, /* _PLAYER_EVENT_TYPE_CAPTURE */
	NULL, /* _PLAYER_EVENT_TYPE_SEEK */
	_set_media_packet_video_frame_cb, /* _PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME */
	NULL, /* _PLAYER_EVENT_TYPE_AUDIO_FRAME */
	NULL, /* _PLAYER_EVENT_TYPE_VIDEO_FRAME_RENDER_ERROR */
	_set_pd_msg_cb, /* _PLAYER_EVENT_TYPE_PD */
	NULL, /* _PLAYER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT */
	NULL, /* _PLAYER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT_PRESET */
	NULL, /* _PLAYER_EVENT_TYPE_MISSED_PLUGIN */
#ifdef _PLAYER_FOR_PRODUCT
	NULL, /* _PLAYER_EVENT_TYPE_IMAGE_BUFFER */
	NULL, /*_PLAYER_EVENT_TYPE_SELECTED_SUBTITLE_LANGUAGE */
#endif
	_set_media_stream_video_buffer_cb,	/*_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS*/
	_set_media_stream_audio_buffer_cb,	/*_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS*/
	_set_media_stream_video_seek_cb,	/*_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK*/
	_set_media_stream_audio_seek_cb,	/*_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK*/
	NULL,	/*_PLAYER_EVENT_TYPE_AUDIO_STREAM_CHANGED*/
	_set_video_stream_changed_cb,	/*_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED*/
	NULL,	/*_PLAYER_EVENT_TYPE_VIDEO_BIN_CREATED*/
};

static int player_disp_set_callback(Client client)
{
	intptr_t handle;
	_player_event_e type;
	int set;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(type, mmsvc_core_client_get_msg(client));
	player_msg_get(set, mmsvc_core_client_get_msg(client));

	if(type < _PLAYER_EVENT_TYPE_NUM && set_callback_func[type] != NULL){
		set_callback_func[type]((player_h)handle, client, set);
	}

	return PLAYER_ERROR_NONE;
}

static int player_disp_create(Client client)
{
	int ret = -1;
	player_h player;
	mm_player_api_e api = MM_PLAYER_API_CREATE;
	intptr_t handle = 0;
	intptr_t client_addr = (intptr_t)client;
	data_thread_info_t *thread_i;
	static guint stream_id = 0;
	char stream_path[STREAM_PATH_LENGTH];
	int pid;

	ret = player_create(&player);
	LOGD("handle : %p, client : %p", player, client);

	player_msg_get(pid, mmsvc_core_client_get_msg(client));

	if(ret == PLAYER_ERROR_NONE)
		ret = player_sound_register(player, pid);
	else
		player_msg_return(api, ret, client);

	if(ret == PLAYER_ERROR_NONE) {
		thread_i = g_new(data_thread_info_t, 1);
		thread_i->running = 1;
		g_mutex_init(&thread_i->mutex);
		g_cond_init(&thread_i->cond);
		thread_i->queue = g_queue_new();
		thread_i->thread = (gpointer) g_thread_new("push_media",
				_player_push_media_stream_handler, client);

		mmsvc_core_client_set_cust_data(client, thread_i);

		bufmgr = tbm_bufmgr_init (-1);

		stream_id = mmsvc_core_get_atomic_uint();
		snprintf(stream_path, STREAM_PATH_LENGTH, STREAM_PATH_BASE, stream_id);
		unlink(stream_path);
		ret = player_set_shm_stream_path_for_mused(player, stream_path);

		handle = (intptr_t)player;
		player_msg_return3(api, ret, client,
				POINTER, handle, POINTER, client_addr, STRING, stream_path);
	}
	else
		player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_set_uri(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_URI;
	char uri[MM_URI_MAX_LENGTH] = { 0, };

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get_string(uri, mmsvc_core_client_get_msg(client));

	ret = player_set_uri((player_h) handle, uri);

	player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_prepare(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_PREPARE;
	player_h player;
	char *caps = NULL;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	player = (player_h) handle;

	ret = player_prepare(player);
	if(ret == PLAYER_ERROR_NONE) {
		ret = player_get_raw_video_caps(player, &caps);
		if(ret == PLAYER_ERROR_NONE && caps) {
			player_msg_return1(api, ret, client, STRING, caps);
			g_free(caps);
			return 1;
		}
	}

	player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_prepare_async(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_PREPARE_ASYNC;
	player_h player;
	prepare_data_t *prepare_data;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	player = (player_h) handle;

	prepare_data = g_new(prepare_data_t, 1);
	prepare_data->player = player;
	prepare_data->client = client;

	ret = player_prepare_async(player, _prepare_async_cb, prepare_data);

	player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_unprepare(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_UNPREPARE;
	player_h player;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player = (player_h) handle;

	ret = player_unprepare(player);

	player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_destroy(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_DESTROY;
	data_thread_info_t *thread_i;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_destroy((player_h) handle);

	thread_i = (data_thread_info_t *)mmsvc_core_client_get_cust_data(client);
	thread_i->running = 0;
	g_cond_signal(&thread_i->cond);

	g_thread_join(thread_i->thread);
	g_thread_unref(thread_i->thread);
	g_queue_free(thread_i->queue);
	g_mutex_clear(&thread_i->mutex);
	g_cond_clear(&thread_i->cond);
	g_free(thread_i);
	mmsvc_core_client_set_cust_data(client, NULL);

	tbm_bufmgr_deinit (bufmgr);
	if(audio_format) {
		media_format_unref(audio_format);
		audio_format = NULL;
	}
	if(video_format) {
		media_format_unref(video_format);
		video_format = NULL;
	}

	player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_start(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_START;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_start((player_h) handle);

	player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_stop(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_STOP;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_stop((player_h) handle);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_pause(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_PAUSE;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_pause((player_h) handle);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_set_memory_buffer(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_MEMORY_BUFFER;
	tbm_bo bo;
	tbm_bo_handle thandle;
	tbm_key key;
	int size;
	intptr_t bo_addr;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(key, mmsvc_core_client_get_msg(client));
	player_msg_get(size, mmsvc_core_client_get_msg(client));

	bo = tbm_bo_import(bufmgr, key);
	if(bo == NULL) {
		LOGE("TBM get error : bo is NULL");
		ret = PLAYER_ERROR_INVALID_OPERATION;
		player_msg_return(api, ret, client);
		return ret;
	}
	thandle = tbm_bo_map (bo, TBM_DEVICE_CPU, TBM_OPTION_READ);
	if(thandle.ptr == NULL)
	{
		LOGE("TBM get error : handle pointer is NULL");
		ret = PLAYER_ERROR_INVALID_OPERATION;
		player_msg_return(api, ret, client);
		return ret;
	}

	bo_addr = (intptr_t)bo;
	ret = player_set_memory_buffer((player_h) handle, thandle.ptr, size);
	player_msg_return1(api, ret, client, INT, bo_addr);

	return ret;
}

static int player_disp_deinit_memory_buffer(Client client)
{
	intptr_t handle;
	intptr_t bo_addr;
	tbm_bo bo;

	if( player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER)
			&& player_msg_get(bo_addr, mmsvc_core_client_get_msg(client))) {

		bo = (tbm_bo)bo_addr;

		tbm_bo_unmap(bo);
		tbm_bo_unref(bo);
	}

	return PLAYER_ERROR_NONE;
}

static int player_disp_set_volume(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_VOLUME;
	double left, right;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(left, mmsvc_core_client_get_msg(client));
	player_msg_get(right, mmsvc_core_client_get_msg(client));

	ret = player_set_volume((player_h) handle, (float)left, (float)right);

	player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_get_volume(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_VOLUME;
	float left, right;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_volume((player_h) handle, &left, &right);

	player_msg_return2(api, ret, client, DOUBLE, left, DOUBLE, right);

	return 1;
}

static int player_disp_get_state(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_STATE;
	player_state_e state;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_state((player_h) handle, &state);

	player_msg_return1(api, ret, client, INT, state);

	return 1;
}


static int player_disp_set_play_position(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_PLAY_POSITION;
	int pos;
	int accurate;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(pos, mmsvc_core_client_get_msg(client));
	player_msg_get(accurate, mmsvc_core_client_get_msg(client));

	ret = player_set_play_position((player_h) handle, pos, accurate, _seek_complate_cb ,client);

	player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_get_play_position(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_PLAY_POSITION;
	int pos;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_play_position((player_h) handle, &pos);

	player_msg_return1(api, ret, client, INT, pos);

	return 1;
}

static int player_disp_set_display(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_DISPLAY;
#ifdef HAVE_WAYLAND
	wl_win_msg_type wl_win;
	char *wl_win_msg = (char *)&wl_win;
#else
	int type;
	unsigned int xhandle;
#endif

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
#ifdef HAVE_WAYLAND
	player_msg_get_array(wl_win_msg, mmsvc_core_client_get_msg(client));

	ret = player_set_display_wl_for_mused((player_h) handle, wl_win.type, wl_win.surface,
			wl_win.wl_window_x, wl_win.wl_window_y, wl_win.wl_window_width, wl_win.wl_window_height);
#else
	player_msg_get(type, mmsvc_core_client_get_msg(client));
	player_msg_get(xhandle, mmsvc_core_client_get_msg(client));

	ret = player_set_display_for_mused((player_h) handle, type, xhandle);
#endif
	player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_set_mute(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_MUTE;
	int mute;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(mute, mmsvc_core_client_get_msg(client));

	ret = player_set_mute((player_h) handle, (bool)mute);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_is_muted(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_IS_MUTED;
	bool mute;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_is_muted((player_h) handle, &mute);

	player_msg_return1(api, ret, client, INT, mute);

	return ret;
}

static int player_disp_get_duration(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_DURATION;
	int duration = 0;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_duration((player_h) handle, &duration);

	player_msg_return1(api, ret, client, INT, duration);

	return ret;
}

static int player_disp_set_sound_type(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_SOUND_TYPE;
	int type;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(type, mmsvc_core_client_get_msg(client));

	ret = player_set_sound_type((player_h) handle, (sound_type_e)type);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_set_audio_policy_info(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_AUDIO_POLICY_INFO;
	int stream_index;
	char stream_type[MM_URI_MAX_LENGTH] = { 0, };

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(stream_index, mmsvc_core_client_get_msg(client));
	player_msg_get_string(stream_type, mmsvc_core_client_get_msg(client));

	ret = player_set_audio_policy_info_for_mused((player_h) handle,
			stream_type, stream_index);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_set_latency_mode(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_AUDIO_LATENCY_MODE;
	int latency_mode;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(latency_mode, mmsvc_core_client_get_msg(client));

	ret = player_set_audio_latency_mode((player_h) handle,
			(audio_latency_mode_e)latency_mode);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_get_latency_mode(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_AUDIO_LATENCY_MODE;
	audio_latency_mode_e latency_mode;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_audio_latency_mode((player_h) handle, &latency_mode);

	player_msg_return1(api, ret, client, INT, latency_mode);

	return ret;
}

static int player_disp_set_looping(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_LOOPING;
	int looping;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(looping, mmsvc_core_client_get_msg(client));

	ret = player_set_looping((player_h) handle, (bool)looping);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_is_looping(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_IS_LOOPING;
	bool looping;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_is_looping((player_h) handle, &looping);

	player_msg_return1(api, ret, client, INT, looping);

	return ret;
}

static int player_disp_set_display_mode(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_DISPLAY_MODE;
	int mode = 0;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(mode, mmsvc_core_client_get_msg(client));

	ret = player_set_display_mode((player_h) handle, (player_display_mode_e)mode);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_get_display_mode(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_DISPLAY_MODE;
	player_display_mode_e mode = 0;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_display_mode((player_h) handle, &mode);

	player_msg_return1(api, ret, client, INT, mode);

	return ret;
}

static int player_disp_set_playback_rate(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_PLAYBACK_RATE;
	double rate = 0;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(rate, mmsvc_core_client_get_msg(client));

	ret = player_set_playback_rate((player_h) handle, (float)rate);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_set_display_rotation(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_DISPLAY_ROTATION;
	int rotation = 0;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(rotation, mmsvc_core_client_get_msg(client));

	ret = player_set_display_rotation((player_h) handle,
			(player_display_rotation_e)rotation);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_get_display_rotation(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_DISPLAY_ROTATION;
	player_display_rotation_e rotation;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_display_rotation((player_h) handle, &rotation);

	player_msg_return1(api, ret, client, INT, rotation);

	return ret;
}

static int player_disp_set_display_visible(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_DISPLAY_VISIBLE;
	int visible = 0;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(visible, mmsvc_core_client_get_msg(client));

	ret = player_set_display_visible((player_h) handle, visible);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_is_display_visible(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_IS_DISPLAY_VISIBLE;
	bool visible = 0;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_is_display_visible((player_h) handle, &visible);

	player_msg_return1(api, ret, client, INT, visible);

	return ret;
}

static int player_disp_get_content_info(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_CONTENT_INFO;
	char *value;
	player_content_info_e key;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(key, mmsvc_core_client_get_msg(client));

	ret = player_get_content_info((player_h) handle, key, &value);

	if(ret == PLAYER_ERROR_NONE) {
		player_msg_return1(api, ret, client, STRING, value);
		free(value);
	}
	else
		player_msg_return(api, ret, client);


	return 1;
}

static int player_disp_get_codec_info(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_CODEC_INFO;
	char *video_codec;
	char *audio_codec;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_codec_info((player_h) handle, &audio_codec, &video_codec);

	if(ret == PLAYER_ERROR_NONE) {
		player_msg_return2(api, ret, client,
				STRING, audio_codec, STRING, video_codec);

		free(audio_codec);
		free(video_codec);
	}
	else
		player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_get_audio_stream_info(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_AUDIO_STREAM_INFO;
	int sample_rate = 0;
	int channel = 0;
	int bit_rate = 0;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_audio_stream_info((player_h) handle,
			&sample_rate, &channel, &bit_rate);

	player_msg_return3(api, ret, client,
			INT, sample_rate, INT, channel, INT, bit_rate);

	return 1;
}

static int player_disp_get_video_stream_info(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_VIDEO_STREAM_INFO;
	int fps = 0;
	int bit_rate = 0;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_video_stream_info((player_h) handle, &fps, &bit_rate);

	player_msg_return2(api, ret, client,
			INT, fps, INT, bit_rate);

	return 1;
}

static int player_disp_get_video_size(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_VIDEO_SIZE;
	int width = 0;
	int height = 0;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_video_size((player_h) handle, &width, &height);

	player_msg_return2(api, ret, client,
			INT, width, INT, height);

	return 1;
}

static int player_disp_get_album_art(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_ALBUM_ART;
	void *album_art;
	int size;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_album_art((player_h) handle, &album_art, &size);

	if(ret == PLAYER_ERROR_NONE) {
		player_msg_return_array(api, ret, client,
				album_art, size, sizeof(char));
	}
	else
		player_msg_return(api, ret, client);

	return 1;
}

static int player_disp_get_eq_bands_count(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BANDS_COUNT;
	int count;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_audio_effect_get_equalizer_bands_count((player_h) handle, &count);

	player_msg_return1(api, ret, client, INT, count);

	return ret;
}

static int player_disp_set_eq_all_bands(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_ALL_BANDS;
	int *band_levels;
	int length;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(length, mmsvc_core_client_get_msg(client));
	band_levels = (int *)g_try_new(int, length);

	if(band_levels) {
		player_msg_get_array(band_levels, mmsvc_core_client_get_msg(client));
		ret = player_audio_effect_set_equalizer_all_bands((player_h) handle, band_levels, length);
		g_free(band_levels);
	}
	else
		ret = PLAYER_ERROR_INVALID_OPERATION;

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_set_eq_band_level(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_BAND_LEVEL;
	int index, level;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(index, mmsvc_core_client_get_msg(client));
	player_msg_get(level, mmsvc_core_client_get_msg(client));

	ret = player_audio_effect_set_equalizer_band_level((player_h) handle, index, level);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_get_eq_band_level(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_LEVEL;
	int index, level;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(index, mmsvc_core_client_get_msg(client));

	ret = player_audio_effect_get_equalizer_band_level((player_h) handle, index, &level);

	player_msg_return1(api, ret, client, INT, level);

	return ret;
}

static int player_disp_get_eq_level_range(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_LEVEL_RANGE;
	int min, max;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_audio_effect_get_equalizer_level_range((player_h) handle, &min, &max);

	player_msg_return2(api, ret, client, INT, min, INT, max);

	return ret;
}

static int player_disp_get_eq_band_frequency(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY;
	int index, frequency;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(index, mmsvc_core_client_get_msg(client));

	ret = player_audio_effect_get_equalizer_band_frequency((player_h) handle, index, &frequency);

	player_msg_return1(api, ret, client, INT, frequency);

	return ret;
}

static int player_disp_get_eq_band_frequency_range(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY_RANGE;
	int index, range;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(index, mmsvc_core_client_get_msg(client));

	ret = player_audio_effect_get_equalizer_band_frequency_range((player_h) handle,
			index, &range);

	player_msg_return1(api, ret, client, INT, range);

	return ret;
}

static int player_disp_eq_clear(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_EQUALIZER_CLEAR;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_audio_effect_equalizer_clear((player_h) handle);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_eq_is_available(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_EQUALIZER_IS_AVAILABLE;
	bool available;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_audio_effect_equalizer_is_available((player_h) handle, &available);

	player_msg_return1(api, ret, client, INT, available);

	return ret;
}

static int player_disp_set_subtitle_path(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_SUBTITLE_PATH;
	char path[MM_URI_MAX_LENGTH] = { 0, };

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get_string(path, mmsvc_core_client_get_msg(client));

	ret = player_set_subtitle_path((player_h) handle, path);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_set_subtitle_position_offset(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_SUBTITLE_POSITION_OFFSET;
	int millisecond;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(millisecond, mmsvc_core_client_get_msg(client));

	ret = player_set_subtitle_position_offset((player_h) handle, millisecond);

	player_msg_return(api, ret, client);

	return ret;
}

static void _video_bin_created_cb(const char *caps, void *user_data)
{
	Client client = (Client)user_data;
	mm_player_cb_e api = MM_PLAYER_CB_EVENT;
	_player_event_e ev = _PLAYER_EVENT_TYPE_VIDEO_BIN_CREATED;

	LOGD("Enter");

	player_msg_event1(api, ev, client, STRING, caps);
}

static int player_disp_set_progressive_download_path(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_PROGRESSIVE_DOWNLOAD_PATH;
	char path[MM_URI_MAX_LENGTH] = { 0, };

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get_string(path, mmsvc_core_client_get_msg(client));

	ret = player_set_progressive_download_path((player_h) handle, path);
	if(ret == PLAYER_ERROR_NONE) {
		player_set_video_bin_created_cb((player_h) handle,
				_video_bin_created_cb, client);
	}

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_get_progressive_download_status(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_PROGRESSIVE_DOWNLOAD_STATUS;
	unsigned long current = 0;
	unsigned long total_size = 0;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_progressive_download_status((player_h) handle,
			&current, &total_size);

	player_msg_return2(api, ret, client, POINTER, current, POINTER, total_size);

	return ret;
}

static int player_disp_capture_video(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_CAPTURE_VIDEO;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_capture_video((player_h) handle, _capture_video_cb, client);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_set_streaming_cookie(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_STREAMING_COOKIE;
	char *cookie = NULL;
	int size;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(size, mmsvc_core_client_get_msg(client));
	cookie = (char *)g_try_new(char, size+1);
	if(cookie) {
		player_msg_get_string(cookie, mmsvc_core_client_get_msg(client));
		ret = player_set_streaming_cookie((player_h) handle, cookie, size);
		g_free(cookie);
	}
	else
		ret = PLAYER_ERROR_INVALID_OPERATION;

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_set_streaming_user_agent(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_STREAMING_USER_AGENT;
	char *user_agent;
	int size;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(size, mmsvc_core_client_get_msg(client));
	user_agent = (char *)g_try_new(char, size+1);
	if(user_agent) {
		player_msg_get_string(user_agent, mmsvc_core_client_get_msg(client));
		ret = player_set_streaming_user_agent((player_h) handle, user_agent, size);
		g_free(user_agent);
	}
	else
		ret = PLAYER_ERROR_INVALID_OPERATION;

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_get_streaming_download_progress(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_STREAMING_DOWNLOAD_PROGRESS;
	int start, current;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);

	ret = player_get_streaming_download_progress((player_h) handle, &start, &current);

	player_msg_return2(api, ret, client, INT, start, INT, current);

	return ret;
}

static gpointer _player_push_media_stream_handler(gpointer param)
{
	Client client = (Client)param;
	char *buf;
	push_data_q_t *qData;
	player_push_media_msg_type push_media;

	LOGD("Enter");

	if(!client){
		LOGE("Null parameter");
		return NULL;
	}
	data_thread_info_t *thread_i =
		(data_thread_info_t *)mmsvc_core_client_get_cust_data(client);
	if(!thread_i){
		LOGE("Null parameter");
		return NULL;
	}

	g_mutex_lock(&thread_i->mutex);
	while(thread_i->running) {
		if(g_queue_is_empty(thread_i->queue)) {
			g_cond_wait(&thread_i->cond, &thread_i->mutex);
			if(!thread_i->running) {
				break;
			}
		}
		while(1) {
			qData = g_queue_pop_head(thread_i->queue);
			g_mutex_unlock(&thread_i->mutex);
			if(qData) {
				push_media.mimetype = qData->mimetype;
				push_media.pts = qData->pts;
				push_media.size = qData->size;
				buf = mmsvc_core_ipc_get_data(client);
				if(buf) {
					_push_media_stream(qData->handle, &push_media, buf);
					mmsvc_core_ipc_delete_data(buf);
				}
				g_free(qData);
			} else {
				g_mutex_lock(&thread_i->mutex);
				break;
			}
			g_mutex_lock(&thread_i->mutex);
		}
	}
	g_mutex_unlock(&thread_i->mutex);

	LOGD("Leave");

	return NULL;
}

static int _push_media_stream(intptr_t handle, player_push_media_msg_type *push_media, char *buf)
{
	int ret = MEDIA_FORMAT_ERROR_NONE;
	media_format_h format;
	media_packet_h packet;
	media_format_mimetype_e mimetype;

	if(push_media->mimetype & MEDIA_FORMAT_VIDEO) {
		if(!video_format) {
			media_format_create(&video_format);
			if(!video_format) {
				LOGE("fail to create media format");
				return PLAYER_ERROR_INVALID_PARAMETER;
			}
			ret |= media_format_set_video_mime(video_format, push_media->mimetype);
		}
		ret |= media_format_get_video_info(video_format, &mimetype, NULL, NULL, NULL, NULL);
		if(mimetype != push_media->mimetype) {
			media_format_unref(video_format);
			media_format_create(&video_format);
			ret |= media_format_set_video_mime(video_format, push_media->mimetype);
		}
		format = video_format;
	}
	else if(push_media->mimetype & MEDIA_FORMAT_AUDIO) {
		if(!audio_format) {
			media_format_create(&audio_format);
			if(!audio_format) {
				LOGE("fail to create media format");
				return PLAYER_ERROR_INVALID_PARAMETER;
			}
			ret |= media_format_set_audio_mime(audio_format, push_media->mimetype);
		}
		ret |= media_format_get_audio_info(audio_format, &mimetype, NULL, NULL, NULL, NULL);
		if(mimetype != push_media->mimetype) {
			media_format_unref(audio_format);
			media_format_create(&audio_format);
			ret |= media_format_set_audio_mime(audio_format, push_media->mimetype);
		}
		format = audio_format;
	}
	else
		ret = MEDIA_FORMAT_ERROR_INVALID_PARAMETER;


	if(ret != MEDIA_FORMAT_ERROR_NONE) {
		LOGE("Invalid MIME %d", push_media->mimetype);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	ret = media_packet_create_from_external_memory(format, buf,
			push_media->size, NULL, NULL, &packet);
	if(ret != MEDIA_PACKET_ERROR_NONE) {
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	media_packet_set_pts(packet, push_media->pts);

	ret = player_push_media_stream((player_h)handle, packet);
	if(ret != PLAYER_ERROR_NONE)
		LOGE("ret %d", ret);

	media_packet_destroy(packet);

	return ret;
}

static int player_disp_push_media_stream(Client client)
{
	int ret = MEDIA_FORMAT_ERROR_NONE;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_PUSH_MEDIA_STREAM;
	player_push_media_msg_type push_media;
	char *push_media_msg = (char *)&push_media;
	tbm_bo bo = NULL;
	tbm_bo_handle thandle;
	char *buf = NULL;

	if(!player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER) ||
		!player_msg_get_array(push_media_msg, mmsvc_core_client_get_msg(client))) {
		ret = PLAYER_ERROR_INVALID_OPERATION;
		goto push_media_stream_exit1;
	}

	if(push_media.buf_type == PUSH_MEDIA_BUF_TYPE_TBM) {
		bo = tbm_bo_import(bufmgr, push_media.key);
		if(bo == NULL) {
			LOGE("TBM get error : bo is NULL");
			ret = PLAYER_ERROR_INVALID_OPERATION;
			goto push_media_stream_exit1;
		}
		thandle = tbm_bo_map (bo, TBM_DEVICE_CPU, TBM_OPTION_READ);
		if(thandle.ptr == NULL)
		{
			LOGE("TBM get error : handle pointer is NULL");
			ret = PLAYER_ERROR_INVALID_OPERATION;
			goto push_media_stream_exit2;
		}
		buf = thandle.ptr;
	} else if(push_media.buf_type == PUSH_MEDIA_BUF_TYPE_MSG) {
		buf = g_new(char,push_media.size);
		if(!buf) {
			ret = PLAYER_ERROR_OUT_OF_MEMORY;
			goto push_media_stream_exit1;
		}
		player_msg_get_array(buf, mmsvc_core_client_get_msg(client));
	} else if(push_media.buf_type == PUSH_MEDIA_BUF_TYPE_RAW) {
		push_data_q_t *qData = g_new(push_data_q_t, 1);
		data_thread_info_t *thread_i =
			(data_thread_info_t *)mmsvc_core_client_get_cust_data(client);
		if(!qData) {
			ret = PLAYER_ERROR_OUT_OF_MEMORY;
			goto push_media_stream_exit1;
		}
		qData->handle = handle;
		qData->mimetype = push_media.mimetype;
		qData->pts = push_media.pts;
		qData->size = push_media.size;
		g_queue_push_tail(thread_i->queue ,qData);
		g_cond_signal(&thread_i->cond);
	}

	if(push_media.buf_type == PUSH_MEDIA_BUF_TYPE_RAW)
		ret = PLAYER_ERROR_NONE;
	else
		ret = _push_media_stream(handle, &push_media, buf);

	if(push_media.buf_type == PUSH_MEDIA_BUF_TYPE_TBM)
		tbm_bo_unmap(bo);
push_media_stream_exit2:
	if(push_media.buf_type == PUSH_MEDIA_BUF_TYPE_TBM)
		tbm_bo_unref(bo);
	else if(push_media.buf_type == PUSH_MEDIA_BUF_TYPE_MSG)
		g_free(buf);

push_media_stream_exit1:
	player_msg_return(api, ret, client);
	return ret;
}

static int player_disp_set_media_stream_info(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_MEDIA_STREAM_INFO;
	media_format_mimetype_e mimetype;
	player_stream_type_e type;
	int width;
	int height;
	int avg_bps;
	int max_bps;
	int channel;
	int samplerate;
	int bit;
	media_format_h format;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(mimetype, mmsvc_core_client_get_msg(client));
	player_msg_get(type, mmsvc_core_client_get_msg(client));
	player_msg_get(avg_bps, mmsvc_core_client_get_msg(client));
	if(type == PLAYER_STREAM_TYPE_VIDEO) {
		player_msg_get(width, mmsvc_core_client_get_msg(client));
		player_msg_get(height, mmsvc_core_client_get_msg(client));
		player_msg_get(max_bps, mmsvc_core_client_get_msg(client));
	} else if(type == PLAYER_STREAM_TYPE_AUDIO) {
		player_msg_get(channel, mmsvc_core_client_get_msg(client));
		player_msg_get(samplerate, mmsvc_core_client_get_msg(client));
		player_msg_get(bit, mmsvc_core_client_get_msg(client));
	} else {
		ret = PLAYER_ERROR_INVALID_PARAMETER;
		goto set_media_stream_info_exit;
	}

	if(media_format_create(&format) == MEDIA_FORMAT_ERROR_NONE) {
		if(type == PLAYER_STREAM_TYPE_VIDEO) {
			ret = media_format_set_video_mime(format, mimetype);
			ret |= media_format_set_video_width(format, width);
			ret |= media_format_set_video_height(format, height);
			ret |= media_format_set_video_avg_bps(format, avg_bps);
			ret |= media_format_set_video_max_bps(format, max_bps);
		} else if(type == PLAYER_STREAM_TYPE_AUDIO) {
			ret = media_format_set_audio_mime(format, mimetype);
			ret |= media_format_set_audio_channel(format, channel);
			ret |= media_format_set_audio_samplerate(format, samplerate);
			ret |= media_format_set_audio_bit(format, bit);
			ret |= media_format_set_audio_avg_bps(format, avg_bps);
		}
		if(ret != MEDIA_FORMAT_ERROR_NONE) {
			ret = PLAYER_ERROR_INVALID_OPERATION;
			goto set_media_stream_info_exit;
		}
	}
	else {
		ret = PLAYER_ERROR_OUT_OF_MEMORY;
		goto set_media_stream_info_exit;
	}

	ret = player_set_media_stream_info((player_h)handle, type, format);

set_media_stream_info_exit:
	player_msg_return(api, ret, client);
	return ret;
}

static int player_disp_media_packet_finalize_cb(Client client)
{
	media_packet_h packet;

	player_msg_get_type(packet, mmsvc_core_client_get_msg(client), POINTER);

	media_packet_destroy(packet);

	return PLAYER_ERROR_NONE;
}

static int player_disp_set_media_stream_buffer_max_size(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MAX_SIZE;
	player_stream_type_e type;
	unsigned long long max_size;
	//unsigned upper_max_size, lower_max_size;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(type, mmsvc_core_client_get_msg(client));
#if 0
	player_msg_get(upper_max_size, mmsvc_core_client_get_msg(client));
	player_msg_get(lower_max_size, mmsvc_core_client_get_msg(client));

	max_size = (((unsigned long long)upper_max_size << 32) & 0xffffffff00000000)
		| (lower_max_size & 0xffffffff);
#else
	player_msg_get_type(max_size, mmsvc_core_client_get_msg(client), INT64);
#endif

	ret = player_set_media_stream_buffer_max_size((player_h) handle, type, max_size);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_get_media_stream_buffer_max_size(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MAX_SIZE;
	player_stream_type_e type;
	unsigned long long max_size;
	//unsigned upper_max_size, lower_max_size;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(type, mmsvc_core_client_get_msg(client));

	ret = player_get_media_stream_buffer_max_size((player_h) handle, type, &max_size);
	if(ret == PLAYER_ERROR_NONE) {
#if 0
		upper_max_size = (unsigned)((max_size >> 32) & 0xffffffff);
		lower_max_size = (unsigned)(max_size & 0xffffffff);
		player_msg_return2(api, ret, client,
				INT, upper_max_size, INT, lower_max_size);
#else
		player_msg_return1(api, ret, client,
				INT64, max_size);
#endif
	}
	else
		player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_set_media_stream_buffer_min_threshold(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD;
	player_stream_type_e type;
	unsigned percent;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(type, mmsvc_core_client_get_msg(client));
	player_msg_get(percent, mmsvc_core_client_get_msg(client));

	ret = player_set_media_stream_buffer_min_threshold((player_h) handle, type, percent);

	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_get_media_stream_buffer_min_threshold(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD;
	player_stream_type_e type;
	unsigned percent;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(type, mmsvc_core_client_get_msg(client));

	ret = player_get_media_stream_buffer_min_threshold((player_h) handle, type, &percent);
	if(ret == PLAYER_ERROR_NONE) {
		player_msg_return1(api, ret, client, INT, percent);
	}
	else
		player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_get_track_count(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_TRACK_COUNT;
	player_stream_type_e type;
	int count;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(type, mmsvc_core_client_get_msg(client));

	ret = player_get_track_count((player_h) handle, type, &count);
	if(ret == PLAYER_ERROR_NONE) {
		player_msg_return1(api, ret, client, INT, count);
	}
	else
		player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_get_current_track(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_CURRENT_TRACK;
	player_stream_type_e type;
	int index;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(type, mmsvc_core_client_get_msg(client));

	ret = player_get_current_track((player_h) handle, type, &index);
	if(ret == PLAYER_ERROR_NONE) {
		player_msg_return1(api, ret, client, INT, index);
	}
	else
		player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_select_track(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_SELECT_TRACK;
	player_stream_type_e type;
	int index;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(type, mmsvc_core_client_get_msg(client));
	player_msg_get(index, mmsvc_core_client_get_msg(client));

	ret = player_select_track((player_h) handle, type, index);
	player_msg_return(api, ret, client);

	return ret;
}

static int player_disp_get_track_language_code(Client client)
{
	int ret = -1;
	intptr_t handle;
	mm_player_api_e api = MM_PLAYER_API_GET_TRACK_LANGUAGE_CODE;
	player_stream_type_e type;
	int index;
	char *code;
	const int code_len=2;

	player_msg_get_type(handle, mmsvc_core_client_get_msg(client), POINTER);
	player_msg_get(type, mmsvc_core_client_get_msg(client));
	player_msg_get(index, mmsvc_core_client_get_msg(client));

	ret = player_get_track_language_code((player_h) handle, type, index, &code);
	if(ret == PLAYER_ERROR_NONE) {
		player_msg_return_array(api, ret, client, code, code_len, sizeof(char));
	}
	else
		player_msg_return(api, ret, client);

	return ret;
}

int (*dispatcher[MM_PLAYER_API_MAX]) (Client client) = {
	player_disp_create,	/* MM_PLAYER_API_CREATE */
		player_disp_destroy,	/* MM_PLAYER_API_DESTROY */
		player_disp_prepare,	/* MM_PLAYER_API_PREPARE */
		player_disp_prepare_async,	/* MM_PLAYER_API_PREPARE_ASYNC */
		player_disp_unprepare,	/* MM_PLAYER_API_UNPREPARE */
		player_disp_set_uri,	/* MM_PLAYER_API_SET_URI */
		player_disp_start,	/* MM_PLAYER_API_START */
		player_disp_stop,	/* MM_PLAYER_API_STOP */
		player_disp_pause,	/* MM_PLAYER_API_PAUSE */
		player_disp_set_memory_buffer,		/* MM_PLAYER_API_SET_MEMORY_BUFFER */
		player_disp_deinit_memory_buffer,		/* MM_PLAYER_API_DEINIT_MEMORY_BUFFER */
		player_disp_get_state,		/* MM_PLAYER_API_GET_STATE */
		player_disp_set_volume,	/* MM_PLAYER_API_SET_VOLUME */
		player_disp_get_volume,	/* MM_PLAYER_API_GET_VOLUME */
		player_disp_set_sound_type,		/* MM_PLAYER_API_SET_SOUND_TYPE */
#ifndef USE_ASM
		player_disp_set_audio_policy_info, /* MM_PLAYER_API_SET_AUDIO_POLICY_INFO */
#endif
		player_disp_set_latency_mode,		/* MM_PLAYER_API_SET_AUDIO_LATENCY_MODE */
		player_disp_get_latency_mode,		/* MM_PLAYER_API_GET_AUDIO_LATENCY_MODE */
		player_disp_set_play_position,		/* MM_PLAYER_API_SET_PLAY_POSITION */
		player_disp_get_play_position,		/* MM_PLAYER_API_GET_PLAY_POSITION */
		player_disp_set_mute,		/* MM_PLAYER_API_SET_MUTE */
		player_disp_is_muted,		/* MM_PLAYER_API_IS_MUTED */
		player_disp_set_looping,		/* MM_PLAYER_API_SET_LOOPING */
		player_disp_is_looping,		/* MM_PLAYER_API_IS_LOOPING */
		player_disp_get_duration,		/* MM_PLAYER_API_GET_DURATION */
		player_disp_set_display,		/* MM_PLAYER_API_SET_DISPLAY */
		player_disp_set_display_mode,		/* MM_PLAYER_API_SET_DISPLAY_MODE */
		player_disp_get_display_mode,		/* MM_PLAYER_API_GET_DISPLAY_MODE */
		player_disp_set_playback_rate,		/* MM_PLAYER_API_SET_PLAYBACK_RATE */
		player_disp_set_display_rotation,		/* MM_PLAYER_API_SET_DISPLAY_ROTATION */
		player_disp_get_display_rotation,		/* MM_PLAYER_API_GET_DISPLAY_ROTATION */
		player_disp_set_display_visible,		/* MM_PLAYER_API_SET_DISPLAY_VISIBLE */
		player_disp_is_display_visible,		/* MM_PLAYER_API_IS_DISPLAY_VISIBLE */
		player_disp_get_content_info,		/* MM_PLAYER_API_GET_CONTENT_INFO */
		player_disp_get_codec_info,		/* MM_PLAYER_API_GET_CODEC_INFO */
		player_disp_get_audio_stream_info,		/* MM_PLAYER_API_GET_AUDIO_STREAM_INFO */
		player_disp_get_video_stream_info,		/* MM_PLAYER_API_GET_VIDEO_STREAM_INFO */
		player_disp_get_video_size,		/* MM_PLAYER_API_GET_VIDEO_SIZE */
		player_disp_get_album_art,		/* MM_PLAYER_API_GET_ALBUM_ART */
		player_disp_get_eq_bands_count,		/* MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BANDS_COUNT */
		player_disp_set_eq_all_bands,		/* MM_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_ALL_BANDS */
		player_disp_set_eq_band_level,		/* MM_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_BAND_LEVEL */
		player_disp_get_eq_band_level,		/* MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_LEVEL */
		player_disp_get_eq_level_range,		/* MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_LEVEL_RANGE */
		player_disp_get_eq_band_frequency,		/* MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY */
		player_disp_get_eq_band_frequency_range,		/* MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY_RANGE */
		player_disp_eq_clear,		/* MM_PLAYER_API_AUDIO_EFFECT_EQUALIZER_CLEAR */
		player_disp_eq_is_available,		/* MM_PLAYER_API_AUDIO_EFFECT_EQUALIZER_IS_AVAILABLE */
		player_disp_set_progressive_download_path,		/* MM_PLAYER_API_SET_PROGRESSIVE_DOWNLOAD_PATH */
		player_disp_get_progressive_download_status,		/* MM_PLAYER_API_GET_PROGRESSIVE_DOWNLOAD_STATUS */
		player_disp_capture_video,		/* MM_PLAYER_API_CAPTURE_VIDEO */
		player_disp_set_streaming_cookie,		/* MM_PLAYER_API_SET_STREAMING_COOKIE */
		player_disp_set_streaming_user_agent,		/* MM_PLAYER_API_SET_STREAMING_USER_AGENT */
		player_disp_get_streaming_download_progress,		/* MM_PLAYER_API_GET_STREAMING_DOWNLOAD_PROGRESS */
		player_disp_set_subtitle_path,		/* MM_PLAYER_API_SET_SUBTITLE_PATH */
		player_disp_set_subtitle_position_offset,		/* MM_PLAYER_API_SET_SUBTITLE_POSITION_OFFSET */
		player_disp_push_media_stream, /* MM_PLAYER_API_PUSH_MEDIA_STREAM */
		player_disp_set_media_stream_info, /* MM_PLAYER_API_SET_MEDIA_STREAM_INFO */
		player_disp_set_callback, /* MM_PLAYER_API_SET_CALLBACK */
		player_disp_media_packet_finalize_cb, /* MM_PLAYER_API_MEDIA_PACKET_FINALIZE_CB */
		player_disp_set_media_stream_buffer_max_size, /* MM_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MAX_SIZE */
		player_disp_get_media_stream_buffer_max_size, /* MM_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MAX_SIZE */
		player_disp_set_media_stream_buffer_min_threshold, /* MM_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD */
		player_disp_get_media_stream_buffer_min_threshold, /* MM_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD */
		player_disp_get_track_count, /* MM_PLAYER_API_GET_TRACK_COUNT */
		player_disp_get_current_track, /* MM_PLAYER_API_GET_CURRENT_TRACK */
		player_disp_select_track, /* MM_PLAYER_API_SELECT_TRACK */
		player_disp_get_track_language_code, /* MM_PLAYER_API_GET_TRACK_LANGUAGE_CODE */
};


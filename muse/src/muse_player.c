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

#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>
#include <media_packet.h>
#include "muse_core.h"
#include "muse_core_ipc.h"
#include "muse_player.h"
#include "muse_player_msg.h"
#include "muse_player_api.h"
#include "legacy_player_private.h"
#include "legacy_player_internal.h"

static tbm_bufmgr bufmgr;
__thread media_format_h audio_format = NULL;
__thread media_format_h video_format = NULL;

typedef struct {
	player_h player;
	muse_module_h module;
} prepare_data_t;

static void _prepare_async_cb(void *user_data)
{
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_PREPARE;
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	prepare_data_t *prepare_data = (prepare_data_t *)user_data;
	muse_module_h module;

	if (!prepare_data) {
		LOGE("user data of callback is NULL");
		return;
	}
	module = prepare_data->module;
	g_free(prepare_data);

	player_msg_event(api, ev, module);
}

static void __player_callback(muse_player_event_e ev, muse_module_h module)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;

	LOGD("ENTER");

	player_msg_event(api, ev, module);
}

static void _seek_complate_cb(void *user_data)
{
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_SEEK;
	__player_callback(ev, (muse_module_h)user_data);
}

static void _completed_cb(void *user_data)
{
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_COMPLETE;
	__player_callback(ev, (muse_module_h)user_data);
}

static void _set_completed_cb(player_h player, void *module, bool set)
{
	if (set)
		legacy_player_set_completed_cb(player, _completed_cb, module);
	else
		legacy_player_unset_completed_cb(player);
}

static void _capture_video_cb(unsigned char *data, int width, int height, unsigned int size, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_CAPTURE;
	muse_module_h module = (muse_module_h)user_data;
	tbm_bo bo;
	tbm_bo_handle thandle;
	tbm_key key;
	char checker = 1;
	unsigned int expired = 0x0fffffff;

	LOGD("ENTER");

	bo = tbm_bo_alloc(bufmgr, size + 1, TBM_BO_DEFAULT);
	if (!bo) {
		LOGE("TBM get error : tbm_bo_alloc return NULL");
		return;
	}
	thandle = tbm_bo_map(bo, TBM_DEVICE_CPU, TBM_OPTION_WRITE);
	if (thandle.ptr == NULL) {
		LOGE("TBM get error : handle pointer is NULL");
		goto capture_event_exit1;
	}
	memcpy(thandle.ptr, data, size);
	key = tbm_bo_export(bo);
	if (key == 0) {
		LOGE("TBM get error : export key is 0");
		checker = 0;
		goto capture_event_exit2;
	}
	/* mark to write */
	*((char *)thandle.ptr + size) = 1;

	player_msg_event4(api, ev, module, INT, width, INT, height, INT, size, INT, key);

capture_event_exit2:
	tbm_bo_unmap(bo);

	while (checker && expired--) {
		thandle = tbm_bo_map(bo, TBM_DEVICE_CPU, TBM_OPTION_READ);
		checker = *((char *)thandle.ptr + size);
		tbm_bo_unmap(bo);
	}

capture_event_exit1:
	tbm_bo_unref(bo);
}

static void _pd_msg_cb(player_pd_message_type_e type, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_PD;
	muse_module_h module = (muse_module_h)user_data;

	LOGD("ENTER");

	player_msg_event1(api, ev, module, INT, type);
}

static void _media_packet_video_decoded_cb(media_packet_h pkt, void *user_data)
{
	int ret;
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME;
	muse_module_h module = (muse_module_h)user_data;
	tbm_surface_h suf;
	tbm_bo bo[4];
	int bo_num;
	tbm_key key[4] = {0, };
	tbm_surface_info_s sinfo;
	int i;
	char *surface_info = (char *)&sinfo;
	int surface_info_size = sizeof(tbm_surface_info_s);
	intptr_t packet = (intptr_t)pkt;
	media_format_mimetype_e mimetype = MEDIA_FORMAT_NV12;
	media_format_h fmt;
	uint64_t pts = 0;

	memset(&sinfo, 0, sizeof(tbm_surface_info_s));

	ret = media_packet_get_tbm_surface(pkt, &suf);
	if (ret != MEDIA_PACKET_ERROR_NONE) {
		LOGE("get tbm surface error %d", ret);
		return;
	}

	bo_num = tbm_surface_internal_get_num_bos(suf);
	for (i = 0; i < bo_num; i++) {
		bo[i] = tbm_surface_internal_get_bo(suf, i);
		if (bo[i])
			key[i] = tbm_bo_export(bo[i]);
		else
			LOGE("bo_num is %d, bo[%d] is NULL", bo_num, i);
	}

	ret = tbm_surface_get_info(suf, &sinfo);
	if (ret != TBM_SURFACE_ERROR_NONE) {
		LOGE("tbm_surface_get_info error %d", ret);
		return;
	}
	media_packet_get_format(pkt, &fmt);
	media_format_get_video_info(fmt, &mimetype, NULL, NULL, NULL, NULL);
	media_packet_get_pts(pkt, &pts);
	player_msg_event7_array(api, ev, module, INT, key[0], INT, key[1], INT, key[2], INT, key[3], POINTER, packet, INT, mimetype, INT64, pts, surface_info, surface_info_size, sizeof(char));
}

static void _video_stream_changed_cb(int width, int height, int fps, int bit_rate, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED;
	muse_module_h module = (muse_module_h)user_data;

	player_msg_event4(api, ev, module, INT, width, INT, height, INT, fps, INT, bit_rate);
}

static void _media_stream_audio_buffer_status_cb(player_media_stream_buffer_status_e status, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS;
	muse_module_h module = (muse_module_h)user_data;

	player_msg_event1(api, ev, module, INT, status);
}

static void _media_stream_video_buffer_status_cb(player_media_stream_buffer_status_e status, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS;
	muse_module_h module = (muse_module_h)user_data;

	player_msg_event1(api, ev, module, INT, status);
}

static void _media_stream_audio_buffer_status_cb_ex(player_media_stream_buffer_status_e status, unsigned long long bytes, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS_WITH_INFO;
	muse_module_h module = (muse_module_h)user_data;

	player_msg_event2(api, ev, module, INT, status, INT64, bytes);
}

static void _media_stream_video_buffer_status_cb_ex(player_media_stream_buffer_status_e status, unsigned long long bytes, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS_WITH_INFO;
	muse_module_h module = (muse_module_h)user_data;

	player_msg_event2(api, ev, module, INT, status, INT64, bytes);
}

static void _media_stream_audio_seek_cb(unsigned long long offset, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK;
	muse_module_h module = (muse_module_h)user_data;

	player_msg_event1(api, ev, module, INT64, offset);
}

static void _media_stream_video_seek_cb(unsigned long long offset, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK;
	muse_module_h module = (muse_module_h)user_data;

	player_msg_event1(api, ev, module, INT64, offset);
}

static void _interrupted_cb(player_interrupted_code_e code, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_INTERRUPT;
	muse_module_h module = (muse_module_h)user_data;

	LOGD("ENTER");

	player_msg_event1(api, ev, module, INT, code);
}

static void _set_interrupted_cb(player_h player, void *module, bool set)
{
	if (set)
		legacy_player_set_interrupted_cb(player, _interrupted_cb, module);
	else
		legacy_player_unset_interrupted_cb(player);
}

static void _error_cb(int code, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_ERROR;
	muse_module_h module = (muse_module_h)user_data;

	LOGD("ENTER");

	player_msg_event1(api, ev, module, INT, code);
}

static void _set_error_cb(player_h player, void *module, bool set)
{
	if (set)
		legacy_player_set_error_cb(player, _error_cb, module);
	else
		legacy_player_unset_error_cb(player);
}

static void _subtitle_updated_cb(unsigned long duration, char *text, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_SUBTITLE;
	muse_module_h module = (muse_module_h)user_data;

	LOGD("ENTER");

	player_msg_event2(api, ev, module, INT, duration, STRING, text);
}

static void _set_subtitle_cb(player_h player, void *module, bool set)
{
	if (set)
		legacy_player_set_subtitle_updated_cb(player, _subtitle_updated_cb, module);
	else
		legacy_player_unset_subtitle_updated_cb(player);
}

static void _buffering_cb(int percent, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_BUFFERING;
	muse_module_h module = (muse_module_h)user_data;

	LOGD("ENTER");

	player_msg_event1(api, ev, module, INT, percent);

}

static void _set_buffering_cb(player_h player, void *module, bool set)
{
	if (set)
		legacy_player_set_buffering_cb(player, _buffering_cb, module);
	else
		legacy_player_unset_buffering_cb(player);
}

static void _set_pd_msg_cb(player_h player, void *module, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_CALLBACK;

	if (set)
		ret = legacy_player_set_progressive_download_message_cb(player, _pd_msg_cb, module);
	else
		ret = legacy_player_unset_progressive_download_message_cb(player);

	player_msg_return(api, ret, module);
}

static void _set_media_packet_video_frame_cb(player_h player, void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_CALLBACK;
	muse_module_h module = (muse_module_h)data;

	if (set)
		ret = legacy_player_set_media_packet_video_frame_decoded_cb(player, _media_packet_video_decoded_cb, module);
	else
		ret = legacy_player_unset_media_packet_video_frame_decoded_cb(player);

	player_msg_return(api, ret, module);
}

static void _set_video_stream_changed_cb(player_h player, void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_CALLBACK;
	muse_module_h module = (muse_module_h)data;

	if (set)
		ret = legacy_player_set_video_stream_changed_cb(player, _video_stream_changed_cb, module);
	else
		ret = legacy_player_unset_video_stream_changed_cb(player);

	player_msg_return(api, ret, module);
}

static void _set_media_stream_audio_seek_cb(player_h player, void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_CALLBACK;
	muse_module_h module = (muse_module_h)data;

	if (set)
		ret = legacy_player_set_media_stream_seek_cb(player, PLAYER_STREAM_TYPE_AUDIO, _media_stream_audio_seek_cb, module);
	else
		ret = legacy_player_unset_media_stream_seek_cb(player, PLAYER_STREAM_TYPE_AUDIO);

	player_msg_return(api, ret, module);
}

static void _set_media_stream_video_seek_cb(player_h player, void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_CALLBACK;
	muse_module_h module = (muse_module_h)data;

	if (set)
		ret = legacy_player_set_media_stream_seek_cb(player, PLAYER_STREAM_TYPE_VIDEO, _media_stream_video_seek_cb, module);
	else
		ret = legacy_player_unset_media_stream_seek_cb(player, PLAYER_STREAM_TYPE_VIDEO);

	player_msg_return(api, ret, module);
}

static void _set_media_stream_audio_buffer_cb(player_h player, void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_CALLBACK;
	muse_module_h module = (muse_module_h)data;

	if (set)
		ret = legacy_player_set_media_stream_buffer_status_cb(player, PLAYER_STREAM_TYPE_AUDIO, _media_stream_audio_buffer_status_cb, module);
	else
		ret = legacy_player_unset_media_stream_buffer_status_cb(player, PLAYER_STREAM_TYPE_AUDIO);

	player_msg_return(api, ret, module);
}

static void _set_media_stream_video_buffer_cb(player_h player, void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_CALLBACK;
	muse_module_h module = (muse_module_h)data;

	if (set)
		ret = legacy_player_set_media_stream_buffer_status_cb(player, PLAYER_STREAM_TYPE_VIDEO, _media_stream_video_buffer_status_cb, module);
	else
		ret = legacy_player_unset_media_stream_buffer_status_cb(player, PLAYER_STREAM_TYPE_VIDEO);

	player_msg_return(api, ret, module);
}

static void _set_media_stream_audio_buffer_cb_ex(player_h player, void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_CALLBACK;
	muse_module_h module = (muse_module_h)data;

	if (set)
		ret = legacy_player_set_media_stream_buffer_status_cb_ex(player, PLAYER_STREAM_TYPE_AUDIO, _media_stream_audio_buffer_status_cb_ex, module);
	else
		ret = legacy_player_unset_media_stream_buffer_status_cb_ex(player, PLAYER_STREAM_TYPE_AUDIO);

	player_msg_return(api, ret, module);
}

static void _set_media_stream_video_buffer_cb_ex(player_h player, void *data, bool set)
{
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_CALLBACK;
	muse_module_h module = (muse_module_h)data;

	if (set)
		ret = legacy_player_set_media_stream_buffer_status_cb_ex(player, PLAYER_STREAM_TYPE_VIDEO, _media_stream_video_buffer_status_cb_ex, module);
	else
		ret = legacy_player_unset_media_stream_buffer_status_cb_ex(player, PLAYER_STREAM_TYPE_VIDEO);

	player_msg_return(api, ret, module);
}


static void (*set_callback_func[MUSE_PLAYER_EVENT_TYPE_NUM])(player_h player, void *user_data, bool set) = {
	NULL,					/* MUSE_PLAYER_EVENT_TYPE_PREPARE */
	_set_completed_cb,		/* MUSE_PLAYER_EVENT_TYPE_COMPLETE */
	_set_interrupted_cb,	/* MUSE_PLAYER_EVENT_TYPE_INTERRUPT */
	_set_error_cb,			/* MUSE_PLAYER_EVENT_TYPE_ERROR */
	_set_buffering_cb,		/* MUSE_PLAYER_EVENT_TYPE_BUFFERING */
	_set_subtitle_cb,		/* MUSE_PLAYER_EVENT_TYPE_SUBTITLE */
	NULL,					/* MUSE_PLAYER_EVENT_TYPE_CAPTURE */
	NULL,					/* MUSE_PLAYER_EVENT_TYPE_SEEK */
	_set_media_packet_video_frame_cb,	/* MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME */
	NULL,					/* MUSE_PLAYER_EVENT_TYPE_AUDIO_FRAME */
	NULL,					/* MUSE_PLAYER_EVENT_TYPE_VIDEO_FRAME_RENDER_ERROR */
	_set_pd_msg_cb,			/* MUSE_PLAYER_EVENT_TYPE_PD */
	NULL,					/* MUSE_PLAYER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT */
	NULL,					/* MUSE_PLAYER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT_PRESET */
	NULL,					/* MUSE_PLAYER_EVENT_TYPE_MISSED_PLUGIN */
#ifdef _PLAYER_FOR_PRODUCT
	NULL,					/* MUSE_PLAYER_EVENT_TYPE_IMAGE_BUFFER */
	NULL,					/* MUSE_PLAYER_EVENT_TYPE_SELECTED_SUBTITLE_LANGUAGE */
#endif
	_set_media_stream_video_buffer_cb,	/* MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS */
	_set_media_stream_audio_buffer_cb,	/* MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS */
	_set_media_stream_video_buffer_cb_ex,	/* MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS_WITH_INFO */
	_set_media_stream_audio_buffer_cb_ex,	/* MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS_WITH_INFO */
	_set_media_stream_video_seek_cb,	/* MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK */
	_set_media_stream_audio_seek_cb,	/* MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK */
	NULL,								/* MUSE_PLAYER_EVENT_TYPE_AUDIO_STREAM_CHANGED */
	_set_video_stream_changed_cb,		/* MUSE_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED */
};

static int _push_media_stream(intptr_t handle, player_push_media_msg_type *push_media, char *buf)
{
	int ret = MEDIA_FORMAT_ERROR_NONE;
	media_format_h format;
	media_packet_h packet;
	media_format_mimetype_e mimetype;
	int width = 0;
	int height = 0;

	if (push_media->mimetype & MEDIA_FORMAT_VIDEO) {
		if (!video_format) {
			media_format_create(&video_format);
			if (!video_format) {
				LOGE("fail to create media format");
				return PLAYER_ERROR_INVALID_PARAMETER;
			}
			ret |= media_format_set_video_mime(video_format, push_media->mimetype);
			ret |= media_format_set_video_width(video_format, push_media->width);
			ret |= media_format_set_video_height(video_format, push_media->height);
		}
		ret |= media_format_get_video_info(video_format, &mimetype, &width, &height, NULL, NULL);
		if (mimetype != push_media->mimetype) {
			media_format_unref(video_format);
			media_format_create(&video_format);
			ret |= media_format_set_video_mime(video_format, push_media->mimetype);
			ret |= media_format_set_video_width(video_format, push_media->width);
			ret |= media_format_set_video_height(video_format, push_media->height);
		}
		format = video_format;
	} else if (push_media->mimetype & MEDIA_FORMAT_AUDIO) {
		if (!audio_format) {
			media_format_create(&audio_format);
			if (!audio_format) {
				LOGE("fail to create media format");
				return PLAYER_ERROR_INVALID_PARAMETER;
			}
			ret |= media_format_set_audio_mime(audio_format, push_media->mimetype);
		}
		ret |= media_format_get_audio_info(audio_format, &mimetype, NULL, NULL, NULL, NULL);
		if (mimetype != push_media->mimetype) {
			media_format_unref(audio_format);
			media_format_create(&audio_format);
			ret |= media_format_set_audio_mime(audio_format, push_media->mimetype);
		}
		format = audio_format;
	} else
		ret = MEDIA_FORMAT_ERROR_INVALID_PARAMETER;

	if (ret != MEDIA_FORMAT_ERROR_NONE) {
		LOGE("Invalid MIME %d", push_media->mimetype);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	if (buf) {
		ret = media_packet_create_from_external_memory(format, buf, push_media->size, NULL, NULL, &packet);
		if (ret != MEDIA_PACKET_ERROR_NONE) {
			LOGE("fail to create media packet with external mem");
			return PLAYER_ERROR_INVALID_PARAMETER;
		}
	} else {
		ret = media_packet_create(format, NULL, NULL, &packet);
		if (ret != MEDIA_PACKET_ERROR_NONE) {
			LOGE("fail to create media packet");
			return PLAYER_ERROR_INVALID_PARAMETER;
		}
	}

	media_packet_set_pts(packet, push_media->pts);
	media_packet_set_flags(packet, push_media->flags);

	ret = legacy_player_push_media_stream((player_h)handle, packet);
	if (ret != PLAYER_ERROR_NONE)
		LOGE("ret %d", ret);

	media_packet_destroy(packet);

	return ret;
}

static void _audio_frame_decoded_cb(player_audio_raw_data_s * audio_frame, void *user_data)
{
	muse_player_cb_e api = MUSE_PLAYER_CB_EVENT;
	muse_player_event_e ev = MUSE_PLAYER_EVENT_TYPE_AUDIO_FRAME;
	muse_module_h module = (muse_module_h)user_data;
	tbm_bo bo;
	tbm_bo_handle thandle;
	tbm_key key;
	char checker = 1;
	unsigned int expired = 0x0fffffff;
	int size = 0;
	void *data = NULL;
	if (audio_frame) {
		size = audio_frame->size;
		data = audio_frame->data;
	} else {
		LOGE("audio frame is NULL");
		return;
	}

	LOGD("ENTER");

	muse_core_ipc_get_bufmgr(&bufmgr);
	bo = tbm_bo_alloc(bufmgr, size + 1, TBM_BO_DEFAULT);
	if (!bo) {
		LOGE("TBM get error : tbm_bo_alloc return NULL");
		return;
	}
	thandle = tbm_bo_map(bo, TBM_DEVICE_CPU, TBM_OPTION_WRITE);
	if (thandle.ptr == NULL) {
		LOGE("TBM get error : handle pointer is NULL");
		tbm_bo_unref(bo);
		return;
	}
	memcpy(thandle.ptr, data, size);
	key = tbm_bo_export(bo);
	if (key == 0) {
		LOGE("TBM get error : export key is 0");
		checker = 0;
		tbm_bo_unmap(bo);
		tbm_bo_unref(bo);
		return;
	}
	/* mark to write */
	*((char *)thandle.ptr + size) = 1;

	tbm_bo_unmap(bo);

	player_msg_event2_array(api, ev, module, INT, key, INT, size, audio_frame, sizeof(player_audio_raw_data_s), sizeof(char));

	while (checker && expired--) {
		thandle = tbm_bo_map(bo, TBM_DEVICE_CPU, TBM_OPTION_READ);
		checker = *((char *)thandle.ptr + size);
		tbm_bo_unmap(bo);
	}

	tbm_bo_unref(bo);
}

int player_disp_create(muse_module_h module)
{
	int ret = -1;
	player_h player;
	muse_player_api_e api = MUSE_PLAYER_API_CREATE;
	intptr_t handle = 0;
	intptr_t module_addr = (intptr_t)module;
	int pid;

	ret = legacy_player_create(&player);
	LOGD("handle : %p, module : %p", player, module);

	player_msg_get(pid, muse_core_client_get_msg(module));

	if (ret == PLAYER_ERROR_NONE)
		ret = legacy_player_sound_register(player, pid);
	else
		player_msg_return(api, ret, module);

	if (ret == PLAYER_ERROR_NONE) {
		handle = (intptr_t)player;
		muse_core_ipc_set_handle(module, handle);
		muse_core_ipc_get_bufmgr(&bufmgr);
		player_msg_return1(api, ret, module, POINTER, module_addr);
	} else
		player_msg_return(api, ret, module);

	return ret;
}

int player_disp_destroy(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_DESTROY;
	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_destroy((player_h)handle);

	if (audio_format) {
		media_format_unref(audio_format);
		audio_format = NULL;
	}
	if (video_format) {
		media_format_unref(video_format);
		video_format = NULL;
	}

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_prepare(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_PREPARE;
	player_h player;
	int timeout = 0;

	handle = muse_core_ipc_get_handle(module);

	player = (player_h)handle;

	ret = legacy_player_prepare(player);
	if (ret == PLAYER_ERROR_NONE) {
		legacy_player_get_timeout_for_muse(player, &timeout);
		player_msg_return1(api, ret, module, INT, timeout);
	} else {
		player_msg_return(api, ret, module);
	}

	return ret;
}

int player_disp_prepare_async(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_PREPARE_ASYNC;
	player_h player;
	prepare_data_t *prepare_data;
	int timeout = 0;

	handle = muse_core_ipc_get_handle(module);

	player = (player_h)handle;

	prepare_data = g_new(prepare_data_t, 1);
	prepare_data->player = player;
	prepare_data->module = module;

	ret = legacy_player_prepare_async(player, _prepare_async_cb, prepare_data);
	if (ret == PLAYER_ERROR_NONE) {
		legacy_player_get_timeout_for_muse(player, &timeout);
		player_msg_return1(api, ret, module, INT, timeout);
	} else {
		player_msg_return(api, ret, module);
	}

	return ret;
}

int player_disp_unprepare(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_UNPREPARE;
	player_h player;

	handle = muse_core_ipc_get_handle(module);
	player = (player_h)handle;

	ret = legacy_player_unprepare(player);
	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_uri(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_URI;
	char uri[MUSE_URI_MAX_LENGTH] = { 0, };

	handle = muse_core_ipc_get_handle(module);
	player_msg_get_string(uri, muse_core_client_get_msg(module));

	ret = legacy_player_set_uri((player_h)handle, uri);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_start(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_START;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_start((player_h)handle);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_stop(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_STOP;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_stop((player_h)handle);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_pause(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_PAUSE;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_pause((player_h)handle);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_memory_buffer(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_MEMORY_BUFFER;
	tbm_bo bo;
	tbm_bo_handle thandle;
	tbm_key key;
	int size;
	intptr_t bo_addr;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(key, muse_core_client_get_msg(module));
	player_msg_get(size, muse_core_client_get_msg(module));

	bo = tbm_bo_import(bufmgr, key);
	if (bo == NULL) {
		LOGE("TBM get error : bo is NULL");
		ret = PLAYER_ERROR_INVALID_OPERATION;
		player_msg_return(api, ret, module);
		return ret;
	}
	thandle = tbm_bo_map(bo, TBM_DEVICE_CPU, TBM_OPTION_READ);
	if (thandle.ptr == NULL) {
		LOGE("TBM get error : handle pointer is NULL");
		ret = PLAYER_ERROR_INVALID_OPERATION;
		player_msg_return(api, ret, module);
		return ret;
	}

	bo_addr = (intptr_t)bo;
	ret = legacy_player_set_memory_buffer((player_h)handle, thandle.ptr, size);
	player_msg_return1(api, ret, module, INT, bo_addr);

	return ret;
}

int player_disp_deinit_memory_buffer(muse_module_h module)	/* MUSE_PLAYER_API_DEINIT_MEMORY_BUFFER */
{
	intptr_t bo_addr;
	tbm_bo bo;

	if (player_msg_get(bo_addr, muse_core_client_get_msg(module))) {

		bo = (tbm_bo) bo_addr;

		tbm_bo_unmap(bo);
		tbm_bo_unref(bo);
	}

	return PLAYER_ERROR_NONE;
}

int player_disp_get_state(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_STATE;
	player_state_e state;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_state((player_h)handle, &state);

	player_msg_return1(api, ret, module, INT, state);

	return ret;
}

int player_disp_set_volume(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_VOLUME;
	double left, right;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get_type(left, muse_core_client_get_msg(module), DOUBLE);
	player_msg_get_type(right, muse_core_client_get_msg(module), DOUBLE);

	ret = legacy_player_set_volume((player_h)handle, (float)left, (float)right);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_volume(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_VOLUME;
	float left, right;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_volume((player_h)handle, &left, &right);

	player_msg_return2(api, ret, module, DOUBLE, left, DOUBLE, right);

	return ret;
}

int player_disp_set_sound_type(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_SOUND_TYPE;
	int type;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(type, muse_core_client_get_msg(module));

	ret = legacy_player_set_sound_type((player_h)handle, (sound_type_e)type);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_audio_policy_info(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_AUDIO_POLICY_INFO;
	int stream_index;
	char stream_type[MUSE_URI_MAX_LENGTH] = { 0, };

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(stream_index, muse_core_client_get_msg(module));
	player_msg_get_string(stream_type, muse_core_client_get_msg(module));

	ret = legacy_player_set_audio_policy_info_for_mused((player_h)handle, stream_type, stream_index);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_audio_latency_mode(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_AUDIO_LATENCY_MODE;
	int latency_mode;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(latency_mode, muse_core_client_get_msg(module));

	ret = legacy_player_set_audio_latency_mode((player_h)handle, (audio_latency_mode_e)latency_mode);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_audio_latency_mode(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_AUDIO_LATENCY_MODE;
	audio_latency_mode_e latency_mode;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_audio_latency_mode((player_h)handle, &latency_mode);

	player_msg_return1(api, ret, module, INT, latency_mode);

	return ret;
}

int player_disp_set_play_position(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_PLAY_POSITION;
	int pos;
	int accurate;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(pos, muse_core_client_get_msg(module));
	player_msg_get(accurate, muse_core_client_get_msg(module));

	ret = legacy_player_set_play_position((player_h)handle, pos, accurate, _seek_complate_cb, module);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_play_position(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_PLAY_POSITION;
	int pos;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_play_position((player_h)handle, &pos);

	player_msg_return1(api, ret, module, INT, pos);

	return ret;
}

int player_disp_set_mute(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_MUTE;
	int mute;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(mute, muse_core_client_get_msg(module));

	ret = legacy_player_set_mute((player_h)handle, (bool)mute);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_is_muted(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_IS_MUTED;
	bool mute;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_is_muted((player_h)handle, &mute);

	player_msg_return1(api, ret, module, INT, mute);

	return ret;
}

int player_disp_set_looping(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_LOOPING;
	int looping;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(looping, muse_core_client_get_msg(module));

	ret = legacy_player_set_looping((player_h)handle, (bool)looping);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_is_looping(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_IS_LOOPING;
	bool looping;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_is_looping((player_h)handle, &looping);

	player_msg_return1(api, ret, module, INT, looping);

	return ret;
}

int player_disp_get_duration(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_DURATION;
	int duration = 0;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_duration((player_h)handle, &duration);

	player_msg_return1(api, ret, module, INT, duration);

	return ret;
}

int player_disp_set_display(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_DISPLAY;
#ifdef HAVE_WAYLAND
	wl_win_msg_type wl_win;
	char *wl_win_msg = (char *)&wl_win;
#else
	int type;
	unsigned int xhandle;
#endif

	handle = muse_core_ipc_get_handle(module);
#ifdef HAVE_WAYLAND
	player_msg_get_array(wl_win_msg, muse_core_client_get_msg(module));

	ret = legacy_player_set_display_wl_for_mused((player_h)handle, wl_win.type, wl_win.wl_surface_id, wl_win.wl_window_x, wl_win.wl_window_y, wl_win.wl_window_width, wl_win.wl_window_height);
#else
	player_msg_get(type, muse_core_client_get_msg(module));
	player_msg_get(xhandle, muse_core_client_get_msg(module));

	ret = legacy_player_set_display_for_mused((player_h)handle, type, xhandle);
#endif
	player_msg_return(api, ret, module);

	return ret;
}


int player_disp_set_display_mode(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_DISPLAY_MODE;
	int mode = 0;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(mode, muse_core_client_get_msg(module));

	ret = legacy_player_set_display_mode((player_h)handle, (player_display_mode_e)mode);

	player_msg_return(api, ret, module);

	return ret;
}


int player_disp_get_display_mode(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_DISPLAY_MODE;
	player_display_mode_e mode = 0;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_display_mode((player_h)handle, &mode);

	player_msg_return1(api, ret, module, INT, mode);

	return ret;
}

int player_disp_set_playback_rate(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_PLAYBACK_RATE;
	double rate = 0;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get_type(rate, muse_core_client_get_msg(module), DOUBLE);

	ret = legacy_player_set_playback_rate((player_h)handle, (float)rate);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_display_rotation(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_DISPLAY_ROTATION;
	int rotation = 0;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(rotation, muse_core_client_get_msg(module));

	ret = legacy_player_set_display_rotation((player_h)handle, (player_display_rotation_e)rotation);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_display_rotation(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_DISPLAY_ROTATION;
	player_display_rotation_e rotation;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_display_rotation((player_h)handle, &rotation);

	player_msg_return1(api, ret, module, INT, rotation);

	return ret;
}

int player_disp_set_display_visible(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_DISPLAY_VISIBLE;
	int visible = 0;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(visible, muse_core_client_get_msg(module));

	ret = legacy_player_set_display_visible((player_h)handle, visible);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_is_display_visible(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_IS_DISPLAY_VISIBLE;
	bool visible = 0;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_is_display_visible((player_h)handle, &visible);

	player_msg_return1(api, ret, module, INT, visible);

	return ret;
}

#ifdef HAVE_WAYLAND
int player_disp_resize_video_render_rect(muse_module_h module)	/* private */
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_RESIZE_VIDEO_RENDER_RECT;

	wl_win_msg_type wl_win;
	char *wl_win_msg = (char *)&wl_win;

	handle = muse_core_ipc_get_handle(module);

	player_msg_get_array(wl_win_msg, muse_core_client_get_msg(module));

	ret = legacy_player_resize_video_render_rect((player_h)handle, wl_win.wl_window_x, wl_win.wl_window_y, wl_win.wl_window_width, wl_win.wl_window_height);
	player_msg_return(api, ret, module);

	return ret;

}
#endif

int player_disp_get_content_info(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_CONTENT_INFO;
	char *value;
	player_content_info_e key;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(key, muse_core_client_get_msg(module));

	ret = legacy_player_get_content_info((player_h)handle, key, &value);

	if (ret == PLAYER_ERROR_NONE) {
		player_msg_return1(api, ret, module, STRING, value);
		free(value);
	} else
		player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_codec_info(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_CODEC_INFO;
	char *video_codec;
	char *audio_codec;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_codec_info((player_h)handle, &audio_codec, &video_codec);

	if (ret == PLAYER_ERROR_NONE) {
		player_msg_return2(api, ret, module, STRING, audio_codec, STRING, video_codec);

		free(audio_codec);
		free(video_codec);
	} else
		player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_audio_stream_info(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_AUDIO_STREAM_INFO;
	int sample_rate = 0;
	int channel = 0;
	int bit_rate = 0;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_audio_stream_info((player_h)handle, &sample_rate, &channel, &bit_rate);

	player_msg_return3(api, ret, module, INT, sample_rate, INT, channel, INT, bit_rate);

	return ret;
}

int player_disp_get_video_stream_info(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_VIDEO_STREAM_INFO;
	int fps = 0;
	int bit_rate = 0;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_video_stream_info((player_h)handle, &fps, &bit_rate);

	player_msg_return2(api, ret, module, INT, fps, INT, bit_rate);

	return ret;
}

int player_disp_get_video_size(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_VIDEO_SIZE;
	int width = 0;
	int height = 0;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_video_size((player_h)handle, &width, &height);

	player_msg_return2(api, ret, module, INT, width, INT, height);

	return ret;
}

int player_disp_get_album_art(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_ALBUM_ART;
	void *album_art;
	int size;
	tbm_bo bo;
	tbm_bo_handle thandle;
	tbm_key key;
	char checker = 1;
	unsigned int expired = 0x0fffffff;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_album_art((player_h)handle, &album_art, &size);
	if (ret == PLAYER_ERROR_NONE && size > 0) {
		bo = tbm_bo_alloc(bufmgr, size+1, TBM_BO_DEFAULT);
		if (!bo) {
			LOGE("TBM get error : tbm_bo_alloc return NULL");
			ret = PLAYER_ERROR_INVALID_OPERATION; /* according to the api desc */
			goto exit;
		}
		thandle = tbm_bo_map(bo, TBM_DEVICE_CPU, TBM_OPTION_WRITE);
		if (thandle.ptr == NULL) {
			LOGE("TBM get error : handle pointer is NULL");
			ret = PLAYER_ERROR_INVALID_OPERATION; /* according to the api desc */
			tbm_bo_unref(bo);
			goto exit;
		}

		memcpy(thandle.ptr, album_art, size);
		/* mark to write */
		*((char *)thandle.ptr + size) = 1;
		tbm_bo_unmap(bo);

		key = tbm_bo_export(bo);
		if (key == 0) {
			LOGE("TBM get error : export key is 0");
			ret = PLAYER_ERROR_INVALID_OPERATION; /* according to the api desc */
			tbm_bo_unref(bo);
			goto exit;
		}

		LOGE("size %d key %d", size, key);
		player_msg_return2(api, ret, module, INT, size, INT, key);

		while (checker && expired--) {
			thandle = tbm_bo_map(bo, TBM_DEVICE_CPU, TBM_OPTION_READ);
			checker = *((char *)thandle.ptr + size);
			tbm_bo_unmap(bo);
		}
		tbm_bo_unref(bo);
	} else if (ret == PLAYER_ERROR_NONE) {
		LOGD("album art size is zero, didn't make tbm_bo");
		player_msg_return2(api, ret, module, INT, size, INT, 0);
	}
	else
		player_msg_return(api, ret, module);

	return ret;

exit:
	player_msg_return(api, ret, module);
	return ret;
}

int player_disp_audio_effect_get_equalizer_bands_count(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BANDS_COUNT;
	int count;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_audio_effect_get_equalizer_bands_count((player_h)handle, &count);

	player_msg_return1(api, ret, module, INT, count);

	return ret;
}

int player_disp_audio_effect_set_equalizer_all_bands(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_ALL_BANDS;
	int *band_levels;
	int length;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(length, muse_core_client_get_msg(module));
	band_levels = (int *)g_try_new(int, length);

	if (band_levels) {
		player_msg_get_array(band_levels, muse_core_client_get_msg(module));
		ret = legacy_player_audio_effect_set_equalizer_all_bands((player_h)handle, band_levels, length);
		g_free(band_levels);
	} else
		ret = PLAYER_ERROR_INVALID_OPERATION;

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_audio_effect_set_equalizer_band_level(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_BAND_LEVEL;
	int index, level;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(index, muse_core_client_get_msg(module));
	player_msg_get(level, muse_core_client_get_msg(module));

	ret = legacy_player_audio_effect_set_equalizer_band_level((player_h)handle, index, level);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_audio_effect_get_equalizer_band_level(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_LEVEL;
	int index, level;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(index, muse_core_client_get_msg(module));

	ret = legacy_player_audio_effect_get_equalizer_band_level((player_h)handle, index, &level);

	player_msg_return1(api, ret, module, INT, level);

	return ret;
}

int player_disp_audio_effect_get_equalizer_level_range(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_LEVEL_RANGE;
	int min, max;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_audio_effect_get_equalizer_level_range((player_h)handle, &min, &max);

	player_msg_return2(api, ret, module, INT, min, INT, max);

	return ret;
}

int player_disp_audio_effect_get_equalizer_band_frequency(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY;
	int index, frequency;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(index, muse_core_client_get_msg(module));

	ret = legacy_player_audio_effect_get_equalizer_band_frequency((player_h)handle, index, &frequency);

	player_msg_return1(api, ret, module, INT, frequency);

	return ret;
}

int player_disp_audio_effect_get_equalizer_band_frequency_range(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY_RANGE;
	int index, range;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(index, muse_core_client_get_msg(module));

	ret = legacy_player_audio_effect_get_equalizer_band_frequency_range((player_h)handle, index, &range);

	player_msg_return1(api, ret, module, INT, range);

	return ret;
}

int player_disp_audio_effect_equalizer_clear(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_AUDIO_EFFECT_EQUALIZER_CLEAR;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_audio_effect_equalizer_clear((player_h)handle);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_audio_effect_equalizer_is_available(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_AUDIO_EFFECT_EQUALIZER_IS_AVAILABLE;
	bool available;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_audio_effect_equalizer_is_available((player_h)handle, &available);

	player_msg_return1(api, ret, module, INT, available);

	return ret;
}

int player_disp_set_progressive_download_path(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_PROGRESSIVE_DOWNLOAD_PATH;
	char path[MUSE_URI_MAX_LENGTH] = { 0, };

	handle = muse_core_ipc_get_handle(module);
	player_msg_get_string(path, muse_core_client_get_msg(module));

	ret = legacy_player_set_progressive_download_path((player_h)handle, path);
	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_progressive_download_status(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_PROGRESSIVE_DOWNLOAD_STATUS;
	unsigned long current = 0;
	unsigned long total_size = 0;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_progressive_download_status((player_h)handle, &current, &total_size);

	player_msg_return2(api, ret, module, POINTER, current, POINTER, total_size);

	return ret;
}

int player_disp_capture_video(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_CAPTURE_VIDEO;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_capture_video((player_h)handle, _capture_video_cb, module);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_streaming_cookie(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_STREAMING_COOKIE;
	char *cookie = NULL;
	int size;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(size, muse_core_client_get_msg(module));
	cookie = (char *)g_try_new(char, size + 1);
	if (cookie) {
		player_msg_get_string(cookie, muse_core_client_get_msg(module));
		ret = legacy_player_set_streaming_cookie((player_h)handle, cookie, size);
		g_free(cookie);
	} else
		ret = PLAYER_ERROR_INVALID_OPERATION;

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_streaming_user_agent(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_STREAMING_USER_AGENT;
	char *user_agent;
	int size;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(size, muse_core_client_get_msg(module));
	user_agent = (char *)g_try_new(char, size + 1);
	if (user_agent) {
		player_msg_get_string(user_agent, muse_core_client_get_msg(module));
		ret = legacy_player_set_streaming_user_agent((player_h)handle, user_agent, size);
		g_free(user_agent);
	} else
		ret = PLAYER_ERROR_INVALID_OPERATION;

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_streaming_download_progress(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_STREAMING_DOWNLOAD_PROGRESS;
	int start, current;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_streaming_download_progress((player_h)handle, &start, &current);

	player_msg_return2(api, ret, module, INT, start, INT, current);

	return ret;
}

int player_disp_set_subtitle_path(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_SUBTITLE_PATH;
	char path[MUSE_URI_MAX_LENGTH] = { 0, };

	handle = muse_core_ipc_get_handle(module);
	player_msg_get_string(path, muse_core_client_get_msg(module));

	ret = legacy_player_set_subtitle_path((player_h)handle, path);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_subtitle_position_offset(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_SUBTITLE_POSITION_OFFSET;
	int millisecond;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(millisecond, muse_core_client_get_msg(module));

	ret = legacy_player_set_subtitle_position_offset((player_h)handle, millisecond);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_push_media_stream(muse_module_h module)
{
	int ret = MEDIA_FORMAT_ERROR_NONE;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_PUSH_MEDIA_STREAM;
	player_push_media_msg_type push_media;
	char *push_media_msg = (char *)&push_media;
	tbm_bo bo = NULL;
	tbm_bo_handle thandle;
	char *buf = NULL;

	handle = muse_core_ipc_get_handle(module);
	if (!player_msg_get_array(push_media_msg, muse_core_client_get_msg(module))) {
		ret = PLAYER_ERROR_INVALID_OPERATION;
		goto push_media_stream_exit1;
	}

	if (push_media.buf_type == PUSH_MEDIA_BUF_TYPE_TBM) {
		bo = tbm_bo_import(bufmgr, push_media.key);
		if (bo == NULL) {
			LOGE("TBM get error : bo is NULL");
			ret = PLAYER_ERROR_INVALID_OPERATION;
			goto push_media_stream_exit1;
		}
		thandle = tbm_bo_map(bo, TBM_DEVICE_CPU, TBM_OPTION_READ);
		if (thandle.ptr == NULL) {
			LOGE("TBM get error : handle pointer is NULL");
			ret = PLAYER_ERROR_INVALID_OPERATION;
			goto push_media_stream_exit2;
		}
		buf = thandle.ptr;
	} else if (push_media.buf_type == PUSH_MEDIA_BUF_TYPE_MSG) {
		buf = g_new(char, push_media.size);
		if (!buf) {
			ret = PLAYER_ERROR_OUT_OF_MEMORY;
			goto push_media_stream_exit1;
		}
		player_msg_get_array(buf, muse_core_client_get_msg(module));
	} else if (push_media.buf_type == PUSH_MEDIA_BUF_TYPE_RAW) {
		buf = muse_core_ipc_get_data(module);
	}

	ret = _push_media_stream(handle, &push_media, buf);

	if (push_media.buf_type == PUSH_MEDIA_BUF_TYPE_TBM)
		tbm_bo_unmap(bo);
push_media_stream_exit2:
	if (push_media.buf_type == PUSH_MEDIA_BUF_TYPE_TBM)
		tbm_bo_unref(bo);
	else if (push_media.buf_type == PUSH_MEDIA_BUF_TYPE_MSG)
		g_free(buf);
	else if (push_media.buf_type == PUSH_MEDIA_BUF_TYPE_RAW && buf)
		muse_core_ipc_delete_data(buf);
push_media_stream_exit1:
	player_msg_return(api, ret, module);
	return ret;
}

int player_disp_set_media_stream_info(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_MEDIA_STREAM_INFO;
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

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(mimetype, muse_core_client_get_msg(module));
	player_msg_get(type, muse_core_client_get_msg(module));
	player_msg_get(avg_bps, muse_core_client_get_msg(module));
	if (type == PLAYER_STREAM_TYPE_VIDEO) {
		player_msg_get(width, muse_core_client_get_msg(module));
		player_msg_get(height, muse_core_client_get_msg(module));
		player_msg_get(max_bps, muse_core_client_get_msg(module));
	} else if (type == PLAYER_STREAM_TYPE_AUDIO) {
		player_msg_get(channel, muse_core_client_get_msg(module));
		player_msg_get(samplerate, muse_core_client_get_msg(module));
		player_msg_get(bit, muse_core_client_get_msg(module));
	} else {
		ret = PLAYER_ERROR_INVALID_PARAMETER;
		goto set_media_stream_info_exit;
	}

	if (media_format_create(&format) == MEDIA_FORMAT_ERROR_NONE) {
		if (type == PLAYER_STREAM_TYPE_VIDEO) {
			ret = media_format_set_video_mime(format, mimetype);
			ret |= media_format_set_video_width(format, width);
			ret |= media_format_set_video_height(format, height);
			ret |= media_format_set_video_avg_bps(format, avg_bps);
			ret |= media_format_set_video_max_bps(format, max_bps);
		} else if (type == PLAYER_STREAM_TYPE_AUDIO) {
			ret = media_format_set_audio_mime(format, mimetype);
			ret |= media_format_set_audio_channel(format, channel);
			ret |= media_format_set_audio_samplerate(format, samplerate);
			ret |= media_format_set_audio_bit(format, bit);
			ret |= media_format_set_audio_avg_bps(format, avg_bps);
		}
		if (ret != MEDIA_FORMAT_ERROR_NONE) {
			ret = PLAYER_ERROR_INVALID_OPERATION;
			goto set_media_stream_info_exit;
		}
	} else {
		ret = PLAYER_ERROR_OUT_OF_MEMORY;
		goto set_media_stream_info_exit;
	}

	ret = legacy_player_set_media_stream_info((player_h)handle, type, format);

 set_media_stream_info_exit:
	player_msg_return(api, ret, module);
	return ret;
}


int player_disp_set_callback(muse_module_h module)
{
	intptr_t handle;
	muse_player_event_e type;
	int set;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(type, muse_core_client_get_msg(module));
	player_msg_get(set, muse_core_client_get_msg(module));

	if (type < MUSE_PLAYER_EVENT_TYPE_NUM && set_callback_func[type] != NULL)
		set_callback_func[type] ((player_h)handle, module, set);

	return PLAYER_ERROR_NONE;
}

int player_disp_media_packet_finalize_cb(muse_module_h module)
{
	media_packet_h packet;

	player_msg_get_type(packet, muse_core_client_get_msg(module), POINTER);

	media_packet_destroy(packet);

	return PLAYER_ERROR_NONE;
}

int player_disp_set_media_stream_buffer_max_size(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MAX_SIZE;
	player_stream_type_e type;
	unsigned long long max_size;
	/* unsigned upper_max_size, lower_max_size; */

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(type, muse_core_client_get_msg(module));
#if 0
	player_msg_get(upper_max_size, muse_core_client_get_msg(module));
	player_msg_get(lower_max_size, muse_core_client_get_msg(module));

	max_size = (((unsigned long long)upper_max_size << 32) & 0xffffffff00000000)
		| (lower_max_size & 0xffffffff);
#else
	player_msg_get_type(max_size, muse_core_client_get_msg(module), INT64);
#endif

	ret = legacy_player_set_media_stream_buffer_max_size((player_h) handle, type, max_size);

	player_msg_return(api, ret, module);

	return ret;
}


int player_disp_get_media_stream_buffer_max_size(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MAX_SIZE;
	player_stream_type_e type;
	unsigned long long max_size;
	/* unsigned upper_max_size, lower_max_size; */

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(type, muse_core_client_get_msg(module));

	ret = legacy_player_get_media_stream_buffer_max_size((player_h)handle, type, &max_size);
	if (ret == PLAYER_ERROR_NONE) {
#if 0
		upper_max_size = (unsigned)((max_size >> 32) & 0xffffffff);
		lower_max_size = (unsigned)(max_size & 0xffffffff);
		player_msg_return2(api, ret, module, INT, upper_max_size, INT, lower_max_size);
#else
		player_msg_return1(api, ret, module, INT64, max_size);
#endif
	} else
		player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_media_stream_buffer_min_threshold(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD;
	player_stream_type_e type;
	unsigned percent;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(type, muse_core_client_get_msg(module));
	player_msg_get(percent, muse_core_client_get_msg(module));

	ret = legacy_player_set_media_stream_buffer_min_threshold((player_h)handle, type, percent);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_media_stream_buffer_min_threshold(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD;
	player_stream_type_e type;
	unsigned percent;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(type, muse_core_client_get_msg(module));

	ret = legacy_player_get_media_stream_buffer_min_threshold((player_h)handle, type, &percent);
	if (ret == PLAYER_ERROR_NONE)
		player_msg_return1(api, ret, module, INT, percent);
	else
		player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_track_count(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_TRACK_COUNT;
	player_stream_type_e type;
	int count;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(type, muse_core_client_get_msg(module));

	ret = legacy_player_get_track_count((player_h)handle, type, &count);
	if (ret == PLAYER_ERROR_NONE)
		player_msg_return1(api, ret, module, INT, count);
	else
		player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_current_track(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_CURRENT_TRACK;
	player_stream_type_e type;
	int index;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(type, muse_core_client_get_msg(module));

	ret = legacy_player_get_current_track((player_h)handle, type, &index);
	if (ret == PLAYER_ERROR_NONE)
		player_msg_return1(api, ret, module, INT, index);
	else
		player_msg_return(api, ret, module);

	return ret;
}

int player_disp_select_track(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SELECT_TRACK;
	player_stream_type_e type;
	int index;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(type, muse_core_client_get_msg(module));
	player_msg_get(index, muse_core_client_get_msg(module));

	ret = legacy_player_select_track((player_h)handle, type, index);
	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_get_track_language_code(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_GET_TRACK_LANGUAGE_CODE;
	player_stream_type_e type;
	int index;
	char *code;
	const int code_len = 2;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(type, muse_core_client_get_msg(module));
	player_msg_get(index, muse_core_client_get_msg(module));

	ret = legacy_player_get_track_language_code((player_h)handle, type, index, &code);
	if (ret == PLAYER_ERROR_NONE)
		player_msg_return_array(api, ret, module, code, code_len, sizeof(char));
	else
		player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_pcm_extraction_mode(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_PCM_EXTRACTION_MODE;
	int sync;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get(sync, muse_core_client_get_msg(module));

	ret = legacy_player_set_pcm_extraction_mode((player_h)handle, sync, _audio_frame_decoded_cb, module);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_pcm_spec(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_PCM_SPEC;
	char format[MUSE_URI_MAX_LENGTH] = { 0, };
	int samplerate;
	int channel;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get_string(format, muse_core_client_get_msg(module));
	player_msg_get(samplerate, muse_core_client_get_msg(module));
	player_msg_get(channel, muse_core_client_get_msg(module));

	ret = legacy_player_set_pcm_spec((player_h)handle, format, samplerate, channel);

	player_msg_return(api, ret, module);

	return ret;
}

int player_disp_set_streaming_playback_rate(muse_module_h module)
{
	int ret = -1;
	intptr_t handle;
	muse_player_api_e api = MUSE_PLAYER_API_SET_STREAMING_PLAYBACK_RATE;
	double rate = 0;

	handle = muse_core_ipc_get_handle(module);
	player_msg_get_type(rate, muse_core_client_get_msg(module), DOUBLE);

	ret = legacy_player_set_streaming_playback_rate((player_h)handle, (float)rate);

	player_msg_return(api, ret, module);

	return ret;
}


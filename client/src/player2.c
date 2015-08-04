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
#include "glib.h"
#include "tbm_bufmgr.h"
#include "tbm_surface.h"
#include "tbm_surface_internal.h"
#include "Evas.h"
#include "Elementary.h"
#include "Ecore.h"
#ifdef HAVE_WAYLAND
#include "Ecore_Wayland.h"
#endif
#include "mmsvc_core.h"
#include "mmsvc_core_msg_json.h"
#include "mmsvc_core_ipc.h"
#include "player2_private.h"
#include "player_msg_private.h"
#include "sound_manager_internal.h"
#include "mm_error.h"
#include "mm_player.h"
#include "mm_player_mused.h"
#include "dlog.h"

#define CALLBACK_TIME_OUT 12
static tbm_bufmgr bufmgr;

typedef struct {
	int int_data;
	char *buf;
	callback_cb_info_s *cb_info;
}_player_cb_data;

typedef struct {
	int remote_pkt;
	callback_cb_info_s *cb_info;
}_media_pkt_fin_data;


/*
 Global varialbe
*/

/*
* Internal Implementation
*/
static int _player_deinit_memory_buffer(player_cli_s *pc);

int _player_media_packet_finalize(media_packet_h pkt, int error_code,
				  void *user_data)
{
	int ret = 0;
	tbm_surface_h tsurf = NULL;
	mm_player_api_e api = MM_PLAYER_API_MEDIA_PACKET_FINALIZE_CB;
	_media_pkt_fin_data *fin_data = (_media_pkt_fin_data *)user_data;
	int packet;
	char *sndMsg;

	if (pkt == NULL || user_data == NULL) {
		LOGE("invalid parameter buffer %p, user_data %p", pkt,
		     user_data);
		return MEDIA_PACKET_FINALIZE;
	}

	ret = media_packet_get_tbm_surface(pkt, &tsurf);
	if (ret != MEDIA_PACKET_ERROR_NONE) {
		LOGE("media_packet_get_tbm_surface failed 0x%x", ret);
		return MEDIA_PACKET_FINALIZE;
	}

	if (tsurf) {
		tbm_surface_destroy(tsurf);
		tsurf = NULL;
	}

	packet = fin_data->remote_pkt;
	sndMsg = mmsvc_core_msg_json_factory_new(api, "packet", packet, 0);
	mmsvc_core_ipc_send_msg(fin_data->cb_info->fd, sndMsg);
	mmsvc_core_msg_json_factory_free(sndMsg);

	g_free(fin_data);

	return MEDIA_PACKET_FINALIZE;
}

static int __player_convert_error_code(int code, char* func_name)
{
	int ret = PLAYER_ERROR_INVALID_OPERATION;
	char* msg="PLAYER_ERROR_INVALID_OPERATION";
	switch(code)
	{
		case MM_ERROR_NONE:
		case MM_ERROR_PLAYER_AUDIO_CODEC_NOT_FOUND:
		case MM_ERROR_PLAYER_VIDEO_CODEC_NOT_FOUND:
			ret = PLAYER_ERROR_NONE;
			msg = "PLAYER_ERROR_NONE";
			break;
		case MM_ERROR_INVALID_ARGUMENT:
			ret = PLAYER_ERROR_INVALID_PARAMETER;
			msg = "PLAYER_ERROR_INVALID_PARAMETER";
			break;
		case MM_ERROR_PLAYER_CODEC_NOT_FOUND:
		case MM_ERROR_PLAYER_STREAMING_UNSUPPORTED_AUDIO:
		case MM_ERROR_PLAYER_STREAMING_UNSUPPORTED_VIDEO:
		case MM_ERROR_PLAYER_STREAMING_UNSUPPORTED_MEDIA_TYPE:
		case MM_ERROR_PLAYER_NOT_SUPPORTED_FORMAT:
			ret = PLAYER_ERROR_NOT_SUPPORTED_FILE;
			msg = "PLAYER_ERROR_NOT_SUPPORTED_FILE";
			break;
		case MM_ERROR_PLAYER_INVALID_STATE:
		case MM_ERROR_PLAYER_NOT_INITIALIZED:
			ret = PLAYER_ERROR_INVALID_STATE;
			msg = "PLAYER_ERROR_INVALID_STATE";
			break;
		case MM_ERROR_PLAYER_INTERNAL:
		case MM_ERROR_PLAYER_INVALID_STREAM:
		case MM_ERROR_PLAYER_STREAMING_FAIL:
		case MM_ERROR_PLAYER_NO_OP:
			ret = PLAYER_ERROR_INVALID_OPERATION;
			msg = "PLAYER_ERROR_INVALID_OPERATION";
			break;
		case MM_ERROR_PLAYER_SOUND_EFFECT_INVALID_STATUS:
		case MM_ERROR_NOT_SUPPORT_API:
		case MM_ERROR_PLAYER_SOUND_EFFECT_NOT_SUPPORTED_FILTER:
			ret =PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE;
			msg = "PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE";
			break;
		case  MM_ERROR_PLAYER_NO_FREE_SPACE:
			ret = PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE;
			msg = "PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE";
			break;
		case MM_ERROR_PLAYER_FILE_NOT_FOUND:
			ret = PLAYER_ERROR_NO_SUCH_FILE;
			msg = "PLAYER_ERROR_NO_SUCH_FILE";
			break;
		case MM_ERROR_PLAYER_SEEK:
			ret = PLAYER_ERROR_SEEK_FAILED;
			msg = "PLAYER_ERROR_SEEK_FAILED";
			break;
		case MM_ERROR_PLAYER_INVALID_URI:
		case MM_ERROR_PLAYER_STREAMING_INVALID_URL:
			ret = PLAYER_ERROR_INVALID_URI;
			msg = "PLAYER_ERROR_INVALID_URI";
			break;
		case MM_ERROR_PLAYER_STREAMING_CONNECTION_FAIL:
		case MM_ERROR_PLAYER_STREAMING_DNS_FAIL	:
		case MM_ERROR_PLAYER_STREAMING_SERVER_DISCONNECTED:
		case MM_ERROR_PLAYER_STREAMING_INVALID_PROTOCOL:
		case MM_ERROR_PLAYER_STREAMING_UNEXPECTED_MSG:
		case MM_ERROR_PLAYER_STREAMING_OUT_OF_MEMORIES:
		case MM_ERROR_PLAYER_STREAMING_RTSP_TIMEOUT:
		case MM_ERROR_PLAYER_STREAMING_BAD_REQUEST:
		case MM_ERROR_PLAYER_STREAMING_NOT_AUTHORIZED:
		case MM_ERROR_PLAYER_STREAMING_PAYMENT_REQUIRED:
		case MM_ERROR_PLAYER_STREAMING_FORBIDDEN:
		case MM_ERROR_PLAYER_STREAMING_CONTENT_NOT_FOUND:
		case MM_ERROR_PLAYER_STREAMING_METHOD_NOT_ALLOWED:
		case MM_ERROR_PLAYER_STREAMING_NOT_ACCEPTABLE:
		case MM_ERROR_PLAYER_STREAMING_PROXY_AUTHENTICATION_REQUIRED:
		case MM_ERROR_PLAYER_STREAMING_SERVER_TIMEOUT:
		case MM_ERROR_PLAYER_STREAMING_GONE:
		case MM_ERROR_PLAYER_STREAMING_LENGTH_REQUIRED:
		case MM_ERROR_PLAYER_STREAMING_PRECONDITION_FAILED:
		case MM_ERROR_PLAYER_STREAMING_REQUEST_ENTITY_TOO_LARGE:
		case MM_ERROR_PLAYER_STREAMING_REQUEST_URI_TOO_LARGE:
		case MM_ERROR_PLAYER_STREAMING_PARAMETER_NOT_UNDERSTOOD:
		case MM_ERROR_PLAYER_STREAMING_CONFERENCE_NOT_FOUND:
		case MM_ERROR_PLAYER_STREAMING_NOT_ENOUGH_BANDWIDTH:
		case MM_ERROR_PLAYER_STREAMING_NO_SESSION_ID:
		case MM_ERROR_PLAYER_STREAMING_METHOD_NOT_VALID_IN_THIS_STATE:
		case MM_ERROR_PLAYER_STREAMING_HEADER_FIELD_NOT_VALID_FOR_SOURCE:
		case MM_ERROR_PLAYER_STREAMING_INVALID_RANGE:
		case MM_ERROR_PLAYER_STREAMING_PARAMETER_IS_READONLY:
		case MM_ERROR_PLAYER_STREAMING_AGGREGATE_OP_NOT_ALLOWED:
		case MM_ERROR_PLAYER_STREAMING_ONLY_AGGREGATE_OP_ALLOWED:
		case MM_ERROR_PLAYER_STREAMING_BAD_TRANSPORT:
		case MM_ERROR_PLAYER_STREAMING_DESTINATION_UNREACHABLE:
		case MM_ERROR_PLAYER_STREAMING_INTERNAL_SERVER_ERROR:
		case MM_ERROR_PLAYER_STREAMING_NOT_IMPLEMENTED:
		case MM_ERROR_PLAYER_STREAMING_BAD_GATEWAY:
		case MM_ERROR_PLAYER_STREAMING_SERVICE_UNAVAILABLE:
		case MM_ERROR_PLAYER_STREAMING_GATEWAY_TIME_OUT:
		case MM_ERROR_PLAYER_STREAMING_OPTION_NOT_SUPPORTED:
			ret = PLAYER_ERROR_CONNECTION_FAILED;
			msg = "PLAYER_ERROR_CONNECTION_FAILED";
			break;
		case MM_ERROR_POLICY_BLOCKED:
		case MM_ERROR_POLICY_INTERRUPTED:
		case MM_ERROR_POLICY_INTERNAL:
		case MM_ERROR_POLICY_DUPLICATED:
			ret = PLAYER_ERROR_SOUND_POLICY;
			msg = "PLAYER_ERROR_SOUND_POLICY";
			break;
		case MM_ERROR_PLAYER_DRM_EXPIRED:
			ret = PLAYER_ERROR_DRM_EXPIRED;
			msg = "PLAYER_ERROR_DRM_EXPIRED";
			break;
		case MM_ERROR_PLAYER_DRM_NOT_AUTHORIZED:
		case MM_ERROR_PLAYER_DRM_NO_LICENSE:
			ret = PLAYER_ERROR_DRM_NO_LICENSE;
			msg = "PLAYER_ERROR_DRM_NO_LICENSE";
			break;
		case MM_ERROR_PLAYER_DRM_FUTURE_USE:
			ret = PLAYER_ERROR_DRM_FUTURE_USE;
			msg = "PLAYER_ERROR_DRM_FUTURE_USE";
			break;
		case MM_ERROR_PLAYER_DRM_OUTPUT_PROTECTION:
			ret = PLAYER_ERROR_DRM_NOT_PERMITTED;
			msg = "PLAYER_ERROR_DRM_NOT_PERMITTED";
			break;
		case MM_ERROR_PLAYER_RESOURCE_LIMIT:
			ret = PLAYER_ERROR_RESOURCE_LIMIT;
			msg = "PLAYER_ERROR_RESOURCE_LIMIT";
			break;
		case MM_ERROR_PLAYER_PERMISSION_DENIED:
			ret = PLAYER_ERROR_PERMISSION_DENIED;
			msg = "PLAYER_ERROR_PERMISSION_DENIED";
	}
	LOGE("[%s] %s(0x%08x) : core fw error(0x%x)",func_name,msg, ret, code);
	return ret;
}

static void * _get_mem(player_cli_s *player, int size)
{
	player_data_s *mem = g_new(player_data_s, sizeof(player_data_s));
	if(mem){
		mem->data = g_new(void, size);
		mem->next = player->head;
		player->head = mem;
		return mem->data;
	}
	return NULL;
}

static void _del_mem(player_cli_s *player)
{
	player_data_s *mem;
	while(player->head){
		mem = player->head->next;
		g_free(player->head->data);
		g_free(player->head);
		player->head = mem;
	}
}

static int player_recv_msg(callback_cb_info_s *cb_info, int len)
{
	int recvLen;
	msg_buff_s *buff = &cb_info->buff;
	char *new;

	if(len && buff->bufLen - MM_MSG_MAX_LENGTH <= len) {
		LOGD("realloc Buffer %d -> %d, Msg Length %d",
				buff->bufLen, buff->bufLen + MM_MSG_MAX_LENGTH, len);
		buff->bufLen += MM_MSG_MAX_LENGTH;
		new = g_renew(char, buff->recvMsg, buff->bufLen);
		if(new && new != buff->recvMsg){
			buff->recvMsg = new;
		}
	}

	recvLen = mmsvc_core_ipc_recv_msg(cb_info->fd, buff->recvMsg + len);
	len += recvLen;

	return len;
}

static int __set_callback(_player_event_e type, player_h player, void *callback,
			  void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	player_cli_s *handle = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = handle->cb_info->fd;
	int set = 1;

	player_msg_send2_async(api, handle->remote_handle, sock_fd,
			INT, type, INT, set);

	handle->cb_info->user_cb[type] = callback;
	handle->cb_info->user_data[type] = user_data;
	LOGI("[%s] Event type : %d ", __FUNCTION__, type);
	return PLAYER_ERROR_NONE;
}

static int __unset_callback(_player_event_e type, player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	player_cli_s *handle = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = handle->cb_info->fd;
	int set = 0;

	player_msg_send2_async(api, handle->remote_handle, sock_fd,
			INT, type, INT, set);

	handle->cb_info->user_cb[type] = NULL;
	handle->cb_info->user_data[type] = NULL;
	LOGI("[%s] Event type : %d ", __FUNCTION__, type);
	return PLAYER_ERROR_NONE;
}

static void set_null_user_cb(callback_cb_info_s *cb_info, _player_event_e event)
{
	if(event < _PLAYER_EVENT_TYPE_NUM){
		cb_info->user_cb[event] = NULL;
		cb_info->user_data[event] = NULL;
	}
}

static void __prepare_cb_handler(callback_cb_info_s * cb_info, char *recvMsg)
{
	char caps[MM_MSG_MAX_LENGTH] = {0};

	if(player_msg_get_string(caps, recvMsg))
		if(strlen(caps) > 0)
			mm_player_mused_realize(cb_info->local_handle, caps);

	((player_prepared_cb)
	 cb_info->user_cb[_PLAYER_EVENT_TYPE_PREPARE])(
		 cb_info->user_data[_PLAYER_EVENT_TYPE_PREPARE]);

	set_null_user_cb(cb_info, _PLAYER_EVENT_TYPE_PREPARE);
}

static void __complete_cb_handler(callback_cb_info_s * cb_info, char *recvMsg)
{
	((player_completed_cb)
	 cb_info->user_cb[_PLAYER_EVENT_TYPE_COMPLETE])(
		 cb_info->user_data[_PLAYER_EVENT_TYPE_COMPLETE]);
}

static void __interrupt_cb_handler(callback_cb_info_s * cb_info, char *recvMsg)
{
	int interrupt;
	if(player_msg_get(interrupt, recvMsg)) {
		player_interrupted_code_e ev = interrupt;
		((player_interrupted_cb)
		 cb_info->user_cb[_PLAYER_EVENT_TYPE_INTERRUPT])(
			ev, cb_info->user_data[_PLAYER_EVENT_TYPE_INTERRUPT]);
	}
}

static void __error_cb_handler(callback_cb_info_s * cb_info, char *recvMsg)
{
	int code;
	if(player_msg_get(code, recvMsg)) {
		((player_error_cb) cb_info->user_cb[_PLAYER_EVENT_TYPE_ERROR])(
			code, cb_info->user_data[_PLAYER_EVENT_TYPE_ERROR]);
	}
}

static void __buffering_cb_handler(callback_cb_info_s * cb_info, char *recvMsg)
{
	int percent;
	if(player_msg_get(percent, recvMsg)) {
		((player_buffering_cb) cb_info->user_cb[_PLAYER_EVENT_TYPE_BUFFERING])(
			percent, cb_info->user_data[_PLAYER_EVENT_TYPE_BUFFERING]);
	}
}

static void __subtitle_cb_handler(callback_cb_info_s * cb_info, char *recvMsg)
{
	int duration = 0;
	char text[MM_URI_MAX_LENGTH];
	if(player_msg_get(duration, recvMsg)
			&& player_msg_get_string(text, recvMsg)) {
		((player_subtitle_updated_cb)
		 cb_info->user_cb[_PLAYER_EVENT_TYPE_SUBTITLE]) (
			duration, text, cb_info->user_data[_PLAYER_EVENT_TYPE_SUBTITLE]);
	}
}

static void __capture_cb_handler(callback_cb_info_s * cb_info, char *recvMsg)
{
	unsigned char *data = NULL;
	int width = 0;
	int height = 0;
	unsigned int size = 0;
	tbm_bo bo;
	tbm_bo_handle thandle;
	tbm_key key;

	if(player_msg_get(width, recvMsg) && player_msg_get(height, recvMsg)
			&& player_msg_get(size, recvMsg)) {
		if(!player_msg_get(key, recvMsg))
			goto capture_event_exit1;

		bo = tbm_bo_import(bufmgr, key);
		if(bo == NULL) {
			LOGE("TBM get error : bo is NULL");
			goto capture_event_exit1;
		}
		thandle = tbm_bo_map (bo, TBM_DEVICE_CPU,
				TBM_OPTION_WRITE | TBM_OPTION_READ);
		if(thandle.ptr == NULL)
		{
			LOGE("TBM get error : handle pointer is NULL");
			goto capture_event_exit2;
		}
		data = g_new(unsigned char, size);
		if(data){
			memcpy(data, thandle.ptr, size);
			((player_video_captured_cb)
			cb_info->user_cb[_PLAYER_EVENT_TYPE_CAPTURE]) (
				data, width, height, size,
				cb_info->user_data[_PLAYER_EVENT_TYPE_CAPTURE]);
			g_free(data);
		}
		else
			LOGE("g_new failure");

		/* mark to read */
		*((char *)thandle.ptr+size) = 0;

		tbm_bo_unmap(bo);
capture_event_exit2:
		tbm_bo_unref(bo);
	}
capture_event_exit1:
	set_null_user_cb(cb_info, _PLAYER_EVENT_TYPE_CAPTURE);
}

static void __seek_cb_handler(callback_cb_info_s * cb_info, char *recvMsg)
{
	((player_seek_completed_cb)cb_info->user_cb[_PLAYER_EVENT_TYPE_SEEK])
		(cb_info->user_data[_PLAYER_EVENT_TYPE_SEEK]);

	set_null_user_cb(cb_info, _PLAYER_EVENT_TYPE_SEEK);
}

static void __media_packet_video_frame_cb_handler(
		callback_cb_info_s * cb_info, char *recvMsg)
{
	tbm_bo bo[4] = {NULL, };
	tbm_key key[4] = {0, };
	tbm_surface_info_s sinfo;
	char *surface_info = (char *)&sinfo;
	media_packet_h pkt = NULL;
	tbm_surface_h tsurf = NULL;
	int bo_num = 0;
	media_format_mimetype_e mimetype = MEDIA_FORMAT_NV12;
	bool make_pkt_fmt = false;
	int ret;
	_media_pkt_fin_data *fin_data;
	int packet;
	int i;

	player_msg_get(key[0], recvMsg);
	player_msg_get(key[1], recvMsg);
	player_msg_get(key[2], recvMsg);
	player_msg_get(key[3], recvMsg);
	player_msg_get(packet, recvMsg);
	player_msg_get(mimetype, recvMsg);
	player_msg_get_array(surface_info, recvMsg);

	LOGD("width %d, height %d", sinfo.width, sinfo.height);

	for(i = 0; i < 4; i++) {
		if(key[i]){
			bo_num++;
			bo[i] = tbm_bo_import(bufmgr, key[i]);
		}
	}

	tsurf = tbm_surface_internal_create_with_bos(&sinfo ,bo, bo_num);
	if (tsurf) {
		/* check media packet format */
		if (cb_info->pkt_fmt) {
			int pkt_fmt_width = 0;
			int pkt_fmt_height = 0;
			media_format_mimetype_e pkt_fmt_mimetype = MEDIA_FORMAT_NV12;

			media_format_get_video_info(cb_info->pkt_fmt, &pkt_fmt_mimetype,
					&pkt_fmt_width, &pkt_fmt_height, NULL, NULL);
			if (pkt_fmt_mimetype != mimetype ||
			    pkt_fmt_width != sinfo.width ||
			    pkt_fmt_height != sinfo.height) {
				LOGW("different format. current 0x%x, %dx%d, new 0x%x, %dx%d",
				     pkt_fmt_mimetype, pkt_fmt_width, pkt_fmt_height, mimetype,
					 sinfo.width, sinfo.height);
				media_format_unref(cb_info->pkt_fmt);
				cb_info->pkt_fmt = NULL;
				make_pkt_fmt = true;
			}
		} else {
			make_pkt_fmt = true;
		}
		/* create packet format */
		if (make_pkt_fmt) {
			LOGI("make new pkt_fmt - mimetype 0x%x, %dx%d", mimetype,
					sinfo.width, sinfo.height);
			ret = media_format_create(&cb_info->pkt_fmt);
			if (ret == MEDIA_FORMAT_ERROR_NONE) {
				ret = media_format_set_video_mime(cb_info->pkt_fmt, mimetype);
				ret |= media_format_set_video_width(cb_info->pkt_fmt, sinfo.width);
				ret |= media_format_set_video_height(cb_info->pkt_fmt, sinfo.height);
				LOGI("media_format_set_video_mime,width,height ret : 0x%x", ret);
			} else {
				LOGE("media_format_create failed");
			}
		}

		fin_data = g_new(_media_pkt_fin_data, 1);
		fin_data->remote_pkt = packet;
		fin_data->cb_info = cb_info;
		ret = media_packet_create_from_tbm_surface(cb_info->pkt_fmt, tsurf,
				(media_packet_finalize_cb)_player_media_packet_finalize,
				(void *)fin_data, &pkt);
		if (ret != MEDIA_PACKET_ERROR_NONE) {
			LOGE("media_packet_create_from_tbm_surface failed");
			tbm_surface_destroy(tsurf);
			tsurf = NULL;
		}
	}
	if (pkt) {
		/* call media packet callback */
		((player_media_packet_video_decoded_cb)
		 cb_info->user_cb[_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME])(
			 pkt, cb_info->user_data[_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME]);
	}
	for(i = 0; i < bo_num; i++) {
		if(bo[i])
			tbm_bo_unref(bo[i]);
	}
}

static void __audio_frame_cb_handler(
		callback_cb_info_s * cb_info, char *recvMsg)
{
}

static void __video_frame_render_error_cb_handler(
		callback_cb_info_s * cb_info, char *recvMsg)
{
}

static void __pd_cb_handler(callback_cb_info_s * cb_info, char *recvMsg)
{
}

static void __supported_audio_effect_cb_handler(
		callback_cb_info_s * cb_info, char *recvMsg)
{
}

static void __supported_audio_effect_freset_cb_handler(
		callback_cb_info_s * cb_info, char *recvMsg)
{
}

static void __missed_plugin_cb_handler(
		callback_cb_info_s * cb_info, char *recvMsg)
{
}

static void __media_stream_video_buffer_cb_handler(
		callback_cb_info_s * cb_info, char *recvMsg)
{
	//player_media_stream_buffer_status_e status;
	int status;

	if(player_msg_get(status, recvMsg)) {
		((player_media_stream_buffer_status_cb)
			cb_info->user_cb[
				_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS])(
			(player_media_stream_buffer_status_e)status, cb_info->user_data[
				_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS]);
	}
}

static void __media_stream_audio_buffer_cb_handler(
		callback_cb_info_s * cb_info, char *recvMsg)
{
	//player_media_stream_buffer_status_e status;
	int status;

	if(player_msg_get(status, recvMsg)) {
		((player_media_stream_buffer_status_cb)
			cb_info->user_cb[
				_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS])(
			(player_media_stream_buffer_status_e)status, cb_info->user_data[
				_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS]);
	}

}


static void __media_stream_video_seek_cb_handler(
		callback_cb_info_s * cb_info, char *recvMsg)
{
	unsigned long long offset;

	if(player_msg_get_type(offset, recvMsg, INT64)) {
		((player_media_stream_seek_cb)
			cb_info->user_cb[
				_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK])(
			offset, cb_info->user_data[
				_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK]);
	}
}


static void __media_stream_audio_seek_cb_handler(
		callback_cb_info_s * cb_info, char *recvMsg)
{
	unsigned long long offset;

	if(player_msg_get_type(offset, recvMsg, INT64)) {
		((player_media_stream_seek_cb)
			cb_info->user_cb[
				_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK])(
			offset, cb_info->user_data[
				_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK]);
	}
}

static void __video_stream_changed_cb_handler(
		callback_cb_info_s * cb_info, char *recvMsg)
{
	int width;
	int height;
	int fps;
	int bit_rate;
	if(player_msg_get(width, recvMsg)
			&& player_msg_get(height, recvMsg)
			&& player_msg_get(fps, recvMsg)
			&& player_msg_get(bit_rate, recvMsg)) {
		((player_video_stream_changed_cb)
			cb_info->user_cb[
				_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED])(
			width, height, fps, bit_rate, cb_info->user_data[
				_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED]);
	}
}

static void (*_user_callbacks[_PLAYER_EVENT_TYPE_NUM])
					(callback_cb_info_s * cb_info, char *recvMsg) = {
	__prepare_cb_handler,	/*_PLAYER_EVENT_TYPE_PREPARE*/
	__complete_cb_handler,	/*_PLAYER_EVENT_TYPE_COMPLETE*/
	__interrupt_cb_handler,	/*_PLAYER_EVENT_TYPE_INTERRUPT*/
	__error_cb_handler,	/*_PLAYER_EVENT_TYPE_ERROR*/
	__buffering_cb_handler,	/*_PLAYER_EVENT_TYPE_BUFFERING*/
	__subtitle_cb_handler,	/*_PLAYER_EVENT_TYPE_SUBTITLE*/
	__capture_cb_handler,	/*_PLAYER_EVENT_TYPE_CAPTURE*/
	__seek_cb_handler,	/*_PLAYER_EVENT_TYPE_SEEK*/
	__media_packet_video_frame_cb_handler,	/*_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME*/
	__audio_frame_cb_handler,	/*_PLAYER_EVENT_TYPE_AUDIO_FRAME*/
	__video_frame_render_error_cb_handler,	/*_PLAYER_EVENT_TYPE_VIDEO_FRAME_RENDER_ERROR*/
	__pd_cb_handler,	/*_PLAYER_EVENT_TYPE_PD*/
	__supported_audio_effect_cb_handler,	/*_PLAYER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT*/
	__supported_audio_effect_freset_cb_handler,	/*_PLAYER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT_PRESET*/
	__missed_plugin_cb_handler,	/*_PLAYER_EVENT_TYPE_MISSED_PLUGIN*/
#ifdef _PLAYER_FOR_PRODUCT
	NULL,	/*_PLAYER_EVENT_TYPE_IMAGE_BUFFER*/
	NULL,	/*_PLAYER_EVENT_TYPE_SELECTED_SUBTITLE_LANGUAGE*/
#endif
	__media_stream_video_buffer_cb_handler,	/*_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS*/
	__media_stream_audio_buffer_cb_handler,	/*_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS*/
	__media_stream_video_seek_cb_handler,	/*_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK*/
	__media_stream_audio_seek_cb_handler,	/*_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK*/
	NULL,	/*_PLAYER_EVENT_TYPE_AUDIO_STREAM_CHANGED*/
	__video_stream_changed_cb_handler,	/*_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED*/
};

static void _player_event_job_function(_player_cb_data *data)
{
	_user_callbacks[data->int_data](data->cb_info, data->buf);

	g_free(data->buf);
	g_free(data);
}

static void * _player_event_queue_loop(void *param)
{
	if(!param) {
		LOGE("NULL parameter");
		return NULL;
	}
	callback_cb_info_s *cb_info = param;
	player_event_queue *ev = &cb_info->event_queue;
	_player_cb_data *event_data;

	g_mutex_lock(&ev->mutex);
	while(ev->running) {
		if(g_queue_is_empty(ev->queue)) {
			g_cond_wait(&ev->cond, &ev->mutex);
			if(!ev->running)
				break;
		}
		while(1) {
			event_data = (_player_cb_data *)g_queue_pop_head(ev->queue);
			g_mutex_unlock(&ev->mutex);
			if(event_data)
				_player_event_job_function(event_data);
			else {
				g_mutex_lock(&ev->mutex);
				break;
			}
			g_mutex_lock(&ev->mutex);
		}
	}
	g_mutex_unlock(&ev->mutex);
	LOGI("Exit event loop");
	return NULL;
}

static gboolean _player_event_queue_new(callback_cb_info_s *cb_info)
{
	g_return_val_if_fail(cb_info, FALSE);
	player_event_queue *ev = &cb_info->event_queue;

	ev->queue = g_queue_new();
	g_return_val_if_fail(ev->queue, FALSE);

	g_mutex_init(&ev->mutex);
	g_cond_init(&ev->cond);
	ev->running = TRUE;
	ev->thread =
		g_thread_new("cb_event_thread", _player_event_queue_loop,
			     (gpointer) cb_info);
	g_return_val_if_fail(ev->thread, FALSE);
	LOGI("event queue thread %p", ev->thread);

	return TRUE;

}

static void _player_event_queue_destroy(callback_cb_info_s *cb_info)
{
	g_return_if_fail(cb_info);
	player_event_queue *ev = &cb_info->event_queue;

	LOGI("event queue thread %p", ev->thread);

	g_mutex_lock(&ev->mutex);
	ev->running = FALSE;
	g_cond_broadcast(&ev->cond);
	g_mutex_unlock(&ev->mutex);

	g_thread_join(ev->thread);
	g_thread_unref(ev->thread);

	g_queue_free(ev->queue);
	g_mutex_clear(&ev->mutex);
	g_cond_clear(&ev->cond);

}

static void _player_event_queue_add(player_event_queue *ev, _player_cb_data *data)
{
	g_mutex_lock(&ev->mutex);
	if(ev->running){
		g_queue_push_tail(ev->queue, (gpointer)data);
		g_cond_signal(&ev->cond);
	}
	g_mutex_unlock(&ev->mutex);
}

static void _user_callback_handler(callback_cb_info_s * cb_info,
		_player_event_e event, char *buffer)
{
	LOGD("get event %d", event);

	if(event < _PLAYER_EVENT_TYPE_NUM){
		if(cb_info->user_cb[event] && _user_callbacks[event]){
			_player_cb_data *data = NULL;
			data = g_new( _player_cb_data, 1);
			if(!data) {
				LOGE("fail to alloc mem");
				return;
			}
			data->int_data = (int)event;
			data->cb_info = cb_info;
			data->buf = buffer;
			_player_event_queue_add(&cb_info->event_queue, data);
		}
	}
}

static void _add_ret_msg(mm_player_api_e api, callback_cb_info_s *cb_info,
		int offset, int parse_len)
{
	ret_msg_s *msg = NULL;
	ret_msg_s *last = cb_info->buff.retMsgHead;


	msg = g_new(ret_msg_s, 1);
	if(msg)
	{
		msg->api = api;
		msg->msg = strndup(cb_info->buff.recvMsg + offset, parse_len);
		msg->next = NULL;
		if(last == NULL)
			cb_info->buff.retMsgHead = msg;
		else {
			while(last->next)
				last = last->next;
			last->next = msg;
		}
	}
	else
		LOGE("g_new failure");
}

static ret_msg_s * _get_ret_msg(mm_player_api_e api, callback_cb_info_s *cb_info)
{
	ret_msg_s *msg = cb_info->buff.retMsgHead;
	ret_msg_s *prev = NULL;
	while(msg){
		if(msg->api == api){
			if(!prev) {
				cb_info->buff.retMsgHead = msg->next;
			}
			else {
				prev->next = msg->next;
			}
			return msg;
		}
		prev = msg;
		msg = msg->next;
	}
	return NULL;
}

static void *client_cb_handler(gpointer data)
{
	int api;
	int len = 0;
	int parse_len = 0;
	int offset = 0;
	callback_cb_info_s *cb_info = data;
	char *recvMsg = cb_info->buff.recvMsg;
	mused_msg_parse_err_e err;

	while (g_atomic_int_get(&cb_info->running)) {
		len = 0;
		err = MUSED_MSG_PARSE_ERROR_NONE;
		do {
			len = player_recv_msg(cb_info, len);
			if (len <= 0)
				break;
			recvMsg[len] = '\0';
			parse_len = len;
			offset = 0;
			while(offset < len){
				api = MM_PLAYER_API_MAX;
				if(player_msg_get_error_e(api, recvMsg + offset, parse_len, err)) {
					if(api < MM_PLAYER_API_MAX){
						g_mutex_lock(&cb_info->player_mutex);
						cb_info->buff.recved++;
						_add_ret_msg(api, cb_info, offset, parse_len);
						g_cond_signal(&cb_info->player_cond[api]);
						g_mutex_unlock(&cb_info->player_mutex);
						if (api == MM_PLAYER_API_DESTROY) {
							g_atomic_int_set(&cb_info->running, 0);
						}
					}
					else if(api == MM_PLAYER_CB_EVENT) {
						int event;
						char *buffer;
						g_mutex_lock(&cb_info->player_mutex);
						buffer = strndup(recvMsg + offset, parse_len);
						g_mutex_unlock(&cb_info->player_mutex);
						if(player_msg_get(event, buffer)) {
							_user_callback_handler(cb_info, event, buffer);
						}
					}
				}
				if(parse_len == 0) break;
				offset += parse_len;
				parse_len = len - parse_len;
			}
		}while(err == MUSED_MSG_PARSE_ERROR_CONTINUE);
		if (len <= 0)
			break;
	}
	LOGD("client cb exit");

	return NULL;
}

static callback_cb_info_s *callback_new(gint sockfd)
{
	callback_cb_info_s *cb_info;
	msg_buff_s *buff;
	int i;

	g_return_val_if_fail(sockfd > 0, NULL);

	cb_info = g_new(callback_cb_info_s, 1);
	memset(cb_info, 0, sizeof(callback_cb_info_s));

	g_mutex_init(&cb_info->player_mutex);
	for(i=0; i<MM_PLAYER_API_MAX; i++)
		g_cond_init(&cb_info->player_cond[i]);

	buff = &cb_info->buff;
	buff->recvMsg = g_new(char, MM_MSG_MAX_LENGTH+1);
	buff->bufLen = MM_MSG_MAX_LENGTH+1;
	buff->recved = 0;
	buff->retMsgHead = NULL;

	g_atomic_int_set(&cb_info->running, 1);
	cb_info->fd = sockfd;
	cb_info->thread =
		g_thread_new("callback_thread", client_cb_handler,
			     (gpointer) cb_info);

	return cb_info;
}

static void callback_destroy(callback_cb_info_s * cb_info)
{
	int i;
	g_return_if_fail(cb_info);

	mmsvc_core_connection_close(cb_info->fd);

	g_thread_join(cb_info->thread);
	g_thread_unref(cb_info->thread);

	LOGI("%p Callback destroyed", cb_info->thread);

	g_mutex_clear(&cb_info->player_mutex);
	for(i=0; i<MM_PLAYER_API_MAX; i++)
		g_cond_clear(&cb_info->player_cond[i]);

	g_free(cb_info->buff.recvMsg);
	g_free(cb_info);
}

static int wait_for_cb_return(mm_player_api_e api, callback_cb_info_s *cb_info,
		char **ret_buf, int time_out)
{
	int ret = PLAYER_ERROR_NONE;
	gint64 end_time = g_get_monotonic_time() + time_out * G_TIME_SPAN_SECOND;
	msg_buff_s *buff = &cb_info->buff;
	ret_msg_s *msg = NULL;

	g_mutex_lock(&cb_info->player_mutex);

	msg = _get_ret_msg(api, cb_info);
	if(!buff->recved || !msg) {
		if (!g_cond_wait_until(&cb_info->player_cond[api], &cb_info->player_mutex, end_time)) {
			g_mutex_unlock(&cb_info->player_mutex);
			return PLAYER_ERROR_INVALID_OPERATION;
		}
	}
	if(!msg)
		msg = _get_ret_msg(api, cb_info);
	if(msg) {
		*ret_buf = msg->msg;
		g_free(msg);
		if(!player_msg_get(ret, *ret_buf))
			ret = PLAYER_ERROR_INVALID_OPERATION;
	}
	else {
		LOGE("api %d return msg is not exist", api);
		ret = PLAYER_ERROR_INVALID_OPERATION;
	}
	buff->recved--;

	g_mutex_unlock(&cb_info->player_mutex);

	return ret;
}

/*
* Public Implementation
*/

int player_create(player_h * player)
{
	PLAYER_INSTANCE_CHECK(player);

	int ret = PLAYER_ERROR_NONE;
	int sock_fd = -1;

	mm_player_api_e api = MM_PLAYER_API_CREATE;
	mmsvc_api_client_e client = MMSVC_PLAYER;
	player_cli_s *pc = NULL;
	char *ret_buf = NULL;

	LOGD("ENTER");

	sock_fd = mmsvc_core_client_new();
	if(sock_fd < 0){
		LOGE("connection failure %d", errno);
		ret = PLAYER_ERROR_INVALID_OPERATION;
		goto ErrorExit;
	}
	player_msg_send1_async(api, (intptr_t)player, sock_fd, INT, client);

	pc = g_new0(player_cli_s, 1);
	if (pc == NULL) {
		ret = PLAYER_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	pc->cb_info = callback_new(sock_fd);
	if(!pc->cb_info) {
		LOGE("fail to create callback");
		ret = PLAYER_ERROR_INVALID_OPERATION;
		goto ErrorExit;
	}
	if(!_player_event_queue_new(pc->cb_info)) {
		LOGE("fail to create event queue");
		ret = PLAYER_ERROR_INVALID_OPERATION;
		goto ErrorExit;
	}

	ret = wait_for_cb_return(api, pc->cb_info, &ret_buf, CALLBACK_TIME_OUT);
	if (ret == PLAYER_ERROR_NONE) {
		intptr_t handle;
		intptr_t client_addr;
		char stream_path[MM_MSG_MAX_LENGTH] = {0,};
		if(player_msg_get_type(handle, ret_buf, POINTER)) {
			EXT_HANDLE(pc) = handle;
			LOGD("Player create 0x%x", EXT_HANDLE(pc));
			*player = (player_h) pc;
		}
		if(player_msg_get_type(client_addr, ret_buf, POINTER)) {
			pc->cb_info->data_fd = mmsvc_core_client_new_data_ch();
			mmsvc_core_send_client_addr(client_addr, pc->cb_info->data_fd);
			LOGD("Data channel fd %d, server side client info addr %p",
					pc->cb_info->data_fd, client_addr);
		}

		if(mm_player_mused_create(&INT_HANDLE(pc)) != MM_ERROR_NONE) {
			LOGE("create failure");
			ret = PLAYER_ERROR_INVALID_OPERATION;
			goto ErrorExit;
		}
		if(player_msg_get_string(stream_path, ret_buf)) {
			LOGD("shmsrc stream path : %s", stream_path);
			if(mm_player_set_shm_stream_path(INT_HANDLE(pc), stream_path)
					!= MM_ERROR_NONE)
				goto ErrorExit;
		}
	} else
		goto ErrorExit;

	bufmgr = tbm_bufmgr_init (-1);

	g_free(ret_buf);
	return ret;

ErrorExit:
	if(pc && pc->cb_info) {
		if(pc->cb_info->event_queue.running)
			_player_event_queue_destroy(pc->cb_info);
		callback_destroy(pc->cb_info);
		pc->cb_info = NULL;
	}
	g_free(pc);
	g_free(ret_buf);
	LOGD("ret value : %d", ret);
	return ret;
}

int player_destroy(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);

	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_DESTROY;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);

	if(mm_player_mused_destroy(INT_HANDLE(pc)) != MM_ERROR_NONE)
		ret = PLAYER_ERROR_INVALID_OPERATION;

	_player_event_queue_destroy(pc->cb_info);
	callback_destroy(pc->cb_info);

	tbm_bufmgr_deinit (bufmgr);

	g_free(pc);
	pc = NULL;

	g_free(ret_buf);
	return ret;
}

int player_prepare_async(player_h player, player_prepared_cb callback,
			 void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_PREPARE_ASYNC;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd;
	char *ret_buf = NULL;

	LOGD("ENTER");
	sock_fd = pc->cb_info->fd;

	if (pc->cb_info->user_cb[_PLAYER_EVENT_TYPE_PREPARE]) {
		LOGE("[%s] PLAYER_ERROR_INVALID_OPERATION (0x%08x) : preparing... we can't do any more ", __FUNCTION__, PLAYER_ERROR_INVALID_OPERATION);
		return PLAYER_ERROR_INVALID_OPERATION;
	} else {
		pc->cb_info->user_cb[_PLAYER_EVENT_TYPE_PREPARE] = callback;
		pc->cb_info->user_data[_PLAYER_EVENT_TYPE_PREPARE] = user_data;
	}
	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);

	g_free(ret_buf);
	return ret;
}

int player_prepare(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_PREPARE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	char caps[MM_MSG_MAX_LENGTH] = {0};

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);

	if(ret == PLAYER_ERROR_NONE) {
		player_msg_get_string(caps, ret_buf);
		if(strlen(caps) > 0 &&
				mm_player_mused_realize(INT_HANDLE(pc), caps) != MM_ERROR_NONE)
			ret = PLAYER_ERROR_INVALID_OPERATION;
	}

	g_free(ret_buf);
	return ret;
}

int player_unprepare(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_UNPREPARE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	mm_player_mused_pre_unrealize(INT_HANDLE(pc));

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		set_null_user_cb(pc->cb_info, _PLAYER_EVENT_TYPE_SEEK);
		set_null_user_cb(pc->cb_info, _PLAYER_EVENT_TYPE_PREPARE);
		_del_mem(pc);
		_player_deinit_memory_buffer(pc);
	}

	if(mm_player_mused_unrealize(INT_HANDLE(pc)) != MM_ERROR_NONE)
		ret = PLAYER_ERROR_INVALID_OPERATION;

	g_free(ret_buf);
	return ret;
}

int player_set_uri(player_h player, const char *uri)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(uri);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_URI;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			STRING, uri);

	g_free(ret_buf);
	return ret;
}

int player_set_memory_buffer(player_h player, const void *data, int size)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(data);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_MEMORY_BUFFER;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	tbm_bo bo;
	tbm_bo_handle thandle;
	tbm_key key;

	if(pc->server_tbm.bo){
		LOGE("Already set the memory buffer. unprepare please");
		return PLAYER_ERROR_INVALID_OPERATION;
	}

	bo = tbm_bo_alloc (bufmgr, size, TBM_BO_DEFAULT);
	if(bo == NULL) {
		LOGE("TBM get error : bo is NULL");
		return PLAYER_ERROR_INVALID_OPERATION;
	}
	thandle = tbm_bo_map (bo, TBM_DEVICE_CPU, TBM_OPTION_WRITE);
	if(thandle.ptr == NULL)
	{
		LOGE("TBM get error : handle pointer is NULL");
		ret = PLAYER_ERROR_INVALID_OPERATION;
		goto set_memory_exit;
	}
	memcpy(thandle.ptr, data, size);
	tbm_bo_unmap(bo);

	key = tbm_bo_export(bo);
	if(key == 0) {
		LOGE("TBM get error : key is 0");
		ret = PLAYER_ERROR_INVALID_OPERATION;
		goto set_memory_exit;
	}

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, key, INT, size);

set_memory_exit:
	tbm_bo_unref(bo);

	if(ret == PLAYER_ERROR_NONE) {
		intptr_t bo_addr = 0;
		if(player_msg_get_type(bo_addr, ret_buf, POINTER))
			pc->server_tbm.bo = (intptr_t)bo_addr;
	}

	g_free(ret_buf);
	return ret;
}

static int _player_deinit_memory_buffer(player_cli_s *pc)
{
	PLAYER_INSTANCE_CHECK(pc);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_DEINIT_MEMORY_BUFFER;
	int sock_fd = pc->cb_info->fd;
	intptr_t bo_addr = pc->server_tbm.bo;

	if(!bo_addr)
		return ret;

	player_msg_send1_async(api, EXT_HANDLE(pc), sock_fd,
			POINTER, bo_addr);
	pc->server_tbm.bo = 0;

	return ret;
}

int player_get_state(player_h player, player_state_e * pstate)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pstate);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_STATE;
	player_cli_s *pc = (player_cli_s *) player;
	int state;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);

	if (ret == PLAYER_ERROR_NONE) {
		player_msg_get(state, ret_buf);
		*pstate = state;
	}

	g_free(ret_buf);
	return ret;
}

int player_set_volume(player_h player, float left, float right)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_CHECK_CONDITION(left>=0 && left <= 1.0 ,PLAYER_ERROR_INVALID_PARAMETER,"PLAYER_ERROR_INVALID_PARAMETER" );
	PLAYER_CHECK_CONDITION(right>=0 && right <= 1.0 ,PLAYER_ERROR_INVALID_PARAMETER, "PLAYER_ERROR_INVALID_PARAMETER" );
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_VOLUME;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			DOUBLE, right, DOUBLE, left);
	g_free(ret_buf);
	return ret;
}

int player_get_volume(player_h player, float *pleft, float *pright)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pleft);
	PLAYER_NULL_ARG_CHECK(pright);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_VOLUME;
	player_cli_s *pc = (player_cli_s *) player;
	double left = -1;
	double right = -1;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);

	if(ret == PLAYER_ERROR_NONE) {
		player_msg_get(left, ret_buf);
		player_msg_get(right, ret_buf);
		*pleft = (float)left;
		*pright = (float)right;
	}

	g_free(ret_buf);
	return ret;
}

int player_set_sound_type(player_h player, sound_type_e type)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_SOUND_TYPE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type);
	g_free(ret_buf);
	return ret;
}

int player_set_audio_policy_info(player_h player, sound_stream_info_h stream_info)
{
	PLAYER_INSTANCE_CHECK(player);

	mm_player_api_e api = MM_PLAYER_API_SET_AUDIO_POLICY_INFO;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	bool is_available = false;

	/* check if stream_info is valid */
	int ret = __player_convert_error_code(
			sound_manager_is_available_stream_information(stream_info, NATIVE_API_PLAYER, &is_available)
			, (char*)__FUNCTION__);

	if(ret == PLAYER_ERROR_NONE)
	{
		if(is_available == false)
			ret = PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE;
		else
		{
			char *stream_type = NULL;
			int stream_index = 0;
			ret = sound_manager_get_type_from_stream_information(stream_info, &stream_type);
			ret = sound_manager_get_index_from_stream_information(stream_info, &stream_index);
			if (ret == SOUND_MANAGER_ERROR_NONE)
			{
				player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
						STRING, stream_type, INT, stream_index);
			}
			else
				ret = PLAYER_ERROR_INVALID_OPERATION;
		}
	}

	g_free(ret_buf);
	return ret;

}

int player_set_audio_latency_mode(player_h player,
				  audio_latency_mode_e latency_mode)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_AUDIO_LATENCY_MODE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, latency_mode);
	g_free(ret_buf);
	return ret;
}

int player_get_audio_latency_mode(player_h player,
				  audio_latency_mode_e * platency_mode)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(platency_mode);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_AUDIO_LATENCY_MODE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int latency_mode = -1;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);

	if (ret == PLAYER_ERROR_NONE) {
		player_msg_get(latency_mode, ret_buf);
		*platency_mode = latency_mode;
	}

	g_free(ret_buf);
	return ret;

}

int player_start(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_START;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	sock_fd = pc->cb_info->fd;

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);

	g_free(ret_buf);
	return ret;
}

int player_stop(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_STOP;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE) {
		set_null_user_cb(pc->cb_info, _PLAYER_EVENT_TYPE_SEEK);
	}

	g_free(ret_buf);
	return ret;
}

int player_pause(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_PAUSE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	sock_fd = pc->cb_info->fd;

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	g_free(ret_buf);
	return ret;
}

int player_set_play_position(player_h player, int millisecond, bool accurate,
			     player_seek_completed_cb callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_CHECK_CONDITION(millisecond >= 0, PLAYER_ERROR_INVALID_PARAMETER,
			       "PLAYER_ERROR_INVALID_PARAMETER");

	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_PLAY_POSITION;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int pos = millisecond;

	LOGD("ENTER");

	if (pc->cb_info->user_cb[_PLAYER_EVENT_TYPE_SEEK]) {
		LOGE("[%s] PLAYER_ERROR_SEEK_FAILED (0x%08x) : seeking... we can't do any more ", __FUNCTION__, PLAYER_ERROR_SEEK_FAILED);
		return PLAYER_ERROR_SEEK_FAILED;
	} else {
		LOGI("[%s] Event type : %d, pos : %d ", __FUNCTION__,
		     _PLAYER_EVENT_TYPE_SEEK, millisecond);
		pc->cb_info->user_cb[_PLAYER_EVENT_TYPE_SEEK] = callback;
		pc->cb_info->user_data[_PLAYER_EVENT_TYPE_SEEK] = user_data;
	}

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, pos, INT, accurate);

	if (ret != PLAYER_ERROR_NONE) {
		pc->cb_info->user_cb[_PLAYER_EVENT_TYPE_SEEK] = NULL;
		pc->cb_info->user_data[_PLAYER_EVENT_TYPE_SEEK] = NULL;
	}

	g_free(ret_buf);
	return ret;

}

int player_get_play_position(player_h player, int *millisecond)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(millisecond);

	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_PLAY_POSITION;
	player_cli_s *pc = (player_cli_s *) player;
	int pos;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);

	if (ret == PLAYER_ERROR_NONE) {
		player_msg_get(pos, ret_buf);
		*millisecond = pos;
	}

	g_free(ret_buf);
	return ret;
}

int player_set_mute(player_h player, bool muted)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_MUTE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int mute = (int)muted;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, mute);
	g_free(ret_buf);
	return ret;
}

int player_is_muted(player_h player, bool * muted)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(muted);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_IS_MUTED;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int mute = -1;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(mute, ret_buf);
		*muted = (bool)mute;
	}

	g_free(ret_buf);
	return ret;
}

int player_set_looping(player_h player, bool looping)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_LOOPING;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, looping);
	g_free(ret_buf);
	return ret;
}

int player_is_looping(player_h player, bool * plooping)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(plooping);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_IS_LOOPING;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int looping = 0;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(looping, ret_buf);
		*plooping = looping;
	}
	g_free(ret_buf);
	return ret;
}

int player_get_duration(player_h player, int *pduration)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pduration);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_DURATION;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int duration = 0;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(duration, ret_buf);
		*pduration = duration;
	}

	g_free(ret_buf);
	return ret;
}

int player_set_display(player_h player, player_display_type_e type,
		       player_display_h display)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_DISPLAY;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	Evas_Object *obj = NULL;
	const char *object_type = NULL;
#ifdef HAVE_WAYLAND
	void *set_handle = NULL;
	void *set_wl_display = NULL;
	Ecore_Wl_Window * wl_window = NULL;
	wl_win_msg_type wl_win;
	char *wl_win_msg = (char *)&wl_win;
#else
	unsigned int xhandle = 0;
#endif

	LOGD("ENTER");

	if(type != PLAYER_DISPLAY_TYPE_NONE) {
		obj = (Evas_Object *) display;
		if(!obj)
			return PLAYER_ERROR_INVALID_PARAMETER;

		object_type = evas_object_type_get(obj);
		if (object_type) {
			if (type == PLAYER_DISPLAY_TYPE_OVERLAY
				&& !strcmp(object_type, "elm_win")) {
#ifdef HAVE_WAYLAND
				/* wayland overlay surface*/
				LOGI("Wayland overlay surface type");
				wl_win.type = type;

				evas_object_geometry_get(obj, &wl_win.wl_window_x, &wl_win.wl_window_y,
				                         &wl_win.wl_window_width, &wl_win.wl_window_height);

				wl_window = elm_win_wl_window_get(obj);
				set_handle = (void *)ecore_wl_window_surface_get(wl_window);

				/* get wl_display */
				set_wl_display = (void *)ecore_wl_display_get();

				LOGI("xid %d, surface_id %d, surface %p(%d), win_id %d", elm_win_xwindow_get(obj),
						ecore_wl_window_surface_id_get(wl_window),
						ecore_wl_window_surface_get(wl_window), *(int *)ecore_wl_window_surface_get(wl_window),
						ecore_wl_window_id_get(wl_window));
#else
				/* x window overlay surface */
				LOGI("overlay surface type");
				xhandle = elm_win_xwindow_get(obj);
#endif
			}
			else
				return PLAYER_ERROR_INVALID_PARAMETER;
		}
		else
			return PLAYER_ERROR_INVALID_PARAMETER;
	}
#ifdef HAVE_WAYLAND
	else {
		LOGI("Wayland surface type is NONE");
		wl_win.type = type;
		wl_win.wl_window_x = 0;
		wl_win.wl_window_y = 0;
		wl_win.wl_window_width = 0;
		wl_win.wl_window_height = 0;
	}
	player_msg_send_array(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			wl_win_msg, sizeof(wl_win_msg_type), sizeof(char));

	ret = mm_player_set_attribute(INT_HANDLE(pc), NULL,
		"display_surface_type", type,
		"wl_display", set_wl_display,
		sizeof(void*),
		"display_overlay", set_handle,
		sizeof(display), (char*)NULL);
	if (ret != MM_ERROR_NONE)
		LOGE("Failed to display surface change :%d", ret);

	ret = mm_player_set_attribute(INT_HANDLE(pc), NULL,
			"wl_window_render_x", wl_win.wl_window_x,
			"wl_window_render_y", wl_win.wl_window_y,
			"wl_window_render_width", wl_win.wl_window_width,
			"wl_window_render_height", wl_win.wl_window_height,
			(char*)NULL);

	if (ret != MM_ERROR_NONE)
		LOGE("Failed to set wl_window render rectangle :%d", ret);

#else
	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, xhandle);
#endif

	g_free(ret_buf);
	return ret;
}

int player_set_display_mode(player_h player, player_display_mode_e mode)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_DISPLAY_MODE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, mode);
	g_free(ret_buf);
	return ret;
}

int player_get_display_mode(player_h player, player_display_mode_e * pmode)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pmode);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_DISPLAY_MODE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int mode = -1;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(mode, ret_buf);
		*pmode = mode;
	}
	g_free(ret_buf);
	return ret;
}

int player_set_playback_rate(player_h player, float rate)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_CHECK_CONDITION(rate>=-5.0 && rate <= 5.0 ,PLAYER_ERROR_INVALID_PARAMETER,"PLAYER_ERROR_INVALID_PARAMETER" );
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_PLAYBACK_RATE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			DOUBLE, rate);
	g_free(ret_buf);
	return ret;
}

int player_set_display_rotation(player_h player,
				player_display_rotation_e rotation)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_DISPLAY_ROTATION;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, rotation);
	g_free(ret_buf);
	return ret;
}

int player_get_display_rotation(player_h player,
				player_display_rotation_e * protation)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(protation);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_DISPLAY_ROTATION;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int rotation = -1;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(rotation, ret_buf);
		*protation = rotation;
	}
	g_free(ret_buf);
	return ret;
}

int player_set_display_visible(player_h player, bool visible)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_DISPLAY_VISIBLE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, visible);
	g_free(ret_buf);
	return ret;
}

int player_is_display_visible(player_h player, bool * pvisible)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pvisible);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_IS_DISPLAY_VISIBLE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int visible = -1;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(visible, ret_buf);
		*pvisible = visible;
	}
	g_free(ret_buf);
	return ret;
}

int player_get_content_info(player_h player, player_content_info_e key,
			    char **pvalue)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pvalue);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_CONTENT_INFO;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	char value[MM_MSG_MAX_LENGTH] = {0,};

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, key);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get_string(value, ret_buf);
		*pvalue = strndup(value, MM_MSG_MAX_LENGTH);
	}
	g_free(ret_buf);
	return ret;
}

int player_get_codec_info(player_h player, char **paudio_codec,
			  char **pvideo_codec)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(paudio_codec);
	PLAYER_NULL_ARG_CHECK(pvideo_codec);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_CODEC_INFO;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	char video_codec[MM_MSG_MAX_LENGTH] = {0,};
	char audio_codec[MM_MSG_MAX_LENGTH] = {0,};

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get_string(video_codec, ret_buf);
		player_msg_get_string(audio_codec, ret_buf);
		*pvideo_codec = strndup(video_codec, MM_MSG_MAX_LENGTH);
		*paudio_codec = strndup(audio_codec, MM_MSG_MAX_LENGTH);
	}
	g_free(ret_buf);
	return ret;
}

int player_get_audio_stream_info(player_h player, int *psample_rate,
				 int *pchannel, int *pbit_rate)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(psample_rate);
	PLAYER_NULL_ARG_CHECK(pchannel);
	PLAYER_NULL_ARG_CHECK(pbit_rate);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_AUDIO_STREAM_INFO;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int sample_rate = 0;
	int channel = 0;
	int bit_rate = 0;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(sample_rate, ret_buf);
		player_msg_get(channel, ret_buf);
		player_msg_get(bit_rate, ret_buf);
		*psample_rate = sample_rate;
		*pchannel = channel;
		*pbit_rate = bit_rate;
	}
	g_free(ret_buf);
	return ret;
}

int player_get_video_stream_info(player_h player, int *pfps, int *pbit_rate)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pfps);
	PLAYER_NULL_ARG_CHECK(pbit_rate);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_VIDEO_STREAM_INFO;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int fps = 0;
	int bit_rate = 0;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(fps, ret_buf);
		player_msg_get(bit_rate, ret_buf);
		*pfps = fps;
		*pbit_rate = bit_rate;
	}
	g_free(ret_buf);
	return ret;
}

int player_get_video_size(player_h player, int *pwidth, int *pheight)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pwidth);
	PLAYER_NULL_ARG_CHECK(pheight);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_VIDEO_SIZE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int width = 0;
	int height = 0;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(width, ret_buf);
		player_msg_get(height, ret_buf);
		*pwidth = width;
		*pheight = height;
	}
	g_free(ret_buf);
	return ret;
}

int player_get_album_art(player_h player, void **palbum_art, int *psize)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(palbum_art);
	PLAYER_NULL_ARG_CHECK(psize);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_ALBUM_ART;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	char *album_art;
	int size = 0;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(size, ret_buf);
		if(size > 0){
			album_art = _get_mem(pc, size);
			player_msg_get_array(album_art, ret_buf);
			*palbum_art = album_art;
		}
		else
			*palbum_art = NULL;

		*psize = size;
	}
	g_free(ret_buf);
	return ret;
}

int player_audio_effect_get_equalizer_bands_count(player_h player, int *pcount)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pcount);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BANDS_COUNT;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int count;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(count, ret_buf);
		*pcount = count;
	}
	g_free(ret_buf);
	return ret;
}

int player_audio_effect_set_equalizer_all_bands(player_h player,
						int *band_levels, int length)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(band_levels);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_ALL_BANDS;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send_array(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			band_levels, length, sizeof(int));

	g_free(ret_buf);
	return ret;

}

int player_audio_effect_set_equalizer_band_level(player_h player, int index,
						 int level)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_BAND_LEVEL;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, index, INT, level);

	g_free(ret_buf);
	return ret;
}

int player_audio_effect_get_equalizer_band_level(player_h player, int index,
						 int *plevel)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(plevel);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_LEVEL;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int level;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, index);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(level, ret_buf);
		*plevel = level;
	}
	g_free(ret_buf);
	return ret;
}

int player_audio_effect_get_equalizer_level_range(player_h player, int *pmin,
						  int *pmax)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pmin);
	PLAYER_NULL_ARG_CHECK(pmax);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_LEVEL_RANGE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int min,max;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(min, ret_buf);
		player_msg_get(max, ret_buf);
		*pmin = min;
		*pmax = max;
	}
	g_free(ret_buf);
	return ret;
}

int player_audio_effect_get_equalizer_band_frequency(player_h player, int index,
						     int *pfrequency)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pfrequency);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int frequency;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, index);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(frequency, ret_buf);
		*pfrequency = frequency;
	}
	g_free(ret_buf);
	return ret;
}

int player_audio_effect_get_equalizer_band_frequency_range(player_h player,
							   int index,
							   int *prange)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(prange);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY_RANGE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int range;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, index);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(range, ret_buf);
		*prange = range;
	}
	g_free(ret_buf);
	return ret;
}

int player_audio_effect_equalizer_clear(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_EQUALIZER_CLEAR;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	g_free(ret_buf);
	return ret;
}

int player_audio_effect_equalizer_is_available(player_h player,
					       bool * pavailable)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pavailable);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_AUDIO_EFFECT_EQUALIZER_IS_AVAILABLE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int available;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(available, ret_buf);
		*pavailable = available;
	}
	g_free(ret_buf);
	return ret;
}

int player_set_subtitle_path(player_h player, const char *path)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_SUBTITLE_PATH;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			STRING, path);
	g_free(ret_buf);
	return ret;
}

int player_set_subtitle_position_offset(player_h player, int millisecond)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_SUBTITLE_POSITION_OFFSET;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, millisecond);

	g_free(ret_buf);
	return ret;
}

int player_set_progressive_download_path(player_h player, const char *path)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(path);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_PROGRESSIVE_DOWNLOAD_PATH;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			STRING, path);
	g_free(ret_buf);
	return ret;
}

int player_get_progressive_download_status(player_h player,
					   unsigned long *pcurrent,
					   unsigned long *ptotal_size)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pcurrent);
	PLAYER_NULL_ARG_CHECK(ptotal_size);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_PROGRESSIVE_DOWNLOAD_STATUS;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int current, total_size;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(current, ret_buf);
		player_msg_get(total_size, ret_buf);
		*pcurrent = current;
		*ptotal_size = total_size;
	}
	g_free(ret_buf);
	return ret;

}

int player_capture_video(player_h player, player_video_captured_cb callback,
			 void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_CAPTURE_VIDEO;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");
	if (pc->cb_info->user_cb[_PLAYER_EVENT_TYPE_CAPTURE]) {
		LOGE("[%s] PLAYER_ERROR_VIDEO_CAPTURE_FAILED (0x%08x) : capturing... we can't do any more ", __FUNCTION__, PLAYER_ERROR_VIDEO_CAPTURE_FAILED);
		return PLAYER_ERROR_VIDEO_CAPTURE_FAILED;
	} else {
		LOGI("[%s] Event type : %d ", __FUNCTION__,
		     _PLAYER_EVENT_TYPE_CAPTURE);
		pc->cb_info->user_cb[_PLAYER_EVENT_TYPE_CAPTURE] = callback;
		pc->cb_info->user_data[_PLAYER_EVENT_TYPE_CAPTURE] = user_data;
	}

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);

	if (ret != PLAYER_ERROR_NONE) {
		set_null_user_cb(pc->cb_info, _PLAYER_EVENT_TYPE_CAPTURE);
	}

	g_free(ret_buf);
	return ret;
}

int player_set_streaming_cookie(player_h player, const char *cookie, int size)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(cookie);
	PLAYER_CHECK_CONDITION(size>=0,PLAYER_ERROR_INVALID_PARAMETER,"PLAYER_ERROR_INVALID_PARAMETER" );
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_STREAMING_COOKIE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			STRING, cookie, INT, size);
	g_free(ret_buf);
	return ret;
}

int player_set_streaming_user_agent(player_h player, const char *user_agent,
				    int size)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(user_agent);
	PLAYER_CHECK_CONDITION(size>=0,PLAYER_ERROR_INVALID_PARAMETER,"PLAYER_ERROR_INVALID_PARAMETER" );
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_SET_STREAMING_COOKIE;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			STRING, user_agent, INT, size);
	g_free(ret_buf);
	return ret;
}

int player_get_streaming_download_progress(player_h player, int *pstart,
					   int *pcurrent)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pstart);
	PLAYER_NULL_ARG_CHECK(pcurrent);
	int ret = PLAYER_ERROR_NONE;
	mm_player_api_e api = MM_PLAYER_API_GET_STREAMING_DOWNLOAD_PROGRESS;
	player_cli_s *pc = (player_cli_s *) player;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int start, current;

	LOGD("ENTER");

	player_msg_send(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(start, ret_buf);
		player_msg_get(current, ret_buf);
		*pstart = start;
		*pcurrent = current;
	}
	g_free(ret_buf);
	return ret;

}

int player_set_completed_cb(player_h player, player_completed_cb callback,
			    void *user_data)
{
	return __set_callback(_PLAYER_EVENT_TYPE_COMPLETE, player, callback,
			      user_data);
}

int player_unset_completed_cb(player_h player)
{
	return __unset_callback(_PLAYER_EVENT_TYPE_COMPLETE, player);
}

int player_set_interrupted_cb(player_h player, player_interrupted_cb callback,
			      void *user_data)
{
	return __set_callback(_PLAYER_EVENT_TYPE_INTERRUPT, player, callback,
			      user_data);
}

int player_unset_interrupted_cb(player_h player)
{
	return __unset_callback(_PLAYER_EVENT_TYPE_INTERRUPT, player);
}

int player_set_error_cb(player_h player, player_error_cb callback,
			void *user_data)
{
	return __set_callback(_PLAYER_EVENT_TYPE_ERROR, player, callback,
			      user_data);
}

int player_unset_error_cb(player_h player)
{
	return __unset_callback(_PLAYER_EVENT_TYPE_ERROR, player);
}

int player_set_buffering_cb(player_h player, player_buffering_cb callback,
			    void *user_data)
{
	return __set_callback(_PLAYER_EVENT_TYPE_BUFFERING, player, callback,
			      user_data);
}

int player_unset_buffering_cb(player_h player)
{
	return __unset_callback(_PLAYER_EVENT_TYPE_BUFFERING, player);
}

int player_set_subtitle_updated_cb(player_h player,
				   player_subtitle_updated_cb callback,
				   void *user_data)
{
	return __set_callback(_PLAYER_EVENT_TYPE_SUBTITLE, player, callback,
			      user_data);
}

int player_unset_subtitle_updated_cb(player_h player)
{
	return __unset_callback(_PLAYER_EVENT_TYPE_SUBTITLE, player);
}

int player_set_progressive_download_message_cb(player_h player,
					       player_pd_message_cb callback,
					       void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = pc->cb_info->fd;
	int set = 1;
	_player_event_e type = _PLAYER_EVENT_TYPE_PD;

	player_msg_send2_async(api, EXT_HANDLE(pc), sock_fd,
			INT, type, INT, set);

	pc->cb_info->user_cb[_PLAYER_EVENT_TYPE_PD] = callback;
	pc->cb_info->user_data[_PLAYER_EVENT_TYPE_PD] = user_data;
	LOGI("[%s] Event type : %d ", __FUNCTION__, _PLAYER_EVENT_TYPE_PD);
	return PLAYER_ERROR_NONE;
}

int player_unset_progressive_download_message_cb(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = pc->cb_info->fd;
	int set = 0;
	_player_event_e type = _PLAYER_EVENT_TYPE_PD;

	player_msg_send2_async(api, EXT_HANDLE(pc), sock_fd,
			INT, type, INT, set);

	set_null_user_cb(pc->cb_info, type);
	LOGI("[%s] Event type : %d ", __FUNCTION__, _PLAYER_EVENT_TYPE_PD);

	return PLAYER_ERROR_NONE;
}

int player_set_media_packet_video_frame_decoded_cb(player_h player,
						   player_media_packet_video_decoded_cb
						   callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	_player_event_e type = _PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME;
	int set = 1;

	LOGD("ENTER");

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, set);

	if(ret == PLAYER_ERROR_NONE){
		pc->cb_info->user_cb[_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME] =
			callback;
		pc->cb_info->user_data[_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME] =
			user_data;
		LOGI("Event type : %d ", _PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME);
	}

	g_free(ret_buf);
	return ret;
}

int player_unset_media_packet_video_frame_decoded_cb(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	_player_event_e type = _PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME;
	int set = 0;

	LOGD("ENTER");

	set_null_user_cb(pc->cb_info, type);

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, set);

	g_free(ret_buf);
	return ret;
}

int player_set_video_stream_changed_cb (player_h player,
		player_video_stream_changed_cb callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	_player_event_e type = _PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED;
	int set = 1;

	LOGD("ENTER");

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, set);

	if(ret == PLAYER_ERROR_NONE){
		pc->cb_info->user_cb[type] = callback;
		pc->cb_info->user_data[type] = user_data;
		LOGI("Event type : %d ", type);
	}

	g_free(ret_buf);
	return ret;
}

int player_unset_video_stream_changed_cb (player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	_player_event_e type = _PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED;
	int set = 0;

	LOGD("ENTER");

	set_null_user_cb(pc->cb_info, type);

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, set);

	g_free(ret_buf);
	return ret;
}

int player_set_media_stream_buffer_status_cb ( player_h player,
		player_stream_type_e stream_type,
		player_media_stream_buffer_status_cb callback,
		void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	_player_event_e type;
	int set = 1;

	LOGD("ENTER");

	if(stream_type == PLAYER_STREAM_TYPE_VIDEO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS;
	else if(stream_type == PLAYER_STREAM_TYPE_AUDIO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS;
	else {
		LOGE("PLAYER_ERROR_INVALID_PARAMETER(type : %d)", stream_type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, set);

	if(ret == PLAYER_ERROR_NONE){
		pc->cb_info->user_cb[type] = callback;
		pc->cb_info->user_data[type] = user_data;
		LOGI("Event type : %d ", type);
	}

	g_free(ret_buf);
	return ret;
}

int player_unset_media_stream_buffer_status_cb (player_h player,
		player_stream_type_e stream_type)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	_player_event_e type;
	int set = 0;

	LOGD("ENTER");

	if(stream_type == PLAYER_STREAM_TYPE_VIDEO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS;
	else if(stream_type == PLAYER_STREAM_TYPE_AUDIO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS;
	else {
		LOGE("PLAYER_ERROR_INVALID_PARAMETER(type : %d)", stream_type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	set_null_user_cb(pc->cb_info, type);

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, set);

	g_free(ret_buf);
	return ret;
}

int player_set_media_stream_seek_cb (player_h player,
		player_stream_type_e stream_type,
		player_media_stream_seek_cb callback,
		void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	_player_event_e type;
	int set = 1;

	LOGD("ENTER");

	if(stream_type == PLAYER_STREAM_TYPE_VIDEO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK;
	else if(stream_type == PLAYER_STREAM_TYPE_AUDIO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK;
	else {
		LOGE("PLAYER_ERROR_INVALID_PARAMETER(type : %d)", stream_type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, set);

	if(ret == PLAYER_ERROR_NONE){
		pc->cb_info->user_cb[type] = callback;
		pc->cb_info->user_data[type] = user_data;
		LOGI("Event type : %d ", type);
	}

	g_free(ret_buf);
	return ret;
}

int player_unset_media_stream_seek_cb (player_h player,
		player_stream_type_e stream_type)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_CALLBACK;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	_player_event_e type;
	int set = 0;

	LOGD("ENTER");

	if(stream_type == PLAYER_STREAM_TYPE_VIDEO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK;
	else if(stream_type == PLAYER_STREAM_TYPE_AUDIO)
		type = _PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK;
	else {
		LOGE("PLAYER_ERROR_INVALID_PARAMETER(type : %d)", stream_type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	set_null_user_cb(pc->cb_info, type);

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, set);

	g_free(ret_buf);
	return ret;
}

/* TODO Implement raw data socket channel */
int player_push_media_stream(player_h player, media_packet_h packet)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(packet);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_PUSH_MEDIA_STREAM;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	player_push_media_msg_type push_media;
	char *push_media_msg = (char *)&push_media;
	int msg_size = sizeof(player_push_media_msg_type);
#ifdef __UN_USED
	tbm_bo bo = NULL;
	tbm_bo_handle thandle;
	int buf_size;
#endif
	char *buf;
	media_format_h format;
	bool is_video;
	bool is_audio;

	LOGD("ENTER");

	media_packet_get_buffer_data_ptr(packet, (void **)&buf);
	media_packet_get_buffer_size(packet, &push_media.size);
	media_packet_get_pts(packet, &push_media.pts);
	media_packet_get_format(packet, &format);

	push_media.buf_type = PUSH_MEDIA_BUF_TYPE_RAW;

	media_packet_is_video(packet, &is_video);
	media_packet_is_audio(packet, &is_audio);
	if(is_video)
		media_format_get_video_info(format, &push_media.mimetype, NULL, NULL, NULL, NULL);
	else if(is_audio)
		media_format_get_audio_info(format, &push_media.mimetype, NULL, NULL, NULL, NULL);

#ifdef __UN_USED
	if(push_media.buf_type == PUSH_MEDIA_BUF_TYPE_TBM) {
		bo = tbm_bo_alloc (bufmgr, push_media.size, TBM_BO_DEFAULT);
		if(bo == NULL) {
			LOGE("TBM get error : bo is NULL");
			return PLAYER_ERROR_INVALID_OPERATION;
		}
		thandle = tbm_bo_map (bo, TBM_DEVICE_CPU, TBM_OPTION_WRITE);
		if(thandle.ptr == NULL)
		{
			LOGE("TBM get error : handle pointer is NULL");
			ret = PLAYER_ERROR_INVALID_OPERATION;
			goto push_media_error;
		}
		memcpy(thandle.ptr, buf, push_media.size);
		tbm_bo_unmap(bo);

		push_media.key = tbm_bo_export(bo);
		if(push_media.key == 0) {
			LOGE("TBM get error : key is 0");
			ret = PLAYER_ERROR_INVALID_OPERATION;
			goto push_media_error;
		}

		player_msg_send_array(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
				push_media_msg, msg_size, sizeof(char));
	} else if(push_media.buf_type == PUSH_MEDIA_BUF_TYPE_MSG) {
		buf_size = (int)push_media.size;
		player_msg_send_array2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
				push_media_msg, msg_size, sizeof(char),
				buf, buf_size, sizeof(char));
	} else
#endif
	if(push_media.buf_type == PUSH_MEDIA_BUF_TYPE_RAW) {
		mmsvc_core_ipc_push_data(pc->cb_info->data_fd, buf, push_media.size, push_media.pts);
		player_msg_send_array(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
				push_media_msg, msg_size, sizeof(char));
	}

	LOGD("ret_buf %s", ret_buf);

#ifdef __UN_USED
push_media_error:
	if(push_media.buf_type == PUSH_MEDIA_BUF_TYPE_TBM)
		tbm_bo_unref(bo);
#endif

	g_free(ret_buf);
	return ret;
}

int player_set_media_stream_info(player_h player,
		player_stream_type_e type, media_format_h format)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(format);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_MEDIA_STREAM_INFO;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	media_format_mimetype_e mimetype;
	int width;
	int height;
	int avg_bps;
	int max_bps;
	int channel;
	int samplerate;
	int bit;

	LOGD("ENTER");

	media_format_ref(format);
	if(type == PLAYER_STREAM_TYPE_VIDEO) {
		media_format_get_video_info(format, &mimetype, &width, &height, &avg_bps, &max_bps);
		player_msg_send6(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
				INT, type, INT, mimetype, INT, width, INT, height, INT, avg_bps, INT, max_bps);
	} else if(type == PLAYER_STREAM_TYPE_AUDIO) {
		media_format_get_audio_info(format, &mimetype, &channel, &samplerate, &bit, &avg_bps);
		player_msg_send6(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
				INT, type, INT, mimetype, INT, channel, INT, samplerate, INT, avg_bps, INT, bit);
	}
	media_format_unref(format);

	g_free(ret_buf);
	return ret;
}

int player_set_media_stream_buffer_max_size(player_h player,
		player_stream_type_e type, unsigned long long max_size)
{
	int ret = PLAYER_ERROR_NONE;
	PLAYER_INSTANCE_CHECK(player);
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MAX_SIZE;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT64, max_size);

	g_free(ret_buf);
	return ret;
}

int player_get_media_stream_buffer_max_size(player_h player,
		player_stream_type_e type, unsigned long long *pmax_size)
{
	int ret = PLAYER_ERROR_NONE;
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pmax_size);
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MAX_SIZE;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	unsigned long long max_size;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get_type(max_size, ret_buf, INT64);
		*pmax_size = max_size;
	}
	g_free(ret_buf);
	return ret;
}

int player_set_media_stream_buffer_min_threshold(player_h player,
		player_stream_type_e type, unsigned int percent)
{
	int ret = PLAYER_ERROR_NONE;
	PLAYER_INSTANCE_CHECK(player);
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, percent);

	g_free(ret_buf);
	return ret;
}

int player_get_media_stream_buffer_min_threshold(player_h player,
		player_stream_type_e type, unsigned int *ppercent)
{
	int ret = PLAYER_ERROR_NONE;
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(ppercent);
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	uint percent;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(percent, ret_buf);
		*ppercent = percent;
	}

	g_free(ret_buf);
	return ret;
}

int player_get_track_count(player_h player, player_stream_type_e type, int *pcount)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pcount);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_GET_TRACK_COUNT;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int count;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(count, ret_buf);
		*pcount = count;
	}

	g_free(ret_buf);
	return ret;
}

int player_get_current_track(player_h player, player_stream_type_e type, int *pindex)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pindex);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_GET_CURRENT_TRACK;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	int index;

	LOGD("ENTER");

	player_msg_send1(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type);
	if(ret == PLAYER_ERROR_NONE){
		player_msg_get(index, ret_buf);
		*pindex = index;
	}

	g_free(ret_buf);
	return ret;
}

int player_select_track(player_h player, player_stream_type_e type, int index)
{
	PLAYER_INSTANCE_CHECK(player);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_SELECT_TRACK;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, index);

	g_free(ret_buf);
	return ret;
}

int player_get_track_language_code(player_h player, player_stream_type_e type, int index, char **pcode)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(pcode);
	int ret = PLAYER_ERROR_NONE;
	player_cli_s *pc = (player_cli_s *) player;
	mm_player_api_e api = MM_PLAYER_API_GET_TRACK_LANGUAGE_CODE;
	int sock_fd = pc->cb_info->fd;
	char *ret_buf = NULL;
	char code[MM_MSG_MAX_LENGTH] = {0,};
	const int code_len=2;

	LOGD("ENTER");

	player_msg_send2(api, EXT_HANDLE(pc), sock_fd, pc->cb_info, ret_buf, ret,
			INT, type, INT, index);
	if(ret == PLAYER_ERROR_NONE){
		if(player_msg_get_string(code, ret_buf))
			*pcode = strndup(code, code_len);
	}

	g_free(ret_buf);
	return ret;
}

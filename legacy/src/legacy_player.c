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
#include <mm.h>
#include <mm_player.h>
#include <mm_player_audioeffect.h>
#include <mm_player_internal.h>
#include <mm_types.h>
#include <sound_manager.h>
#include <sound_manager_internal.h>
#include <dlog.h>
#include <Evas.h>
#include <Ecore.h>
#include <Elementary.h>
#include <Ecore.h>
#include <Ecore_Wayland.h>
#include <tbm_bufmgr.h>
#include <tbm_surface_internal.h>
#include <mm_sound.h>
#ifdef PLAYER_ASM_COMPATIBILITY
#include <mm_session.h>
#include <mm_session_private.h>
#endif
#include "muse_player.h"
#include "legacy_player.h"
#include "legacy_player_private.h"

#define __JOB_KEY_PREPARED		"prepared"
#define __JOB_KEY_ERROR		"error"
#define __JOB_KEY_SEEK_DONE	"seek_done"
#define __JOB_KEY_EOS			"eos"

#define __RELEASEIF_PREPARE_THREAD(thread_id) \
	do { \
		if (thread_id) { \
			pthread_join(thread_id, NULL); \
			thread_id = 0; \
			LOGI("prepare thread released\n"); \
		} \
	} while (0)

#ifndef USE_ECORE_FUNCTIONS
#define __RELEASEIF_MESSAGE_THREAD(thread_id) \
	do { \
		if (thread_id) { \
			pthread_join(thread_id, NULL); \
			thread_id = 0; \
			LOGI("message thread released\n"); \
		} \
	} while (0)
#endif

#ifdef USE_ECORE_FUNCTIONS
#define __DELETE_ECORE_ITEM(ecore_job) \
	do { \
		if (ecore_job) { \
			ecore_job_del(ecore_job); \
			ecore_job = NULL; \
		} \
	} while (0)

#define	__ADD_ECORE_JOB(handle, key, job_callback)	\
	do {	\
		Ecore_Job *job = NULL; \
		job = ecore_job_add(job_callback, (void *)handle); \
		LOGI("adding %s job - %p\n", key, job); \
		g_hash_table_insert(handle->ecore_jobs, g_strdup(key), job); \
	} while (0)

#define	__REMOVE_ECORE_JOB(handle, job_key) \
	do {	\
		LOGI("%s done so, remove\n", job_key); \
		g_hash_table_remove(handle->ecore_jobs, job_key); \
		handle->is_doing_jobs = FALSE;	\
	} while (0)
#else
#define	__GET_MESSAGE(handle) \
	do { \
		if (handle && handle->message_queue) { \
			g_mutex_lock(&handle->message_queue_lock); \
			if (g_queue_is_empty(handle->message_queue)) { \
				g_cond_wait(&handle->message_queue_cond, &handle->message_queue_lock); \
			} \
			handle->current_message = (int)(intptr_t)g_queue_pop_head(handle->message_queue); \
			g_mutex_unlock(&handle->message_queue_lock); \
			LOGI("Retrieved  message [%d] from queue", handle->current_message); \
		} else { \
			LOGI("Failed to retrive message from queue"); \
			handle->current_message = PLAYER_MESSAGE_NONE; \
		} \
	} while (0)

#define	__ADD_MESSAGE(handle, message) \
	do { \
		if (handle && handle->message_queue) { \
			g_mutex_lock(&handle->message_queue_lock); \
			if (message == PLAYER_MESSAGE_LOOP_EXIT) \
				g_queue_clear(handle->message_queue); \
			g_queue_push_tail(handle->message_queue, (gpointer)message); \
			g_cond_signal(&handle->message_queue_cond); \
			g_mutex_unlock(&handle->message_queue_lock); \
			LOGI("Adding message [%d] to queue", message); \
		} else { \
			LOGI("Failed to add message [%d] to queue", message); \
		} \
	} while (0)
#endif

/*
* Internal Implementation
*/
int __player_convert_error_code(int code, char *func_name)
{
	int ret = PLAYER_ERROR_INVALID_OPERATION;
	char *msg = "PLAYER_ERROR_INVALID_OPERATION";
	switch (code) {
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
		ret = PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE;
		msg = "PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE";
		break;
	case MM_ERROR_PLAYER_NO_FREE_SPACE:
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
	case MM_ERROR_PLAYER_STREAMING_DNS_FAIL:
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
		break;
	case MM_ERROR_PLAYER_BUFFER_SPACE:
		ret = PLAYER_ERROR_BUFFER_SPACE;
		msg = "PLAYER_ERROR_BUFFER_SPACE";
		break;
	default:
		break;
	}
	LOGE("[%s] %s(0x%08x) : core fw error(0x%x)", func_name, msg, ret, code);
	return ret;
}

int _player_get_tbm_surface_format(int in_format, uint32_t *out_format)
{
	if (in_format <= MM_PIXEL_FORMAT_INVALID || in_format >= MM_PIXEL_FORMAT_NUM || out_format == NULL) {
		LOGE("INVALID_PARAMETER : in_format %d, out_format ptr %p", in_format, out_format);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	switch (in_format) {
	case MM_PIXEL_FORMAT_NV12:
	case MM_PIXEL_FORMAT_NV12T:
		*out_format = TBM_FORMAT_NV12;
		break;
	case MM_PIXEL_FORMAT_NV16:
		*out_format = TBM_FORMAT_NV16;
		break;
	case MM_PIXEL_FORMAT_NV21:
		*out_format = TBM_FORMAT_NV21;
		break;
	case MM_PIXEL_FORMAT_YUYV:
		*out_format = TBM_FORMAT_YUYV;
		break;
	case MM_PIXEL_FORMAT_UYVY:
	case MM_PIXEL_FORMAT_ITLV_JPEG_UYVY:
		*out_format = TBM_FORMAT_UYVY;
		break;
	case MM_PIXEL_FORMAT_422P:
		*out_format = TBM_FORMAT_YUV422;
		break;
	case MM_PIXEL_FORMAT_I420:
		*out_format = TBM_FORMAT_YUV420;
		break;
	case MM_PIXEL_FORMAT_YV12:
		*out_format = TBM_FORMAT_YVU420;
		break;
	case MM_PIXEL_FORMAT_RGB565:
		*out_format = TBM_FORMAT_RGB565;
		break;
	case MM_PIXEL_FORMAT_RGB888:
		*out_format = TBM_FORMAT_RGB888;
		break;
	case MM_PIXEL_FORMAT_RGBA:
		*out_format = TBM_FORMAT_RGBA8888;
		break;
	case MM_PIXEL_FORMAT_ARGB:
		*out_format = TBM_FORMAT_ARGB8888;
		break;
	default:
		LOGE("invalid in_format %d", in_format);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	return PLAYER_ERROR_NONE;
}

int _player_get_media_packet_mimetype(int in_format, media_format_mimetype_e *mimetype)
{
	if (in_format <= MM_PIXEL_FORMAT_INVALID || in_format >= MM_PIXEL_FORMAT_NUM || mimetype == NULL) {
		LOGE("INVALID_PARAMETER : in_format %d, mimetype ptr %p", in_format, mimetype);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	switch (in_format) {
	case MM_PIXEL_FORMAT_NV12:
	case MM_PIXEL_FORMAT_NV12T:
		*mimetype = MEDIA_FORMAT_NV12;
		break;
	case MM_PIXEL_FORMAT_NV16:
		*mimetype = MEDIA_FORMAT_NV16;
		break;
	case MM_PIXEL_FORMAT_NV21:
		*mimetype = MEDIA_FORMAT_NV21;
		break;
	case MM_PIXEL_FORMAT_YUYV:
		*mimetype = MEDIA_FORMAT_YUYV;
		break;
	case MM_PIXEL_FORMAT_UYVY:
	case MM_PIXEL_FORMAT_ITLV_JPEG_UYVY:
		*mimetype = MEDIA_FORMAT_UYVY;
		break;
	case MM_PIXEL_FORMAT_422P:
		*mimetype = MEDIA_FORMAT_422P;
		break;
	case MM_PIXEL_FORMAT_I420:
		*mimetype = MEDIA_FORMAT_I420;
		break;
	case MM_PIXEL_FORMAT_YV12:
		*mimetype = MEDIA_FORMAT_YV12;
		break;
	case MM_PIXEL_FORMAT_RGB565:
		*mimetype = MEDIA_FORMAT_RGB565;
		break;
	case MM_PIXEL_FORMAT_RGB888:
		*mimetype = MEDIA_FORMAT_RGB888;
		break;
	case MM_PIXEL_FORMAT_RGBA:
		*mimetype = MEDIA_FORMAT_RGBA;
		break;
	case MM_PIXEL_FORMAT_ARGB:
		*mimetype = MEDIA_FORMAT_ARGB;
		break;
	default:
		LOGE("invalid in_format %d", in_format);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	return PLAYER_ERROR_NONE;
}

int _player_media_packet_finalize(media_packet_h pkt, int error_code, void *user_data)
{
	int ret = 0;
	void *internal_buffer = NULL;
	tbm_surface_h tsurf = NULL;
	player_s *handle = NULL;

	if (pkt == NULL || user_data == NULL) {
		LOGE("invalid parameter buffer %p, user_data %p", pkt, user_data);
		return MEDIA_PACKET_FINALIZE;
	}

	handle = (player_s *)user_data;

	ret = media_packet_get_extra(pkt, &internal_buffer);
	if (ret != MEDIA_PACKET_ERROR_NONE) {
		LOGE("media_packet_get_extra failed 0x%x", ret);
		return MEDIA_PACKET_FINALIZE;
	}

	/* LOGD("pointer gst buffer %p, ret 0x%x", internal_buffer, ret); */
	mm_player_media_packet_video_stream_internal_buffer_unref(internal_buffer);

	ret = media_packet_get_tbm_surface(pkt, &tsurf);
	if (ret != MEDIA_PACKET_ERROR_NONE) {
		LOGE("media_packet_get_tbm_surface failed 0x%x", ret);
		return MEDIA_PACKET_FINALIZE;
	}

	if (tsurf && (tbm_surface_get_format(tsurf) == TBM_FORMAT_YUV420)) {
#define NUM_OF_SW_CODEC_BO 1
		int bo_num = 0;
		tbm_bo bo = NULL;

		bo_num = tbm_surface_internal_get_num_bos(tsurf);
		if (bo_num == NUM_OF_SW_CODEC_BO) {
			bo = tbm_surface_internal_get_bo(tsurf, 0);
			if (bo) {
				mm_player_release_video_stream_bo(handle->mm_handle, bo);
			} else {
				LOGE("bo is NULL");
			}
		}
		tbm_surface_destroy(tsurf);
		tsurf = NULL;
	}

	return MEDIA_PACKET_FINALIZE;
}

static bool _player_network_availability_check()
{
#define _FEATURE_NAME_WIFI		"http://tizen.org/feature/network.wifi"
#define _FEATURE_NAME_TELEPHONY	"http://tizen.org/feature/network.telephony"
#define _FEATURE_NAME_ETHERNET	"http://tizen.org/feature/network.ethernet"
	bool enabled = FALSE;
	bool supported = FALSE;

	if (SYSTEM_INFO_ERROR_NONE == system_info_get_platform_bool(_FEATURE_NAME_WIFI, &enabled)) {
		LOGI("wifi status = %d", enabled);
		if (enabled)
			supported = TRUE;
	} else {
		LOGE("SYSTEM_INFO_ERROR");
	}

	if (SYSTEM_INFO_ERROR_NONE == system_info_get_platform_bool(_FEATURE_NAME_TELEPHONY, &enabled)) {
		LOGI("telephony status = %d", enabled);
		if (enabled)
			supported = TRUE;
	} else {
		LOGE("SYSTEM_INFO_ERROR");
	}

	if (SYSTEM_INFO_ERROR_NONE == system_info_get_platform_bool(_FEATURE_NAME_ETHERNET, &enabled)) {
		LOGI("ethernet status = %d", enabled);
		if (enabled)
			supported = TRUE;
	} else {
		LOGE("SYSTEM_INFO_ERROR");
	}

	if (!supported)
		return FALSE;

	return TRUE;
}

static player_interrupted_code_e __convert_interrupted_code(int code)
{
	player_interrupted_code_e ret = PLAYER_INTERRUPTED_BY_RESOURCE_CONFLICT;
	switch (code) {
	case MM_MSG_CODE_INTERRUPTED_BY_CALL_END:
	case MM_MSG_CODE_INTERRUPTED_BY_ALARM_END:
	case MM_MSG_CODE_INTERRUPTED_BY_EMERGENCY_END:
	case MM_MSG_CODE_INTERRUPTED_BY_NOTIFICATION_END:
		ret = PLAYER_INTERRUPTED_COMPLETED;
		break;
	case MM_MSG_CODE_INTERRUPTED_BY_MEDIA:
	case MM_MSG_CODE_INTERRUPTED_BY_OTHER_PLAYER_APP:
		ret = PLAYER_INTERRUPTED_BY_MEDIA;
		break;
	case MM_MSG_CODE_INTERRUPTED_BY_CALL_START:
		ret = PLAYER_INTERRUPTED_BY_CALL;
		break;
	case MM_MSG_CODE_INTERRUPTED_BY_EARJACK_UNPLUG:
		ret = PLAYER_INTERRUPTED_BY_EARJACK_UNPLUG;
		break;
	case MM_MSG_CODE_INTERRUPTED_BY_ALARM_START:
		ret = PLAYER_INTERRUPTED_BY_ALARM;
		break;
	case MM_MSG_CODE_INTERRUPTED_BY_NOTIFICATION_START:
		ret = PLAYER_INTERRUPTED_BY_NOTIFICATION;
		break;
	case MM_MSG_CODE_INTERRUPTED_BY_EMERGENCY_START:
		ret = PLAYER_INTERRUPTED_BY_EMERGENCY;
		break;
	case MM_MSG_CODE_INTERRUPTED_BY_RESOURCE_CONFLICT:
	default:
		ret = PLAYER_INTERRUPTED_BY_RESOURCE_CONFLICT;
		break;
	}
	LOGE("[%s] interrupted code(%d) => ret(%d)", __FUNCTION__, code, ret);
	return ret;
}

static player_state_e __convert_player_state(MMPlayerStateType state)
{
	if (state == MM_PLAYER_STATE_NONE)
		return PLAYER_STATE_NONE;
	else
		return state + 1;
}

bool __player_state_validate(player_s *handle, player_state_e threshold)
{
	if (handle->state < threshold)
		return FALSE;
	return TRUE;
}

static int __set_callback(muse_player_event_e type, player_h player, void *callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	if (MUSE_PLAYER_EVENT_TYPE_BUFFERING == type) {
		if (!_player_network_availability_check())
			return PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE;
	}

	player_s *handle = (player_s *)player;
	handle->user_cb[type] = callback;
	handle->user_data[type] = user_data;
	LOGI("[%s] Event type : %d ", __FUNCTION__, type);
	return PLAYER_ERROR_NONE;
}

static int __unset_callback(muse_player_event_e type, player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	handle->user_cb[type] = NULL;
	handle->user_data[type] = NULL;
	LOGI("[%s] Event type : %d ", __FUNCTION__, type);
	return PLAYER_ERROR_NONE;
}

#ifdef USE_ECORE_FUNCTIONS
static void __job_prepared_cb(void *user_data)
{
	player_s *handle = (player_s *)user_data;
	LOGI("Start");
	handle->is_doing_jobs = TRUE;
	handle->state = PLAYER_STATE_READY;
	((player_prepared_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PREPARE])(handle->user_data[MUSE_PLAYER_EVENT_TYPE_PREPARE]);
	handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PREPARE] = NULL;
	handle->user_data[MUSE_PLAYER_EVENT_TYPE_PREPARE] = NULL;
	__REMOVE_ECORE_JOB(handle, __JOB_KEY_PREPARED);
	LOGI("End");
}

static void __job_error_cb(void *user_data)
{
	player_s *handle = (player_s *)user_data;
	LOGI("Start");
	handle->is_doing_jobs = TRUE;
	((player_error_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_ERROR])(handle->error_code, handle->user_data[MUSE_PLAYER_EVENT_TYPE_ERROR]);
	__REMOVE_ECORE_JOB(handle, __JOB_KEY_ERROR);
	LOGI("End");
}

static void __job_seek_done_cb(void *user_data)
{
	player_s *handle = (player_s *)user_data;
	LOGI("Start");
	handle->is_doing_jobs = TRUE;
	((player_seek_completed_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK])(handle->user_data[MUSE_PLAYER_EVENT_TYPE_SEEK]);
	handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK] = NULL;
	handle->user_data[MUSE_PLAYER_EVENT_TYPE_SEEK] = NULL;
	__REMOVE_ECORE_JOB(handle, __JOB_KEY_SEEK_DONE);
	LOGI("End");
}

static void __job_eos_cb(void *user_data)
{
	player_s *handle = (player_s *)user_data;
	LOGI("Start");
	handle->is_doing_jobs = TRUE;
	((player_completed_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_COMPLETE])(handle->user_data[MUSE_PLAYER_EVENT_TYPE_COMPLETE]);
	__REMOVE_ECORE_JOB(handle, __JOB_KEY_EOS);
	LOGI("End");
}
#else
static void __message_cb_loop(void *data)
{
	bool running = TRUE;
	player_s *handle = (player_s *)data;
	if (!handle) {
		LOGE("null handle in __message_cb_loop");
		return;
	}
	do {
		__GET_MESSAGE(handle);
		switch (handle->current_message) {
		case PLAYER_MESSAGE_NONE:
			{
				LOGW("PLAYER_MESSAGE_NONE");
				running = FALSE;
			}
			break;
		case PLAYER_MESSAGE_PREPARED:
			{
				LOGW("PLAYER_MESSAGE_PREPARED");
				if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PREPARE]) {
					handle->is_doing_jobs = TRUE;
					handle->state = PLAYER_STATE_READY;
					((player_prepared_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PREPARE])(handle->user_data[MUSE_PLAYER_EVENT_TYPE_PREPARE]);
					handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PREPARE] = NULL;
					handle->user_data[MUSE_PLAYER_EVENT_TYPE_PREPARE] = NULL;
					handle->is_doing_jobs = FALSE;
				} else {
					LOGE("null handle in PLAYER_MESSAGE_PREPARED");
				}
			}
			break;
		case PLAYER_MESSAGE_ERROR:
			{
				LOGW("PLAYER_MESSAGE_ERROR");
				if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_ERROR]) {
					handle->is_doing_jobs = TRUE;
					((player_error_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_ERROR])(handle->error_code, handle->user_data[MUSE_PLAYER_EVENT_TYPE_ERROR]);
					handle->is_doing_jobs = FALSE;
				} else {
					LOGE("null handle in PLAYER_MESSAGE_ERROR");
				}
			}
			break;
		case PLAYER_MESSAGE_SEEK_DONE:
			{
				LOGW("PLAYER_MESSAGE_SEEK_DONE");
				if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK]) {
					handle->is_doing_jobs = TRUE;
					((player_seek_completed_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK])(handle->user_data[MUSE_PLAYER_EVENT_TYPE_SEEK]);
					handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK] = NULL;
					handle->user_data[MUSE_PLAYER_EVENT_TYPE_SEEK] = NULL;
					handle->is_doing_jobs = FALSE;
				} else {
					LOGE("null handle in PLAYER_MESSAGE_SEEK_DONE");
				}
			}
			break;
		case PLAYER_MESSAGE_EOS:
			{
				LOGW("PLAYER_MESSAGE_EOS");
				if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_COMPLETE]) {
					handle->is_doing_jobs = TRUE;
					((player_completed_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_COMPLETE])(handle->user_data[MUSE_PLAYER_EVENT_TYPE_COMPLETE]);
					handle->is_doing_jobs = FALSE;
				} else {
					LOGE("null handle in PLAYER_MESSAGE_EOS");
				}
			}
			break;
		case PLAYER_MESSAGE_LOOP_EXIT:
			{
				LOGW("PLAYER_MESSAGE_LOOP_EXIT");
				running = FALSE;
			}
			break;
		case PLAYER_MESSAGE_MAX:
			{
				LOGW("PLAYER_MESSAGE_MAX");
				running = FALSE;
			}
			break;
		default:
			break;
		}
	} while (running);
	return;
}
#endif

static int __msg_callback(int message, void *param, void *user_data)
{
	player_s *handle = (player_s *)user_data;
	MMMessageParamType *msg = (MMMessageParamType *)param;
	LOGW("[%s] Got message type : 0x%x", __FUNCTION__, message);
	player_error_e err_code = PLAYER_ERROR_NONE;
	switch (message) {
	case MM_MESSAGE_ERROR:	/* 0x01 */
		err_code = __player_convert_error_code(msg->code, (char *)__FUNCTION__);
		break;
	case MM_MESSAGE_STATE_CHANGED:	/* 0x03 */
		LOGI("STATE CHANGED INTERNALLY - from : %d,  to : %d (CAPI State : %d)", msg->state.previous, msg->state.current, handle->state);
		if ((handle->is_progressive_download && msg->state.previous == MM_PLAYER_STATE_NULL && msg->state.current == MM_PLAYER_STATE_READY) || (msg->state.previous == MM_PLAYER_STATE_READY && msg->state.current == MM_PLAYER_STATE_PAUSED)) {
			if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PREPARE]) {
				/* asyc && prepared cb has been set */
				LOGI("[%s] Prepared! [current state : %d]", __FUNCTION__, handle->state);
				PLAYER_TRACE_ASYNC_END("MM:PLAYER:PREPARE_ASYNC", *(int *)handle);
#ifdef USE_ECORE_FUNCTIONS
				__ADD_ECORE_JOB(handle, __JOB_KEY_PREPARED, __job_prepared_cb);
#else
				__ADD_MESSAGE(handle, PLAYER_MESSAGE_PREPARED);
#endif
			}
		}
		break;
	case MM_MESSAGE_READY_TO_RESUME:	/* 0x05 */
		if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_INTERRUPT])
			((player_interrupted_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_INTERRUPT])(PLAYER_INTERRUPTED_COMPLETED, handle->user_data[MUSE_PLAYER_EVENT_TYPE_INTERRUPT]);
		break;
	case MM_MESSAGE_BEGIN_OF_STREAM:	/* 0x104 */
		LOGI("[%s] Ready to streaming information (BOS) [current state : %d]", __FUNCTION__, handle->state);
		break;
	case MM_MESSAGE_END_OF_STREAM:	/* 0x105 */
		if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_COMPLETE]) {
#ifdef USE_ECORE_FUNCTIONS
			__ADD_ECORE_JOB(handle, __JOB_KEY_EOS, __job_eos_cb);
#else
			__ADD_MESSAGE(handle, PLAYER_MESSAGE_EOS);
#endif
		}
		break;
	case MM_MESSAGE_BUFFERING:	/* 0x103 */
		if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_BUFFERING])
			((player_buffering_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_BUFFERING])(msg->connection.buffering, handle->user_data[MUSE_PLAYER_EVENT_TYPE_BUFFERING]);
		break;
	case MM_MESSAGE_STATE_INTERRUPTED:	/* 0x04 */
		if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_INTERRUPT]) {
			handle->state = __convert_player_state(msg->state.current);
			if (handle->state == PLAYER_STATE_READY)
				handle->is_stopped = TRUE;
			((player_interrupted_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_INTERRUPT])(__convert_interrupted_code(msg->code), handle->user_data[MUSE_PLAYER_EVENT_TYPE_INTERRUPT]);
		}
		break;
	case MM_MESSAGE_CONNECTION_TIMEOUT:	/* 0x102 */
		LOGI("[%s] PLAYER_ERROR_CONNECTION_FAILED (0x%08x) : CONNECTION_TIMEOUT", __FUNCTION__, PLAYER_ERROR_CONNECTION_FAILED);
		err_code = PLAYER_ERROR_CONNECTION_FAILED;
		break;
	case MM_MESSAGE_UPDATE_SUBTITLE:	/* 0x109 */
		if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SUBTITLE])
			((player_subtitle_updated_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SUBTITLE])(msg->subtitle.duration, (char *)msg->data, handle->user_data[MUSE_PLAYER_EVENT_TYPE_SUBTITLE]);
		break;
	case MM_MESSAGE_VIDEO_NOT_CAPTURED:	/* 0x113 */
		LOGI("[%s] PLAYER_ERROR_VIDEO_CAPTURE_FAILED (0x%08x)", __FUNCTION__, PLAYER_ERROR_VIDEO_CAPTURE_FAILED);
		if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_ERROR])
			((player_error_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_ERROR])(PLAYER_ERROR_VIDEO_CAPTURE_FAILED, handle->user_data[MUSE_PLAYER_EVENT_TYPE_ERROR]);
		break;
	case MM_MESSAGE_VIDEO_CAPTURED:	/* 0x110 */
		if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_CAPTURE]) {
			int w;
			int h;
			int ret = mm_player_get_attribute(handle->mm_handle, NULL, MM_PLAYER_VIDEO_WIDTH, &w, MM_PLAYER_VIDEO_HEIGHT, &h, (char *)NULL);
			if (ret != MM_ERROR_NONE && handle->user_cb[MUSE_PLAYER_EVENT_TYPE_ERROR]) {
				LOGE("[%s] PLAYER_ERROR_VIDEO_CAPTURE_FAILED (0x%08x) : Failed to get video size on video captured (0x%x)", __FUNCTION__, PLAYER_ERROR_VIDEO_CAPTURE_FAILED, ret);
				err_code = PLAYER_ERROR_VIDEO_CAPTURE_FAILED;
			} else {
				MMPlayerVideoCapture *capture = (MMPlayerVideoCapture *)msg->data;

				switch (msg->captured_frame.orientation) {
				case 0:
				case 180:
					{
						/* use video resolution from above */
					}
					break;
				case 90:
				case 270:
					{
						/* use calculated size during rotation */
						w = msg->captured_frame.width;
						h = msg->captured_frame.height;
					}
					break;
				default:
					break;
				}

				LOGI("[%s] captured image width : %d   height : %d", __FUNCTION__, w, h);

				/* call application callback */
				((player_video_captured_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_CAPTURE])(capture->data, w, h, capture->size, handle->user_data[MUSE_PLAYER_EVENT_TYPE_CAPTURE]);

				if (capture->data) {
					g_free(capture->data);
					capture->data = NULL;
				}
			}
			handle->user_cb[MUSE_PLAYER_EVENT_TYPE_CAPTURE] = NULL;
			handle->user_data[MUSE_PLAYER_EVENT_TYPE_CAPTURE] = NULL;
		}
		break;
	case MM_MESSAGE_FILE_NOT_SUPPORTED:	/* 0x10f */
		LOGI("[%s] PLAYER_ERROR_NOT_SUPPORTED_FILE (0x%08x) : FILE_NOT_SUPPORTED", __FUNCTION__, PLAYER_ERROR_NOT_SUPPORTED_FILE);
		err_code = PLAYER_ERROR_NOT_SUPPORTED_FILE;
		break;
	case MM_MESSAGE_FILE_NOT_FOUND:	/* 0x110 */
		LOGI("[%s] PLAYER_ERROR_NOT_SUPPORTED_FILE (0x%08x) : FILE_NOT_FOUND", __FUNCTION__, PLAYER_ERROR_NOT_SUPPORTED_FILE);
		err_code = PLAYER_ERROR_NOT_SUPPORTED_FILE;
		break;
	case MM_MESSAGE_SEEK_COMPLETED:	/* 0x114 */
		if (handle->display_type != PLAYER_DISPLAY_TYPE_NONE && handle->state == PLAYER_STATE_READY) {
			if (handle->is_display_visible)
				mm_player_set_attribute(handle->mm_handle, NULL, "display_visible", 1, (char *)NULL);
		}
		if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK]) {
#ifdef USE_ECORE_FUNCTIONS
			__ADD_ECORE_JOB(handle, __JOB_KEY_SEEK_DONE, __job_seek_done_cb);
#else
			__ADD_MESSAGE(handle, PLAYER_MESSAGE_SEEK_DONE);
#endif
		}
		break;
	case MM_MESSAGE_UNKNOWN:	/* 0x00 */
	case MM_MESSAGE_WARNING:	/* 0x02 */
	case MM_MESSAGE_CONNECTING:	/* 0x100 */
	case MM_MESSAGE_CONNECTED:	/* 0x101 */
	case MM_MESSAGE_BLUETOOTH_ON:	/* 0x106 */
	case MM_MESSAGE_BLUETOOTH_OFF:	/* 0x107 */
	case MM_MESSAGE_RTP_SENDER_REPORT:	/* 0x10a */
	case MM_MESSAGE_RTP_RECEIVER_REPORT:	/* 0x10b */
	case MM_MESSAGE_RTP_SESSION_STATUS:	/* 0x10c */
	case MM_MESSAGE_SENDER_STATE:	/* 0x10d */
	case MM_MESSAGE_RECEIVER_STATE:	/* 0x10e */
	default:
		break;
	}

	if (err_code != PLAYER_ERROR_NONE && handle->user_cb[MUSE_PLAYER_EVENT_TYPE_ERROR]) {
		handle->error_code = err_code;
#ifdef USE_ECORE_FUNCTIONS
		__ADD_ECORE_JOB(handle, __JOB_KEY_ERROR, __job_error_cb);
#else
		__ADD_MESSAGE(handle, PLAYER_MESSAGE_ERROR);
#endif
	}
	LOGW("[%s] End", __FUNCTION__);
	return 1;
}

static bool __video_stream_callback(void *stream, void *user_data)
{
	player_s *handle = (player_s *)user_data;
	MMPlayerVideoStreamDataType *video_stream = (MMPlayerVideoStreamDataType *)stream;

	if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME]) {
		/* media packet and zero-copy */
		media_packet_h pkt = NULL;
		tbm_surface_h tsurf = NULL;
		uint32_t bo_format = 0;
		int i;
		int bo_num;
		int ret = 0;
		media_format_mimetype_e mimetype = MEDIA_FORMAT_NV12;
		bool make_pkt_fmt = FALSE;

		/* create tbm surface */
		for (i = 0, bo_num = 0; i < BUFFER_MAX_PLANE_NUM; i++) {
			if (video_stream->bo[i])
				bo_num++;
		}

		/* get tbm surface format */
		ret = _player_get_tbm_surface_format(video_stream->format, &bo_format);
		ret |= _player_get_media_packet_mimetype(video_stream->format, &mimetype);

		if (bo_num > 0 && ret == PLAYER_ERROR_NONE) {
			tbm_surface_info_s info;
			memset(&info, 0, sizeof(tbm_surface_info_s));
			info.width = video_stream->width;
			info.height = video_stream->height;
			info.format = bo_format;
			info.bpp = tbm_surface_internal_get_bpp(bo_format);
			info.num_planes = tbm_surface_internal_get_num_planes(bo_format);
			info.size = 0;
			for (i = 0; i < info.num_planes; i++) {
				info.planes[i].stride = video_stream->stride[i];
				info.planes[i].size = video_stream->stride[i] * video_stream->elevation[i];
				if (i < bo_num)
					info.planes[i].offset = 0;
				else
					info.planes[i].offset = info.planes[i - 1].offset + info.planes[i - 1].size;
				info.size += info.planes[i].size;
			}
			tsurf = tbm_surface_internal_create_with_bos(&info, (tbm_bo *)video_stream->bo, bo_num);
			/*LOGD("tbm surface %p", tsurf); */
		} else if (bo_num == 0) {
			int plane_idx = 0;
			tbm_surface_info_s tsuri;
			unsigned char *ptr = video_stream->data[0];
			unsigned char *ptr2 = video_stream->data[1];

			if (!ptr)
				return TRUE;
			if (!ptr2 && video_stream->format == MM_PIXEL_FORMAT_NV12T)
				return TRUE;

			tsurf = tbm_surface_create(video_stream->width, video_stream->height, bo_format);
			if (tsurf) {
				/* map surface to set data */
				if (tbm_surface_map(tsurf, TBM_SURF_OPTION_READ | TBM_SURF_OPTION_WRITE, &tsuri)) {
					LOGE("tbm_surface_map failed");
					return TRUE;
				}

				if (video_stream->format == MM_PIXEL_FORMAT_NV12T) {
					memcpy(tsuri.planes[0].ptr, ptr, tsuri.planes[0].size);
					memcpy(tsuri.planes[1].ptr, ptr2, tsuri.planes[1].size);
				} else {
					for (plane_idx = 0; plane_idx < tsuri.num_planes; plane_idx++) {
						memcpy(tsuri.planes[plane_idx].ptr, ptr, tsuri.planes[plane_idx].size);
						ptr += tsuri.planes[plane_idx].size;
					}
				}
			} else {
				LOGW("tbm_surface_create failed");
			}
		}

		if (tsurf) {
			/* check media packet format */
			if (handle->pkt_fmt) {
				int pkt_fmt_width = 0;
				int pkt_fmt_height = 0;
				media_format_mimetype_e pkt_fmt_mimetype = MEDIA_FORMAT_NV12;

				media_format_get_video_info(handle->pkt_fmt, &pkt_fmt_mimetype,
					&pkt_fmt_width, &pkt_fmt_height, NULL, NULL);
				if (pkt_fmt_mimetype != mimetype ||
					pkt_fmt_width != video_stream->width ||
					pkt_fmt_height != video_stream->height) {
					LOGW("different format. current 0x%x, %dx%d, new 0x%x, %dx%d",
						pkt_fmt_mimetype, pkt_fmt_width, pkt_fmt_height, mimetype,
						video_stream->width, video_stream->height);
					media_format_unref(handle->pkt_fmt);
					handle->pkt_fmt = NULL;
					make_pkt_fmt = TRUE;
				}
			} else {
				make_pkt_fmt = TRUE;
			}

			/* create packet format */
			if (make_pkt_fmt) {
				LOGW("make new pkt_fmt - mimetype 0x%x, %dx%d", mimetype, video_stream->width, video_stream->height);
				ret = media_format_create(&handle->pkt_fmt);
				if (ret == MEDIA_FORMAT_ERROR_NONE) {
					ret = media_format_set_video_mime(handle->pkt_fmt, mimetype);
					ret |= media_format_set_video_width(handle->pkt_fmt, video_stream->width);
					ret |= media_format_set_video_height(handle->pkt_fmt, video_stream->height);
					LOGW("media_format_set_video_mime,width,height ret : 0x%x", ret);
				} else {
					LOGW("media_format_create failed");
				}
			}

			/* create media packet */
			ret = media_packet_create_from_tbm_surface(handle->pkt_fmt, tsurf, (media_packet_finalize_cb)_player_media_packet_finalize, (void *)handle, &pkt);
			if (ret != MEDIA_PACKET_ERROR_NONE) {
				LOGE("media_packet_create_from_tbm_surface failed");

				tbm_surface_destroy(tsurf);
				tsurf = NULL;
			}
		} else {
			LOGE("failed to create tbm surface %dx%d, format %d, bo_num %d",
				video_stream->width, video_stream->height, video_stream->format, bo_num);
		}

		if (pkt) {
			/* LOGD("media packet %p, internal buffer %p", pkt, stream->internal_buffer); */
			if (video_stream->timestamp) {
				ret = media_packet_set_pts(pkt, (uint64_t)video_stream->timestamp * 1000000);
				if (ret != MEDIA_PACKET_ERROR_NONE) {
					LOGE("media_packet_set_pts failed");

					media_packet_destroy(pkt);
					pkt = NULL;
				}
			} else {
				LOGD("media packet %p, didn't have video-stream timestamp", pkt);
			}

			/* set internal buffer */
			if (video_stream->internal_buffer)
				ret = media_packet_set_extra(pkt, video_stream->internal_buffer);

			if (ret != MEDIA_PACKET_ERROR_NONE) {
				LOGE("media_packet_set_extra failed");

				media_packet_destroy(pkt);
				pkt = NULL;
			} else {
				mm_player_media_packet_video_stream_internal_buffer_ref(video_stream->internal_buffer);

				/* call media packet callback */
				((player_media_packet_video_decoded_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME])(pkt, handle->user_data[MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME]);

				if (bo_num == 0)
					tbm_surface_unmap(tsurf);
			}
		}
	}
	return TRUE;
}

static int __pd_message_callback(int message, void *param, void *user_data)
{
	player_s *handle = (player_s *)user_data;
	player_pd_message_type_e type;
	switch (message) {
	case MM_MESSAGE_PD_DOWNLOADER_START:
		type = PLAYER_PD_STARTED;
		break;
	case MM_MESSAGE_PD_DOWNLOADER_END:
		type = PLAYER_PD_COMPLETED;
		break;
	default:
		return 0;
	}

	if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PD])
		((player_pd_message_cb)handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PD])(type, handle->user_data[MUSE_PLAYER_EVENT_TYPE_PD]);

	return 0;
}

#ifdef USE_ECORE_FUNCTIONS
static void __job_key_to_remove(gpointer key)
{
	LOGI("%s", key);
	g_free(key);
}

static void __job_value_to_destroy(gpointer value)
{
	Ecore_Job *job = (Ecore_Job *) value;
	LOGI("%p", job);
	__DELETE_ECORE_ITEM(job);
}
#endif

static MMDisplaySurfaceType __player_convet_display_type(player_display_type_e type)
{
	switch (type) {
	case PLAYER_DISPLAY_TYPE_OVERLAY:
		return MM_DISPLAY_SURFACE_OVERLAY;
#ifdef EVAS_RENDERER_SUPPORT
	case PLAYER_DISPLAY_TYPE_EVAS:
		return MM_DISPLAY_SURFACE_REMOTE;
#endif
	case PLAYER_DISPLAY_TYPE_NONE:
		return MM_DISPLAY_SURFACE_NULL;
	default:
		return MM_DISPLAY_SURFACE_NULL;
	}
}

/*
* Public Implementation
*/

int legacy_player_create(player_h *player)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_TRACE_BEGIN("MM:PLAYER:CREATE");
	player_s *handle;
	handle = (player_s *)malloc(sizeof(player_s));
	if (handle != NULL)
		memset(handle, 0, sizeof(player_s));
	else {
		LOGE("[%s] PLAYER_ERROR_OUT_OF_MEMORY(0x%08x)", __FUNCTION__, PLAYER_ERROR_OUT_OF_MEMORY);
		return PLAYER_ERROR_OUT_OF_MEMORY;
	}
	int ret = mm_player_create(&handle->mm_handle);
	if (ret != MM_ERROR_NONE) {
		LOGE("[%s] PLAYER_ERROR_INVALID_OPERATION(0x%08x)", __FUNCTION__, PLAYER_ERROR_INVALID_OPERATION);
		handle->state = PLAYER_STATE_NONE;
		free(handle);
		handle = NULL;
		return PLAYER_ERROR_INVALID_OPERATION;
	} else {
		*player = (player_h)handle;
		handle->state = PLAYER_STATE_IDLE;
		handle->display_type = PLAYER_DISPLAY_TYPE_NONE;
		handle->is_stopped = FALSE;
		handle->is_display_visible = TRUE;
		handle->is_media_stream = FALSE;
#ifdef USE_ECORE_FUNCTIONS
		handle->ecore_jobs = g_hash_table_new_full(g_str_hash, g_str_equal, __job_key_to_remove, __job_value_to_destroy);
#else
		handle->message_queue = g_queue_new();
		g_mutex_init(&handle->message_queue_lock);
		g_cond_init(&handle->message_queue_cond);
		ret = pthread_create(&handle->message_thread, NULL, (void *)__message_cb_loop, (void *)handle);
		if (ret != 0) {
			LOGE("[%s] failed to create message thread ret = %d", __FUNCTION__, ret);
			return PLAYER_ERROR_OUT_OF_MEMORY;
		}
#endif
		LOGI("[%s] new handle : %p", __FUNCTION__, *player);
		PLAYER_TRACE_END();
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_destroy(player_h player)
{
	LOGI("[%s] Start, handle to destroy : %p", __FUNCTION__, player);
	PLAYER_TRACE_BEGIN("MM:PLAYER:DESTROY");
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	if (handle->is_doing_jobs) {
		LOGE("jobs not completed. can't do destroy");
		return PLAYER_ERROR_INVALID_OPERATION;
	}
#ifdef USE_ECORE_FUNCTIONS
	g_hash_table_remove_all(handle->ecore_jobs);
	g_hash_table_unref(handle->ecore_jobs);
	handle->ecore_jobs = NULL;
#else
	__ADD_MESSAGE(handle, PLAYER_MESSAGE_LOOP_EXIT);
#endif
	__RELEASEIF_PREPARE_THREAD(handle->prepare_async_thread);

#ifndef USE_ECORE_FUNCTIONS
	__RELEASEIF_MESSAGE_THREAD(handle->message_thread);
#endif

	int ret = mm_player_destroy(handle->mm_handle);

	if (handle->pkt_fmt) {
		media_format_unref(handle->pkt_fmt);
		handle->pkt_fmt = NULL;
	}

	LOGI("[%s] Done mm_player_destroy", __FUNCTION__);

	if (ret != MM_ERROR_NONE) {
		LOGE("[%s] PLAYER_ERROR_INVALID_OPERATION(0x%08x)", __FUNCTION__, PLAYER_ERROR_INVALID_OPERATION);
		return PLAYER_ERROR_INVALID_OPERATION;
	} else {
		handle->state = PLAYER_STATE_NONE;
#ifndef USE_ECORE_FUNCTIONS
		if (handle->message_queue) {
			g_queue_free(handle->message_queue);
			handle->message_queue = NULL;
		}

		g_cond_broadcast(&handle->message_queue_cond);
		g_mutex_clear(&handle->message_queue_lock);
		g_cond_clear(&handle->message_queue_cond);
#endif
		free(handle);
		handle = NULL;
		LOGI("[%s] End", __FUNCTION__);
		PLAYER_TRACE_END();
		return PLAYER_ERROR_NONE;
	}
}

static void *__prepare_async_thread_func(void *data)
{
	player_s *handle = data;
	int ret = MM_ERROR_NONE;
	LOGI("[%s]", __FUNCTION__);

	ret = mm_player_pause(handle->mm_handle);
	if (ret != MM_ERROR_NONE) {
		LOGE("[%s] Failed to pause - core fw error(0x%x)", __FUNCTION__, ret);
		/*MM_MESSAGE_ERROR will not be posted as legacy_player_prepare(sync API) works with return value
		   of mm_player_pause So in case of async API we post the error message to application from here */
		MMMessageParamType msg_param;
		msg_param.code = ret;
		__msg_callback(MM_MESSAGE_ERROR, (void *)&msg_param, (void *)handle);

		ret = mm_player_unrealize(handle->mm_handle);
		if (ret != MM_ERROR_NONE)
			LOGE("[%s] Failed to unrealize - 0x%x", __FUNCTION__, ret);
	}
	LOGI("[%s], done", __FUNCTION__);
	return NULL;
}

int legacy_player_prepare_async(player_h player, player_prepared_cb callback, void *user_data)
{
	LOGI("[%s] Start", __FUNCTION__);
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	PLAYER_TRACE_ASYNC_BEGIN("MM:PLAYER:PREPARE_ASYNC", *(int *)handle);
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	int ret;
	int visible;
	int value;

	if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PREPARE]) {
		LOGE("[%s] PLAYER_ERROR_INVALID_OPERATION (0x%08x) : preparing... we can't do any more ", __FUNCTION__, PLAYER_ERROR_INVALID_OPERATION);
		return PLAYER_ERROR_INVALID_OPERATION;
	} else {
		/* LOGI("[%s] Event type : %d ",__FUNCTION__, MUSE_PLAYER_EVENT_TYPE_PREPARE); */
		handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PREPARE] = callback;
		handle->user_data[MUSE_PLAYER_EVENT_TYPE_PREPARE] = user_data;
	}

	ret = mm_player_set_message_callback(handle->mm_handle, __msg_callback, (void *)handle);
	if (ret != MM_ERROR_NONE)
		LOGW("[%s] Failed to set message callback function (0x%x)", __FUNCTION__, ret);

	if (handle->display_type == PLAYER_DISPLAY_TYPE_NONE) {
		ret = mm_player_set_attribute(handle->mm_handle, NULL, "display_surface_type", MM_DISPLAY_SURFACE_NULL, (char *)NULL);
		if (ret != MM_ERROR_NONE)
			LOGW("[%s] Failed to set display surface type 'MM_DISPLAY_SURFACE_NULL' (0x%x)", __FUNCTION__, ret);
	} else {
		ret = mm_player_get_attribute(handle->mm_handle, NULL, "display_visible", &visible, (char *)NULL);
		if (ret != MM_ERROR_NONE)
			return __player_convert_error_code(ret, (char *)__FUNCTION__);

		if (!visible)
			value = FALSE;
		else
			value = TRUE;

		ret = mm_player_set_attribute(handle->mm_handle, NULL, "display_visible", value, (char *)NULL);
		if (ret != MM_ERROR_NONE)
			return __player_convert_error_code(ret, (char *)__FUNCTION__);
	}

	ret = mm_player_realize(handle->mm_handle);
	if (ret != MM_ERROR_NONE) {
		LOGE("[%s] Failed to realize - 0x%x", __FUNCTION__, ret);
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	}

	if (!handle->is_progressive_download) {
		ret = pthread_create(&handle->prepare_async_thread, NULL, (void *)__prepare_async_thread_func, (void *)handle);

		if (ret != 0) {
			LOGE("[%s] failed to create thread ret = %d", __FUNCTION__, ret);
			return PLAYER_ERROR_OUT_OF_MEMORY;
		}
	}

	LOGI("[%s] End", __FUNCTION__);
	return PLAYER_ERROR_NONE;
}

int legacy_player_prepare(player_h player)
{
	LOGI("[%s] Start", __FUNCTION__);
	PLAYER_TRACE_BEGIN("MM:PLAYER:PREPARE");
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	int ret;
	int visible;
	int value;
	ret = mm_player_set_message_callback(handle->mm_handle, __msg_callback, (void *)handle);
	if (ret != MM_ERROR_NONE)
		LOGW("[%s] Failed to set message callback function (0x%x)", __FUNCTION__, ret);

	if (handle->display_type == PLAYER_DISPLAY_TYPE_NONE) {
		ret = mm_player_set_attribute(handle->mm_handle, NULL, "display_surface_type", MM_DISPLAY_SURFACE_NULL, (char *)NULL);
		if (ret != MM_ERROR_NONE)
			LOGW("[%s] Failed to set display surface type 'MM_DISPLAY_SURFACE_NULL' (0x%x)", __FUNCTION__, ret);
	} else {
		ret = mm_player_get_attribute(handle->mm_handle, NULL, "display_visible", &visible, (char *)NULL);
		if (ret != MM_ERROR_NONE)
			return __player_convert_error_code(ret, (char *)__FUNCTION__);

		if (!visible)
			value = FALSE;
		else
			value = TRUE;

		mm_player_set_attribute(handle->mm_handle, NULL, "display_visible", value, (char *)NULL);

		if (ret != MM_ERROR_NONE)
			LOGW("[%s] Failed to set display display_visible '0' (0x%x)", __FUNCTION__, ret);
	}

	ret = mm_player_realize(handle->mm_handle);
	if (ret != MM_ERROR_NONE) {
		LOGE("[%s] Failed to realize - 0x%x", __FUNCTION__, ret);
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	}

	if (!handle->is_progressive_download)
		ret = mm_player_pause(handle->mm_handle);

	if (ret != MM_ERROR_NONE) {
		int uret;
		uret = mm_player_unrealize(handle->mm_handle);
		if (uret != MM_ERROR_NONE)
			LOGE("[%s] Failed to unrealize - 0x%x", __FUNCTION__, uret);

		LOGE("[%s] Failed to pause - 0x%x", __FUNCTION__, ret);
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		handle->state = PLAYER_STATE_READY;
		LOGI("[%s] End", __FUNCTION__);
		PLAYER_TRACE_END();
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_unprepare(player_h player)
{
	LOGI("[%s] Start", __FUNCTION__);
	PLAYER_TRACE_BEGIN("MM:PLAYER:UNPREPARE");
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	__RELEASEIF_PREPARE_THREAD(handle->prepare_async_thread);

	int ret = mm_player_unrealize(handle->mm_handle);

	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK]) {
			handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK] = NULL;
			handle->user_data[MUSE_PLAYER_EVENT_TYPE_SEEK] = NULL;
		}

		if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PREPARE]) {
			handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PREPARE] = NULL;
			handle->user_data[MUSE_PLAYER_EVENT_TYPE_PREPARE] = NULL;
		}

		handle->state = PLAYER_STATE_IDLE;
		handle->display_type = PLAYER_DISPLAY_TYPE_NONE;
		handle->is_set_pixmap_cb = FALSE;
		handle->is_stopped = FALSE;
		handle->is_display_visible = TRUE;
		handle->is_progressive_download = FALSE;
		LOGI("[%s] End", __FUNCTION__);
		PLAYER_TRACE_END();
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_set_uri(player_h player, const char *uri)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(uri);
	player_s *handle = (player_s *)player;
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	handle->is_media_stream = FALSE;
	int ret = mm_player_set_uri(handle->mm_handle, uri);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_set_memory_buffer(player_h player, const void *data, int size)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(data);
	PLAYER_CHECK_CONDITION(size >= 0, PLAYER_ERROR_INVALID_PARAMETER, "PLAYER_ERROR_INVALID_PARAMETER");
	player_s *handle = (player_s *)player;
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	char uri[PATH_MAX];

	handle->is_media_stream = FALSE;
	snprintf(uri, sizeof(uri), "mem:///ext=%s,size=%d", "", size);
	int ret = mm_player_set_attribute(handle->mm_handle, NULL, MM_PLAYER_CONTENT_URI, uri, strlen(uri), MM_PLAYER_MEMORY_SRC, data, size, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_get_state(player_h player, player_state_e *state)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(state);
	player_s *handle = (player_s *)player;
	*state = handle->state;
	MMPlayerStateType currentStat = MM_PLAYER_STATE_NULL;
	mm_player_get_state(handle->mm_handle, &currentStat);
	/* LOGI("[%s] State : %d (FW state : %d)", __FUNCTION__,handle->state, currentStat); */
	return PLAYER_ERROR_NONE;
}

int legacy_player_set_volume(player_h player, float left, float right)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_CHECK_CONDITION(left >= 0 && left <= 1.0, PLAYER_ERROR_INVALID_PARAMETER, "PLAYER_ERROR_INVALID_PARAMETER");
	PLAYER_CHECK_CONDITION(right >= 0 && right <= 1.0, PLAYER_ERROR_INVALID_PARAMETER, "PLAYER_ERROR_INVALID_PARAMETER");
	player_s *handle = (player_s *)player;
	MMPlayerVolumeType vol;
	vol.level[MM_VOLUME_CHANNEL_LEFT] = left;
	vol.level[MM_VOLUME_CHANNEL_RIGHT] = right;
	int ret = mm_player_set_volume(handle->mm_handle, &vol);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_get_volume(player_h player, float *left, float *right)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(left);
	PLAYER_NULL_ARG_CHECK(right);
	player_s *handle = (player_s *)player;
	MMPlayerVolumeType vol;
	int ret = mm_player_get_volume(handle->mm_handle, &vol);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		*left = vol.level[MM_VOLUME_CHANNEL_LEFT];
		*right = vol.level[MM_VOLUME_CHANNEL_RIGHT];
		return PLAYER_ERROR_NONE;
	}
}
#ifdef PLAYER_ASM_COMPATIBILITY
int legacy_player_set_sound_type(player_h player, sound_type_e type)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	char *stream_type = NULL;
	int stream_index = -1;
	int ret = 0;
	int pid = -1;
	int session_type = 0;
	int session_flags = 0;

	ret = mm_player_get_client_pid (handle->mm_handle, &pid);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	}

	/* read session information */
	ret = _mm_session_util_read_information(pid, &session_type, &session_flags);
	if (ret == MM_ERROR_NONE) {
		/* in this case, this process is using stream info created by using sound-manager,
		 * we're going to skip working on backward compatibility of session. */
		if (session_type == MM_SESSION_TYPE_REPLACED_BY_STREAM)
			return PLAYER_ERROR_SOUND_POLICY;
	} else if (ret == MM_ERROR_INVALID_HANDLE) { /* if there is no session */
		/* convert volume_type to stream_type */
		switch(type) {
		case SOUND_TYPE_SYSTEM:
			stream_type = "system";
			break;
		case SOUND_TYPE_NOTIFICATION:
			stream_type = "notification";
			break;
		case SOUND_TYPE_ALARM:
			stream_type = "alarm";
			break;
		case SOUND_TYPE_RINGTONE:
			stream_type = "ringtone-voip";
			break;
		case SOUND_TYPE_MEDIA:
		case SOUND_TYPE_CALL:
			stream_type = "media";
			break;
		case SOUND_TYPE_VOIP:
			stream_type = "voip";
			break;
		case SOUND_TYPE_VOICE:
			stream_type = "voice-information";
			break;
		default:
			LOGW("check the value[%d].\n", type);
			return PLAYER_ERROR_INVALID_PARAMETER;
		}
		LOGI("[%s] sound type = %s", __FUNCTION__, stream_type);

		ret = mm_player_set_attribute(handle->mm_handle, NULL, "sound_stream_type", stream_type, strlen(stream_type), "sound_stream_index", stream_index, (char *)NULL);
		if (ret == MM_ERROR_NONE)
			return PLAYER_ERROR_NONE;
	}
	return __player_convert_error_code(ret, (char *)__FUNCTION__);
}
#endif
int legacy_player_set_audio_policy_info(player_h player, sound_stream_info_h stream_info)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	bool is_available = FALSE;

	/* check if stream_info is valid */
	int ret = sound_manager_is_available_stream_information(stream_info, NATIVE_API_PLAYER, &is_available);

	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		if (is_available == FALSE)
			ret = MM_ERROR_NOT_SUPPORT_API;
		else {
			char *stream_type = NULL;
			int stream_index = 0;
			ret = sound_manager_get_type_from_stream_information(stream_info, &stream_type);
			ret = sound_manager_get_index_from_stream_information(stream_info, &stream_index);
			if (ret == SOUND_MANAGER_ERROR_NONE)
				ret = mm_player_set_attribute(handle->mm_handle, NULL, "sound_stream_type", stream_type, strlen(stream_type), "sound_stream_index", stream_index, (char *)NULL);
			else
				ret = MM_ERROR_PLAYER_INTERNAL;
		}
	}

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_set_audio_latency_mode(player_h player, audio_latency_mode_e latency_mode)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	int ret = mm_player_set_attribute(handle->mm_handle, NULL, "sound_latency_mode", latency_mode, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_get_audio_latency_mode(player_h player, audio_latency_mode_e * latency_mode)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(latency_mode);
	player_s *handle = (player_s *)player;

	int ret = mm_player_get_attribute(handle->mm_handle, NULL, "sound_latency_mode", latency_mode, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_start(player_h player)
{
	LOGI("[%s] Start", __FUNCTION__);
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	int ret;
	if (handle->state == PLAYER_STATE_READY || handle->state == PLAYER_STATE_PAUSED) {
		if (handle->display_type == PLAYER_DISPLAY_TYPE_OVERLAY
#ifdef EVAS_RENDERER_SUPPORT
			|| handle->display_type == PLAYER_DISPLAY_TYPE_EVAS
#endif
			)
		{
			/* Apps can set display_rotation before creating videosink */
			/* Content which have orientation need to set display_rotation because player can get orientation from msg tag */
			int rotation;
			ret = mm_player_get_attribute(handle->mm_handle, NULL, MM_PLAYER_VIDEO_ROTATION, &rotation, (char *)NULL);
			if (ret != MM_ERROR_NONE)
				return __player_convert_error_code(ret, (char *)__FUNCTION__);

			ret = mm_player_set_attribute(handle->mm_handle, NULL, MM_PLAYER_VIDEO_ROTATION, rotation, (char *)NULL);
				if (ret != MM_ERROR_NONE)
					return __player_convert_error_code(ret, (char *)__FUNCTION__);

			if (handle->is_display_visible)
				ret = mm_player_set_attribute(handle->mm_handle, NULL, "display_visible", 1, (char *)NULL);
		}

		if (handle->is_stopped) {
			if (handle->is_progressive_download) {
				LOGE("[%s] PLAYER_ERROR_INVALID_OPERATION(0x%08x)", __FUNCTION__, PLAYER_ERROR_INVALID_OPERATION);
				return PLAYER_ERROR_INVALID_OPERATION;
			}

			ret = mm_player_start(handle->mm_handle);
			LOGI("[%s] stop -> start() ", __FUNCTION__);
		} else {
			if (handle->is_progressive_download && handle->state == PLAYER_STATE_READY)
				ret = mm_player_start(handle->mm_handle);
			else
				ret = mm_player_resume(handle->mm_handle);
		}
	} else {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		handle->is_stopped = FALSE;
		handle->state = PLAYER_STATE_PLAYING;
		LOGI("[%s] End", __FUNCTION__);
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_stop(player_h player)
{
	LOGI("[%s] Start", __FUNCTION__);
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	if (handle->state == PLAYER_STATE_PLAYING || handle->state == PLAYER_STATE_PAUSED) {
		int ret = mm_player_stop(handle->mm_handle);

		if (handle->display_type == PLAYER_DISPLAY_TYPE_OVERLAY
#ifdef EVAS_RENDERER_SUPPORT
			|| handle->display_type == PLAYER_DISPLAY_TYPE_EVAS
#endif
			) {
			ret = mm_player_set_attribute(handle->mm_handle, NULL, "display_visible", 0, (char *)NULL);
		}

		if (ret != MM_ERROR_NONE) {
			return __player_convert_error_code(ret, (char *)__FUNCTION__);
		} else {
			if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK]) {
				handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK] = NULL;
				handle->user_data[MUSE_PLAYER_EVENT_TYPE_SEEK] = NULL;
			}

			handle->state = PLAYER_STATE_READY;
			handle->is_stopped = TRUE;
			LOGI("[%s] End", __FUNCTION__);
			return PLAYER_ERROR_NONE;
		}
	} else {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
}

int legacy_player_pause(player_h player)
{
	LOGI("[%s] Start", __FUNCTION__);
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_PLAYING);

	int ret = mm_player_pause(handle->mm_handle);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		handle->state = PLAYER_STATE_PAUSED;
		LOGI("[%s] End", __FUNCTION__);
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_set_play_position(player_h player, int millisecond, bool accurate, player_seek_completed_cb callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_CHECK_CONDITION(millisecond >= 0, PLAYER_ERROR_INVALID_PARAMETER, "PLAYER_ERROR_INVALID_PARAMETER");
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK] && handle->is_media_stream == FALSE)
	{
		LOGE("[%s] PLAYER_ERROR_SEEK_FAILED temp (0x%08x) : seeking... we can't do any more ", __FUNCTION__, PLAYER_ERROR_SEEK_FAILED);
		return PLAYER_ERROR_SEEK_FAILED;
	} else {
		LOGI("[%s] Event type : %d, pos : %d ", __FUNCTION__, MUSE_PLAYER_EVENT_TYPE_SEEK, millisecond);
		handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK] = callback;
		handle->user_data[MUSE_PLAYER_EVENT_TYPE_SEEK] = user_data;
	}
	int accurated = accurate ? 1 : 0;
	int ret = mm_player_set_attribute(handle->mm_handle, NULL, "accurate_seek", accurated, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	ret = mm_player_set_position(handle->mm_handle, MM_PLAYER_POS_FORMAT_TIME, millisecond);
	if (ret != MM_ERROR_NONE) {
		handle->user_cb[MUSE_PLAYER_EVENT_TYPE_SEEK] = NULL;
		handle->user_data[MUSE_PLAYER_EVENT_TYPE_SEEK] = NULL;
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_get_play_position(player_h player, int *millisecond)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(millisecond);
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
	unsigned long pos;
	int ret = mm_player_get_position(handle->mm_handle, MM_PLAYER_POS_FORMAT_TIME, &pos);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		*millisecond = (int)pos;
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_set_mute(player_h player, bool muted)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	int ret = mm_player_set_mute(handle->mm_handle, muted);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_is_muted(player_h player, bool *muted)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(muted);
	player_s *handle = (player_s *)player;

	int _mute;
	int ret = mm_player_get_mute(handle->mm_handle, &_mute);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		if (_mute)
			*muted = TRUE;
		else
			*muted = FALSE;
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_set_looping(player_h player, bool looping)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	int value = 0;
	if (looping == TRUE)
		value = -1;

	int ret = mm_player_set_attribute(handle->mm_handle, NULL, MM_PLAYER_PLAYBACK_COUNT, value, (char *)NULL);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_is_looping(player_h player, bool *looping)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(looping);
	player_s *handle = (player_s *)player;
	int count;

	int ret = mm_player_get_attribute(handle->mm_handle, NULL, MM_PLAYER_PLAYBACK_COUNT, &count, (char *)NULL);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		if (count == -1)
			*looping = TRUE;
		else
			*looping = FALSE;

		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_get_duration(player_h player, int *duration)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(duration);
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
	int _duration;
	int ret = mm_player_get_attribute(handle->mm_handle, NULL, MM_PLAYER_CONTENT_DURATION, &_duration, (char *)NULL);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		*duration = _duration;
		/* LOGI("[%s] duration : %d",__FUNCTION__,_duration); */
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_set_display(player_h player, player_display_type_e type, player_display_h display)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	Evas_Object *obj = NULL;
	const char *object_type = NULL;
	void *set_handle = NULL;
	void *set_wl_display = NULL;
	Ecore_Wl_Window *wl_window = NULL;
	int wl_window_x = 0;
	int wl_window_y = 0;
	int wl_window_width = 0;
	int wl_window_height = 0;
	int ret;

	if (type != PLAYER_DISPLAY_TYPE_NONE && display == NULL) {
		LOGE("display type[%d] is not NONE, but display handle is NULL", type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	if (handle->is_set_pixmap_cb) {
		if (handle->state < PLAYER_STATE_READY) {
			/* just set below and go to "changing surface case" */
			handle->is_set_pixmap_cb = FALSE;
		} else {
			LOGE("[%s] pixmap callback was set, try it again after calling legacy_player_unprepare()", __FUNCTION__, PLAYER_ERROR_INVALID_OPERATION);
			LOGE("[%s] PLAYER_ERROR_INVALID_OPERATION(0x%08x)", __FUNCTION__, PLAYER_ERROR_INVALID_OPERATION);
			return PLAYER_ERROR_INVALID_OPERATION;
		}
	}

	void *temp = NULL;
	if (type == PLAYER_DISPLAY_TYPE_NONE) {
		/* NULL surface */
		handle->display_handle = 0;
		handle->display_type = PLAYER_DISPLAY_TYPE_NONE;
		set_handle = NULL;
	} else {
		/* get handle from overlay or evas surface */
		obj = (Evas_Object *) display;
		object_type = evas_object_type_get(obj);
		if (object_type) {
			temp = handle->display_handle;
			if (type == PLAYER_DISPLAY_TYPE_OVERLAY && !strcmp(object_type, "elm_win")) {
				/* wayland overlay surface */
				LOGI("Wayland overlay surface type");

				evas_object_geometry_get(obj, &wl_window_x, &wl_window_y, &wl_window_width, &wl_window_height);
				LOGI("get window rectangle: x(%d) y(%d) width(%d) height(%d)", wl_window_x, wl_window_y, wl_window_width, wl_window_height);

				wl_window = elm_win_wl_window_get(obj);

				/* get wl_surface */
				handle->display_handle = (void *)ecore_wl_window_surface_get(wl_window);
				set_handle = handle->display_handle;

				/* get wl_display */
				handle->wl_display = (void *)ecore_wl_display_get();
				set_wl_display = handle->wl_display;
			}
#ifdef EVAS_RENDERER_SUPPORT
			else if (type == PLAYER_DISPLAY_TYPE_EVAS && !strcmp(object_type, "image")) {
				/* evas object surface */
				LOGI("evas surface type");
				handle->display_handle = display;
				set_handle = display;
			}
#endif
			else {
				LOGE("invalid surface type");
				return PLAYER_ERROR_INVALID_PARAMETER;
			}
		} else {
			LOGE("falied to get evas object type from %p", obj);
			return PLAYER_ERROR_INVALID_PARAMETER;
		}
	}

	/* set display handle */
	if (handle->display_type == PLAYER_DISPLAY_TYPE_NONE || type == handle->display_type) {
		/* first time or same type */
		ret = mm_player_set_attribute(handle->mm_handle, NULL, "display_surface_type", __player_convet_display_type(type),
									  "use_wl_surface", TRUE,
									  "wl_display", set_wl_display, sizeof(void *),
									  "display_overlay", set_handle, sizeof(player_display_h), (char *)NULL);

		if (ret != MM_ERROR_NONE) {
			handle->display_handle = temp;
			LOGE("[%s] Failed to display surface change :%d", __FUNCTION__, ret);
		} else {
			if (type != PLAYER_DISPLAY_TYPE_NONE) {
				handle->display_type = type;
				LOGI("[%s] video display has been changed- type :%d, addr : 0x%x", __FUNCTION__, handle->display_type, handle->display_handle);
		} else
				LOGI("NULL surface");
		}
		ret = mm_player_set_attribute(handle->mm_handle, NULL, "wl_window_render_x", wl_window_x, "wl_window_render_y", wl_window_y, "wl_window_render_width", wl_window_width, "wl_window_render_height", wl_window_height, (char *)NULL);

		if (ret != MM_ERROR_NONE) {
			handle->display_handle = temp;
			LOGE("[%s] Failed to set wl_window render rectangle :%d", __FUNCTION__, ret);
		}
	} else {
		/* changing surface case */
		if (handle->state >= PLAYER_STATE_READY) {
			LOGE("[%s] it is not available to change display surface from %d to %d", __FUNCTION__, handle->display_type, type);
			return PLAYER_ERROR_INVALID_OPERATION;
		}
		ret = mm_player_change_videosink(handle->mm_handle, __player_convet_display_type(type), set_handle);
		if (ret != MM_ERROR_NONE) {
			handle->display_handle = temp;
			if (ret == MM_ERROR_NOT_SUPPORT_API) {
				LOGE("[%s] change video sink is not available.", __FUNCTION__);
				ret = PLAYER_ERROR_NONE;
			} else {
				LOGE("[%s] Failed to display surface change :%d", __FUNCTION__, ret);
			}
		} else {
			handle->display_type = type;
			LOGI("[%s] video display has been changed- type :%d, addr : 0x%x", __FUNCTION__, handle->display_type, handle->display_handle);
		}
	}

	if (ret != MM_ERROR_NONE) {
		handle->display_type = PLAYER_DISPLAY_TYPE_NONE;
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_set_display_mode(player_h player, player_display_mode_e mode)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	LOGI("[%s] mode:%d", __FUNCTION__, mode);

	int ret = mm_player_set_attribute(handle->mm_handle, NULL, "display_method", mode, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_get_display_mode(player_h player, player_display_mode_e *mode)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(mode);
	player_s *handle = (player_s *)player;
	int ret = mm_player_get_attribute(handle->mm_handle, NULL, "display_method", mode, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_set_playback_rate(player_h player, float rate)
{
	LOGI("[%s] rate : %0.1f", __FUNCTION__, rate);
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_CHECK_CONDITION(rate >= -5.0 && rate <= 5.0, PLAYER_ERROR_INVALID_PARAMETER, "PLAYER_ERROR_INVALID_PARAMETER");
	player_s *handle = (player_s *)player;

	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	int ret = mm_player_set_play_speed(handle->mm_handle, rate, FALSE);

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

int legacy_player_set_display_rotation(player_h player, player_display_rotation_e rotation)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	int ret = mm_player_set_attribute(handle->mm_handle, NULL, MM_PLAYER_VIDEO_ROTATION, rotation, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_get_display_rotation(player_h player, player_display_rotation_e *rotation)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(rotation);
	player_s *handle = (player_s *)player;
	int ret = mm_player_get_attribute(handle->mm_handle, NULL, MM_PLAYER_VIDEO_ROTATION, rotation, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_set_display_visible(player_h player, bool visible)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	int value = 0;
	if (visible == TRUE)
		value = 1;

	int ret = mm_player_set_attribute(handle->mm_handle, NULL, "display_visible", value, (char *)NULL);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		handle->is_display_visible = visible;
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_is_display_visible(player_h player, bool *visible)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(visible);
	player_s *handle = (player_s *)player;
	int count;
	int ret = mm_player_get_attribute(handle->mm_handle, NULL, "display_visible", &count, (char *)NULL);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		if (count == 0)
			*visible = FALSE;
		else
			*visible = TRUE;

		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_get_content_info(player_h player, player_content_info_e key, char **value)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(value);
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE (0x%08x) :  current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	char *attr = NULL;
	char *val = NULL;
	int val_len = 0;

	switch (key) {
	case PLAYER_CONTENT_INFO_ALBUM:
		attr = MM_PLAYER_TAG_ALBUM;
		break;
	case PLAYER_CONTENT_INFO_ARTIST:
		attr = MM_PLAYER_TAG_ARTIST;
		break;
	case PLAYER_CONTENT_INFO_AUTHOR:
		attr = MM_PLAYER_TAG_AUTHOUR;
		break;
	case PLAYER_CONTENT_INFO_GENRE:
		attr = MM_PLAYER_TAG_GENRE;
		break;
	case PLAYER_CONTENT_INFO_TITLE:
		attr = MM_PLAYER_TAG_TITLE;
		break;
	case PLAYER_CONTENT_INFO_YEAR:
		attr = MM_PLAYER_TAG_DATE;
		break;
	default:
		attr = NULL;
	}

	int ret = mm_player_get_attribute(handle->mm_handle, NULL, attr, &val, &val_len, (char *)NULL);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		*value = NULL;
		if (val != NULL)
			*value = strndup(val, val_len);
		else
			*value = strndup("", 0);

		if (*value == NULL) {
			LOGE("[%s] PLAYER_ERROR_OUT_OF_MEMORY(0x%08x) : fail to strdup ", __FUNCTION__, PLAYER_ERROR_OUT_OF_MEMORY);
			return PLAYER_ERROR_OUT_OF_MEMORY;
		}
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_get_codec_info(player_h player, char **audio_codec, char **video_codec)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(audio_codec);
	PLAYER_NULL_ARG_CHECK(video_codec);
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE (0x%08x) :  current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	char *audio = NULL;
	int audio_len = 0;
	char *video = NULL;
	int video_len = 0;

	int ret = mm_player_get_attribute(handle->mm_handle, NULL, MM_PLAYER_AUDIO_CODEC, &audio, &audio_len, MM_PLAYER_VIDEO_CODEC, &video, &video_len, (char *)NULL);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		*audio_codec = NULL;
		if (audio != NULL)
			*audio_codec = strndup(audio, audio_len);
		else
			*audio_codec = strndup("", 0);

		*video_codec = NULL;
		if (video != NULL)
			*video_codec = strndup(video, video_len);
		else
			*video_codec = strndup("", 0);

		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_get_audio_stream_info(player_h player, int *sample_rate, int *channel, int *bit_rate)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(sample_rate);
	PLAYER_NULL_ARG_CHECK(channel);
	PLAYER_NULL_ARG_CHECK(bit_rate);
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE (0x%08x) :  current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
	int ret = mm_player_get_attribute(handle->mm_handle, NULL, MM_PLAYER_AUDIO_SAMPLERATE, sample_rate, MM_PLAYER_AUDIO_CHANNEL, channel, MM_PLAYER_AUDIO_BITRATE, bit_rate, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	return PLAYER_ERROR_NONE;
}

int legacy_player_get_video_stream_info(player_h player, int *fps, int *bit_rate)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(fps);
	PLAYER_NULL_ARG_CHECK(bit_rate);
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE (0x%08x) :  current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
	int ret = mm_player_get_attribute(handle->mm_handle, NULL, "content_video_fps", fps, "content_video_bitrate", bit_rate, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	return PLAYER_ERROR_NONE;
}

int legacy_player_get_video_size(player_h player, int *width, int *height)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(width);
	PLAYER_NULL_ARG_CHECK(height);
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE (0x%08x) :  current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
	int w;
	int h;
	int ret = mm_player_get_attribute(handle->mm_handle, NULL, MM_PLAYER_VIDEO_WIDTH, &w, MM_PLAYER_VIDEO_HEIGHT, &h, (char *)NULL);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		*width = w;
		*height = h;
		LOGI("[%s] width : %d, height : %d", __FUNCTION__, w, h);
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_get_album_art(player_h player, void **album_art, int *size)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(size);
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE (0x%08x) :  current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	int ret = mm_player_get_attribute(handle->mm_handle, NULL, "tag_album_cover", album_art, size, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	return PLAYER_ERROR_NONE;
}

int legacy_player_audio_effect_get_equalizer_bands_count(player_h player, int *count)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(count);
	player_s *handle = (player_s *)player;

	int ret = mm_player_audio_effect_custom_get_eq_bands_number(handle->mm_handle, count);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_audio_effect_set_equalizer_all_bands(player_h player, int *band_levels, int length)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(band_levels);
	player_s *handle = (player_s *)player;

	int ret = mm_player_audio_effect_custom_set_level_eq_from_list(handle->mm_handle, band_levels, length);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		ret = mm_player_audio_effect_custom_apply(handle->mm_handle);
		return (ret == MM_ERROR_NONE) ? PLAYER_ERROR_NONE : __player_convert_error_code(ret, (char *)__FUNCTION__);
	}
}

int legacy_player_audio_effect_set_equalizer_band_level(player_h player, int index, int level)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	int ret = mm_player_audio_effect_custom_set_level(handle->mm_handle, MM_AUDIO_EFFECT_CUSTOM_EQ, index, level);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		ret = mm_player_audio_effect_custom_apply(handle->mm_handle);
		return (ret == MM_ERROR_NONE) ? PLAYER_ERROR_NONE : __player_convert_error_code(ret, (char *)__FUNCTION__);
	}
}

int legacy_player_audio_effect_get_equalizer_band_level(player_h player, int index, int *level)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(level);
	player_s *handle = (player_s *)player;

	int ret = mm_player_audio_effect_custom_get_level(handle->mm_handle, MM_AUDIO_EFFECT_CUSTOM_EQ, index, level);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_audio_effect_get_equalizer_level_range(player_h player, int *min, int *max)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(min);
	PLAYER_NULL_ARG_CHECK(max);
	player_s *handle = (player_s *)player;

	int ret = mm_player_audio_effect_custom_get_level_range(handle->mm_handle, MM_AUDIO_EFFECT_CUSTOM_EQ, min, max);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_audio_effect_get_equalizer_band_frequency(player_h player, int index, int *frequency)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(frequency);
	player_s *handle = (player_s *)player;

	int ret = mm_player_audio_effect_custom_get_eq_bands_freq(handle->mm_handle, index, frequency);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_audio_effect_get_equalizer_band_frequency_range(player_h player, int index, int *range)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(range);
	player_s *handle = (player_s *)player;

	int ret = mm_player_audio_effect_custom_get_eq_bands_width(handle->mm_handle, index, range);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_audio_effect_equalizer_clear(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	int ret = mm_player_audio_effect_custom_clear_eq_all(handle->mm_handle);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		ret = mm_player_audio_effect_custom_apply(handle->mm_handle);
		return (ret == MM_ERROR_NONE) ? PLAYER_ERROR_NONE : __player_convert_error_code(ret, (char *)__FUNCTION__);
	}
}

int legacy_player_audio_effect_equalizer_is_available(player_h player, bool *available)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(available);
	player_s *handle = (player_s *)player;

	int ret = mm_player_is_supported_custom_effect_type(handle->mm_handle, MM_AUDIO_EFFECT_CUSTOM_EQ);
	if (ret != MM_ERROR_NONE)
		*available = FALSE;
	else
		*available = TRUE;
	return PLAYER_ERROR_NONE;
}

int legacy_player_set_subtitle_path(player_h player, const char *path)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	if ((path == NULL) && (handle->state != PLAYER_STATE_IDLE))
		return PLAYER_ERROR_INVALID_PARAMETER;

	int ret = mm_player_set_external_subtitle_path(handle->mm_handle, path);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_set_subtitle_position_offset(player_h player, int millisecond)
{
	PLAYER_INSTANCE_CHECK(player);
	/* PLAYER_CHECK_CONDITION(millisecond>=0  ,PLAYER_ERROR_INVALID_PARAMETER ,"PLAYER_ERROR_INVALID_PARAMETER" ); */
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_PLAYING)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	int ret = mm_player_adjust_subtitle_position(handle->mm_handle, MM_PLAYER_POS_FORMAT_TIME, millisecond);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_set_progressive_download_path(player_h player, const char *path)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(path);
	if (!_player_network_availability_check())
		return PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE;

	player_s *handle = (player_s *)player;
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	int ret = mm_player_set_attribute(handle->mm_handle, NULL, "pd_mode", MM_PLAYER_PD_MODE_URI, "pd_location", path, strlen(path), (char *)NULL);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		handle->is_progressive_download = 1;
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_get_progressive_download_status(player_h player, unsigned long *current, unsigned long *total_size)
{

	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(current);
	PLAYER_NULL_ARG_CHECK(total_size);
	player_s *handle = (player_s *)player;
	if (handle->state != PLAYER_STATE_PLAYING && handle->state != PLAYER_STATE_PAUSED) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
	guint64 _current;
	guint64 _total;
	int ret = mm_player_get_pd_status(handle->mm_handle, &_current, &_total);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		*current = _current;
		*total_size = _total;
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_capture_video(player_h player, player_video_captured_cb callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);

	player_s *handle = (player_s *)player;
	if (handle->user_cb[MUSE_PLAYER_EVENT_TYPE_CAPTURE]) {
		LOGE("[%s] PLAYER_ERROR_VIDEO_CAPTURE_FAILED (0x%08x) : capturing... we can't do any more ", __FUNCTION__, PLAYER_ERROR_VIDEO_CAPTURE_FAILED);
		return PLAYER_ERROR_VIDEO_CAPTURE_FAILED;
	} else {
		LOGI("[%s] Event type : %d ", __FUNCTION__, MUSE_PLAYER_EVENT_TYPE_CAPTURE);
		handle->user_cb[MUSE_PLAYER_EVENT_TYPE_CAPTURE] = callback;
		handle->user_data[MUSE_PLAYER_EVENT_TYPE_CAPTURE] = user_data;
	}

	if (handle->state >= PLAYER_STATE_READY) {
		int ret = mm_player_do_video_capture(handle->mm_handle);
		if (ret == MM_ERROR_PLAYER_NO_OP) {
			handle->user_cb[MUSE_PLAYER_EVENT_TYPE_CAPTURE] = NULL;
			handle->user_data[MUSE_PLAYER_EVENT_TYPE_CAPTURE] = NULL;
			LOGE("[%s] PLAYER_ERROR_INVALID_OPERATION (0x%08x) : video display must be set : %d", __FUNCTION__, PLAYER_ERROR_INVALID_OPERATION, handle->display_type);
			return PLAYER_ERROR_INVALID_OPERATION;
		}
		if (ret != MM_ERROR_NONE) {
			handle->user_cb[MUSE_PLAYER_EVENT_TYPE_CAPTURE] = NULL;
			handle->user_data[MUSE_PLAYER_EVENT_TYPE_CAPTURE] = NULL;
			return __player_convert_error_code(ret, (char *)__FUNCTION__);
		} else
			return PLAYER_ERROR_NONE;
	} else {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE (0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		handle->user_cb[MUSE_PLAYER_EVENT_TYPE_CAPTURE] = NULL;
		handle->user_data[MUSE_PLAYER_EVENT_TYPE_CAPTURE] = NULL;
		return PLAYER_ERROR_INVALID_STATE;
	}
}

int legacy_player_set_streaming_cookie(player_h player, const char *cookie, int size)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(cookie);
	PLAYER_CHECK_CONDITION(size >= 0, PLAYER_ERROR_INVALID_PARAMETER, "PLAYER_ERROR_INVALID_PARAMETER");
	player_s *handle = (player_s *)player;
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	int ret = mm_player_set_attribute(handle->mm_handle, NULL, "streaming_cookie", cookie, size, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_set_streaming_user_agent(player_h player, const char *user_agent, int size)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(user_agent);
	PLAYER_CHECK_CONDITION(size >= 0, PLAYER_ERROR_INVALID_PARAMETER, "PLAYER_ERROR_INVALID_PARAMETER");
	player_s *handle = (player_s *)player;
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	int ret = mm_player_set_attribute(handle->mm_handle, NULL, "streaming_user_agent", user_agent, size, (char *)NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_get_streaming_download_progress(player_h player, int *start, int *current)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(start);
	PLAYER_NULL_ARG_CHECK(current);
	player_s *handle = (player_s *)player;
	if (handle->state != PLAYER_STATE_PLAYING && handle->state != PLAYER_STATE_PAUSED) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
	unsigned long _current = 0;
	unsigned long _start = 0;
	int ret = mm_player_get_buffer_position(handle->mm_handle, MM_PLAYER_POS_FORMAT_PERCENT, &_start, &_current);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		*start = (int)_start;
		*current = (int)_current;
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_set_completed_cb(player_h player, player_completed_cb callback, void *user_data)
{
	return __set_callback(MUSE_PLAYER_EVENT_TYPE_COMPLETE, player, callback, user_data);
}

int legacy_player_unset_completed_cb(player_h player)
{
	return __unset_callback(MUSE_PLAYER_EVENT_TYPE_COMPLETE, player);
}

int legacy_player_set_interrupted_cb(player_h player, player_interrupted_cb callback, void *user_data)
{
	return __set_callback(MUSE_PLAYER_EVENT_TYPE_INTERRUPT, player, callback, user_data);
}

int legacy_player_unset_interrupted_cb(player_h player)
{
	return __unset_callback(MUSE_PLAYER_EVENT_TYPE_INTERRUPT, player);
}

int legacy_player_set_error_cb(player_h player, player_error_cb callback, void *user_data)
{
	return __set_callback(MUSE_PLAYER_EVENT_TYPE_ERROR, player, callback, user_data);
}

int legacy_player_unset_error_cb(player_h player)
{
	return __unset_callback(MUSE_PLAYER_EVENT_TYPE_ERROR, player);
}

int legacy_player_set_buffering_cb(player_h player, player_buffering_cb callback, void *user_data)
{
	return __set_callback(MUSE_PLAYER_EVENT_TYPE_BUFFERING, player, callback, user_data);
}

int legacy_player_unset_buffering_cb(player_h player)
{
	return __unset_callback(MUSE_PLAYER_EVENT_TYPE_BUFFERING, player);
}

int legacy_player_set_subtitle_updated_cb(player_h player, player_subtitle_updated_cb callback, void *user_data)
{
	return __set_callback(MUSE_PLAYER_EVENT_TYPE_SUBTITLE, player, callback, user_data);
}

int legacy_player_unset_subtitle_updated_cb(player_h player)
{
	return __unset_callback(MUSE_PLAYER_EVENT_TYPE_SUBTITLE, player);
}

int legacy_player_set_progressive_download_message_cb(player_h player, player_pd_message_cb callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	if (!_player_network_availability_check())
		return PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE;

	player_s *handle = (player_s *)player;

	if (handle->state != PLAYER_STATE_IDLE && handle->state != PLAYER_STATE_READY) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	int ret = mm_player_set_pd_message_callback(handle->mm_handle, __pd_message_callback, (void *)handle);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PD] = callback;
	handle->user_data[MUSE_PLAYER_EVENT_TYPE_PD] = user_data;
	LOGI("[%s] Event type : %d ", __FUNCTION__, MUSE_PLAYER_EVENT_TYPE_PD);
	return PLAYER_ERROR_NONE;
}

int legacy_player_unset_progressive_download_message_cb(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	handle->user_cb[MUSE_PLAYER_EVENT_TYPE_PD] = NULL;
	handle->user_data[MUSE_PLAYER_EVENT_TYPE_PD] = NULL;
	LOGI("[%s] Event type : %d ", __FUNCTION__, MUSE_PLAYER_EVENT_TYPE_PD);

	int ret = mm_player_set_pd_message_callback(handle->mm_handle, NULL, NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_set_media_packet_video_frame_decoded_cb(player_h player, player_media_packet_video_decoded_cb callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);

	player_s *handle = (player_s *)player;
	if (handle->state != PLAYER_STATE_IDLE) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	mm_player_enable_media_packet_video_stream(handle->mm_handle, TRUE);

	int ret = mm_player_set_video_stream_callback(handle->mm_handle, __video_stream_callback, (void *)handle);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	handle->user_cb[MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME] = callback;
	handle->user_data[MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME] = user_data;
	LOGI("Event type : %d ", MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME);

	return PLAYER_ERROR_NONE;
}

int legacy_player_unset_media_packet_video_frame_decoded_cb(player_h player)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	handle->user_cb[MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME] = NULL;
	handle->user_data[MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME] = NULL;

	LOGI("Event type : %d ", MUSE_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME);

	int ret = mm_player_set_video_stream_callback(handle->mm_handle, NULL, NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

static bool __video_stream_changed_callback(void *user_data)
{
	player_s *handle = (player_s *)user_data;
	muse_player_event_e event_type = MUSE_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED;

	LOGE("[%s] event type %d", __FUNCTION__, event_type);

	if (handle->user_cb[event_type]) {
		int width = 0, height = 0, fps = 0, bit_rate = 0;
		int ret = mm_player_get_attribute(handle->mm_handle, NULL,
										  MM_PLAYER_VIDEO_WIDTH, &width,
										  MM_PLAYER_VIDEO_HEIGHT, &height,
										  "content_video_fps", &fps,
										  "content_video_bitrate", &bit_rate, (char *)NULL);

		if (ret != MM_ERROR_NONE) {
			LOGE("[%s] get attr is failed", __FUNCTION__);
			return FALSE;
		}

		((player_video_stream_changed_cb)handle->user_cb[event_type])(width, height, fps, bit_rate, handle->user_data[event_type]);
	} else {
		LOGE("[%s] video stream changed cb was not set.", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

int legacy_player_set_video_stream_changed_cb(player_h player, player_video_stream_changed_cb callback, void *user_data)
{
	int ret;
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	player_s *handle = (player_s *)player;

	if (handle->state != PLAYER_STATE_IDLE) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	ret = mm_player_set_video_stream_changed_callback(handle->mm_handle, (mm_player_stream_changed_callback)__video_stream_changed_callback, (void *)handle);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	return __set_callback(MUSE_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED, player, callback, user_data);
}

int legacy_player_unset_video_stream_changed_cb(player_h player)
{
	int ret;
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	__unset_callback(MUSE_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED, player);

	ret = mm_player_set_video_stream_changed_callback(handle->mm_handle, NULL, NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

static bool __media_stream_buffer_status_callback(player_stream_type_e type, player_media_stream_buffer_status_e status, unsigned long long bytes, void *user_data)
{
	player_s *handle = (player_s *)user_data;
	muse_player_event_e event_type;

	if (type == PLAYER_STREAM_TYPE_AUDIO)
		event_type = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS;
	else if (type == PLAYER_STREAM_TYPE_VIDEO)
		event_type = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS;
	else
		return FALSE;

	LOGE("[%s] event type %d", __FUNCTION__, event_type);

	if (handle->user_cb[event_type]) {
		((player_media_stream_buffer_status_cb)handle->user_cb[event_type])(status, handle->user_data[event_type]);
	} else {
		LOGE("[%s][type:%d] buffer status cb was not set.", __FUNCTION__, type);
		return FALSE;
	}

	return TRUE;
}

static bool __media_stream_seek_data_callback(player_stream_type_e type, unsigned long long offset, void *user_data)
{
	player_s *handle = (player_s *)user_data;
	muse_player_event_e event_type;

	if (type == PLAYER_STREAM_TYPE_AUDIO)
		event_type = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK;
	else if (type == PLAYER_STREAM_TYPE_VIDEO)
		event_type = MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK;
	else
		return FALSE;

	LOGE("[%s] event type %d", __FUNCTION__, event_type);

	if (handle->user_cb[event_type]) {
		((player_media_stream_seek_cb)handle->user_cb[event_type])(offset, handle->user_data[event_type]);
	} else {
		LOGE("[%s][type:%d] seek cb was not set.", __FUNCTION__, type);
		return FALSE;
	}

	return TRUE;
}

int legacy_player_set_media_stream_buffer_status_cb(player_h player, player_stream_type_e type, player_media_stream_buffer_status_cb callback, void *user_data)
{
	int ret;
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	player_s *handle = (player_s *)player;

	if (handle->state != PLAYER_STATE_IDLE) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
	/* the type can be expaned with default and text. */
	if ((type != PLAYER_STREAM_TYPE_VIDEO) && (type != PLAYER_STREAM_TYPE_AUDIO)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_PARAMETER(type : %d)", __FUNCTION__, type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	ret = mm_player_set_media_stream_buffer_status_callback(handle->mm_handle, type, (mm_player_media_stream_buffer_status_callback)__media_stream_buffer_status_callback, (void *)handle);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	if (type == PLAYER_STREAM_TYPE_VIDEO)
		return __set_callback(MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS, player, callback, user_data);
	else
		return __set_callback(MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS, player, callback, user_data);
}

int legacy_player_unset_media_stream_buffer_status_cb(player_h player, player_stream_type_e type)
{
	int ret;
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	if (type == PLAYER_STREAM_TYPE_VIDEO)
		__unset_callback(MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS, player);
	else if (type == PLAYER_STREAM_TYPE_AUDIO)
		__unset_callback(MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS, player);
	else
		return PLAYER_ERROR_INVALID_PARAMETER;

	ret = mm_player_set_media_stream_buffer_status_callback(handle->mm_handle, type, NULL, NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_set_media_stream_seek_cb(player_h player, player_stream_type_e type, player_media_stream_seek_cb callback, void *user_data)
{
	int ret;
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	player_s *handle = (player_s *)player;

	if (handle->state != PLAYER_STATE_IDLE) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
	/* the type can be expaned with default and text. */
	if ((type != PLAYER_STREAM_TYPE_VIDEO) && (type != PLAYER_STREAM_TYPE_AUDIO)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_PARAMETER(type : %d)", __FUNCTION__, type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	ret = mm_player_set_media_stream_seek_data_callback(handle->mm_handle, type, (mm_player_media_stream_seek_data_callback)__media_stream_seek_data_callback, (void *)handle);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	if (type == PLAYER_STREAM_TYPE_VIDEO)
		return __set_callback(MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK, player, callback, user_data);
	else
		return __set_callback(MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK, player, callback, user_data);
}

int legacy_player_unset_media_stream_seek_cb(player_h player, player_stream_type_e type)
{
	int ret;
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	if (type == PLAYER_STREAM_TYPE_VIDEO)
		__unset_callback(MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK, player);
	else if (type == PLAYER_STREAM_TYPE_AUDIO)
		__unset_callback(MUSE_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK, player);
	else
		return PLAYER_ERROR_INVALID_PARAMETER;

	ret = mm_player_set_media_stream_seek_data_callback(handle->mm_handle, type, NULL, NULL);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_push_media_stream(player_h player, media_packet_h packet)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;

	PLAYER_CHECK_CONDITION(handle->error_code == PLAYER_ERROR_NONE, PLAYER_ERROR_NOT_SUPPORTED_FILE, "can't support this format");

	int ret = mm_player_submit_packet(handle->mm_handle, packet);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_set_media_stream_info(player_h player, player_stream_type_e type, media_format_h format)
{
	int ret;
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	handle->is_media_stream = TRUE;

	if (type == PLAYER_STREAM_TYPE_VIDEO)
		ret = mm_player_set_video_info(handle->mm_handle, format);
	else if (type == PLAYER_STREAM_TYPE_AUDIO)
		ret = mm_player_set_audio_info(handle->mm_handle, format);
	else
		return PLAYER_ERROR_INVALID_PARAMETER;

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;

	return PLAYER_ERROR_NONE;
}

int legacy_player_set_media_stream_buffer_max_size(player_h player, player_stream_type_e type, unsigned long long max_size)
{
	int ret;
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_IDLE)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	ret = mm_player_set_media_stream_buffer_max_size(handle->mm_handle, type, max_size);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_get_media_stream_buffer_max_size(player_h player, player_stream_type_e type, unsigned long long *max_size)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(max_size);
	player_s *handle = (player_s *)player;

	unsigned long long _max_size;
	int ret = mm_player_get_media_stream_buffer_max_size(handle->mm_handle, type, &_max_size);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		*max_size = _max_size;
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_set_media_stream_buffer_min_threshold(player_h player, player_stream_type_e type, unsigned int percent)
{
	int ret;
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_IDLE)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	ret = mm_player_set_media_stream_buffer_min_percent(handle->mm_handle, type, percent);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_get_media_stream_buffer_min_threshold(player_h player, player_stream_type_e type, unsigned int *percent)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(percent);
	player_s *handle = (player_s *)player;

	unsigned int _value;
	int ret = mm_player_get_media_stream_buffer_min_percent(handle->mm_handle, type, &_value);
	if (ret != MM_ERROR_NONE) {
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		*percent = _value;
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_get_track_count(player_h player, player_stream_type_e type, int *count)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(count);
	player_s *handle = (player_s *)player;

	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE (0x%08x) :  current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	MMPlayerTrackType track_type = 0;
	switch (type) {
	case PLAYER_STREAM_TYPE_AUDIO:
		track_type = MM_PLAYER_TRACK_TYPE_AUDIO;
		break;
	case PLAYER_STREAM_TYPE_TEXT:
		track_type = MM_PLAYER_TRACK_TYPE_TEXT;
		break;
	default:
		LOGE("invalid stream type %d", type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	int ret = mm_player_get_track_count(handle->mm_handle, track_type, count);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	return PLAYER_ERROR_NONE;
}

int legacy_player_get_current_track(player_h player, player_stream_type_e type, int *index)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(index);
	player_s *handle = (player_s *)player;

	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE (0x%08x) :  current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	MMPlayerTrackType track_type = 0;
	switch (type) {
	case PLAYER_STREAM_TYPE_AUDIO:
		track_type = MM_PLAYER_TRACK_TYPE_AUDIO;
		break;
	case PLAYER_STREAM_TYPE_TEXT:
		track_type = MM_PLAYER_TRACK_TYPE_TEXT;
		break;
	default:
		LOGE("invalid stream type %d", type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	int ret = mm_player_get_current_track(handle->mm_handle, track_type, index);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	return PLAYER_ERROR_NONE;
}

int legacy_player_select_track(player_h player, player_stream_type_e type, int index)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_CHECK_CONDITION(index >= 0, PLAYER_ERROR_INVALID_PARAMETER, "PLAYER_ERROR_INVALID_PARAMETER");
	player_s *handle = (player_s *)player;

	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE (0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	MMPlayerTrackType track_type = 0;
	switch (type) {
	case PLAYER_STREAM_TYPE_AUDIO:
		track_type = MM_PLAYER_TRACK_TYPE_AUDIO;
		break;
	case PLAYER_STREAM_TYPE_TEXT:
		track_type = MM_PLAYER_TRACK_TYPE_TEXT;
		break;
	default:
		LOGE("invalid stream type %d", type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	int ret = mm_player_select_track(handle->mm_handle, track_type, index);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	return PLAYER_ERROR_NONE;
}

int legacy_player_get_track_language_code(player_h player, player_stream_type_e type, int index, char **code)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(code);
	player_s *handle = (player_s *)player;
	if (!__player_state_validate(handle, PLAYER_STATE_READY)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE (0x%08x) :  current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}

	char *language_code = NULL;
	MMPlayerTrackType track_type = 0;
	switch (type) {
	case PLAYER_STREAM_TYPE_AUDIO:
		track_type = MM_PLAYER_TRACK_TYPE_AUDIO;
		break;
	case PLAYER_STREAM_TYPE_VIDEO:
		track_type = MM_PLAYER_TRACK_TYPE_VIDEO;
		break;
	case PLAYER_STREAM_TYPE_TEXT:
		track_type = MM_PLAYER_TRACK_TYPE_TEXT;
		break;
	default:
		LOGE("invalid stream type %d", type);
		return PLAYER_ERROR_INVALID_PARAMETER;
	}

	int ret = mm_player_get_track_language_code(handle->mm_handle, track_type, index, &language_code);
	if (ret != MM_ERROR_NONE) {
		if (language_code != NULL)
			free(language_code);

		language_code = NULL;
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		int code_len = 0;
		*code = NULL;
		if (language_code != NULL && strncmp(language_code, "und", 3)) {
			code_len = 2;
			*code = strndup(language_code, code_len);
		} else {
			code_len = 3;
			*code = strndup("und", code_len);
		}

		if (language_code)
			free(language_code);

		language_code = NULL;
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_resize_video_render_rect(player_h player, int x, int y, int w, int h)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	int ret;

	LOGD("<Enter>");

	ret = mm_player_set_attribute(handle->mm_handle, NULL, "wl_window_render_x", x, "wl_window_render_y", y, "wl_window_render_width", w, "wl_window_render_height", h, (char *)NULL);

	if (ret != MM_ERROR_NONE) {
		handle->display_type = PLAYER_DISPLAY_TYPE_NONE;
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_set_display_wl_for_mused(player_h player, player_display_type_e type, unsigned int wl_surface_id, int x, int y, int w, int h)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	void *set_handle = NULL;
	MMDisplaySurfaceType mmType = __player_convet_display_type(type);
	MMDisplaySurfaceType mmClientType = MM_DISPLAY_SURFACE_NULL;
	MMPlayerPipelineType mmPipelineType = MM_PLAYER_PIPELINE_SERVER;

	int ret;
	if (!__player_state_validate(handle, PLAYER_STATE_IDLE)) {
		LOGE("[%s] PLAYER_ERROR_INVALID_STATE(0x%08x) : current state - %d", __FUNCTION__, PLAYER_ERROR_INVALID_STATE, handle->state);
		return PLAYER_ERROR_INVALID_STATE;
	}
	if (handle->is_set_pixmap_cb) {
		if (handle->state < PLAYER_STATE_READY) {
			/* just set below and go to "changing surface case" */
			handle->is_set_pixmap_cb = FALSE;
		} else {
			LOGE("[%s] pixmap callback was set, try it again after calling legacy_player_unprepare()", __FUNCTION__, PLAYER_ERROR_INVALID_OPERATION);
			LOGE("[%s] PLAYER_ERROR_INVALID_OPERATION(0x%08x)", __FUNCTION__, PLAYER_ERROR_INVALID_OPERATION);
			return PLAYER_ERROR_INVALID_OPERATION;
		}
	}

	void *temp = NULL;
	if (type == PLAYER_DISPLAY_TYPE_NONE) {
		/* NULL surface */
		handle->display_handle = 0;
		handle->display_type = type;
		set_handle = NULL;
	} else {
		/* get handle from overlay or evas surface */
		temp = handle->display_handle;
		if (type == PLAYER_DISPLAY_TYPE_OVERLAY) {
			LOGI("Wayland overlay surface type");
			LOGI("wl_surface_id %d", wl_surface_id);
			handle->display_handle = (void *)(uintptr_t)wl_surface_id;
			set_handle = &(handle->display_handle);
			mmClientType = MM_DISPLAY_SURFACE_OVERLAY;
#ifdef EVAS_RENDERER_SUPPORT
		} else if (type == PLAYER_DISPLAY_TYPE_EVAS) {
			LOGI("Evas surface type");
			set_handle = &(handle->display_handle);
			mmClientType = MM_DISPLAY_SURFACE_REMOTE;
#endif
		} else {
			LOGE("invalid surface type");
			return PLAYER_ERROR_INVALID_PARAMETER;
		}
	}

	/* set display handle */
	if (handle->display_type == PLAYER_DISPLAY_TYPE_NONE || type == handle->display_type) {
		/* first time or same type */
		LOGW("first time or same type");
		ret = mm_player_set_attribute(handle->mm_handle, NULL, "display_surface_type", mmType, "display_surface_client_type", mmClientType, "display_overlay", set_handle, sizeof(wl_surface_id), "pipeline_type", mmPipelineType, NULL);

		if (ret != MM_ERROR_NONE) {
			handle->display_handle = temp;
			LOGE("[%s] Failed to display surface change :%d", __FUNCTION__, ret);
		} else {
			if (type != PLAYER_DISPLAY_TYPE_NONE) {
				handle->display_type = type;
				LOGI("[%s] video display has been changed- type :%d, addr : 0x%x", __FUNCTION__, handle->display_type, handle->display_handle);
			} else
				LOGI("NULL surface");
		}
		LOGI("get window rectangle: x(%d) y(%d) width(%d) height(%d)", x, y, w, h);
		ret = mm_player_set_attribute(handle->mm_handle, NULL, "wl_window_render_x", x, "wl_window_render_y", y, "wl_window_render_width", w, "wl_window_render_height", h, (char *)NULL);

		if (ret != MM_ERROR_NONE) {
			handle->display_handle = temp;
			LOGE("[%s] Failed to set wl_window render rectangle :%d", __FUNCTION__, ret);
		}
	} else {
		/* changing surface case */
		ret = mm_player_change_videosink(handle->mm_handle, mmType, set_handle);
		if (ret != MM_ERROR_NONE) {
			handle->display_handle = temp;
			if (ret == MM_ERROR_NOT_SUPPORT_API) {
				LOGE("[%s] change video sink is not available.", __FUNCTION__);
				ret = PLAYER_ERROR_NONE;
			} else {
				LOGE("[%s] Failed to display surface change :%d", __FUNCTION__, ret);
			}
		} else {
			handle->display_type = type;
			LOGI("[%s] video display has been changed- type :%d, addr : 0x%x", __FUNCTION__, handle->display_type, handle->display_handle);
		}
	}

	if (ret != MM_ERROR_NONE) {
		handle->display_type = PLAYER_DISPLAY_TYPE_NONE;
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	} else {
		return PLAYER_ERROR_NONE;
	}
}

int legacy_player_set_audio_policy_info_for_mused(player_h player, char *stream_type, int stream_index)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	int ret;

	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	ret = mm_player_set_attribute(handle->mm_handle, NULL, "sound_stream_type", stream_type, strlen(stream_type), "sound_stream_index", stream_index, NULL);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

int legacy_player_sound_register(player_h player, int pid)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	int ret;

	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	ret = mm_player_sound_register(handle->mm_handle, pid);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	return PLAYER_ERROR_NONE;
}

int legacy_player_get_timeout_for_muse(player_h player, int *timeout)
{
	PLAYER_INSTANCE_CHECK(player);
	player_s *handle = (player_s *)player;
	int ret;

	ret = mm_player_get_timeout(handle->mm_handle, timeout);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	return PLAYER_ERROR_NONE;
}

int legacy_player_get_num_of_video_out_buffers(player_h player, int *num, int *extra_num)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(num);
	PLAYER_NULL_ARG_CHECK(extra_num);
	player_s *handle = (player_s *)player;

	int ret = mm_player_get_num_of_video_out_buffers(handle->mm_handle, num, extra_num);
	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);

	return PLAYER_ERROR_NONE;
}

int legacy_player_set_file_buffering_path(player_h player, const char *file_path)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(file_path);
	player_s *handle = (player_s *)player;
	PLAYER_STATE_CHECK(handle, PLAYER_STATE_IDLE);

	int ret = mm_player_set_file_buffering_path(handle->mm_handle, file_path);

	if (ret != MM_ERROR_NONE)
		return __player_convert_error_code(ret, (char *)__FUNCTION__);
	else
		return PLAYER_ERROR_NONE;
}

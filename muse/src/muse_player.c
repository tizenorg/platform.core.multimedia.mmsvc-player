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
#include "muse_core.h"
#include "muse_core_ipc.h"
#include "muse_player.h"
#include "muse_player_msg.h"
#include "muse_player_api.h"
#include "legacy_player_private.h"
#include "legacy_player_internal.h"

static tbm_bufmgr bufmgr;

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



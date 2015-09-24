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
#include "player2_private.h"
#include "player_msg_private.h"
#include "player_internal.h"

int player_set_pcm_extraction_mode(player_h player,
		bool sync, player_audio_pcm_extraction_cb callback, void *user_data)
{
	PLAYER_INSTANCE_CHECK(player);
	PLAYER_NULL_ARG_CHECK(callback);
	int ret = PLAYER_ERROR_NONE;
	muse_player_api_e api = MUSE_PLAYER_API_SET_PCM_EXTRACTION_MODE;
	player_cli_s *pc = (player_cli_s *) player;
	char *ret_buf = NULL;
	_player_event_e event = _PLAYER_EVENT_TYPE_AUDIO_FRAME;

	LOGD("ENTER");

	player_msg_send1(api, pc, ret_buf, ret,
			INT, sync);

	if(ret == PLAYER_ERROR_NONE){
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
	player_cli_s *pc = (player_cli_s *) player;
	char *ret_buf = NULL;

	LOGD("ENTER");

	player_msg_send3(api, pc, ret_buf, ret,
			STRING, format, INT, samplerate, INT, channel);

	g_free(ret_buf);
	return ret;
}

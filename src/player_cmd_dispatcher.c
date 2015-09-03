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

#include "mm_types.h"
#include "mm_debug.h"
#include "mm_error.h"
#include "mm_player.h"
#include "mmsvc_core.h"
#include "mmsvc_core_ipc.h"
#include "player2_private.h"
#include "player_msg_private.h"

static int player_cmd_shutdown(Client client)
{
	intptr_t handle;
	player_state_e state;
	int ret;

	handle = mmsvc_core_ipc_get_handle(client);

	ret = player_get_state((player_h) handle, &state);

	if(ret != PLAYER_ERROR_NONE)
		return ret;

	switch(state) {
	case PLAYER_STATE_PLAYING:
		player_stop((player_h) handle);
		/* FALLTHROUGH */
	case PLAYER_STATE_PAUSED:
	case PLAYER_STATE_READY:
		player_unprepare((player_h) handle);
		/* FALLTHROUGH */
	case PLAYER_STATE_IDLE:
		player_destroy((player_h) handle);
		break;

	default:
		LOGD("Nothing to do");
		break;
	}

	return PLAYER_ERROR_NONE;
}

int (*cmd_dispatcher[MUSED_DOMAIN_EVENT_MAX]) (Client client) = {
	player_cmd_shutdown, /* MUSED_DOMAIN_EVENT_SHUTDOWN */
	NULL, /* MUSED_DOMAIN_EVENT_DEBUG_INFO_DUMP */
};

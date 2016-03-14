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

#include <dlog.h>
#include "muse_core.h"
#include "muse_core_ipc.h"
#include "muse_player.h"
#include "muse_player_api.h"
#include "legacy_player.h"

static int player_cmd_shutdown(muse_module_h module)
{
	intptr_t handle;
	player_state_e state;
	int ret;

	handle = muse_core_ipc_get_handle(module);

	ret = legacy_player_get_state((player_h)handle, &state);

	if (ret != PLAYER_ERROR_NONE)
		return ret;

	switch (state) {
	case PLAYER_STATE_PLAYING:
		legacy_player_stop((player_h)handle);
		/* FALLTHROUGH */
	case PLAYER_STATE_PAUSED:
	case PLAYER_STATE_READY:
		legacy_player_unprepare((player_h)handle);
		/* FALLTHROUGH */
	case PLAYER_STATE_IDLE:
		legacy_player_destroy((player_h)handle);
		break;

	default:
		LOGD("Nothing to do");
		break;
	}

	return PLAYER_ERROR_NONE;
}

int (*cmd_dispatcher[MUSE_MODULE_EVENT_MAX])(muse_module_h module) = {
	player_cmd_shutdown,	/* MUSE_MODULE_EVENT_SHUTDOWN */
	NULL,	/* MUSE_MODULE_EVENT_DEBUG_INFO_DUMP */
};

int (*dispatcher[MUSE_PLAYER_API_MAX])(muse_module_h module) = {
#include "muse_player_api.func"
};


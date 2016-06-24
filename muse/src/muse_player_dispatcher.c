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
#include <muse_core.h>
#include <muse_core_ipc.h>
#include "muse_player.h"
#include "muse_player_api.h" /* generated during build, ref ../make_api.py */
#include "muse_player_private.h"
#include "muse_player_msg.h"
#include "legacy_player.h"

static int player_cmd_shutdown(muse_module_h module)
{
	muse_player_handle_s *muse_player = NULL;
	player_state_e state;
	int ret;

	muse_player = (muse_player_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_player == NULL) {
		LOGE("handle is NULL.");
		return PLAYER_ERROR_NONE;
	}

	ret = legacy_player_get_state(muse_player->player_handle, &state);

	if (ret != PLAYER_ERROR_NONE)
		return ret;

	LOGW("player shutdown, state:%d", state);

	switch (state) {
	case PLAYER_STATE_PLAYING:
		legacy_player_stop(muse_player->player_handle);
		/* FALLTHROUGH */
	case PLAYER_STATE_PAUSED:
	case PLAYER_STATE_READY:
		legacy_player_unprepare(muse_player->player_handle);
		/* FALLTHROUGH */
	case PLAYER_STATE_IDLE:
		legacy_player_destroy(muse_player->player_handle);
		break;

	default:
		LOGD("Nothing to do");
		break;
	}

	return PLAYER_ERROR_NONE;
}

static int player_cmd_resouce_not_available(muse_module_h module)
{
	int ret = PLAYER_ERROR_RESOURCE_LIMIT;
	muse_player_api_e api = MUSE_PLAYER_API_CREATE;
	LOGD("return PLAYER_ERROR_RESOURCE_LIMIT");

	player_msg_return(api, ret, module);

	return PLAYER_ERROR_NONE;
}

int (*cmd_dispatcher[MUSE_MODULE_COMMAND_MAX])(muse_module_h module) = {
	NULL,	/* MUSE_MODULE_COMMAND_INITIALIZE */
	player_cmd_shutdown,	/* MUSE_MODULE_COMMAND_SHUTDOWN */
	NULL,	/* MUSE_MODULE_COMMAND_DEBUG_INFO_DUMP */
	NULL,	/* MUSE_MODULE_COMMAND_CREATE_SERVER_ACK */
	player_cmd_resouce_not_available,	/* MUSE_MODULE_COMMAND_RESOURCE_NOT_AVAILABLE */
};

/**
 * @brief Auto-generated dispatcher for the muse player APIs.
 * @details muse_player_api.func file which is a list of function name
 *          will be created during build time automatically like below.
 *          player_disp_create,
 *          player_disp_destroy,
 *          ...
 * @see ../README_FOR_NEW_API
 * @see ../api.list
 * @see ../make_api.py
 */
int (*dispatcher[MUSE_PLAYER_API_MAX])(muse_module_h module) = {
#include "muse_player_api.func"
};


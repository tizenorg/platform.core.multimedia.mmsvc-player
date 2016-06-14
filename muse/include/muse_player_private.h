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

#ifndef __TIZEN_MEDIA_MUSE_PLAYER_PRIVATE_H__
#define	__TIZEN_MEDIA_MUSE_PLAYER_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <tbm_bufmgr.h>
#include <muse_core.h>
#include "legacy_player.h"

typedef struct {
	player_h player_handle;
	tbm_bufmgr bufmgr;
	media_format_h audio_format;
	media_format_h video_format;
	GList *packet_list;
	GList *data_list;
	GMutex list_lock;
	int total_size_of_buffers;
	int extra_size_of_buffers;
} muse_player_handle_s;

typedef struct {
	tbm_bo bo;
	int key;
//	void *internal_buffer;
} muse_player_export_data_s;

typedef struct {
	player_h player;
	muse_module_h module;
} prepare_data_s;

#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_MEDIA_MUSE_PLAYER_PRIVATE_H__ */

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

#ifndef __TIZEN_MEDIA_PLAYER_2_PRIVATE_H__
#define	__TIZEN_MEDIA_PLAYER_2_PRIVATE_H__
#include "player.h"
#include "mm_types.h"
#include "mmsvc_core.h"
#include "mmsvc_core_ipc.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TIZEN_N_PLAYER"

#define PLAYER_CHECK_CONDITION(condition,error,msg)     \
                if(condition) {} else \
                { LOGE("[%s] %s(0x%08x)",__FUNCTION__, msg,error); return error;}; \

#define PLAYER_INSTANCE_CHECK(player)   \
        PLAYER_CHECK_CONDITION(player != NULL, PLAYER_ERROR_INVALID_PARAMETER,"PLAYER_ERROR_INVALID_PARAMETER")

#define PLAYER_STATE_CHECK(player,expected_state)       \
        PLAYER_CHECK_CONDITION(player->state == expected_state,PLAYER_ERROR_INVALID_STATE,"PLAYER_ERROR_INVALID_STATE")

#define PLAYER_NULL_ARG_CHECK(arg)      \
        PLAYER_CHECK_CONDITION(arg != NULL,PLAYER_ERROR_INVALID_PARAMETER,"PLAYER_ERROR_INVALID_PARAMETER")

#define PLAYER_RANGE_ARG_CHECK(arg, min, max)      \
        PLAYER_CHECK_CONDITION(arg <= max,PLAYER_ERROR_INVALID_PARAMETER,"PLAYER_ERROR_INVALID_PARAMETER")		\
        PLAYER_CHECK_CONDITION(arg >= min,PLAYER_ERROR_INVALID_PARAMETER,"PLAYER_ERROR_INVALID_PARAMETER")

#define MM_PLAYER_HEAD_GAP(api) ((api)/(1000)+(1))*(1000)

	typedef enum {
		MM_PLAYER_API_CREATE = API_CREATE,
		MM_PLAYER_API_DESTROY = API_DESTROY,
		MM_PLAYER_API_PREPARE,
		MM_PLAYER_API_PREPARE_ASYNC,
		MM_PLAYER_API_UNPREPARE,
		MM_PLAYER_API_SET_URI,
		MM_PLAYER_API_START,
		MM_PLAYER_API_STOP,
		MM_PLAYER_API_PAUSE,
		MM_PLAYER_API_SET_MEMORY_BUFFER,
		MM_PLAYER_API_DEINIT_MEMORY_BUFFER,
		MM_PLAYER_API_GET_STATE,
		MM_PLAYER_API_SET_VOLUME,
		MM_PLAYER_API_GET_VOLUME,
		MM_PLAYER_API_SET_SOUND_TYPE,
		MM_PLAYER_API_SET_AUDIO_POLICY_INFO,
		MM_PLAYER_API_SET_AUDIO_LATENCY_MODE,
		MM_PLAYER_API_GET_AUDIO_LATENCY_MODE,
		MM_PLAYER_API_SET_PLAY_POSITION,
		MM_PLAYER_API_GET_PLAY_POSITION,
		MM_PLAYER_API_SET_MUTE,
		MM_PLAYER_API_IS_MUTED,
		MM_PLAYER_API_SET_LOOPING,
		MM_PLAYER_API_IS_LOOPING,
		MM_PLAYER_API_GET_DURATION,
		MM_PLAYER_API_SET_DISPLAY,
		MM_PLAYER_API_SET_DISPLAY_MODE,
		MM_PLAYER_API_GET_DISPLAY_MODE,
		MM_PLAYER_API_SET_PLAYBACK_RATE,
		MM_PLAYER_API_SET_DISPLAY_ROTATION,
		MM_PLAYER_API_GET_DISPLAY_ROTATION,
		MM_PLAYER_API_SET_DISPLAY_VISIBLE,
		MM_PLAYER_API_IS_DISPLAY_VISIBLE,
		MM_PLAYER_API_GET_CONTENT_INFO,
		MM_PLAYER_API_GET_CODEC_INFO,
		MM_PLAYER_API_GET_AUDIO_STREAM_INFO,
		MM_PLAYER_API_GET_VIDEO_STREAM_INFO,
		MM_PLAYER_API_GET_VIDEO_SIZE,
		MM_PLAYER_API_GET_ALBUM_ART,
		MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BANDS_COUNT,
		MM_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_ALL_BANDS,
		MM_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_BAND_LEVEL,
		MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_LEVEL,
		MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_LEVEL_RANGE,
		MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY,
		MM_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY_RANGE,
		MM_PLAYER_API_AUDIO_EFFECT_EQUALIZER_CLEAR,
		MM_PLAYER_API_AUDIO_EFFECT_EQUALIZER_IS_AVAILABLE,
		MM_PLAYER_API_SET_PROGRESSIVE_DOWNLOAD_PATH,
		MM_PLAYER_API_GET_PROGRESSIVE_DOWNLOAD_STATUS,
		MM_PLAYER_API_CAPTURE_VIDEO,
		MM_PLAYER_API_SET_STREAMING_COOKIE,
		MM_PLAYER_API_SET_STREAMING_USER_AGENT,
		MM_PLAYER_API_GET_STREAMING_DOWNLOAD_PROGRESS,
		MM_PLAYER_API_SET_SUBTITLE_PATH,
		MM_PLAYER_API_SET_SUBTITLE_POSITION_OFFSET,
		MM_PLAYER_API_PUSH_MEDIA_STREAM,
		MM_PLAYER_API_SET_MEDIA_STREAM_INFO,
		MM_PLAYER_API_SET_CALLBACK,
		MM_PLAYER_API_MEDIA_PACKET_FINALIZE_CB,
		MM_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MAX_SIZE,
		MM_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MAX_SIZE,
		MM_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD,
		MM_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD,
		MM_PLAYER_API_GET_TRACK_COUNT,
		MM_PLAYER_API_GET_CURRENT_TRACK,
		MM_PLAYER_API_SELECT_TRACK,
		MM_PLAYER_API_GET_TRACK_LANGUAGE_CODE,
		MM_PLAYER_API_MAX
	} mm_player_api_e;

	typedef enum {
		MM_PLAYER_CB_EVENT = MM_PLAYER_HEAD_GAP(MM_PLAYER_API_MAX),
		MM_PLAYER_CB_MAX
	} mm_player_cb_e;

typedef enum {
	_PLAYER_EVENT_TYPE_PREPARE,
	_PLAYER_EVENT_TYPE_COMPLETE,
	_PLAYER_EVENT_TYPE_INTERRUPT,
	_PLAYER_EVENT_TYPE_ERROR,
	_PLAYER_EVENT_TYPE_BUFFERING,
	_PLAYER_EVENT_TYPE_SUBTITLE,
	_PLAYER_EVENT_TYPE_CAPTURE,
	_PLAYER_EVENT_TYPE_SEEK,
	_PLAYER_EVENT_TYPE_MEDIA_PACKET_VIDEO_FRAME,
	_PLAYER_EVENT_TYPE_AUDIO_FRAME,
	_PLAYER_EVENT_TYPE_VIDEO_FRAME_RENDER_ERROR,
	_PLAYER_EVENT_TYPE_PD,
	_PLAYER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT,
	_PLAYER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT_PRESET,
	_PLAYER_EVENT_TYPE_MISSED_PLUGIN,
#ifdef _PLAYER_FOR_PRODUCT
	_PLAYER_EVENT_TYPE_IMAGE_BUFFER,
	_PLAYER_EVENT_TYPE_SELECTED_SUBTITLE_LANGUAGE,
#endif
	_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS,
	_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS,
	_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK,
	_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK,
	_PLAYER_EVENT_TYPE_AUDIO_STREAM_CHANGED,
	_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED,
	_PLAYER_EVENT_TYPE_VIDEO_BIN_CREATED,
	_PLAYER_EVENT_TYPE_NUM
}_player_event_e;

#ifndef USE_ECORE_FUNCTIONS
typedef enum {
	PLAYER_MESSAGE_NONE,
	PLAYER_MESSAGE_PREPARED,
	PLAYER_MESSAGE_ERROR,
	PLAYER_MESSAGE_SEEK_DONE,
	PLAYER_MESSAGE_EOS,
	PLAYER_MESSAGE_LOOP_EXIT,
	PLAYER_MESSAGE_MAX
}_player_message_e;
#endif

typedef struct _player_s{
	MMHandleType mm_handle;
	const void* user_cb[_PLAYER_EVENT_TYPE_NUM];
	void* user_data[_PLAYER_EVENT_TYPE_NUM];
#ifdef HAVE_WAYLAND
	void* wl_display;
#endif
	void* display_handle;
	player_display_type_e display_type;
	int state;
	bool is_set_pixmap_cb;
	bool is_stopped;
	bool is_display_visible;
	bool is_progressive_download;
	pthread_t prepare_async_thread;
#ifdef USE_ECORE_FUNCTIONS
	GHashTable *ecore_jobs;
#else
	pthread_t message_thread;
	GQueue *message_queue;
	GMutex message_queue_lock;
	GCond message_queue_cond;
	int current_message;
#endif
	player_error_e error_code;
	bool is_doing_jobs;
	media_format_h pkt_fmt;
}player_s;

typedef struct _ret_msg_s{
	gint api;
	gchar *msg;
	struct _ret_msg_s *next;
}ret_msg_s;

typedef struct {
	gint bufLen;
	gchar *recvMsg;
	gint recved;
	ret_msg_s *retMsgHead;
}msg_buff_s;

typedef struct _player_data{
	void *data;
	struct _player_data *next;
}player_data_s;

typedef struct {
	GThread *thread;
	GQueue *queue;
	GMutex mutex;
	GCond cond;
	gboolean running;
} player_event_queue;

typedef struct _callback_cb_info {
	GThread *thread;
	gint running;
	gint fd;
	gint data_fd;
	gpointer user_cb[_PLAYER_EVENT_TYPE_NUM];
	gpointer user_data[_PLAYER_EVENT_TYPE_NUM];
	GMutex player_mutex;
	GCond player_cond[MM_PLAYER_API_MAX];
	msg_buff_s buff;
	player_event_queue event_queue;
	media_format_h pkt_fmt;
	MMHandleType local_handle;
} callback_cb_info_s;

typedef struct {
	intptr_t bo;
}server_tbm_info_s;

typedef struct _player_cli_s{
	intptr_t remote_handle;
	callback_cb_info_s *cb_info;
	player_data_s *head;
	server_tbm_info_s server_tbm;
}player_cli_s;

/* Internal handle cast */
#define INT_HANDLE(h)		((h)->cb_info->local_handle)
/* external handle cast */
#define EXT_HANDLE(h)		((h)->remote_handle)

int player_sound_register(player_h player, int pid);

/**
 * @brief Called when the video sink bin is crated.
 * @since_tizen 3.0
 * @param[out] caps			video sink current caps
 * @param[out ] user_data	The user data passed from the callback registration function
 * @see player_set_vidoe_bin_created_cb()
 * @see player_unset_vidoe_bin_created_cb()
 */
typedef void (*player_video_bin_created_cb)(const char *caps, void *user_data);

/**
 * @brief Registers a callback function to be invoked when video sink bin is created.
 * @since_tizen 3.0
 * @param[in] player    The handle to the media player
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @post player_video_bin_created_cb() will be invoked.
 * @see player_unset_vidoe_bin_created_cb()
 */
int player_set_video_bin_created_cb(player_h player, player_video_bin_created_cb callback, void *user_data);

/**
 * @brief Unregisters a callback function to be invoked when video sink bin is created.
 * @since_tizen 3.0
 * @param[in] player    The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @post player_video_bin_created_cb() will be invoked.
 * @see player_unset_vidoe_bin_created_cb()
 */
int player_unset_video_bin_created_cb(player_h player);


#ifdef __cplusplus
}
#endif

#endif //__TIZEN_MEDIA_PLAYER_2_PRIVATE_H__

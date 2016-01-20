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
#include <tbm_bufmgr.h>
#include <Evas.h>
#include "player.h"
#include "mm_types.h"
#include "muse_core.h"
#include "muse_core_ipc.h"
#include "player2_wayland.h"
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

#define MUSE_PLAYER_HEAD_GAP(api) ((api)/(1000)+(1))*(1000)

typedef enum {
	MUSE_PLAYER_API_CREATE = API_CREATE,
	MUSE_PLAYER_API_DESTROY = API_DESTROY,
	MUSE_PLAYER_API_PREPARE,
	MUSE_PLAYER_API_PREPARE_ASYNC,
	MUSE_PLAYER_API_UNPREPARE,
	MUSE_PLAYER_API_SET_URI,
	MUSE_PLAYER_API_START,
	MUSE_PLAYER_API_STOP,
	MUSE_PLAYER_API_PAUSE,
	MUSE_PLAYER_API_SET_MEMORY_BUFFER,
	MUSE_PLAYER_API_DEINIT_MEMORY_BUFFER,
	MUSE_PLAYER_API_GET_STATE,
	MUSE_PLAYER_API_SET_VOLUME,
	MUSE_PLAYER_API_GET_VOLUME,
	MUSE_PLAYER_API_SET_SOUND_TYPE,
	MUSE_PLAYER_API_SET_AUDIO_POLICY_INFO,
	MUSE_PLAYER_API_SET_AUDIO_LATENCY_MODE,
	MUSE_PLAYER_API_GET_AUDIO_LATENCY_MODE,
	MUSE_PLAYER_API_SET_PLAY_POSITION,
	MUSE_PLAYER_API_GET_PLAY_POSITION,
	MUSE_PLAYER_API_SET_MUTE,
	MUSE_PLAYER_API_IS_MUTED,
	MUSE_PLAYER_API_SET_LOOPING,
	MUSE_PLAYER_API_IS_LOOPING,
	MUSE_PLAYER_API_GET_DURATION,
	MUSE_PLAYER_API_SET_DISPLAY,
	MUSE_PLAYER_API_SET_DISPLAY_MODE,
	MUSE_PLAYER_API_GET_DISPLAY_MODE,
	MUSE_PLAYER_API_SET_PLAYBACK_RATE,
	MUSE_PLAYER_API_SET_DISPLAY_ROTATION,
	MUSE_PLAYER_API_GET_DISPLAY_ROTATION,
	MUSE_PLAYER_API_SET_DISPLAY_VISIBLE,
	MUSE_PLAYER_API_IS_DISPLAY_VISIBLE,
	MUSE_PLAYER_API_RESIZE_VIDEO_RENDER_RECT,
	MUSE_PLAYER_API_GET_CONTENT_INFO,
	MUSE_PLAYER_API_GET_CODEC_INFO,
	MUSE_PLAYER_API_GET_AUDIO_STREAM_INFO,
	MUSE_PLAYER_API_GET_VIDEO_STREAM_INFO,
	MUSE_PLAYER_API_GET_VIDEO_SIZE,
	MUSE_PLAYER_API_GET_ALBUM_ART,
	MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BANDS_COUNT,
	MUSE_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_ALL_BANDS,
	MUSE_PLAYER_API_AUDIO_EFFECT_SET_EQUALIZER_BAND_LEVEL,
	MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_LEVEL,
	MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_LEVEL_RANGE,
	MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY,
	MUSE_PLAYER_API_AUDIO_EFFECT_GET_EQUALIZER_BAND_FREQUENCY_RANGE,
	MUSE_PLAYER_API_AUDIO_EFFECT_EQUALIZER_CLEAR,
	MUSE_PLAYER_API_AUDIO_EFFECT_EQUALIZER_IS_AVAILABLE,
	MUSE_PLAYER_API_SET_PROGRESSIVE_DOWNLOAD_PATH,
	MUSE_PLAYER_API_GET_PROGRESSIVE_DOWNLOAD_STATUS,
	MUSE_PLAYER_API_CAPTURE_VIDEO,
	MUSE_PLAYER_API_SET_STREAMING_COOKIE,
	MUSE_PLAYER_API_SET_STREAMING_USER_AGENT,
	MUSE_PLAYER_API_GET_STREAMING_DOWNLOAD_PROGRESS,
	MUSE_PLAYER_API_SET_SUBTITLE_PATH,
	MUSE_PLAYER_API_SET_SUBTITLE_POSITION_OFFSET,
	MUSE_PLAYER_API_PUSH_MEDIA_STREAM,
	MUSE_PLAYER_API_SET_MEDIA_STREAM_INFO,
	MUSE_PLAYER_API_SET_CALLBACK,
	MUSE_PLAYER_API_MEDIA_PACKET_FINALIZE_CB,
	MUSE_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MAX_SIZE,
	MUSE_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MAX_SIZE,
	MUSE_PLAYER_API_SET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD,
	MUSE_PLAYER_API_GET_MEDIA_STREAM_BUFFER_MIN_THRESHOLD,
	MUSE_PLAYER_API_GET_TRACK_COUNT,
	MUSE_PLAYER_API_GET_CURRENT_TRACK,
	MUSE_PLAYER_API_SELECT_TRACK,
	MUSE_PLAYER_API_GET_TRACK_LANGUAGE_CODE,
	MUSE_PLAYER_API_SET_PCM_EXTRACTION_MODE,
	MUSE_PLAYER_API_SET_PCM_SPEC,
	MUSE_PLAYER_API_SET_STREAMING_PLAYBACK_RATE,
	MUSE_PLAYER_API_MAX
} muse_player_api_e;

typedef enum {
	MUSE_PLAYER_CB_EVENT = MUSE_PLAYER_HEAD_GAP(MUSE_PLAYER_API_MAX),
	MUSE_PLAYER_CB_MAX
} muse_player_cb_e;

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
	_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_BUFFER_STATUS_WITH_INFO,
	_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_BUFFER_STATUS_WITH_INFO,
	_PLAYER_EVENT_TYPE_MEDIA_STREAM_VIDEO_SEEK,
	_PLAYER_EVENT_TYPE_MEDIA_STREAM_AUDIO_SEEK,
	_PLAYER_EVENT_TYPE_AUDIO_STREAM_CHANGED,
	_PLAYER_EVENT_TYPE_VIDEO_STREAM_CHANGED,
	_PLAYER_EVENT_TYPE_VIDEO_BIN_CREATED,
	_PLAYER_EVENT_TYPE_NUM
} _player_event_e;

#ifndef USE_ECORE_FUNCTIONS
typedef enum {
	PLAYER_MESSAGE_NONE,
	PLAYER_MESSAGE_PREPARED,
	PLAYER_MESSAGE_ERROR,
	PLAYER_MESSAGE_SEEK_DONE,
	PLAYER_MESSAGE_EOS,
	PLAYER_MESSAGE_LOOP_EXIT,
	PLAYER_MESSAGE_MAX
} _player_message_e;
#endif

typedef struct _player_s {
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
} player_s;

typedef struct _ret_msg_s{
	gint api;
	gchar *msg;
	struct _ret_msg_s *next;
} ret_msg_s;

typedef struct {
	gint bufLen;
	gchar *recvMsg;
	gint recved;
	ret_msg_s *retMsgHead;
} msg_buff_s;

typedef struct _player_data{
	void *data;
	struct _player_data *next;
} player_data_s;

typedef struct {
	GThread *thread;
	GQueue *queue;
	GMutex qlock;
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
	GCond player_cond[MUSE_PLAYER_API_MAX];
	msg_buff_s buff;
	player_event_queue event_queue;
	media_format_h pkt_fmt;
	MMHandleType local_handle;
	tbm_bufmgr bufmgr;
} callback_cb_info_s;

typedef struct {
	intptr_t bo;
	gboolean is_streaming;
	gint timeout;
} server_info_s;

typedef struct _player_cli_s{
	callback_cb_info_s *cb_info;
	player_data_s *head;
	server_info_s server;
	wl_client *wlclient;
	Evas_Object * eo;
	gboolean have_evas_callback;
} player_cli_s;

/* Internal handle */
#define INT_HANDLE(h)		((h)->cb_info->local_handle)

/* player callback infomatnio */
#define CALLBACK_INFO(h)	((h)->cb_info)
/* MSG Channel socket fd */
#define MSG_FD(h)			(CALLBACK_INFO(h)->fd)
/* Data Channel socket fd */
#define DATA_FD(h)			(CALLBACK_INFO(h)->data_fd)
/* TBM buffer manager */
#define TBM_BUFMGR(h)		(CALLBACK_INFO(h)->bufmgr)

/* server tbm bo */
#define SERVER_TBM_BO(h)	((h)->server.bo)
/* content type is streaming */
#define IS_STREAMING_CONTENT(h)		((h)->server.is_streaming)
/* server state change timeout */
#define SERVER_TIMEOUT(h)		((h)->server.timeout)

int _get_api_timeout(player_cli_s *pc, muse_player_api_e api);
int wait_for_cb_return(muse_player_api_e api, callback_cb_info_s *cb_info, char **ret_buf, int time_out);
int player_sound_register(player_h player, int pid);
int player_is_streaming(player_h player, bool *is_streaming);
int player_set_evas_object_cb(player_h player, Evas_Object * eo);
int player_unset_evas_object_cb(player_h player);

#ifdef USE_CLIENT_PIPELINE

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
#endif

#ifdef __cplusplus
}
#endif

#endif //__TIZEN_MEDIA_PLAYER_2_PRIVATE_H__

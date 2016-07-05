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

#ifndef __TIZEN_MEDIA_LEGACY_PLAYER_H__
#define __TIZEN_MEDIA_LEGACY_PLAYER_H__

#include <tizen.h>
#include <sound_manager.h>
#include <media_packet.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLAYER_ERROR_CLASS          TIZEN_ERROR_PLAYER | 0x20

/* This is for custom defined player error. */
#define PLAYER_CUSTOM_ERROR_CLASS   TIZEN_ERROR_PLAYER | 0x1000

/**
 * @file legacy_player.h
 * @brief This file contains the media player API.
 */

/**
 * @addtogroup CAPI_MEDIA_PLAYER_MODULE
 * @{
 */

/**
 * @brief The media player's type handle.
 * @since_tizen 2.3
 */
typedef struct player_s *player_h;

/**
 * @brief Enumeration for media player state.
 * @since_tizen 2.3
 */
typedef enum {
    PLAYER_STATE_NONE,          /**< Player is not created */
    PLAYER_STATE_IDLE,          /**< Player is created, but not prepared */
    PLAYER_STATE_READY,         /**< Player is ready to play media */
    PLAYER_STATE_PLAYING,       /**< Player is playing media */
    PLAYER_STATE_PAUSED,        /**< Player is paused while playing media */
} player_state_e;

/**
 * @brief Enumeration for media player's error codes.
 * @since_tizen 2.3
 */
typedef enum {
    PLAYER_ERROR_NONE   = TIZEN_ERROR_NONE,                                 /**< Successful */
    PLAYER_ERROR_OUT_OF_MEMORY  = TIZEN_ERROR_OUT_OF_MEMORY,                /**< Out of memory */
    PLAYER_ERROR_INVALID_PARAMETER  = TIZEN_ERROR_INVALID_PARAMETER,        /**< Invalid parameter */
    PLAYER_ERROR_NO_SUCH_FILE   = TIZEN_ERROR_NO_SUCH_FILE,                 /**< No such file or directory */
    PLAYER_ERROR_INVALID_OPERATION  = TIZEN_ERROR_INVALID_OPERATION,        /**< Invalid operation */
    PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE    = TIZEN_ERROR_FILE_NO_SPACE_ON_DEVICE,  /**< No space left on the device */
    PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE    = TIZEN_ERROR_NOT_SUPPORTED,    /**< Not supported */
    PLAYER_ERROR_SEEK_FAILED    = PLAYER_ERROR_CLASS | 0x01,                /**< Seek operation failure */
    PLAYER_ERROR_INVALID_STATE  = PLAYER_ERROR_CLASS | 0x02,                /**< Invalid state */
    PLAYER_ERROR_NOT_SUPPORTED_FILE = PLAYER_ERROR_CLASS | 0x03,            /**< File format not supported */
    PLAYER_ERROR_INVALID_URI    = PLAYER_ERROR_CLASS | 0x04,                /**< Invalid URI */
    PLAYER_ERROR_SOUND_POLICY   = PLAYER_ERROR_CLASS | 0x05,                /**< Sound policy error */
    PLAYER_ERROR_CONNECTION_FAILED  = PLAYER_ERROR_CLASS | 0x06,            /**< Streaming connection failed */
    PLAYER_ERROR_VIDEO_CAPTURE_FAILED   = PLAYER_ERROR_CLASS | 0x07,        /**< Video capture failed */
    PLAYER_ERROR_DRM_EXPIRED    = PLAYER_ERROR_CLASS | 0x08,                /**< Expired license */
    PLAYER_ERROR_DRM_NO_LICENSE 	= PLAYER_ERROR_CLASS | 0x09,            /**< No license */
    PLAYER_ERROR_DRM_FUTURE_USE 	= PLAYER_ERROR_CLASS | 0x0a,            /**< License for future use */
    PLAYER_ERROR_DRM_NOT_PERMITTED  = PLAYER_ERROR_CLASS | 0x0b,            /**< Format not permitted */
    PLAYER_ERROR_RESOURCE_LIMIT     = PLAYER_ERROR_CLASS | 0x0c,            /**< Resource limit */
    PLAYER_ERROR_PERMISSION_DENIED  = TIZEN_ERROR_PERMISSION_DENIED,        /**< Permission denied */
    PLAYER_ERROR_SERVICE_DISCONNECTED = PLAYER_ERROR_CLASS | 0x0d,          /**< Socket connection lost (Since 3.0) */
    PLAYER_ERROR_BUFFER_SPACE         = TIZEN_ERROR_BUFFER_SPACE,           /**< No buffer space available (Since 3.0)*/
} player_error_e;

/**
 * @brief Enumeration for media player's interruption type.
 * @since_tizen 2.3
 */
typedef enum {
    PLAYER_INTERRUPTED_COMPLETED = 0,           /**< Interrupt completed (Deprecated since 3.0)*/
    PLAYER_INTERRUPTED_BY_MEDIA,                /**< Interrupted by a non-resumable media application (Deprecated since 3.0)*/
    PLAYER_INTERRUPTED_BY_CALL,                 /**< Interrupted by an incoming call (Deprecated since 3.0)*/
    PLAYER_INTERRUPTED_BY_EARJACK_UNPLUG,       /**< Interrupted by unplugging headphones (Deprecated since 3.0)*/
    PLAYER_INTERRUPTED_BY_RESOURCE_CONFLICT,    /**< Interrupted by a resource conflict */
    PLAYER_INTERRUPTED_BY_ALARM,                /**< Interrupted by an alarm (Deprecated since 3.0)*/
    PLAYER_INTERRUPTED_BY_EMERGENCY,            /**< Interrupted by an emergency (Deprecated since 3.0)*/
    PLAYER_INTERRUPTED_BY_NOTIFICATION,         /**< Interrupted by a notification (Deprecated since 3.0)*/
} player_interrupted_code_e;

/**
 * @brief Enumeration for progressive download message type.
 * @since_tizen 2.3
 */
typedef enum {
    PLAYER_PD_STARTED = 0,              /**< Progressive download is started */
    PLAYER_PD_COMPLETED,                /**< Progressive download is completed */
} player_pd_message_type_e;

/**
 * @brief Enumeration for display type.
 * @since_tizen 2.3
 */
typedef enum {
    PLAYER_DISPLAY_TYPE_OVERLAY = 0,    /**< Overlay surface display */
#ifdef TIZEN_FEATURE_EVAS_RENDERER
    PLAYER_DISPLAY_TYPE_EVAS,           /**< Evas image object surface display */
#endif
    PLAYER_DISPLAY_TYPE_NONE,           /**< This disposes off buffers */
} player_display_type_e;

/**
 * @brief Enumeration for audio latency mode.
 * @since_tizen 2.3
 */
typedef enum {
    AUDIO_LATENCY_MODE_LOW = 0,     /**< Low audio latency mode */
    AUDIO_LATENCY_MODE_MID,         /**< Middle audio latency mode */
    AUDIO_LATENCY_MODE_HIGH,        /**< High audio latency mode */
} audio_latency_mode_e;

/**
 * @brief Enumeration for stream type.
 * @since_tizen 2.4
 */
typedef enum {
    PLAYER_STREAM_TYPE_DEFAULT,	/**< Container type */
    PLAYER_STREAM_TYPE_AUDIO,	/**< Audio element stream type */
    PLAYER_STREAM_TYPE_VIDEO,	/**< Video element stream type */
    PLAYER_STREAM_TYPE_TEXT,	/**< Text type */
} player_stream_type_e;

/**
 * @brief Enumeration of media stream buffer status
 * @since_tizen 2.4
 */
typedef enum {
    PLAYER_MEDIA_STREAM_BUFFER_UNDERRUN,
    PLAYER_MEDIA_STREAM_BUFFER_OVERFLOW,
} player_media_stream_buffer_status_e;

/**
 * @brief The player display handle.
 * @since_tizen 2.3
 */
typedef void* player_display_h;

#ifndef GET_DISPLAY
/**
 * @brief Definition for a display handle from evas object.
 * @since_tizen 2.3
 */
#define GET_DISPLAY(x) (void*)(x)
#endif

/**
 * @}
 */

/**
 * @addtogroup CAPI_MEDIA_PLAYER_DISPLAY_MODULE
 * @{
 */

/**
 * @brief Enumeration for display rotation type.
 * @since_tizen 2.3
 */
typedef enum {
    PLAYER_DISPLAY_ROTATION_NONE,   /**< Display is not rotated */
    PLAYER_DISPLAY_ROTATION_90,     /**< Display is rotated 90 degrees */
    PLAYER_DISPLAY_ROTATION_180,    /**< Display is rotated 180 degrees */
    PLAYER_DISPLAY_ROTATION_270,    /**< Display is rotated 270 degrees */
} player_display_rotation_e;

/**
 * @brief Enumeration for display mode.
 * @since_tizen 2.3
 */
typedef enum {
    PLAYER_DISPLAY_MODE_LETTER_BOX = 0,     /**< Letter box */
    PLAYER_DISPLAY_MODE_ORIGIN_SIZE,        /**< Origin size */
    PLAYER_DISPLAY_MODE_FULL_SCREEN,        /**< Full-screen */
    PLAYER_DISPLAY_MODE_CROPPED_FULL,       /**< Cropped full-screen */
    PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER,   /**< Origin size (if surface size is larger than video size(width/height)) or Letter box (if video size(width/height) is larger than surface size) */
    PLAYER_DISPLAY_MODE_DST_ROI,            /**< Dst ROI mode (Deprecated since [3.0]).*/
    PLAYER_DISPLAY_MODE_NUM
} player_display_mode_e;

/**
 * @}
 */

/**
 * @addtogroup CAPI_MEDIA_PLAYER_STREAM_INFO_MODULE
 * @{
 */

/**
 * @brief Enumeration for media stream content information.
 * @since_tizen 2.3
 */
typedef enum {
    PLAYER_CONTENT_INFO_ALBUM,      /**< Album */
    PLAYER_CONTENT_INFO_ARTIST,     /**< Artist */
    PLAYER_CONTENT_INFO_AUTHOR,     /**< Author */
    PLAYER_CONTENT_INFO_GENRE,      /**< Genre */
    PLAYER_CONTENT_INFO_TITLE,      /**< Title */
    PLAYER_CONTENT_INFO_YEAR,       /**< Year */
} player_content_info_e;

/**
 * @}
 */


/**
 * @addtogroup CAPI_MEDIA_PLAYER_SUBTITLE_MODULE
 * @{
 */

/**
 * @brief  Called when the subtitle is updated.
 * @since_tizen 2.3
 * @param[in]   duration	The duration of the updated subtitle
 * @param[in]   text		The text of the updated subtitle
 * @param[in]   user_data	The user data passed from the callback registration function
 * @see legacy_player_set_subtitle_updated_cb()
 * @see legacy_player_unset_subtitle_updated_cb()
 */
typedef void (*player_subtitle_updated_cb)(unsigned long duration, char *text, void *user_data);

/**
 * @}
 */

/**
 * @addtogroup CAPI_MEDIA_PLAYER_MODULE
 * @{
 */

/**
 * @brief Called when the media player is prepared.
 * @since_tizen 2.3
 * @details It will be invoked when player has reached the begin of stream.
 * @param[in]   user_data  The user data passed from the callback registration function
 * @pre legacy_player_prepare_async() will cause this callback.
 * @post The player state will be #PLAYER_STATE_READY.
 * @see legacy_player_prepare_async()
 */
typedef void (*player_prepared_cb)(void *user_data);

/**
 * @brief Called when the media player is completed.
 * @since_tizen 2.3
 * @details It will be invoked when player has reached the end of the stream.
 * @param[in]   user_data  The user data passed from the callback registration function
 * @pre It will be invoked when the playback is completed if you register this callback using legacy_player_set_completed_cb().
 * @see legacy_player_set_completed_cb()
 * @see legacy_player_unset_completed_cb()
 */
typedef void (*player_completed_cb)(void *user_data);

/**
 * @brief Called when the seek operation is completed.
 * @since_tizen 2.3
 * @param[in]   user_data  The user data passed from the callback registration function
 * @see legacy_player_set_play_position()
 */
typedef void (*player_seek_completed_cb)(void *user_data);

/**
 * @brief Called when the media player is interrupted.
 * @since_tizen 2.3
 * @param[in]	error_code	The interrupted error code
 * @param[in]	user_data	The user data passed from the callback registration function
 * @see legacy_player_set_interrupted_cb()
 * @see legacy_player_unset_interrupted_cb()
 */
typedef void (*player_interrupted_cb)(player_interrupted_code_e code, void *user_data);

/**
 * @brief Called when an error occurs in the media player.
 * @details Following error codes can be delivered.
 *          #PLAYER_ERROR_INVALID_OPERATION
 *          #PLAYER_ERROR_INVALID_STATE
 *          #PLAYER_ERROR_INVALID_URI
 *          #PLAYER_ERROR_CONNECTION_FAILED
 *          #PLAYER_ERROR_DRM_NOT_PERMITTED
 *          #PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE
 *          #PLAYER_ERROR_NOT_SUPPORTED_FILE
 *          #PLAYER_ERROR_SEEK_FAILED
 *          #PLAYER_ERROR_SERVICE_DISCONNECTED
 * @since_tizen 2.3
 * @param[in]	error_code  The error code
 * @param[in]	user_data	The user data passed from the callback registration function
 * @see legacy_player_set_error_cb()
 * @see legacy_player_unset_error_cb()
 */
typedef void (*player_error_cb)(int error_code, void *user_data);

/**
 * @brief Called when the buffering percentage of the media playback is updated.
 * @since_tizen 2.3
 * @details If the buffer is full, it will return 100%.
 * @param[in]   percent	The percentage of buffering completed (0~100)
 * @param[in]   user_data	The user data passed from the callback registration function
 * @see legacy_player_set_buffering_cb()
 * @see legacy_player_unset_buffering_cb()
 */
typedef void (*player_buffering_cb)(int percent, void *user_data);

/**
 * @brief Called when progressive download is started or completed.
 * @since_tizen 2.3
 * @param[in]   type		The message type for progressive download
 * @param[in]   user_data	The user data passed from the callback registration function
 * @see legacy_player_set_progressive_download_path()
 */
typedef void (*player_pd_message_cb)(player_pd_message_type_e type, void *user_data);

/**
 * @brief Called when the video is captured.
 * @since_tizen 2.3
 * @remarks The color space format of the captured image is IMAGE_UTIL_COLORSPACE_RGB888.
 * @param[in]   data	The captured image buffer
 * @param[in]   width	The width of the captured image
 * @param[in]   height  The height of the captured image
 * @param[in]   size	The size of the captured image
 * @param[in]   user_data	The user data passed from the callback registration function
 * @see legacy_player_capture_video()
 */
typedef void (*player_video_captured_cb)(unsigned char *data, int width, int height, unsigned int size, void *user_data);

/**
 * @brief Called to register for notifications about delivering media packet when every video frame is decoded.
 * @since_tizen 2.3
 *
 * @remarks This function is issued in the context of gstreamer so the UI update code should not be directly invoked.\n
 * 		the packet should be released by media_packet_destroy() after use. \n
 *		Otherwises, decoder can't work more because it can't have enough buffer to fill decoded frame.
 *
 * @param[in] pkt Reference pointer to the media packet
 * @param[in] user_data The user data passed from the callback registration function
 */
typedef void (*player_media_packet_video_decoded_cb)(media_packet_h pkt, void *user_data);

/**
 * @brief Called when the buffer level drops below the threshold of max size or no free space in buffer.
 * @since_tizen 2.4
 * @remarks This API is used for media stream playback only.
 * @param[in] user_data The user data passed from the callback registration function
 * @see legacy_player_set_media_stream_buffer_status_cb()
 * @see legacy_player_set_media_stream_buffer_max_size()
 * @see legacy_player_set_media_stream_buffer_min_threshold()
 */
typedef void (*player_media_stream_buffer_status_cb) (player_media_stream_buffer_status_e status, void *user_data);

/**
 * @brief Called to notify the next push-buffer offset when seeking is occurred.
 * @since_tizen 2.4
 * @remarks This API is used for media stream playback only.
 * @details The next push-buffer should produce buffers from the new offset.
 * @param[in] offset The new byte position to seek
 * @param[in] user_data The user data passed from the callback registration function
 */
typedef void (*player_media_stream_seek_cb) (unsigned long long offset, void *user_data);

/**
 * @brief Called to notify the video stream changed.
 * @since_tizen 2.4
 * @details The video stream changing is detected just before rendering operation.
 * @param[in] width	The width of the captured image
 * @param[in] height The height of the captured image
 * @param[in] fps The frame per second of the video \n
              It can be @c 0 if there is no video stream information.
 * @param[in] bit_rate The video bit rate [Hz] \n
 *            It can be an invalid value if there is no video stream information.
 * @param[in] user_data The user data passed from the callback registration function
 * @see legacy_player_set_video_stream_changed_cb()
 */
typedef void (*player_video_stream_changed_cb) (int width, int height, int fps, int bit_rate, void *user_data);

/**
 * @brief Creates a player handle for playing multimedia content.
 * @since_tizen 2.3
 * @remarks You must release @a player by using legacy_player_destroy().\n
 *          Although you can create multiple player handles at the same time,
 *          the player cannot guarantee proper operation because of limited resources, such as
 *          audio or display device.
 *
 * @param[out]  player  A new handle to the media player
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @post The player state will be #PLAYER_STATE_IDLE.
 * @see legacy_player_destroy()
 */
int legacy_player_create(player_h *player);

/**
 * @brief Destroys the media player handle and releases all its resources.
 * @since_tizen 2.3
 * @remarks To completely shutdown player operation, call this function with a valid player handle from any player state.
 * @param[in] player The handle to the media player to be destroyed
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @pre  The player state must be set to #PLAYER_STATE_IDLE.
 * @post The player state will be #PLAYER_STATE_NONE.
 * @see legacy_player_create()
 */
int legacy_player_destroy(player_h player);

/**
 * @brief Prepares the media player for playback.
 * @since_tizen 2.3
 * @remarks The mediastorage privilege(http://tizen.org/privilege/mediastorage) should be added if any video/audio files are used to play located in the internal storage.
 * @remarks The externalstorage privilege(http://tizen.org/privilege/externalstorage) should be added if any video/audio files are used to play located in the external storage.
 * @remarks The internet privilege(http://tizen.org/privilege/internet) should be added if any URLs are used to play from network.
 * @param[in]	player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_URI Invalid URI
 * @retval #PLAYER_ERROR_NO_SUCH_FILE File not found
 * @retval #PLAYER_ERROR_NOT_SUPPORTED_FILE File not supported
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_PERMISSION_DENIED Permission denied
 * @pre	The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare(). After that, call legacy_player_set_uri() to load the media content you want to play.
 * @post The player state will be #PLAYER_STATE_READY.
 * @see legacy_player_prepare_async()
 * @see legacy_player_unprepare()
 * @see legacy_player_set_uri()
 */
int legacy_player_prepare(player_h player);

/**
 * @brief Prepares the media player for playback, asynchronously.
 * @since_tizen 2.3
 * @remarks The mediastorage privilege(http://tizen.org/privilege/mediastorage) should be added if any video/audio files are used to play located in the internal storage.
 * @remarks The externalstorage privilege(http://tizen.org/privilege/externalstorage) should be added if any video/audio files are used to play located in the external storage.
 * @remarks The internet privilege(http://tizen.org/privilege/internet) should be added if any URLs are used to play from network.
 * @param[in] player The handle to the media player
 * @param[in] callback	The callback function to register
 * @param[in] user_data	The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_URI Invalid URI
 * @retval #PLAYER_ERROR_NO_SUCH_FILE File not found
 * @retval #PLAYER_ERROR_NOT_SUPPORTED_FILE File not supported
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_PERMISSION_DENIED Permission denied
 * @pre	The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare(). After that, call legacy_player_set_uri() to load the media content you want to play.
 * @post It invokes legacy_player_prepared_cb() when playback is prepared.
 * @see legacy_player_prepare()
 * @see legacy_player_prepared_cb()
 * @see legacy_player_unprepare()
 * @see legacy_player_set_uri()
 */
int legacy_player_prepare_async (player_h player, player_prepared_cb callback, void* user_data);

/**
 * @brief Resets the media player.
 * @details The most recently used media is reset and no longer associated with the player.
 *          Playback is no longer possible. If you want to use the player again, you will have to set the data URI and call
 *          legacy_player_prepare() again.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre	The player state should be higher than #PLAYER_STATE_IDLE.
 * @post The player state will be #PLAYER_STATE_IDLE.
 * @see legacy_player_prepare()
 */
int legacy_player_unprepare(player_h player);

/**
 * @brief Sets the data source (file-path, HTTP or RSTP URI) to use.
 *
 * @details Associates media contents, referred to by the URI, with the player.
 *          If the function call is successful, subsequent calls to legacy_player_prepare() and legacy_player_start() will start playing the media.
 * @since_tizen 2.3
 * @remarks If you use HTTP or RSTP, URI should start with "http://" or "rtsp://". The default protocol is "file://".
 *          If you provide an invalid URI, you won't receive an error message until you call legacy_player_start().
 *
 * @param[in]   player The handle to the media player
 * @param[in]   uri The content location, such as the file path, the URI of the HTTP or RSTP stream you want to play
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare().
 * @see legacy_player_set_memory_buffer()
 */
int legacy_player_set_uri(player_h player, const char * uri);

/**
 * @brief Sets memory as the data source.
 *
 * @details Associates media content, cached in memory, with the player. Unlike the case of legacy_player_set_uri(), the media resides in memory.
 *          If the function call is successful, subsequent calls to legacy_player_prepare() and legacy_player_start() will start playing the media.
 * @since_tizen 2.3
 * @remarks If you provide an invalid data, you won't receive an error message until you call legacy_player_start().
 *
 * @param[in]   player The handle to the media player
 * @param[in]   data The memory pointer of media data
 * @param[in]   size The size of media data
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre	The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare().
 * @see legacy_player_set_uri()
 */
int legacy_player_set_memory_buffer(player_h player, const void * data, int size);

/**
 * @brief Gets the player's current state.
 * @since_tizen 2.3
 * @param[in]   player	The handle to the media player
 * @param[out]  state	The current state of the player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see #player_state_e
 */
int  legacy_player_get_state(player_h player, player_state_e *state);

/**
 * @brief Sets the player's volume.
 * @since_tizen 2.3
 * @details  Setting this volume adjusts the player's instance volume, not the system volume.
 *	      The valid range is from 0 to 1.0, inclusive (1.0 = 100%). Default value is 1.0.
 *          To change system volume, use the @ref CAPI_MEDIA_SOUND_MANAGER_MODULE API.
 * 	      Finally, it does not support to set other value into each channel currently.
 *
 * @param[in]   player The handle to the media player
 * @param[in]   left The left volume scalar
 * @param[in]   right The right volume scalar
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @see legacy_player_get_volume()
 */
int legacy_player_set_volume(player_h player, float left, float right);

/**
 * @brief Gets the player's current volume factor.
 * @since_tizen 2.3
 * @details The range of @a left and @a right is from @c 0 to @c 1.0, inclusive (1.0 = 100%).
 *          This function gets the player volume, not the system volume.
 *          To get the system volume, use the @ref CAPI_MEDIA_SOUND_MANAGER_MODULE API.
 *
 * @param[in]   player The handle to the media player
 * @param[out]  left The current left volume scalar
 * @param[out]  right The current right volume scalar
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @see legacy_player_set_volume()
 */
int legacy_player_get_volume(player_h player, float *left, float *right);

/**
 * @deprecated Deprecated since 3.0. Use legacy_player_set_audio_policy_info() instead.
 * @brief Sets the player's volume type.
 * @since_tizen 2.3
 * @remarks The default sound type of the player is #SOUND_TYPE_MEDIA.
 *          To get the current sound type, use sound_manager_get_current_sound_type().
 * @remarks If stream_info already exists by calling sound_manager_create_stream_info(),
 *          It will return error since 3.0.
 *
 * @param[in]   player The handle to the media player
 * @param[in]   type The sound type
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_SOUND_POLICY Sound policy error
 * @pre The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create().
 * @see sound_manager_get_current_sound_type()
 */
int legacy_player_set_sound_type(player_h player, sound_type_e type);

/**
 * @brief Sets the player's sound manager stream information.
 * @since_tizen 3.0
 * @remarks You can set sound stream information including audio routing and volume type.
 * For more details, please refer to sound_manager.h
 *
 * @param[in] player The handle to the media player
 * @param[in] stream_info The sound manager info type
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE Unsupported feature
 * @pre The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create().
 * @see #sound_stream_info_h
 * @see sound_manager_create_stream_information()
 * @see sound_manager_destroy_stream_information()
 */
int legacy_player_set_audio_policy_info(player_h player, sound_stream_info_h stream_info);

/**
 * @brief Sets the audio latency mode.
 * @since_tizen 2.3
 * @remarks The default audio latency mode of the player is #AUDIO_LATENCY_MODE_MID.
 *		To get the current audio latency mode, use legacy_player_get_audio_latency_mode().
 *		If it's high mode, audio output interval can be increased so, it can keep more audio data to play.
 *		But, state transition like pause or resume can be more slower than default(mid) mode.
 *
 * @param[in] player The handle to the media player
 * @param[in] latency_mode The latency mode to be applied to the audio
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @see #audio_latency_mode_e
 * @see legacy_player_get_audio_latency_mode()
 */
int legacy_player_set_audio_latency_mode(player_h player, audio_latency_mode_e latency_mode);

/**
 * @brief Gets the current audio latency mode.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[out] latency_mode The latency mode to get from the audio
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @see #audio_latency_mode_e
 * @see legacy_player_set_audio_latency_mode()
 */
int legacy_player_get_audio_latency_mode(player_h player, audio_latency_mode_e *latency_mode);

/**
 * @brief Starts or resumes playback.
 * @since_tizen 2.3
 * @remarks Sound can be mixed with other sounds if you don't control the stream focus in sound-manager module since 3.0.\n
 * You can refer to @ref CAPI_MEDIA_SUOND_MANAGER_MODULE.
 * @details Plays current media content, or resumes play if paused.
 *
 * @param[in]   player The handle to the media player
 * @return @c 0 on success,
 * otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_CONNECTION_FAILED Network connection failed
 * @retval #PLAYER_ERROR_SOUND_POLICY Sound policy error
 * @pre legacy_player_prepare() must be called before calling this function.
 * @pre The player state must be set to #PLAYER_STATE_READY by calling legacy_player_prepare() or set to #PLAYER_STATE_PAUSED by calling legacy_player_pause().
 * @post The player state will be #PLAYER_STATE_PLAYING.
 * @post It invokes legacy_player_completed_cb() when playback completes, if you set a callback with legacy_player_set_completed_cb().
 * @post It invokes legacy_player_pd_message_cb() when progressive download starts or completes, if you set a download path with legacy_player_set_progressive_download_path() and a callback with legacy_player_set_progressive_download_message_cb().
 * @see legacy_player_prepare()
 * @see legacy_player_prepare_async()
 * @see legacy_player_stop()
 * @see legacy_player_pause()
 * @see legacy_player_set_completed_cb()
 * @see legacy_player_completed_cb()
 * @see legacy_player_set_progressive_download_path()
 * @see legacy_player_set_progressive_download_message_cb()
 * @see legacy_player_pd_message_cb()
 */
int legacy_player_start(player_h player);

/**
 * @brief Stops playing media content.
 * @since_tizen 2.3
 * @param[in]   player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid state
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_SOUND_POLICY Sound policy error
 * @pre The player state must be set to #PLAYER_STATE_PLAYING by calling legacy_player_start() or set to #PLAYER_STATE_PAUSED by calling legacy_player_pause().
 * @post The player state will be #PLAYER_STATE_READY.
 * @post The downloading will be aborted if you use progressive download.
 * @see legacy_player_start()
 * @see legacy_player_pause()
 */
int legacy_player_stop(player_h player);

/**
 * @brief Pauses the player.
 * @since_tizen 2.3
 * @remarks	You can resume playback using legacy_player_start().
 *
 * @param[in]   player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid state
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_SOUND_POLICY Sound policy error
 * @pre The player state must be set to #PLAYER_STATE_PLAYING.
 * @post The player state will be #PLAYER_STATE_READY.
 * @see legacy_player_start()
 */
int legacy_player_pause(player_h player);

/**
 * @brief Sets the seek position for playback, asynchronously.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[in] millisecond The position in milliseconds from the start to the seek point
 * @param[in] accurate If @c true the selected position is returned, but this might be considerably slow,
 *                     otherwise @c false
 * @param[in] callback	The callback function to register
 * @param[in] user_data	The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_SEEK_FAILED Seek operation failure
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @post It invokes legacy_player_seek_completed_cb() when seek operation completes, if you set a callback.
 * @see legacy_player_get_play_position()
 */
int legacy_player_set_play_position(player_h player, int millisecond, bool accurate, player_seek_completed_cb callback, void *user_data);

/**
 * @brief Gets the current position in milliseconds.
 * @since_tizen 2.3
 * @param[in]   player The handle to the media player
 * @param[out]  millisecond The current position in milliseconds
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_SEEK_FAILED Seek operation failure
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_set_play_position()
 */
int legacy_player_get_play_position(player_h player, int *millisecond);

/**
 * @brief Sets the player's mute status.
 * @since_tizen 2.3
 * @details If the mute status is @c true, no sounds are played.
 *          If it is @c false, sounds are played at the previously set volume level.
 *          Until this function is called, by default the player is not muted.
 *
 * @param[in]   player The handle to the media player
 * @param[in]   muted The new mute status: (@c true = mute, @c false = not muted)
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_is_muted()
 */
int legacy_player_set_mute(player_h player, bool muted);

/**
 * @brief Gets the player's mute status.
 * @since_tizen 2.3
 * @details If the mute status is @c true, no sounds are played.
 *          If it is @c false, sounds are played at the previously set volume level.
 *
 * @param[in]   player The handle to the media player
 * @param[out]  muted  The current mute status: (@c true = mute, @c false = not muted)
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_set_mute()
 */
int legacy_player_is_muted(player_h player, bool *muted);

/**
 * @brief Sets the player's looping status.
 * @since_tizen 2.3
 * @details If the looping status is @c true, playback automatically restarts upon finishing.
 *          If it is @c false, it won't. The default value is @c false.
 *
 * @param[in]   player The handle to the media player
 * @param[in]   looping The new looping status: (@c true = looping, @c false = non-looping )
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_is_looping()
 */
int legacy_player_set_looping(player_h player, bool looping);

/**
 * @brief Gets the player's looping status.
 * @since_tizen 2.3
 * @details If the looping status is @c true, playback automatically restarts upon finishing.
 *          If it is @c false, it won't.
 *
 * @param[in]   player The handle to the media player
 * @param[out]  looping The looping status: (@c true = looping, @c false = non-looping )
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_set_looping()
 */
int legacy_player_is_looping(player_h player, bool *looping);

/**
 * @brief Sets the video display.
 * @since_tizen 2.3
 * @remarks To get @a display to set, use #GET_DISPLAY().
 * @remarks To use the multiple surface display mode, use legacy_player_set_display() again with a different display type.
 * @remarks We are not supporting changing display between different types. \n
 *          If you want to change display handle after calling legacy_player_prepare(), you must use the same display type as what you set before.
 * @param[in]   player The handle to the media player
 * @param[in]   type The display type
 * @param[in]   display The handle to display
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_set_display_rotation
 */
int legacy_player_set_display(player_h player, player_display_type_e type, player_display_h display);

/**
 * @brief Registers a media packet video callback function to be called once per frame.
 * @since_tizen 2.3
 * @remarks This function should be called before preparing. \n
 *	      A registered callback is called on the internal thread of the player. \n
 *          A video frame can be retrieved using a registered callback as a media packet.\n
 *          The callback function holds the same buffer that will be drawn on the display device.\n
 *          So if you change the media packet in a registerd callback, it will be displayed on the device\n
 *          and the media packet is available until it's destroyed by media_packet_destroy().
 * @param[in] player The handle to the media player
 * @param[in] callback The callback function to be registered
 * @param[in] user_data The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid state
 * @pre	The player's state should be #PLAYER_STATE_IDLE. And, #PLAYER_DISPLAY_TYPE_NONE should be set by calling legacy_player_set_display().
 * @see legacy_player_unset_media_packet_video_frame_decoded_cb
 */
int legacy_player_set_media_packet_video_frame_decoded_cb(player_h player, player_media_packet_video_decoded_cb callback, void *user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @pre The player's state should be #PLAYER_STATE_READY or #PLAYER_STATE_IDLE
 * @see legacy_player_set_media_packet_video_frame_decoded_cb()
 */
int legacy_player_unset_media_packet_video_frame_decoded_cb(player_h player);

/**
 * @brief  Pushes elementary stream to decode audio or video
 * @since_tizen 2.4
 * @remarks legacy_player_set_media_stream_info() should be called before using this API.
 * @remarks The available buffer size can be set by calling legacy_player_set_media_stream_buffer_max_size() API.
 *          If there is no available buffer space, this api will return error since 3.0.
 * @param[in]  player   The handle to media player
 * @param[in]  packet   The media packet to decode
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid state
 * @retval #PLAYER_ERROR_NOT_SUPPORTED_FILE File not supported
 * @retval #PLAYER_ERROR_BUFFER_SPACE No buffer space available (since 3.0)
 * @pre The player state must be set to #PLAYER_STATE_IDLE at least.
 * @see legacy_player_set_media_stream_info()
 * @see legacy_player_set_media_stream_buffer_max_size()
 */
int legacy_player_push_media_stream(player_h player, media_packet_h packet);

/**
 * @brief  Sets contents information for media stream
 * @since_tizen 2.4
 * @remarks AV format should be set before pushing elementary stream with legacy_player_push_media_stream().
 * @remarks AAC can be supported.
 * @remarks H.264 can be supported.
 * @remarks This API should be called before calling the legacy_player_prepare() or legacy_player_prepare_async() \n
            to reflect the media information when pipeline is created.
 * @param[in] player The handle to media player
 * @param[in] type   The type of target stream
 * @param[in] format The media format to set audio information
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid state
 * @pre The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare().
 * @see legacy_player_push_media_stream()
 */
int legacy_player_set_media_stream_info(player_h player, player_stream_type_e type, media_format_h format);

/**
 * @brief Registers a callback function to be invoked when buffer underrun or overflow is occurred.
 * @since_tizen 2.4
 * @remarks This API is used for media stream playback only.
 * @param[in] player   The handle to the media player
 * @param[in] type     The type of target stream
 * @param[in] callback The buffer status callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @pre The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare().
 * @post legacy_player_media_stream_buffer_status_cb() will be invoked.
 * @see legacy_player_unset_media_stream_buffer_status_cb()
 * @see legacy_player_media_stream_buffer_status_cb()
 */
int legacy_player_set_media_stream_buffer_status_cb(player_h player, player_stream_type_e type, player_media_stream_buffer_status_cb callback, void *user_data);

/**
 * @brief Unregisters the buffer status callback function.
 * @since_tizen 2.4
 * @remarks This API is used for media stream playback only.
 * @param[in] player The handle to the media player
 * @param[in] type   The type of target stream
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see legacy_player_set_media_stream_buffer_status_cb()
 */
int legacy_player_unset_media_stream_buffer_status_cb(player_h player, player_stream_type_e type);

/**
 * @brief Registers a callback function to be invoked when seeking is occurred.
 * @since_tizen 2.4
 * @remarks This API is used for media stream playback only.
 * @param[in] player    The handle to the media player
 * @param[in] type      The type of target stream
 * @param[in] callback  The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @pre The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare().
 * @post legacy_player_media_stream_seek_cb() will be invoked.
 * @see legacy_player_unset_media_stream_seek_cb()
 * @see legacy_player_media_stream_seek_cb()
 */
int legacy_player_set_media_stream_seek_cb(player_h player, player_stream_type_e type, player_media_stream_seek_cb callback, void *user_data);

/**
 * @brief Unregisters the seek callback function.
 * @since_tizen 2.4
 * @param[in] player The handle to the media player
 * @param[in] type   The type of target stream
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see legacy_player_set_media_stream_seek_cb()
 */
int legacy_player_unset_media_stream_seek_cb(player_h player, player_stream_type_e type);

/**
 * @brief Sets the max size bytes of buffer.
 * @since_tizen 2.4
 * @remarks This API is used for media stream playback only.
 * @remarks If the buffer level over the max size, legacy_player_media_stream_buffer_status_cb() will be invoked with overflow status.
 * @param[in] player The handle to the media player
 * @param[in] type   The type of target stream
 * @param[in] max_size The max bytes of buffer, it has to be bigger than zero. (default: 200000)
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED. (since 3.0)
 * @see legacy_player_get_media_stream_buffer_max_size()
 * @see legacy_player_media_stream_buffer_status_cb()
 */
int legacy_player_set_media_stream_buffer_max_size(player_h player, player_stream_type_e type, unsigned long long max_size);

/**
 * @brief Gets the max size bytes of buffer.
 * @since_tizen 2.4
 * @remarks This API is used for media stream playback only.
 * @remarks If the buffer level over the max size, legacy_player_media_stream_buffer_status_cb() will be invoked with overflow status.
 * @param[in] player The handle to the media player
 * @param[in] type   The type of target stream
 * @param[out] max_size The max bytes of buffer
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_set_media_stream_buffer_max_size()
 * @see legacy_player_media_stream_buffer_status_cb()
 */
int legacy_player_get_media_stream_buffer_max_size(player_h player, player_stream_type_e type, unsigned long long *max_size);

/**
 * @brief Sets the buffer threshold percent of buffer.
 * @since_tizen 2.4
 * @remarks This API is used for media stream playback only.
 * @remarks If the buffer level drops below the percent value, legacy_player_media_stream_buffer_status_cb() will be invoked with underrun status.
 * @param[in] player The handle to the media player
 * @param[in] type   The type of target stream
 * @param[in] percent The minimum threshold(0~100) of buffer (default: 0)
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED. (since 3.0)
 * @see legacy_player_get_media_stream_buffer_min_threshold()
 * @see legacy_player_media_stream_buffer_status_cb()
 */
int legacy_player_set_media_stream_buffer_min_threshold(player_h player, player_stream_type_e type, unsigned int percent);

/**
 * @brief Gets the buffer threshold percent of buffer.
 * @since_tizen 2.4
 * @remarks This API is used for media stream playback only.
 * @remarks If the buffer level drops below the percent value, legacy_player_media_stream_buffer_status_cb() will be invoked with underrun status.
 * @param[in] player The handle to the media player
 * @param[in] type   The type of target stream
 * @param[out] percent The minimum threshold(0~100) of buffer
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_set_media_stream_buffer_min_threshold()
 * @see legacy_player_media_stream_buffer_status_cb()
 */
int legacy_player_get_media_stream_buffer_min_threshold(player_h player, player_stream_type_e type, unsigned int *percent);

/**
 * @}
 */

/**
 * @addtogroup CAPI_MEDIA_PLAYER_DISPLAY_MODULE
 * @{
 */

/**
 * @brief Sets the video display mode.
 * @since_tizen 2.3
 * @remarks If no display is set, no operation is performed.
 * @param[in] player   The handle to the media player
 * @param[in] mode The display mode
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid state
 * @pre The player should support display mode changes.
 * @see	legacy_player_get_display_mode()
 */
int legacy_player_set_display_mode(player_h player, player_display_mode_e mode);

/**
 * @brief Gets the video display mode.
 * @since_tizen 2.3
 * @remarks If no display is set, no operation is performed.
 * @param[in] player The handle to the media player
 * @param[out] mode The current display mode
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval  #PLAYER_ERROR_NONE Successful
 * @retval  #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see	legacy_player_set_display_mode()
 */
int legacy_player_get_display_mode(player_h player, player_display_mode_e *mode);

/**
 * @brief Sets the visibility of the video surface display
 * @since_tizen 2.3
 * @param[in] player   The handle to the media player
 * @param[in] visible The visibility of the display (@c true = visible, @c false = non-visible )
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid state
 * @see	legacy_player_is_display_visible()
 */
int legacy_player_set_display_visible(player_h player, bool visible);

/**
 * @brief Gets the visibility of the video surface display.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[out] visible The current visibility of the display (@c true = visible, @c false = non-visible )
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval  #PLAYER_ERROR_NONE Successful
 * @retval  #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see	legacy_player_set_display_visible()
 */
int legacy_player_is_display_visible(player_h player, bool* visible);

/**
 * @brief Sets the rotation settings of the video surface display.
 * @since_tizen 2.3
 * @details Use this function to change the video orientation to portrait mode.
 * @param[in] player   The handle to the media player
 * @param[in] rotation The rotation of the display
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid state
 * @retval #PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE Unsupported feature
 * @see  legacy_player_set_display
 * @see  legacy_player_get_display_rotation()
 */
int legacy_player_set_display_rotation(player_h player, player_display_rotation_e rotation);

/**
 * @brief Gets the rotation of the video surface display.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[out] rotation The current rotation of the display
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval  #PLAYER_ERROR_NONE Successful
 * @retval  #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see     legacy_player_set_display_rotation()
 */
int legacy_player_get_display_rotation( player_h player, player_display_rotation_e *rotation);

/**
 * @}
 */


/**
 * @addtogroup CAPI_MEDIA_PLAYER_STREAM_INFO_MODULE
 * @{
 */

 /**
 * @brief Gets the media content information.
 * @since_tizen 2.3
 * @remarks You must release @a value using @c free().
 * @remarks The playback type should be local playback or HTTP streaming playback.
 * @param[in]  player The handle to the media player
 * @param[in] key The key attribute name to get
 * @param[out] value The value of the key attribute \n
 *                   It can be an empty string if there is no content information.
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #PLAYER_ERROR_OUT_OF_MEMORY Not enough memory is available
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING or #PLAYER_STATE_PAUSED.
 */
int legacy_player_get_content_info(player_h player, player_content_info_e key, char ** value);

/**
 * @brief Gets the audio and video codec information.
 * @since_tizen 2.3
 * @remarks You must release @a audio_codec and @a video_codec using free().
 * @remarks The playback type should be local playback or HTTP streaming playback.
 * @param[in] player The handle to the media player
 * @param[out] audio_codec The name of the audio codec \n
 *                         It can be @c NULL if there is no audio codec.
 * @param[out] video_codec The name of the video codec \n
 *                         It can be @c NULL if there is no video codec.
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING or #PLAYER_STATE_PAUSED.
 */
int legacy_player_get_codec_info(player_h player, char **audio_codec, char **video_codec);

/**
 * @brief Gets the audio stream information.
 * @since_tizen 2.3
 * @remarks The playback type should be local playback or HTTP streaming playback.
 * @param[in] player The handle to the media player
 * @param[out]  sample_rate The audio sample rate [Hz] \n
 *                          Value can be invalid if there is no audio stream information.
 * @param[out]  channel The audio channel (1: mono, 2: stereo) \n
 *                      Value can be invalid if there is no audio stream information.
 * @param[out]  bit_rate The audio bit rate [Hz] \n
 *                       Value can be invalid if there is no audio stream information.
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING or #PLAYER_STATE_PAUSED.
 */
int legacy_player_get_audio_stream_info(player_h player, int *sample_rate, int *channel, int *bit_rate);

/**
 * @brief Gets the video stream information.
 * @since_tizen 2.3
 * @remarks The playback type should be local playback or HTTP streaming playback.
 * @param[in] player The handle to the media player
 * @param[out]  fps The frame per second of the video \n
 *                  It can be @c 0 if there is no video stream information.
 * @param[out]  bit_rate The video bit rate [Hz] \n
 *                       It can be an invalid value if there is no video stream information.
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING or #PLAYER_STATE_PAUSED.
 */
int legacy_player_get_video_stream_info(player_h player, int *fps, int *bit_rate);

/**
 * @brief Gets the video display's height and width.
 * @since_tizen 2.3
 * @remarks The playback type should be local playback or HTTP streaming playback.
 * @param[in] player The handle to the media player
 * @param[out] width The width of the video \n
 *                   Value can be invalid if there is no video or no display is set.
 * @param[out] height The height of the video \n
 *                    Value can be invalid value if there is no video or no display is set.
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING or #PLAYER_STATE_PAUSED.
 */
int legacy_player_get_video_size(player_h player, int *width, int *height);

/**
 * @brief Gets the album art in the media resource.
 * @since_tizen 2.3
 * @remarks You must not release @a album_art. It will be released by framework when the player is destroyed.
 * @param[in] player The handle to the media player
 * @param[out] album_art The encoded artwork image
 * @param[out] size The encoded artwork size
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING or #PLAYER_STATE_PAUSED.
 */
int legacy_player_get_album_art(player_h player, void **album_art, int *size);

/**
 * @brief Gets the total running time of the associated media.
 * @since_tizen 2.3
 * @remarks The media source is associated with the player, using either legacy_player_set_uri() or legacy_player_set_memory_buffer().
 * @remarks The playback type should be local playback or HTTP streaming playback.
 * @param[in]   player The handle to the media player
 * @param[out]  duration The duration in milliseconds
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING or #PLAYER_STATE_PAUSED.
 * @see legacy_player_set_uri()
 * @see legacy_player_set_memory_buffer()
 */
int legacy_player_get_duration(player_h player, int *duration);

/**
 * @}
 */


/**
 * @addtogroup CAPI_MEDIA_PLAYER_AUDIO_EFFECT_MODULE
 * @{
 */

/**
 * @brief Gets the number of equalizer bands.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[out] count The number of equalizer bands
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_audio_effect_set_equalizer_band_level()
 * @see legacy_player_audio_effect_set_equalizer_all_bands()
 */
int legacy_player_audio_effect_get_equalizer_bands_count (player_h player, int *count);

/**
 * @brief Sets the gain set for the given equalizer band.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[in] index The index of the equalizer band to be set
 * @param[in] level The new gain in decibel that is set to the given band [dB]
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_audio_effect_get_equalizer_bands_count()
 * @see legacy_player_audio_effect_get_equalizer_level_range()
 * @see legacy_player_audio_effect_get_equalizer_band_level()
 * @see legacy_player_audio_effect_set_equalizer_all_bands()
 */
int legacy_player_audio_effect_set_equalizer_band_level(player_h player, int index, int level);

/**
 * @brief Gets the gain set for the given equalizer band.
 * @since_tizen 2.3
 * @param[in]   player The handle to the media player
 * @param[in]   index The index of the requested equalizer band
 * @param[out]   level The gain in decibel of the given band [dB]
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_audio_effect_set_equalizer_band_level()
 */
int legacy_player_audio_effect_get_equalizer_band_level(player_h player, int index, int *level);

/**
 * @brief Sets all bands of the equalizer.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[in] band_levels The list of band levels to be set
 * @param[in] length The length of the band level
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_audio_effect_get_equalizer_bands_count()
 * @see legacy_player_audio_effect_get_equalizer_level_range()
 * @see legacy_player_audio_effect_set_equalizer_band_level()
 */
int legacy_player_audio_effect_set_equalizer_all_bands(player_h player, int *band_levels, int length);

/**
 * @brief Gets the valid band level range of the equalizer.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[out] min The minimum value to be set [dB]
 * @param[out] max The maximum value to be set [dB]
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_audio_effect_set_equalizer_band_level()
 * @see legacy_player_audio_effect_set_equalizer_all_bands()
 */
int legacy_player_audio_effect_get_equalizer_level_range(player_h player, int* min, int* max);

/**
 * @brief Gets the band frequency of the equalizer.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[in]  index The index of the requested equalizer band
 * @param[out] frequency The frequency of the given band [dB]
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 */
int legacy_player_audio_effect_get_equalizer_band_frequency(player_h player, int index, int *frequency);

/**
 * @brief Gets the band frequency range of the equalizer.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[in]  index The index of the requested equalizer band
 * @param[out] range The frequency range of the given band [dB]
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 */
int legacy_player_audio_effect_get_equalizer_band_frequency_range(player_h player, int index, int *range);

/**
 * @brief Clears the equalizer effect.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_audio_effect_set_equalizer_band_level()
 * @see legacy_player_audio_effect_set_equalizer_all_bands()
 */
int legacy_player_audio_effect_equalizer_clear(player_h player);

/**
 * @brief Checks whether the custom equalizer effect is available.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[out] available If @c true the specified audio effect is available,
 *                       otherwise @c false
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @see legacy_player_audio_effect_set_equalizer_band_level()
 * @see legacy_player_audio_effect_set_equalizer_all_bands()
 */
int legacy_player_audio_effect_equalizer_is_available(player_h player, bool *available);

/**
 * @}
 */


/**
 * @addtogroup CAPI_MEDIA_PLAYER_MODULE
 * @{
 */

/**
 * @brief Captures the video frame, asynchronously.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[in] callback	The callback function to register
 * @param[in] user_data	The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 *		Video type should be set using legacy_player_set_display() otherwises, audio stream is only processed even though video file is set.
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be set to #PLAYER_STATE_PLAYING by calling legacy_player_start() or set to #PLAYER_STATE_PAUSED by calling legacy_player_pause().
 * @post It invokes legacy_player_video_captured_cb() when capture completes, if you set a callback.
 * @see legacy_player_video_captured_cb()
 */
int legacy_player_capture_video(player_h player, player_video_captured_cb callback, void *user_data);

/**
 * @brief Sets the cookie for streaming playback.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[in] cookie The cookie to set
 * @param[in] size The size of the cookie
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare().
 * @see legacy_player_set_streaming_user_agent()
 */
int legacy_player_set_streaming_cookie(player_h player, const char *cookie, int size);

/**
 * @brief Sets the streaming user agent for playback.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[in] user_agent The user agent to set
 * @param[in] size The size of the user agent
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare().
 * @see legacy_player_set_streaming_cookie()
 */
int legacy_player_set_streaming_user_agent(player_h player, const char *user_agent, int size);

/**
 * @brief Gets the download progress for streaming playback.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[out] start The starting position in percentage [0, 100]
 * @param[out] current The current position in percentage [0, 100]
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be set to #PLAYER_STATE_PLAYING by calling legacy_player_start() or set to #PLAYER_STATE_PAUSED by calling legacy_player_pause().
 */
int legacy_player_get_streaming_download_progress(player_h player, int *start, int *current);

/**
 * @brief Registers a callback function to be invoked when the playback is finished.
 * @since_tizen 2.3
 * @param[in] player	The handle to the media player
 * @param[in] callback	The callback function to register
 * @param[in] user_data	The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @post  legacy_player_completed_cb() will be invoked.
 * @see legacy_player_unset_completed_cb()
 * @see legacy_player_completed_cb()
 * @see legacy_player_start()
 */
int legacy_player_set_completed_cb(player_h player, player_completed_cb callback, void *user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @see legacy_player_set_completed_cb()
 */
int legacy_player_unset_completed_cb(player_h player);

/**
 * @brief Registers a callback function to be invoked when the playback is interrupted or the interrupt is completed.
 * @since_tizen 2.3
 * @param[in] player	The handle to the media player
 * @param[in] callback	The callback function to register
 * @param[in] user_data	The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @post  player_interrupted_cb() will be invoked.
 * @see legacy_player_unset_interrupted_cb()
 * @see #player_interrupted_code_e
 * @see legacy_player_interrupted_cb()
 */
int legacy_player_set_interrupted_cb(player_h player, player_interrupted_cb callback, void *user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @see legacy_player_set_interrupted_cb()
 */
int legacy_player_unset_interrupted_cb(player_h player);

/**
 * @brief Registers a callback function to be invoked when an error occurs.
 * @since_tizen 2.3
 * @param[in] player	The handle to the media player
 * @param[in] callback	The callback function to register
 * @param[in] user_data	The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @post  legacy_player_error_cb() will be invoked.
 * @see legacy_player_unset_error_cb()
 * @see legacy_player_error_cb()
 */
int legacy_player_set_error_cb(player_h player, player_error_cb callback, void *user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @see legacy_player_set_error_cb()
 */
int legacy_player_unset_error_cb(player_h player);

/**
 * @brief Registers a callback function to be invoked when there is a change in the buffering status of a media stream.
 * @since_tizen 2.3
 * @remarks The media resource should be streamed over the network.
 * @param[in] player	The handle to the media player
 * @param[in] callback	The callback function to register
 * @param[in] user_data	The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE Unsupported feature
 * @post  legacy_player_buffering_cb() will be invoked.
 * @see legacy_player_unset_buffering_cb()
 * @see legacy_player_set_uri()
 * @see legacy_player_buffering_cb()
 */
int legacy_player_set_buffering_cb(player_h player, player_buffering_cb callback, void *user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @see legacy_player_set_buffering_cb()
 */
int legacy_player_unset_buffering_cb(player_h player);

/**
 * @brief Sets a path to download, progressively.
 * @since_tizen 2.3
 * @remarks Progressive download will be started when you invoke legacy_player_start().
 * @param[in]   player The handle to the media player
 * @param[in]   path The absolute path to download
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE Unsupported feature
 * @pre The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare().
 * @see legacy_player_set_progressive_download_message_cb()
 * @see legacy_player_unset_progressive_download_message_cb()
 */
int legacy_player_set_progressive_download_path(player_h player, const char *path);

/**
 * @brief Gets the status of progressive download.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @param[out] current The current download position (bytes)
 * @param[out] total_size The total size of the file (bytes)
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The progressive download path must be set by calling legacy_player_set_progressive_download_path().
 * @pre The player state must be set to #PLAYER_STATE_PLAYING by calling legacy_player_start() or set to #PLAYER_STATE_PAUSED by calling legacy_player_pause().
 */
int legacy_player_get_progressive_download_status(player_h player, unsigned long *current, unsigned long *total_size);

/**
 * @brief Registers a callback function to be invoked when progressive download is started or completed.
 * @since_tizen 2.3
 * @param[in] player	The handle to the media player
 * @param[in] callback	The callback function to register
 * @param[in] user_data	The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE Unsupported feature
 * @pre The path to download must be set by calling legacy_player_set_progressive_download_path().
 * @post legacy_player_pd_message_cb() will be invoked.
 * @see legacy_player_unset_progressive_download_message_cb()
 * @see legacy_player_set_progressive_download_path()
 */
int legacy_player_set_progressive_download_message_cb(player_h player, player_pd_message_cb callback, void *user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @see legacy_player_set_progressive_download_message_cb()
 */
int legacy_player_unset_progressive_download_message_cb(player_h player);

/**
 * @brief Sets the playback rate.
 * @since_tizen 2.3
 * @details The default value is @c 1.0.
 * @remarks #PLAYER_ERROR_INVALID_OPERATION occurs when streaming playback.
 * @remarks No operation is performed, if @a rate is @c 0.
 * @remarks The sound is muted, when playback rate is under @c 0.0 and over @c 2.0.
 * @param[in]   player The handle to the media player
 * @param[in]   rate The playback rate (-5.0x ~ 5.0x)
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be set to #PLAYER_STATE_PLAYING by calling legacy_player_start().
 * @pre The player state must be set to #PLAYER_STATE_READY by calling legacy_player_prepare() or set to #PLAYER_STATE_PLAYING by calling legacy_player_start() or set to #PLAYER_STATE_PAUSED by calling legacy_player_pause().
 */
int legacy_player_set_playback_rate(player_h player, float rate);

/**
 * @}
 */

/**
 * @addtogroup CAPI_MEDIA_PLAYER_SUBTITLE_MODULE
 * @{
 */

/**
 * @brief Sets a subtitle path.
 * @since_tizen 2.3
 * @remarks Only MicroDVD/SubViewer(*.sub), SAMI(*.smi), and SubRip(*.srt) subtitle formats are supported.
 * @param[in]   player The handle to the media player
 * @param[in]   path The absolute path of the subtitle file, it can be @c NULL in the #PLAYER_STATE_IDLE state.
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The player state must be one of these: #PLAYER_STATE_IDLE, #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED.
 * @pre The path value can be @c NULL for reset when the player state is set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare().
 */
int legacy_player_set_subtitle_path(player_h player,const char *path);

/**
 * @brief Registers a callback function to be invoked when a subtitle updates.
 * @since_tizen 2.3
 * @param[in] player	The handle to the media player
 * @param[in] callback	The callback function to register
 * @param[in] user_data	The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @pre The subtitle must be set by calling legacy_player_set_subtitle_path().
 * @post  legacy_player_subtitle_updated_cb() will be invoked.
 * @see legacy_player_unset_subtitle_updated_cb()
 * @see legacy_player_subtitle_updated_cb()
 * @see legacy_player_set_subtitle_path()
 */
int legacy_player_set_subtitle_updated_cb(player_h player, player_subtitle_updated_cb callback, void *user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen 2.3
 * @param[in] player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @see legacy_player_set_subtitle_updated_cb()
 */
int legacy_player_unset_subtitle_updated_cb(player_h player);

/**
 * @brief Sets the seek position for the subtitle.
 * @since_tizen 2.3.1
 * @remarks Only MicroDVD/SubViewer(*.sub), SAMI(*.smi), and SubRip(*.srt) subtitle formats are supported.
 * @param[in]  player The handle to the media player
 * @param[in]  millisecond The position in milliseconds from the start to the seek point
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @pre The subtitle must be set by calling legacy_player_set_subtitle_path().
 * @pre The player state must be one of these: #PLAYER_STATE_PLAYING or #PLAYER_STATE_PAUSED.
 */
int legacy_player_set_subtitle_position_offset(player_h player, int millisecond);

/**
 * @brief Registers a callback function to be invoked when video stream is changed.
 * @since_tizen 2.4
 * @remarks The stream changing is detected just before rendering operation.
 * @param[in] player   The handle to the media player
 * @param[in] callback The stream changed callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @pre The player state must be set to #PLAYER_STATE_IDLE by calling legacy_player_create() or legacy_player_unprepare().
 * @post legacy_player_video_stream_changed_cb() will be invoked.
 * @see legacy_player_unset_video_stream_changed_cb()
 * @see legacy_player_video_stream_changed_cb()
 */
int legacy_player_set_video_stream_changed_cb (player_h player, player_video_stream_changed_cb callback, void *user_data);

/**
 * @brief Unregisters the video stream changed callback function.
 * @since_tizen 2.4
 * @param[in] player The handle to the media player
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see legacy_player_set_video_stream_changed_cb()
 */
int legacy_player_unset_video_stream_changed_cb (player_h player);

/**
 * @brief Gets current track index.
 * @since_tizen 2.4
 * @details Index starts from 0.
 * @remarks PLAYER_STREAM_TYPE_VIDEO is not supported.
 * @param[in] player The handle to the media player
 * @param[in] type The type of target stream
 * @param[out] index  The index of track
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_NOT_SUPPORTD Not supported
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED
 */
int legacy_player_get_current_track(player_h player, player_stream_type_e type, int *index);

/**
 * @brief Gets language code of a track.
 * @since_tizen 2.4
 * @remarks @a code must be released with @c free() by caller
 * @remarks PLAYER_STREAM_TYPE_VIDEO is not supported.
 * @param[in] player The handle to the media player
 * @param[in] type The type of target stream
 * @param[in] index  The index of track
 * @param[out] code A language code in ISO 639-1. "und" will be returned if the language is undefined.
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_NOT_SUPPORTD Not supported
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED
 */
int legacy_player_get_track_language_code(player_h player, player_stream_type_e type, int index, char **code);

/**
 * @brief Gets the track count.
 * @since_tizen 2.4
 * @remarks PLAYER_STREAM_TYPE_VIDEO is not supported.
 * @param[in] player The handle to the media player
 * @param[in] type The type of target stream
 * @param[out] count The number of track
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_NOT_SUPPORTD Not supported
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED
 */
int legacy_player_get_track_count(player_h player, player_stream_type_e type, int *count);

/**
 * @brief Selects a track to play.
 * @since_tizen 2.4
 * @remarks PLAYER_STREAM_TYPE_VIDEO is not supported.
 * @param[in] player The handle to the media player
 * @param[in] type The type of target stream
 * @param[in] index  The index of track
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #PLAYER_ERROR_NONE Successful
 * @retval #PLAYER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PLAYER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #PLAYER_ERROR_INVALID_STATE Invalid player state
 * @retval #PLAYER_ERROR_NOT_SUPPORTD Not supported
 * @pre The player state must be one of these: #PLAYER_STATE_READY, #PLAYER_STATE_PLAYING, or #PLAYER_STATE_PAUSED
 * @see legacy_player_get_current_track()
 */
int legacy_player_select_track(player_h player, player_stream_type_e type, int index);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_MEDIA_LEGACY_PLAYER_H__ */


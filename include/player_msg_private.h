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

#ifndef __PLAYER_MSG_PRIVATE_H__
#define __PLAYER_MSG_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <muse_core_msg_json.h>
#include <media_format.h>
#include <tbm_bufmgr.h>
#include <player2_private.h>

#define CALLBACK_TIME_OUT 5

typedef int32_t INT;
typedef int64_t INT64;
typedef intptr_t POINTER;
typedef double DOUBLE;
typedef const char* STRING;

typedef enum {
	PUSH_MEDIA_BUF_TYPE_TBM,
	PUSH_MEDIA_BUF_TYPE_MSG,
	PUSH_MEDIA_BUF_TYPE_RAW
} push_media_buf_type_e;

typedef struct _player_push_media_msg_type{
	push_media_buf_type_e buf_type;
	uint64_t size;
	uint64_t pts;
	media_format_mimetype_e mimetype;
	media_buffer_flags_e flags;
	tbm_key key;
}player_push_media_msg_type;

typedef struct {
	int type;
	intptr_t surface;
	int wl_window_x;
	int wl_window_y;
	int wl_window_width;
	int wl_window_height;
}wl_win_msg_type;

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[out] param the name of param is key, must be local variable. never be pointer.
 * @param[in] buf string of message buffer. has key and value
 */
#define player_msg_get(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, &param, NULL, MUSE_TYPE_ANY)

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[out] param the name of param is key, must be local variable. never be pointer.
 * @param[in] buf string of message buffer. has key and value
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 */
#define player_msg_get_type(param, buf, type) \
	muse_core_msg_json_deserialize(#param, buf, NULL, &param, NULL, MUSE_TYPE_##type)

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[out] param the name of param is key, must be local pointer variable.
 * @param[in] buf string of message buffer. has key and value
 */
#define player_msg_get_string(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, param, NULL, MUSE_TYPE_ANY)

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[out] param the name of param is key, must be local pointer variable.
 * @param[in] buf string of message buffer. has key and value
 */
#define player_msg_get_array(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, param, NULL, MUSE_TYPE_ANY)

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[out] param the name of param is key, must be local variable. never be pointer.
 * @param[in] buf string of message buffer. has key and value
 * @param[in/out] len size of buffer. After retrun len is set parsed length.
 * @param[out] e the error number.
 */
#define player_msg_get_error_e(param, buf, len, e) \
	muse_core_msg_json_deserialize(#param, buf, &len, &param, &e, MUSE_TYPE_INT)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The handle of capi media player.
 * @param[out] retbuf The buffer of return message. Must be char pointer.
 * @param[out] ret The return value from server.
 */
#define player_msg_send(api, player, retbuf, ret) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int __fd__; \
		int __timeout__ = _get_api_timeout(player, api); \
		if(CALLBACK_INFO(player)) __fd__ = MSG_FD(player); \
		else {ret = PLAYER_ERROR_INVALID_STATE;break;} \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				0); \
		__len__ = muse_core_ipc_send_msg(__fd__, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, CALLBACK_INFO(player), &retbuf, __timeout__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The handle of capi media player.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_send1(api, player, retbuf, ret, type, param) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int __fd__; \
		int __timeout__ = _get_api_timeout(player, api); \
		type __value__ = (type)param; \
		if(CALLBACK_INFO(player)) __fd__ = MSG_FD(player); \
		else {ret = PLAYER_ERROR_INVALID_STATE;break;} \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_##type, #param, __value__, \
				0); \
		__len__ = muse_core_ipc_send_msg(__fd__, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, CALLBACK_INFO(player), &retbuf, __timeout__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The handle of capi media player.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_send2(api, player, retbuf, ret, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int __fd__; \
		int __timeout__ = _get_api_timeout(player, api); \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		if(CALLBACK_INFO(player)) __fd__ = MSG_FD(player); \
		else {ret = PLAYER_ERROR_INVALID_STATE;break;} \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				0); \
		__len__ = muse_core_ipc_send_msg(__fd__, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, CALLBACK_INFO(player), &retbuf, __timeout__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The handle of capi media player.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_send3(api, player, retbuf, ret, type1, param1, type2, param2, type3, param3) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int __fd__; \
		int __timeout__ = _get_api_timeout(player, api); \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		if(CALLBACK_INFO(player)) __fd__ = MSG_FD(player); \
		else {ret = PLAYER_ERROR_INVALID_STATE;break;} \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				0); \
		__len__ = muse_core_ipc_send_msg(__fd__, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, CALLBACK_INFO(player), &retbuf, __timeout__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The handle of capi media player.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_send6(api, player, retbuf, ret, type1, param1, type2, param2, type3, param3, type4, param4, type5, param5, type6, param6) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int __fd__; \
		int __timeout__ = _get_api_timeout(player, api); \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		type5 __value5__ = (type5)param5; \
		type6 __value6__ = (type6)param6; \
		if(CALLBACK_INFO(player)) __fd__ = MSG_FD(player); \
		else {ret = PLAYER_ERROR_INVALID_STATE;break;} \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				MUSE_TYPE_##type4, #param4, __value4__, \
				MUSE_TYPE_##type5, #param5, __value5__, \
				MUSE_TYPE_##type6, #param6, __value6__, \
				0); \
		__len__ = muse_core_ipc_send_msg(__fd__, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, CALLBACK_INFO(player), &retbuf, __timeout__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The handle of capi media player.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] param the name of param is key, must be local array/pointer variable.
 * @param[in] length The size of array.
 * @param[in] datum_size The size of a array's datum.
 */
#define player_msg_send_array(api, player, retbuf, ret, param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int __fd__; \
		int __timeout__ = _get_api_timeout(player, api); \
		int *__value__ = (int *)param; \
		if(CALLBACK_INFO(player)) __fd__ = MSG_FD(player); \
		else {ret = PLAYER_ERROR_INVALID_STATE;break;} \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, #length, length, \
				MUSE_TYPE_ARRAY, #param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__value__, \
				0); \
		__len__ = muse_core_ipc_send_msg(__fd__, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, CALLBACK_INFO(player), &retbuf, __timeout__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The handle of capi media player.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] param# the name of param is key, must be local array/pointer variable.
 * @param[in] length# The size of array.
 * @param[in] datum_size# The size of a array's datum.
 */
#define player_msg_send_array2(api, player, retbuf, ret, param1, length1, datum_size1, param2, length2, datum_size2) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int __fd__; \
		int __timeout__ = _get_api_timeout(player, api); \
		int *__value1__ = (int *)param1; \
		int *__value2__ = (int *)param2; \
		if(CALLBACK_INFO(player)) __fd__ = MSG_FD(player); \
		else {ret = PLAYER_ERROR_INVALID_STATE;break;} \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, #length1, length1, \
				MUSE_TYPE_ARRAY, #param1, \
					datum_size1 == sizeof(int)? length1 :  \
					length1 / sizeof(int) + (length1 % sizeof(int)?1:0), \
					__value1__, \
				MUSE_TYPE_INT, #length2, length2, \
				MUSE_TYPE_ARRAY, #param2, \
					datum_size2 == sizeof(int)? length2 :  \
					length2 / sizeof(int) + (length2 % sizeof(int)?1:0), \
					__value2__, \
				0); \
		__len__ = muse_core_ipc_send_msg(__fd__, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, CALLBACK_INFO(player), &retbuf, __timeout__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)


/**
 * @brief Create and send message. Does not wait server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The handle of capi media player.
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_send1_async(api, player, type, param) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int __fd__; \
		type __value__ = (type)param; \
		if(CALLBACK_INFO(player)) __fd__ = MSG_FD(player); \
		else {ret = PLAYER_ERROR_INVALID_STATE;break;} \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_##type, #param, __value__, \
				0); \
		__len__ = muse_core_ipc_send_msg(__fd__, __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			return PLAYER_ERROR_INVALID_OPERATION; \
		} \
	}while(0)

/**
 * @brief Create and send message. Does not wait server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The server side handle of media player.
 * @param[in] fd socket fd
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_create_handle(api, fd, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				0); \
		__len__ = muse_core_ipc_send_msg(fd, __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			return PLAYER_ERROR_INVALID_OPERATION; \
		} \
	}while(0)


/**
 * @brief Create and send message for callback. Does not wait server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The handle of capi media player.
 * @param[out] ret set ERROR when fail to send msg.
 * @param[in] event_type type of event (_player_event_e).
 * @param[in] set 1 is set the user callback, 0 is unset the user callback.
 */
#define player_msg_set_callback(api, player, ret, event_type, set) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int __fd__; \
		int __value1__ = (int)event_type; \
		int __value2__ = (int)set; \
		if(CALLBACK_INFO(player)) __fd__ = MSG_FD(player); \
		else {ret = PLAYER_ERROR_INVALID_STATE;break;} \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, #event_type, __value1__, \
				MUSE_TYPE_INT, #set, __value2__, \
				0); \
		__len__ = muse_core_ipc_send_msg(__fd__, __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] ret Thre result of API.
 * @param[in] module mused module information
 */
#define player_msg_return(api, ret, module) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_RETURN, ret, \
				0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] ret Thre result of API.
 * @param[in] module mused module information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_return1(api, ret, module, type, param) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type __value__ = (type)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_RETURN, ret, \
				MUSE_TYPE_##type, #param, __value__, \
				0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] ret Thre result of API.
 * @param[in] module mused module information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_return2(api, ret, module, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_RETURN, ret, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] ret Thre result of API.
 * @param[in] module mused module information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_return3(api, ret, module, type1, param1, type2, param2, type3, param3) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_RETURN, ret, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] ret Thre result of API.
 * @param[in] module mused module information
 * @param[in] param the name of param is key, must be local array/pointer variable.
 * @param[in] length The size of array.
 * @param[in] datum_size The size of a array's datum.
 */
#define player_msg_return_array(api, ret, module, param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int *__value__ = (int *)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_RETURN, ret, \
				MUSE_TYPE_INT, #length, length, \
				MUSE_TYPE_ARRAY, #param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__value__, \
				0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] module mused module information
 */
#define player_msg_event(api, event, module) \
	do{	\
		char *__sndMsg__; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] module mused module information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_event1(api, event, module, type, param) \
	do{	\
		char *__sndMsg__; \
		type __value__ = (type)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				MUSE_TYPE_##type, #param, __value__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] module mused module information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_event2(api, event, module, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] module mused module information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_event3(api, event, module, type1, param1, type2, param2, type3, param3) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] module mused module information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_event4(api, event, module, type1, param1, type2, param2, type3, param3, type4, param4) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				MUSE_TYPE_##type4, #param4, __value4__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] module mused module information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 * @param[in] arr_param the name of param is key, must be local pointer/array variable.
 * @param[in] length The size of array.
 * @param[in] datum_size The size of a array's datum.
 */
#define player_msg_event2_array(api, event, module, type1, param1, type2, param2, arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_INT, #length, length, \
				MUSE_TYPE_ARRAY, #arr_param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__arr_value__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] module mused module information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 * @param[in] arr_param the name of param is key, must be local array/pointer variable.
 * @param[in] length The size of array.
 * @param[in] datum_size The size of a array's datum.
 */
#define player_msg_event6_array(api, event, module, type1, param1, type2, param2, type3, param3, type4, param4, type5, param5, type6, param6, arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		type5 __value5__ = (type5)param5; \
		type6 __value6__ = (type6)param6; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				MUSE_TYPE_##type4, #param4, __value4__, \
				MUSE_TYPE_##type5, #param5, __value5__, \
				MUSE_TYPE_##type6, #param6, __value6__, \
				MUSE_TYPE_INT, #length, length, \
				MUSE_TYPE_ARRAY, #arr_param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__arr_value__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)



#ifdef __cplusplus
}
#endif

#endif /*__PLAYER_MSG_PRIVATE_H__*/

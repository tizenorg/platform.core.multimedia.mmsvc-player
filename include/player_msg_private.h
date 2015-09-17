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

#include "mmsvc_core_msg_json.h"
#include "media_format.h"
#include "tbm_bufmgr.h"

#define CALLBACK_TIME_OUT 35

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
	mmsvc_core_msg_json_deserialize(#param, buf, &param, NULL)

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[out] param the name of param is key, must be local variable. never be pointer.
 * @param[in] buf string of message buffer. has key and value
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 */
#define player_msg_get_type(param, buf, type) \
	mmsvc_core_msg_json_deserialize_type(#param, buf, &param, NULL, MUSED_TYPE_##type)

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[out] param the name of param is key, must be local pointer variable.
 * @param[in] buf string of message buffer. has key and value
 */
#define player_msg_get_string(param, buf) \
	mmsvc_core_msg_json_deserialize(#param, buf, param, NULL)

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[out] param the name of param is key, must be local pointer variable.
 * @param[in] buf string of message buffer. has key and value
 */
#define player_msg_get_array(param, buf) \
	mmsvc_core_msg_json_deserialize(#param, buf, param, NULL)

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[out] param the name of param is key, must be local variable. never be pointer.
 * @param[in] buf string of message buffer. has key and value
 * @param[in/out] len size of buffer. After retrun len is set parsed length.
 * @param[out] e the error number.
 */
#define player_msg_get_error_e(param, buf, len, e) \
	mmsvc_core_msg_json_deserialize_len(#param, buf, &len, &param, &e, MUSED_TYPE_INT)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The server side handle of media player.
 * @param[in] fd socket fd
 * @param[in] cb_info The infomation of callback.
 * @param[out] retbuf The buffer of return message. Must be char pointer.
 * @param[out] ret The return value from server.
 */
#define player_msg_send(api, player, fd, cb_info, retbuf, ret) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_HANDLE, player, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, cb_info, &retbuf, CALLBACK_TIME_OUT); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The server side handle of media player.
 * @param[in] fd socket fd
 * @param[in] cb_info The infomation of callback.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_send1(api, player, fd, cb_info, retbuf, ret, type, param) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type __value__ = (type)param; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_HANDLE, player, \
				MUSED_TYPE_##type, #param, __value__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, cb_info, &retbuf, CALLBACK_TIME_OUT); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The server side handle of media player.
 * @param[in] fd socket fd
 * @param[in] cb_info The infomation of callback.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_send2(api, player, fd, cb_info, retbuf, ret, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_HANDLE, player, \
				MUSED_TYPE_##type1, #param1, __value1__, \
				MUSED_TYPE_##type2, #param2, __value2__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, cb_info, &retbuf, CALLBACK_TIME_OUT); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The server side handle of media player.
 * @param[in] fd socket fd
 * @param[in] cb_info The infomation of callback.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_send3(api, player, fd, cb_info, retbuf, ret, type1, param1, type2, param2, type3, param3) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_HANDLE, player, \
				MUSED_TYPE_##type1, #param1, __value1__, \
				MUSED_TYPE_##type2, #param2, __value2__, \
				MUSED_TYPE_##type3, #param3, __value3__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, cb_info, &retbuf, CALLBACK_TIME_OUT); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The server side handle of media player.
 * @param[in] fd socket fd
 * @param[in] cb_info The infomation of callback.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_send6(api, player, fd, cb_info, retbuf, ret, type1, param1, type2, param2, type3, param3, type4, param4, type5, param5, type6, param6) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		type5 __value5__ = (type5)param5; \
		type6 __value6__ = (type6)param6; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_HANDLE, player, \
				MUSED_TYPE_##type1, #param1, __value1__, \
				MUSED_TYPE_##type2, #param2, __value2__, \
				MUSED_TYPE_##type3, #param3, __value3__, \
				MUSED_TYPE_##type4, #param4, __value4__, \
				MUSED_TYPE_##type5, #param5, __value5__, \
				MUSED_TYPE_##type6, #param6, __value6__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, cb_info, &retbuf, CALLBACK_TIME_OUT); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The server side handle of media player.
 * @param[in] fd socket fd
 * @param[in] cb_info The infomation of callback.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] param the name of param is key, must be local array/pointer variable.
 * @param[in] length The size of array.
 * @param[in] datum_size The size of a array's datum.
 */
#define player_msg_send_array(api, player, fd, cb_info, retbuf, ret, param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int *__value__ = (int *)param; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_HANDLE, player, \
				MUSED_TYPE_INT, #length, length, \
				MUSED_TYPE_ARRAY, #param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__value__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, cb_info, &retbuf, CALLBACK_TIME_OUT); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send message. Wait for server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The server side handle of media player.
 * @param[in] fd socket fd
 * @param[in] cb_info The infomation of callback.
 * @param[out] retbuf The buffer of return message. Must be char pointer.Must free after use.
 * @param[out] ret The return value from server.
 * @param[in] param# the name of param is key, must be local array/pointer variable.
 * @param[in] length# The size of array.
 * @param[in] datum_size# The size of a array's datum.
 */
#define player_msg_send_array2(api, player, fd, cb_info, retbuf, ret, param1, length1, datum_size1, param2, length2, datum_size2) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int *__value1__ = (int *)param1; \
		int *__value2__ = (int *)param2; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_HANDLE, player, \
				MUSED_TYPE_INT, #length1, length1, \
				MUSED_TYPE_ARRAY, #param1, \
					datum_size1 == sizeof(int)? length1 :  \
					length1 / sizeof(int) + (length1 % sizeof(int)?1:0), \
					__value1__, \
				MUSED_TYPE_INT, #length2, length2, \
				MUSED_TYPE_ARRAY, #param2, \
					datum_size2 == sizeof(int)? length2 :  \
					length2 / sizeof(int) + (length2 % sizeof(int)?1:0), \
					__value2__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} else \
			ret = wait_for_cb_return(api, cb_info, &retbuf, CALLBACK_TIME_OUT); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
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
#define player_msg_send1_async(api, player, fd, type, param) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type __value__ = (type)param; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_HANDLE, player, \
				MUSED_TYPE_##type, #param, __value__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(fd, __sndMsg__); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
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
#define player_msg_send2_async(api, player, fd, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_HANDLE, player, \
				MUSED_TYPE_##type1, #param1, __value1__, \
				MUSED_TYPE_##type2, #param2, __value2__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(fd, __sndMsg__); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			return PLAYER_ERROR_INVALID_OPERATION; \
		} \
	}while(0)


/**
 * @brief Create and send message for callback. Does not wait server result.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] player The server side handle of media player.
 * @param[in] fd socket fd
 * @param[out] ret set ERROR when fail to send msg.
 * @param[in] event_type type of event (_player_event_e).
 * @param[in] set 1 is set the user callback, 0 is unset the user callback.
 */
#define player_msg_callback(api, player, fd, ret, event_type, set) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int __value1__ = (int)event_type; \
		int __value2__ = (int)set; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_HANDLE, player, \
				MUSED_TYPE_INT, #event_type, __value1__, \
				MUSED_TYPE_INT, #set, __value2__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(fd, __sndMsg__); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
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
 * @param[in] client socket client information
 */
#define player_msg_return(api, ret, client) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_RETURN, ret, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] ret Thre result of API.
 * @param[in] client socket client information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_return1(api, ret, client, type, param) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type __value__ = (type)param; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_RETURN, ret, \
				MUSED_TYPE_##type, #param, __value__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] ret Thre result of API.
 * @param[in] client socket client information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_return2(api, ret, client, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_RETURN, ret, \
				MUSED_TYPE_##type1, #param1, __value1__, \
				MUSED_TYPE_##type2, #param2, __value2__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] ret Thre result of API.
 * @param[in] client socket client information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_return3(api, ret, client, type1, param1, type2, param2, type3, param3) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_RETURN, ret, \
				MUSED_TYPE_##type1, #param1, __value1__, \
				MUSED_TYPE_##type2, #param2, __value2__, \
				MUSED_TYPE_##type3, #param3, __value3__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] ret Thre result of API.
 * @param[in] client socket client information
 * @param[in] param the name of param is key, must be local array/pointer variable.
 * @param[in] length The size of array.
 * @param[in] datum_size The size of a array's datum.
 */
#define player_msg_return_array(api, ret, client, param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int *__value__ = (int *)param; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_RETURN, ret, \
				MUSED_TYPE_INT, #length, length, \
				MUSED_TYPE_ARRAY, #param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__value__, \
				0); \
		__len__ = mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = PLAYER_ERROR_INVALID_OPERATION; \
		} \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] client socket client information
 */
#define player_msg_event(api, event, client) \
	do{	\
		char *__sndMsg__; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_EVENT, event, \
				0); \
		mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] client socket client information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_event1(api, event, client, type, param) \
	do{	\
		char *__sndMsg__; \
		type __value__ = (type)param; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_EVENT, event, \
				MUSED_TYPE_##type, #param, __value__, \
				0); \
		mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] client socket client information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_event2(api, event, client, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_EVENT, event, \
				MUSED_TYPE_##type1, #param1, __value1__, \
				MUSED_TYPE_##type2, #param2, __value2__, \
				0); \
		mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] client socket client information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_event3(api, event, client, type1, param1, type2, param2, type3, param3) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_EVENT, event, \
				MUSED_TYPE_##type1, #param1, __value1__, \
				MUSED_TYPE_##type2, #param2, __value2__, \
				MUSED_TYPE_##type3, #param3, __value3__, \
				0); \
		mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] client socket client information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_event4(api, event, client, type1, param1, type2, param2, type3, param3, type4, param4) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_EVENT, event, \
				MUSED_TYPE_##type1, #param1, __value1__, \
				MUSED_TYPE_##type2, #param2, __value2__, \
				MUSED_TYPE_##type3, #param3, __value3__, \
				MUSED_TYPE_##type4, #param4, __value4__, \
				0); \
		mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] client socket client information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 * @param[in] arr_param the name of param is key, must be local pointer/array variable.
 * @param[in] length The size of array.
 * @param[in] datum_size The size of a array's datum.
 */
#define player_msg_event2_array(api, event, client, type1, param1, type2, param2, arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_EVENT, event, \
				MUSED_TYPE_##type1, #param1, __value1__, \
				MUSED_TYPE_##type2, #param2, __value2__, \
				MUSED_TYPE_INT, #length, length, \
				MUSED_TYPE_ARRAY, #arr_param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__arr_value__, \
				0); \
		mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

/**
 * @brief Create and send return message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] api The enum of module API.
 * @param[in] event The event number.
 * @param[in] client socket client information
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 * @param[in] arr_param the name of param is key, must be local array/pointer variable.
 * @param[in] length The size of array.
 * @param[in] datum_size The size of a array's datum.
 */
#define player_msg_event6_array(api, event, client, type1, param1, type2, param2, type3, param3, type4, param4, type5, param5, type6, param6, arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		type5 __value5__ = (type5)param5; \
		type6 __value6__ = (type6)param6; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = mmsvc_core_msg_json_factory_new(api, PARAM_EVENT, event, \
				MUSED_TYPE_##type1, #param1, __value1__, \
				MUSED_TYPE_##type2, #param2, __value2__, \
				MUSED_TYPE_##type3, #param3, __value3__, \
				MUSED_TYPE_##type4, #param4, __value4__, \
				MUSED_TYPE_##type5, #param5, __value5__, \
				MUSED_TYPE_##type6, #param6, __value6__, \
				MUSED_TYPE_INT, #length, length, \
				MUSED_TYPE_ARRAY, #arr_param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__arr_value__, \
				0); \
		mmsvc_core_ipc_send_msg(mmsvc_core_client_get_msg_fd(client), __sndMsg__); \
		mmsvc_core_msg_json_factory_free(__sndMsg__); \
	}while(0)



#ifdef __cplusplus
}
#endif

#endif /*__PLAYER_MSG_PRIVATE_H__*/

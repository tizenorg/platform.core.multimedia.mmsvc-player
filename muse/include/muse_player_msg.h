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

#ifndef __TIZEN_MEDIA_MUSE_PLAYER_MSG_H__
#define __TIZEN_MEDIA_MUSE_PLAYER_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <tbm_bufmgr.h>
#include <media_packet.h>
#include <media_format.h>
#include <muse_core_ipc.h>
#include <muse_core_msg_json.h>

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
	int width;
	int height;
	media_format_mimetype_e mimetype;
	media_buffer_flags_e flags;
	tbm_key key;
}player_push_media_msg_type;

typedef struct {
	int type;
	unsigned int wl_surface_id;
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
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE)
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
 * @param[in] buf string of message buffer. has key and value
 * @param[out] param# the name of param is key, must be local variable. never be pointer.
 * @param[in] type# The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE)
 * @param[out] ret result of get value
 */
#define player_msg_get2(buf, param1, type1, param2, type2, ret) \
	do { \
		muse_core_msg_parse_err_e __err__ = MUSE_MSG_PARSE_ERROR_NONE; \
		void *__jobj__ = muse_core_msg_json_object_new(buf, NULL, &__err__); \
		if (!__jobj__) { \
			LOGE("failed to get msg object. err:%d", __err__); \
			ret = FALSE; \
		} else { \
			if (!muse_core_msg_json_object_get_value(#param1, __jobj__, &param1, MUSE_TYPE_##type1)) { \
				ret = FALSE; \
				LOGE("failed to get %s value", #param1); \
			} \
			if (ret && !muse_core_msg_json_object_get_value(#param2, __jobj__, &param2, MUSE_TYPE_##type2)) { \
				ret = FALSE; \
				LOGE("failed to get %s value", #param2); \
			} \
			muse_core_msg_json_object_free(__jobj__); \
		} \
	} while(0)

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] buf string of message buffer. has key and value
 * @param[out] param# the name of param is key, must be local variable. never be pointer.
 * @param[in] type# The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE)
 * @param[out] ret result of get value
 */
#define player_msg_get3(buf, param1, type1, param2, type2, param3, type3, ret) \
	do { \
		muse_core_msg_parse_err_e __err__ = MUSE_MSG_PARSE_ERROR_NONE; \
		void *__jobj__ = muse_core_msg_json_object_new(buf, NULL, &__err__); \
		if (!__jobj__) { \
			LOGE("failed to get msg object. err:%d", __err__); \
			ret = FALSE; \
		} else { \
			if (!muse_core_msg_json_object_get_value(#param1, __jobj__, &param1, MUSE_TYPE_##type1)) { \
				ret = FALSE; \
				LOGE("failed to get %s value", #param1); \
			} \
			if (ret && !muse_core_msg_json_object_get_value(#param2, __jobj__, &param2, MUSE_TYPE_##type2)) { \
				ret = FALSE; \
				LOGE("failed to get %s value", #param2); \
			} \
			if (ret && !muse_core_msg_json_object_get_value(#param3, __jobj__, &param3, MUSE_TYPE_##type3)) { \
				ret = FALSE; \
				LOGE("failed to get %s value", #param3); \
			} \
			muse_core_msg_json_object_free(__jobj__); \
		} \
	} while(0)

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] buf string of message buffer. has key and value
 * @param[out] param# the name of param is key, must be local variable. never be pointer.
 * @param[in] type# The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE)
 * @param[out] ret result of get value
 */
#define player_msg_get4(buf, param1, type1, param2, type2, param3, type3, param4, type4, ret) \
	do { \
		muse_core_msg_parse_err_e __err__ = MUSE_MSG_PARSE_ERROR_NONE; \
		void *__jobj__ = muse_core_msg_json_object_new(buf, NULL, &__err__); \
		if (!__jobj__) { \
			LOGE("failed to get msg object. err:%d", __err__); \
			ret = FALSE; \
		} else { \
			if (!muse_core_msg_json_object_get_value(#param1, __jobj__, &param1, MUSE_TYPE_##type1)) { \
				ret = FALSE; \
				LOGE("failed to get %s value", #param1); \
			} \
			if (ret && !muse_core_msg_json_object_get_value(#param2, __jobj__, &param2, MUSE_TYPE_##type2)) { \
				ret = FALSE; \
				LOGE("failed to get %s value", #param2); \
			} \
			if (ret && !muse_core_msg_json_object_get_value(#param3, __jobj__, &param3, MUSE_TYPE_##type3)) { \
				ret = FALSE; \
				LOGE("failed to get %s value", #param3); \
			} \
			if (ret && !muse_core_msg_json_object_get_value(#param4, __jobj__, &param4, MUSE_TYPE_##type4)) { \
				ret = FALSE; \
				LOGE("failed to get %s value", #param4); \
			} \
			muse_core_msg_json_object_free(__jobj__); \
		} \
	} while(0)

/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] buf string of message buffer. has key and value
 * @param[out] param# the name of param is key, must be local variable. never be pointer.
 * @param[in] type# The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE)
 * @param[out] str_param the name of param is key, must be local pointer variable.
 * @param[out] ret result of get value
 */
#define player_msg_get1_string(buf, param1, type1, str_param, ret) \
	do { \
		muse_core_msg_parse_err_e __err__ = MUSE_MSG_PARSE_ERROR_NONE; \
		void *__jobj__ = muse_core_msg_json_object_new(buf, NULL, &__err__); \
		if (!__jobj__) { \
			LOGE("failed to get msg object. err:%d", __err__); \
			ret = FALSE; \
		} else { \
			if (!muse_core_msg_json_object_get_value(#param1, __jobj__, &param1, MUSE_TYPE_##type1)) { \
				LOGE("failed to get %s value", #param1); \
				ret = FALSE; \
			} \
			if (ret && !muse_core_msg_json_object_get_value(#str_param, __jobj__, str_param, MUSE_TYPE_STRING)) { \
				LOGE("failed to get %s value", #str_param); \
				ret = FALSE; \
			} \
			muse_core_msg_json_object_free(__jobj__); \
		} \
	} while(0)


/**
 * @brief Get value from Message.
 * @remarks Does NOT guarantee thread safe.
 * @param[in] buf string of message buffer. has key and value
 * @param[out] param# the name of param is key, must be local variable. never be pointer.
 * @param[in] type# The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE)
 * @param[out] str_param the name of param is key, must be local pointer variable.
 * @param[out] ret result of get value
 */
#define player_msg_get2_string(buf, param1, type1, param2, type2, str_param, ret) \
	do { \
		muse_core_msg_parse_err_e __err__ = MUSE_MSG_PARSE_ERROR_NONE; \
		void *__jobj__ = muse_core_msg_json_object_new(buf, NULL, &__err__); \
		if (!__jobj__) { \
			LOGE("failed to get msg object. err:%d", __err__); \
			ret = FALSE; \
		} else { \
			if (!muse_core_msg_json_object_get_value(#param1, __jobj__, &param1, MUSE_TYPE_##type1)) { \
				LOGE("failed to get %s value", #param1); \
				ret = FALSE; \
			} \
			if (ret && !muse_core_msg_json_object_get_value(#param2, __jobj__, &param2, MUSE_TYPE_##type2)) { \
				LOGE("failed to get %s value", #param2); \
				ret = FALSE; \
			} \
			if (ret && !muse_core_msg_json_object_get_value(#str_param, __jobj__, str_param, MUSE_TYPE_STRING)) { \
				LOGE("failed to get %s value", #str_param); \
				ret = FALSE; \
			} \
			muse_core_msg_json_object_free(__jobj__); \
		} \
	} while(0)

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
 * @param[in] type The enum of parameter type. Muse be one of thease(INT, INT64, POINTER, DOUBLE, STRING, ARRAY)
 * @param[in] param# the name of param is key, must be local variable. never be pointer.
 */
#define player_msg_return4(api, ret, module, type1, param1, type2, param2, type3, param3, type4, param4) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_RETURN, ret, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				MUSE_TYPE_##type4, #param4, __value4__, \
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
 * @param[in] arr_param the name of param is key, must be local pointer/array variable.
 * @param[in] length The size of array.
 * @param[in] datum_size The size of a array's datum.
 */
#define player_msg_event_array(api, event, module, arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
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
 * @param[in] arr_param the name of param is key, must be local pointer/array variable.
 * @param[in] length The size of array.
 * @param[in] datum_size The size of a array's datum.
 */
#define player_msg_event1_array(api, event, module, type1, param1, arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
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
#define player_msg_event3_array(api, event, module, type1, param1, type2, param2, type3, param3,  arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
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
#define player_msg_event4_array(api, event, module, type1, param1, type2, param2, type3, param3, type4, param4, arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				MUSE_TYPE_##type4, #param4, __value4__, \
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
#define player_msg_event5_array(api, event, module, type1, param1, type2, param2, type3, param3, type4, param4, type5, param5, arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		type5 __value5__ = (type5)param5; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				MUSE_TYPE_##type4, #param4, __value4__, \
				MUSE_TYPE_##type5, #param5, __value5__, \
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
#define player_msg_event7_array(api, event, module, type1, param1, type2, param2, type3, param3, type4, param4, type5, param5, type6, param6, type7, param7, arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		type5 __value5__ = (type5)param5; \
		type6 __value6__ = (type6)param6; \
		type7 __value7__ = (type7)param7; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, MUSE_PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				MUSE_TYPE_##type4, #param4, __value4__, \
				MUSE_TYPE_##type5, #param5, __value5__, \
				MUSE_TYPE_##type6, #param6, __value6__, \
				MUSE_TYPE_##type7, #param7, __value7__, \
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
 * @param[in] module mused module information
 */
#define player_msg_create_ack(module) \
	do {	\
		char *__sndMsg__; \
		__sndMsg__ = muse_core_msg_json_factory_new(MUSE_PLAYER_CB_CREATE_ACK, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)



#ifdef __cplusplus
}
#endif

#endif /*__TIZEN_MEDIA_MUSE_PLAYER_MSG_H__*/

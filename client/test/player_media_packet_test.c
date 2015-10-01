/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Elementary.h>
#include <tbm_surface.h>
#include <dlog.h>
#include <player.h>
#include <glib.h>
#include <appcore-efl.h>

#define KEY_END "XF86Stop"
#define MEDIA_FILE_PATH "/home/owner/content/Color.mp4"
#ifdef PACKAGE
#undef PACKAGE
#endif
#define PACKAGE "player_test"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PLAYER_TEST"

static int app_create(void *data);
static int app_reset(bundle *b, void *data);
static int app_resume(void *data);
static int app_pause(void *data);
static int app_terminate(void *data);

struct appcore_ops ops = {
	.create = app_create,
	.terminate = app_terminate,
	.pause = app_pause,
	.resume = app_resume,
	.reset = app_reset,
};

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *img;
	media_packet_h packet;
	Ecore_Pipe *pipe;
	player_h player_handle;
	GList *packet_list;
	GMutex buffer_lock;
	int w, h;
} appdata_s;

static void
win_delete_request_cb(void *data , Evas_Object *obj , void *event_info)
{
	elm_exit();
}

static Eina_Bool
keydown_cb(void *data , int type , void *event)
{
	//appdata_s *ad = data;
	Ecore_Event_Key *ev = event;

	LOGD("start");

	if (!strcmp(ev->keyname, KEY_END)) {
		/* Let window go to hide state. */
		//elm_win_lower(ad->win);
		LOGD("elm exit");
		elm_exit();

		return ECORE_CALLBACK_DONE;
	}

	LOGD("done");

	return ECORE_CALLBACK_PASS_ON;
}

static void
create_base_gui(appdata_s *ad)
{
	/* Enable GLES Backened */
	elm_config_preferred_engine_set("3d");

	/* Window */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
    /* This is not supported in 3.0
	elm_win_wm_desktop_layout_support_set(ad->win, EINA_TRUE); */
	elm_win_autodel_set(ad->win, EINA_TRUE);
#if 0
	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}
#endif
	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, ad);

	Evas *e = evas_object_evas_get(ad->win);

	elm_win_screen_size_get(ad->win, NULL, NULL, &ad->w, &ad->h);
	LOGD("surface size (%d x %d)\n", ad->w, ad->h);

	/* Image Object */
	ad->img = evas_object_image_filled_add(e);
	evas_object_image_size_set(ad->img, ad->w, ad->h);
	evas_object_size_hint_weight_set(ad->img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(ad->img);

	elm_win_resize_object_add(ad->win, ad->img);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}

static void
_media_packet_video_decoded_cb(media_packet_h packet, void *user_data)
{
	/* This callback function would be called on different thread */
	appdata_s *ad = user_data;

	if (ad == NULL) {
		LOGE("appdata is NULL");
		return;
	}

	g_mutex_lock(&ad->buffer_lock);

	if (ad->pipe == NULL) {
		media_packet_destroy(packet);
		LOGW("release media packet immediately");
		g_mutex_unlock(&ad->buffer_lock);
		return;
	}

	/* add packet list */
	ad->packet_list = g_list_prepend(ad->packet_list, (gpointer)packet);

	LOGD("packet %p", packet);

	/* Send packet to main thread */
	ecore_pipe_write(ad->pipe, &packet, sizeof(media_packet_h));

	g_mutex_unlock(&ad->buffer_lock);

	return;
}

static void
pipe_cb(void *data, void *buf, unsigned int len)
{
	/* Now, we get a player surface to be set here. */
	appdata_s *ad = data;
	tbm_surface_h surface;
#ifdef _CAN_USE_NATIVE_SURFACE_TBM
	Evas_Native_Surface surf;
#endif
	tbm_surface_info_s suf_info;
	uint32_t plane_idx;
	int ret;
	GList *last_item = NULL;

	LOGD("start");

	g_mutex_lock(&ad->buffer_lock);

	/* Destroy previous packet */
	if (ad->packet) {
		ret = media_packet_destroy(ad->packet);
		if (ret != MEDIA_PACKET_ERROR_NONE) {
			LOGE("Failed to destroy media packet. ret (%d)", ret);
		}
		ad->packet = NULL;
	}

	/* remove packet from list */
	last_item = g_list_last(ad->packet_list);
	if (last_item) {
		/* Get new packet */
		ad->packet = (media_packet_h)last_item->data;;
		ad->packet_list = g_list_remove(ad->packet_list, ad->packet);
		LOGD("ad->packet %p", ad->packet);
	}

	if (ad->packet == NULL) {
		LOGW("NULL packet");
		g_mutex_unlock(&ad->buffer_lock);
		return;
	}

	ret = media_packet_get_tbm_surface(ad->packet, &surface);
	if (ret != MEDIA_PACKET_ERROR_NONE) {
		LOGE("Failed to get surface from media packet. ret(0x%x)", ret);

		media_packet_destroy(ad->packet);
		ad->packet = NULL;

		g_mutex_unlock(&ad->buffer_lock);

		return;
	}

	LOGD("surface %p", surface);

	g_mutex_unlock(&ad->buffer_lock);

#ifdef _CAN_USE_NATIVE_SURFACE_TBM
	/* Set tbm surface to image native surface */
	memset(&surf, 0x0, sizeof(surf));
	surf.version = EVAS_NATIVE_SURFACE_VERSION;
	surf.type = EVAS_NATIVE_SURFACE_TBM;
	surf.data.tizen.buffer = surface;
	surf.data.tizen.rot = 270;
	evas_object_image_native_surface_set(ad->img, &surf);

	/* Set dirty image region to be redrawn */
	evas_object_image_data_update_add(ad->img, 0, 0, ad->w, ad->h);
#else
	unsigned char *ptr = NULL;
	unsigned char *buf_data = NULL;
	media_format_h format = NULL;
	media_format_mimetype_e mimetype;

	media_packet_get_format(ad->packet, &format);
	media_format_get_video_info(format, &mimetype, NULL, NULL, NULL, NULL);

	if (mimetype == MEDIA_FORMAT_I420 || mimetype == MEDIA_FORMAT_NV12 || mimetype == MEDIA_FORMAT_NV12T) {

		tbm_surface_get_info(surface,&suf_info);
		buf_data = (unsigned char*)g_malloc0(suf_info.size);
		ptr = buf_data;

		for (plane_idx = 0; plane_idx < suf_info.num_planes; plane_idx++) {
			memcpy(ptr, suf_info.planes[plane_idx].ptr, suf_info.planes[plane_idx].size);
			ptr += suf_info.planes[plane_idx].size;
		}
		/* dump buf data here, if needed */
		g_free(buf_data);
	}
#endif

	LOGD("done");

	return;
}

static int app_create(void *data)
{
	/* Hook to take necessary actions before main event loop starts
	   Initialize UI resources and application's data
	   If this function returns true, the main loop of application starts
	   If this function returns false, the application is terminated */
	appdata_s *ad = data;

	LOGD("start");

	create_base_gui(ad);
	ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, keydown_cb, NULL);

	g_mutex_init(&ad->buffer_lock);

	LOGD("done");

	return 0;
}

static int app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
	appdata_s *ad = (appdata_s *)data;
	GList *list = NULL;
	media_packet_h packet = NULL;
	int ret = PLAYER_ERROR_NONE;

	LOGD("start");

	if (ad == NULL) {
		LOGE("appdata is NULL");
		return -1;
	}

	if (ad->player_handle == NULL) {
		g_print("player_handle is NULL");
		return -1;
	}

	/* stop render last set frame */
	evas_object_image_native_surface_set(ad->img, NULL);

	g_mutex_lock(&ad->buffer_lock);

	/* remove ecore pipe */
	ecore_pipe_del(ad->pipe);
	ad->pipe = NULL;

	/* remove packet list */
	list = ad->packet_list;
	while (list) {
		packet = list->data;
		list = g_list_next(list);

		if (!packet) {
			LOGW("packet is NULL");
		} else {
			LOGD("destroy packet %p", packet);
			media_packet_destroy(packet);
			packet = NULL;
			ad->packet_list = g_list_remove(ad->packet_list, packet);
		}
	}

	if (ad->packet_list) {
		g_list_free(ad->packet_list);
		ad->packet_list = NULL;
	}

	/* Destroy previous packet */
	if (ad->packet) {
		LOGD("destroy packet %p", ad->packet);
		ret = media_packet_destroy(ad->packet);
		if (ret != MEDIA_PACKET_ERROR_NONE) {
			LOGE("Failed to destroy media packet. ret (%d)", ret);
		}
		ad->packet = NULL;
	}

	g_mutex_unlock(&ad->buffer_lock);

	ret = player_unset_media_packet_video_frame_decoded_cb(ad->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		g_print("player_unset_media_packet_video_frame_decoded_cb failed : 0x%x", ret);
		return false;
	}

	ret = player_unprepare(ad->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		g_print("player_unprepare failed : 0x%x", ret);
		return false;
	}

	ret = player_destroy(ad->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		g_print("player_destroy failed : 0x%x", ret);
		return false;
	}

	ad->player_handle = NULL;

	LOGD("done");

	return 0;
}

static int app_resume(void *data)
{
	LOGD("start");

	LOGD("done");

	return 0;
}

static int app_reset(bundle *b, void *data)
{
	/* Take necessary actions when application becomes visible. */
	appdata_s *ad = (appdata_s *)data;
	int ret = PLAYER_ERROR_NONE;

	LOGD("start");

	if (ad == NULL) {
		LOGE("appdata is NULL");
		return -1;
	}

	/* create ecore pipe */
	ad->pipe = ecore_pipe_add(pipe_cb, ad);

	ret = player_create(&ad->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player_create failed : 0x%x", ret);
		return -1;
	}

	ret = player_set_media_packet_video_frame_decoded_cb(ad->player_handle, _media_packet_video_decoded_cb, ad);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player_set_media_packet_video_frame_decoded_cb failed : 0x%x", ret);
		goto FAILED;
	}

	ret = player_set_display(ad->player_handle, PLAYER_DISPLAY_TYPE_NONE, NULL);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player_set_display failed : 0x%x", ret);
		goto FAILED;
	}

	ret = player_set_uri(ad->player_handle, MEDIA_FILE_PATH);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player_set_uri failed : 0x%x", ret);
		goto FAILED;
	}

	ret = player_prepare(ad->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player prepare failed : 0x%x", ret);
		goto FAILED;
	}

	ret = player_start(ad->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player start failed : 0x%x", ret);
		goto FAILED;
	}

	LOGD("done");

	return 0;

FAILED:
	if (ad->player_handle) {
		player_destroy(ad->player_handle);
		ad->player_handle = NULL;
	}

	return -1;

}

static int app_terminate(void *data)
{
	/* Release all resources. */
	appdata_s *ad = (appdata_s *)data;

	LOGD("start");

	if (ad == NULL) {
		LOGE("appdata is NULL");
		return -1;
	}

	app_pause(data);

	g_mutex_clear(&ad->buffer_lock);

	LOGD("done");

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	static appdata_s ad = {0,};

	LOGD("start");

	memset(&ad, 0x0, sizeof(appdata_s));

	LOGD("call appcore_efl_main");

	ops.data = &ad;

	ret = appcore_efl_main(PACKAGE, &argc, &argv, &ops);

	LOGD("appcore_efl_main() ret = 0x%x", ret);

	return ret;
}


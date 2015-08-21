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
#ifdef HAVE_WAYLAND
#include <Ecore.h>
#include <Ecore_Wayland.h>
#endif

#define KEY_END "XF86Stop"
#define ES_FEEDING_PATH "es_buff://push_mode"
//#define ES_FEEDING_PATH "es_buff://pull_mode"

#define ES_DEFAULT_DIR_PATH			"/home/owner/content/"
#define ES_DEFAULT_H264_VIDEO_PATH		ES_DEFAULT_DIR_PATH"Simpsons.h264"
#define ES_DEFAULT_VIDEO_FORMAT_TYPE	MEDIA_FORMAT_H264_SP
#define ES_DEFAULT_VIDEO_FORMAT_WIDTH	1280
#define ES_DEFAULT_VIDEO_FORMAT_HEIGHT 544
#define ES_DEFAULT_VIDEO_PTS_OFFSET	20000000
#define ES_DEFAULT_NUMBER_OF_FEED		2000

unsigned char sps[100];
unsigned char pps[100];
unsigned char tmp_buf[1000000];
static int sps_len, pps_len;

#ifdef PACKAGE
#undef PACKAGE
#endif
#define PACKAGE "player_es_push_test"

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
	Evas_Object *rect;
	player_h player_handle;
	media_packet_h video_pkt;
	media_format_h video_fmt;
	FILE *file_src;
	pthread_t feeding_thread_id;
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

static void win_del(void *data, Evas_Object *obj, void *event)
{
		elm_exit();
}

static Evas_Object* create_win(const char *name)
{
	Evas_Object *eo = NULL;
	int w = 0;
	int h = 0;

	eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
	if (eo) {
		elm_win_title_set(eo, name);
		elm_win_borderless_set(eo, EINA_TRUE);
		evas_object_smart_callback_add(eo, "delete,request",win_del, NULL);
		elm_win_screen_size_get(eo, NULL, NULL, &w, &h);
		g_print ("window size :%d,%d", w, h);
		evas_object_resize(eo, w, h);
		elm_win_autodel_set(eo, EINA_TRUE);
#ifdef HAVE_WAYLAND
		elm_win_alpha_set(eo, EINA_TRUE);
#endif
	}
	return eo;
}

static Evas_Object *create_render_rect(Evas_Object *pParent)
{
	if(!pParent) {
		return NULL;
	}

	Evas *pEvas = evas_object_evas_get(pParent);
	Evas_Object *pObj = evas_object_rectangle_add(pEvas);
	if(pObj == NULL) {
		return NULL;
	}

	evas_object_size_hint_weight_set(pObj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_color_set(pObj, 0, 0, 0, 0);
	evas_object_render_op_set(pObj, EVAS_RENDER_COPY);
	evas_object_show(pObj);
	elm_win_resize_object_add(pParent, pObj);

	return pObj;
}

static void
create_base_gui(appdata_s *ad)
{
	/* Enable GLES Backened */
	elm_config_preferred_engine_set("3d");

	/* Window */
	ad->win = create_win(PACKAGE);//elm_win_util_standard_add(PACKAGE, PACKAGE);
	ad->rect = create_render_rect(ad->win);
        /* This is not supported in 3.0
	elm_win_wm_desktop_layout_support_set(ad->win, EINA_TRUE);*/
	elm_win_autodel_set(ad->win, EINA_TRUE);
	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, ad);

	/* Show window after base gui is set up */
	elm_win_activate(ad->win);
	evas_object_show(ad->win);
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

	/* open test file*/
	ad->file_src = fopen(ES_DEFAULT_H264_VIDEO_PATH, "r");

	LOGD("done");

	return 0;
}

static int app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
	appdata_s *ad = (appdata_s *)data;
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

	if (ad->feeding_thread_id)
	{
		pthread_join(ad->feeding_thread_id, NULL);
		ad->feeding_thread_id = 0;
	}

	player_unset_media_stream_buffer_status_cb(ad->player_handle, PLAYER_STREAM_TYPE_VIDEO);
	player_unset_media_stream_buffer_status_cb(ad->player_handle, PLAYER_STREAM_TYPE_AUDIO);
	player_unset_media_stream_seek_cb(ad->player_handle, PLAYER_STREAM_TYPE_VIDEO);
	player_unset_media_stream_seek_cb(ad->player_handle, PLAYER_STREAM_TYPE_AUDIO);

	ret = player_unprepare(ad->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		g_print("player_unprepare failed : 0x%x", ret);
		return false;
	}

	/* unref media format */
	if (ad->video_fmt)
		media_format_unref(ad->video_fmt);

	fclose(ad->file_src);

	/* destroy player handle */
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

static void _player_prepared_cb(void *user_data)
{
	int ret = PLAYER_ERROR_NONE;
	appdata_s *ad = (appdata_s *)user_data;

	LOGD("prepared");

	ret = player_start(ad->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player start failed : 0x%x", ret);
	}
	LOGD("done");
}

unsigned int bytestream2nalunit(FILE *fd, unsigned char* nal)
{
    int nal_length = 0;
    size_t result;
    int read_size = 1;
    unsigned char buffer[1000000];
    unsigned char val, zero_count, i;
    int nal_unit_type = 0;
    int init;

    zero_count = 0;
    if (feof(fd))
        return -1;

    result = fread(buffer, 1, read_size, fd);

    if(result != read_size)
    {
        //exit(1);
        return -1;
    }
    val = buffer[0];
    while (!val)
    {
        if ((zero_count == 2 || zero_count == 3) && val == 1)
        {
            break;
        }
        zero_count++;
        result = fread(buffer, 1, read_size, fd);

        if(result != read_size)
        {
            break;
        }
        val = buffer[0];
    }
    nal[nal_length++] = 0;
    nal[nal_length++] = 0;
    nal[nal_length++] = 0;
    nal[nal_length++] = 1;
    zero_count = 0;
    init = 1;
    while(1)
    {
        if (feof(fd))
            return nal_length;

        result = fread(buffer, 1, read_size, fd);
        if(result != read_size)
        {
            break;
        }
        val = buffer[0];

        if(init) {
            nal_unit_type = val & 0xf;
            init = 0;
        }
        if (!val)
        {
            zero_count++;
        }
        else
        {
            if ((zero_count == 2 || zero_count == 3 || zero_count == 4) && (val == 1))
            {
                break;
            }
            else
            {
                for (i = 0; i<zero_count; i++)
                {
                    nal[nal_length++] = 0;
                }
                nal[nal_length++] = val;
                zero_count = 0;
            }
        }
    }

    fseek(fd, -(zero_count + 1), SEEK_CUR);

    if (nal_unit_type == 0x7)
    {
        sps_len = nal_length;
        memcpy(sps, nal, nal_length);
        return 0;
    }
    else if (nal_unit_type == 0x8)
    {
        pps_len = nal_length;
        memcpy(pps, nal, nal_length);
        return 0;
    }
    else if (nal_unit_type == 0x5)
    {
        memcpy(tmp_buf, nal, nal_length);
        memcpy(nal, sps, sps_len);
        memcpy(nal + sps_len, pps, pps_len);
        memcpy(nal + sps_len + pps_len, tmp_buf, nal_length);
        nal_length += sps_len + pps_len;
    }

    return nal_length;
}

static void feed_video_data(appdata_s *appdata)
{
	int read = 0;
	static guint64 pts = 0L;
	void *buf_data_ptr = NULL;
	appdata_s *ad = appdata;

	if (media_packet_create_alloc(ad->video_fmt, NULL, NULL, &ad->video_pkt) != MEDIA_PACKET_ERROR_NONE) {
		LOGE("media_packet_create_alloc failed\n");
		return;
	}

	if (media_packet_get_buffer_data_ptr(ad->video_pkt, &buf_data_ptr) != MEDIA_PACKET_ERROR_NONE) {
		LOGE("media_packet_get_buffer_data_ptr failed\n");
		return;
	}

	if (media_packet_set_pts(ad->video_pkt, (uint64_t)(pts/1000000)) != MEDIA_PACKET_ERROR_NONE) {
		LOGE("media_packet_set_pts failed\n");
		return;
	}

	/* NOTE: In case of H.264 video, stream format for feeding is NAL unit.
	 * And, SPS(0x67) and PPS(0x68) should be located before IDR.(0x65).
	 */
	read = bytestream2nalunit(ad->file_src, buf_data_ptr);
	LOGD("real length = %d\n", read);
	if (read == 0) {
		LOGD("input file read failed\n");
		return;
	}

	if (media_packet_set_buffer_size(ad->video_pkt, (uint64_t)read) != MEDIA_PACKET_ERROR_NONE) {
		LOGE("media_packet_set_buffer_size failed\n");
		return;
	}

	/* push media packet */
	player_push_media_stream(ad->player_handle, ad->video_pkt);
	pts += ES_DEFAULT_VIDEO_PTS_OFFSET;

	/* destroy media packet after use*/
	media_packet_destroy(ad->video_pkt);
	ad->video_pkt = NULL;
	return;
}

static void feed_video_data_thread_func(void *data)
{
	gboolean exit = FALSE;
	appdata_s *ad = (appdata_s *)data;

	while (!exit)
	{
		static int frame_count = 0;

		if (frame_count < ES_DEFAULT_NUMBER_OF_FEED) {
			feed_video_data(ad);
			frame_count++;
		} else {
			exit = TRUE;
		}
	}
}

void _video_buffer_status_cb (player_media_stream_buffer_status_e status, void *user_data)
{
	if (status == PLAYER_MEDIA_STREAM_BUFFER_UNDERRUN)
	{
		LOGE("video buffer is underrun state");
	}
	else if (status == PLAYER_MEDIA_STREAM_BUFFER_OVERFLOW)
	{
		LOGE("video buffer is overrun state");
	}
}

void _audio_buffer_status_cb (player_media_stream_buffer_status_e status, void *user_data)
{
	if (status == PLAYER_MEDIA_STREAM_BUFFER_UNDERRUN)
		LOGE("audio buffer is underrun state");
	else if (status == PLAYER_MEDIA_STREAM_BUFFER_OVERFLOW)
		LOGE("audio buffer is overrun state");
}

void _video_seek_data_cb (unsigned long long offset, void *user_data)
{
	LOGE("seek offset of video is %llu", offset);
}

void _audio_seek_data_cb (unsigned long long offset, void *user_data)
{
	LOGE("seek offset of audio is %llu", offset);
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

	ret = player_create(&ad->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player_create failed : 0x%x", ret);
		return -1;
	}

	ret = player_set_display(ad->player_handle, PLAYER_DISPLAY_TYPE_OVERLAY, GET_DISPLAY(ad->win));
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player_set_display failed : 0x%x", ret);
		goto FAILED;
	}

	ret = player_set_uri(ad->player_handle, ES_FEEDING_PATH);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player_set_uri failed : 0x%x", ret);
		goto FAILED;
	}

	/* get media format format */
	ret = media_format_create(&ad->video_fmt);
	if (ret != MEDIA_FORMAT_ERROR_NONE) {
		LOGE("media_format_create : 0x%x", ret);
		goto FAILED;
	}

	/* set video format */
	media_format_set_video_mime(ad->video_fmt, ES_DEFAULT_VIDEO_FORMAT_TYPE);
	media_format_set_video_width(ad->video_fmt, ES_DEFAULT_VIDEO_FORMAT_WIDTH);
	media_format_set_video_height(ad->video_fmt,ES_DEFAULT_VIDEO_FORMAT_HEIGHT);

	ret = player_set_media_stream_buffer_status_cb(ad->player_handle, PLAYER_STREAM_TYPE_VIDEO, _video_buffer_status_cb, (void*)ad);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player set video buffer status cb failed : 0x%x", ret);
		goto FAILED;
	}
	ret = player_set_media_stream_buffer_status_cb(ad->player_handle, PLAYER_STREAM_TYPE_AUDIO, _audio_buffer_status_cb, (void*)ad);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player set audio buffer status cb failed : 0x%x", ret);
		goto FAILED;
	}

	ret = player_set_media_stream_seek_cb(ad->player_handle, PLAYER_STREAM_TYPE_VIDEO, _video_seek_data_cb, (void*)ad);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player set seek data cb for video failed : 0x%x", ret);
		goto FAILED;
	}
	ret = player_set_media_stream_seek_cb(ad->player_handle, PLAYER_STREAM_TYPE_AUDIO, _audio_seek_data_cb, (void*)ad);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player set seek data cb for audio failed : 0x%x", ret);
		goto FAILED;
	}

	/* send media packet to player */
	player_set_media_stream_info(ad->player_handle, PLAYER_STREAM_TYPE_VIDEO, ad->video_fmt);

	ret = player_prepare_async(ad->player_handle, _player_prepared_cb, (void*)ad);
	if (ret != PLAYER_ERROR_NONE) {
		LOGE("player prepare failed : 0x%x", ret);
		goto FAILED;
	}

	pthread_create(&ad->feeding_thread_id, NULL, (void*)feed_video_data_thread_func, (void *)ad);

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

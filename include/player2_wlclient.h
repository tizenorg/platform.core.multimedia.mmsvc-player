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


#ifndef __TIZEN_MEDIA_PLAYER_2_WLCLIENT_H__
#define __TIZEN_MEDIA_PLAYER_2_WLCLIENT_H__
#include <stdio.h>
#include <tbm_bufmgr.h>
#include <tbm_surface_internal.h>
#include <wayland-tbm-client.h>
#include <tizen-extension-client-protocol.h>
#include <wayland-client.h>
#include <gst/gst.h>
#include "mm_types.h"
#include "mm_debug.h"

#ifdef __cplusplus
extern "C"
{
#endif
  typedef struct
  {
    int x;
    int y;
    int width;
    int height;
  } wl_rect;

  typedef struct
  {
//  void *v4l2_ptr;
    int src_input_width;
    int src_input_height;
    int src_input_x;
    int src_input_y;
    int result_width;
    int result_height;
    int result_x;
    int result_y;
    int stereoscopic_info;
    int sw_hres;
    int sw_vres;
    int max_hres;
    int max_vres;
  } wl_meta;


  typedef struct
  {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_subcompositor *subcompositor;
    struct wl_shell *shell;
    struct wl_shm *shm;
    struct tizen_buffer_pool *tz_buffer_pool;
    struct wl_list buffer_list;
    struct wl_surface *surface; //video_surface
    struct wl_subsurface *subsurface;
    struct wl_event_queue *queue;

    void *set_handle;

    struct tizen_video_object *video_object;
    struct tizen_policy *tizen_policy;
    struct tizen_video *tizen_video;

    struct wl_buffer *wbuffer;
    struct wayland_tbm_client *tbm_client;
    tbm_bufmgr tbm_bufmgr;
    tbm_bo tbm_bo;
    tbm_surface_h tsurface;

    GstCaps *caps;

    int display_geo_method;
    int rotate_angle;

    wl_rect rect;
    wl_meta meta;

  } mm_wl_client;


  int mm_player_wlclient_create (mm_wl_client ** wlclient);
  int mm_player_wlclient_start (mm_wl_client * wlclient);
  int mm_player_wlclient_finalize (mm_wl_client * wlclient);
  int mm_player_wlclient_send_meta_buf (mm_wl_client * wlclient,
      char *string_caps);



#ifdef __cplusplus
}
#endif

#endif                          /* __TIZEN_MEDIA_PLAYER_2_WLCLIENT_H__ */

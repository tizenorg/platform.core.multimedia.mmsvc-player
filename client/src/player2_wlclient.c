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

#include <glib.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/video/video.h>

#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <dlog.h>
#include <mm_error.h>

#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>
#include <wayland-tbm-client.h>
#include <tizen-extension-client-protocol.h>
#include <wayland-client.h>
#include "player2_wlclient.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TIZEN_N_PLAYER_WLCLIENT"


#define goto_if_fail(expr,label)	\
{	\
	if (!(expr)) {	\
		debug_error(" failed [%s]\n", #expr);	\
		goto label;	\
	}	\
}

enum
{
  DISP_GEO_METHOD_LETTER_BOX = 0,
  DISP_GEO_METHOD_ORIGIN_SIZE,
  DISP_GEO_METHOD_FULL_SCREEN,
  DISP_GEO_METHOD_CROPPED_FULL_SCREEN,
  DISP_GEO_METHOD_ORIGIN_SIZE_OR_LETTER_BOX,
  DISP_GEO_METHOD_NUM,
};

enum
{
  DEGREE_0,
  DEGREE_90,
  DEGREE_180,
  DEGREE_270,
  DEGREE_NUM,
};

enum
{
  FLIP_NONE = 0,
  FLIP_HORIZONTAL,
  FLIP_VERTICAL,
  FLIP_BOTH,
  FLIP_NUM,
};


static void
buffer_release (void *data, struct wl_buffer *wbuffer)
{
  LOGD ("release wlbuffer (%p)", wbuffer);
  wl_buffer_destroy (wbuffer);
  wbuffer = NULL;
}

void mm_player_wlclient_add_meta_data (mm_wl_client * wlclient, wl_meta * meta,
    GstVideoInfo * info);


static const struct wl_buffer_listener buffer_listener = {
  buffer_release
};

static void
handle_global (void *data, struct wl_registry *registry,
    uint32_t id, const char *interface, uint32_t version)
{

  return_if_fail (data != NULL);
  mm_wl_client *wlclient = data;
  LOGD ("handle_global");

  if (g_strcmp0 (interface, "wl_compositor") == 0) {
    LOGD ("wl_registry_bind : wl_compositor");
    wlclient->compositor =
        wl_registry_bind (registry, id, &wl_compositor_interface, MIN (version,
            3));
  } else if (g_strcmp0 (interface, "wl_subcompositor") == 0) {
    LOGD ("wl_registry_bind : wl_subcompositor");
    wlclient->subcompositor =
        wl_registry_bind (registry, id, &wl_subcompositor_interface, 1);
  } else if (g_strcmp0 (interface, "wl_shell") == 0) {
    LOGD ("wl_registry_bind : wl_shell");
    wlclient->shell = wl_registry_bind (registry, id, &wl_shell_interface, 1);
  } else if (g_strcmp0 (interface, "wl_shm") == 0) {
    LOGD ("wl_registry_bind : wl_shm");
    wlclient->shm = wl_registry_bind (registry, id, &wl_shm_interface, 1);
  } else if (g_strcmp0 (interface, "tizen_policy") == 0) {
    LOGD ("wl_registry_bind : tizen_policy");
    wlclient->tizen_policy =
        wl_registry_bind (registry, id, &tizen_policy_interface, 1);
  } else if (g_strcmp0 (interface, "tizen_video") == 0) {
    LOGD ("wl_registry_bind : tizen_video");
    wlclient->tizen_video =
        wl_registry_bind (registry, id, &tizen_video_interface, version);
    g_return_if_fail (wlclient->tizen_video != NULL);
  }
}

static const struct wl_registry_listener registry_listener = {
  handle_global,
};

int
mm_player_wlclient_create (mm_wl_client ** wlclient)
{
  mm_wl_client *ptr = NULL;

  ptr = g_malloc0 (sizeof (mm_wl_client));
  if (!ptr) {
    LOGE ("Cannot allocate memory for wlclient\n");
    goto ERROR;
  } else {
    *wlclient = ptr;
    LOGD ("Success create wlclient(%p)", *wlclient);
  }

  gst_init (NULL, NULL);

  return MM_ERROR_NONE;

ERROR:
  *wlclient = NULL;
  return MM_ERROR_PLAYER_NO_FREE_SPACE;
}

int
mm_player_wlclient_start (mm_wl_client * wlclient)
{
  LOGD ("start wlclient");

  goto_if_fail (wlclient != NULL, ERROR);
  mm_wl_client *wlc = NULL;
  wlc = wlclient;
  goto_if_fail (wlc->display != NULL, ERROR);
  struct wl_region *region;

  wlc->registry = wl_display_get_registry (wlc->display);
  goto_if_fail (wlc->registry != NULL, ERROR);

  wl_registry_add_listener (wlc->registry, &registry_listener, wlc);
  wl_display_dispatch (wlc->display);
  wl_display_roundtrip (wlc->display);

  /* check global objects */
  goto_if_fail (wlc->compositor != NULL, ERROR);
  goto_if_fail (wlc->subcompositor != NULL, ERROR);
  goto_if_fail (wlc->shell != NULL, ERROR);
  goto_if_fail (wlc->shm != NULL, ERROR);
  goto_if_fail (wlc->tizen_policy != NULL, ERROR);
  goto_if_fail (wlc->tizen_video != NULL, ERROR);
  if (wlc->tizen_video) {
    wlc->tbm_client = wayland_tbm_client_init (wlc->display);
    if (!wlc->tbm_client) {
      LOGE ("Error initializing wayland-tbm");
      return MM_ERROR_PLAYER_INTERNAL;
    }
    LOGD ("tbm_client(%p)", wlc->tbm_client);
  }

  wlc->surface = wl_compositor_create_surface (wlc->compositor);
  goto_if_fail (wlc->surface != NULL, ERROR);

  region = wl_compositor_create_region (wlc->compositor);
  wl_surface_set_input_region (wlc->surface, region);
  wl_region_destroy (region);

  wlc->video_object = tizen_video_get_object (wlc->tizen_video, wlc->surface);

  wlc->subsurface = wl_subcompositor_get_subsurface (wlc->subcompositor, wlc->surface, wlc->set_handle);        //wlc->set_handle is parent
  wl_subsurface_set_desync (wlc->subsurface);

  if (wlc->tizen_policy)
    tizen_policy_place_subsurface_below_parent (wlc->tizen_policy,
        wlc->subsurface);
  wl_surface_commit (wlc->set_handle);

  return MM_ERROR_NONE;

ERROR:
  LOGD ("Failed to start wlclient");

  return MM_ERROR_PLAYER_INTERNAL;

}

int
mm_player_wlclient_send_meta_buf (mm_wl_client * wlclient, char *string_caps)
{

  goto_if_fail (wlclient != NULL, ERROR);
  mm_wl_client *wlc = NULL;
  wlc = wlclient;

  wl_meta *meta;
  tbm_bo_handle virtual_addr;
  tbm_surface_info_s ts_info;
  GstCaps *caps;
  GstVideoInfo info;
  int num_bo = 1;

  struct wl_buffer *wbuffer;

  if (!wlc->tbm_bufmgr)
    wlc->tbm_bufmgr = wayland_tbm_client_get_bufmgr (wlc->tbm_client);
  goto_if_fail (wlc->tbm_bufmgr != NULL, ERROR);

  //need to check meta data size
  if (!wlc->tbm_bo)
    wlc->tbm_bo =
        tbm_bo_alloc (wlc->tbm_bufmgr, sizeof (wlc->meta), TBM_BO_DEFAULT);
  if (!wlc->tbm_bo) {
    LOGE ("alloc tbm bo(size:%d) failed: %s", sizeof (wlc->meta),
        strerror (errno));
    return MM_ERROR_PLAYER_INTERNAL;
  }
  LOGD ("tbm_bo =(%p)", wlc->tbm_bo);
  virtual_addr.ptr = NULL;
  virtual_addr = tbm_bo_get_handle (wlc->tbm_bo, TBM_DEVICE_CPU);
  if (!virtual_addr.ptr) {
    LOGE ("get tbm bo handle failed: %s", strerror (errno));
    tbm_bo_unref (wlc->tbm_bo);
    wlc->tbm_bo = NULL;
    return MM_ERROR_PLAYER_INTERNAL;
  }

  memset (virtual_addr.ptr, 0, sizeof (wlc->meta));
  meta = (wl_meta *) malloc (sizeof (wlc->meta));
  goto_if_fail (meta != NULL, ERROR);
  LOGD ("meta(%p) size (%d)", meta, sizeof (wlc->meta));

  if (string_caps != NULL) {
    caps = gst_caps_from_string (string_caps);
    if (!gst_caps_is_equal (caps, wlc->caps))
      wlc->caps = gst_caps_copy (caps);
  }
  if (!wlc->caps)
    return MM_ERROR_PLAYER_INTERNAL;
  if (!gst_video_info_from_caps (&info, wlc->caps)) {
    LOGE ("Failed to get video info from caps");
  }
  // add meta data
  mm_player_wlclient_add_meta_data (wlc, meta, &info);
  memcpy (virtual_addr.ptr, meta, sizeof (wlc->meta));
  free (meta);

#if META_DEBUG
  wl_meta *debug_meta;
  debug_meta = (wl_meta *) virtual_addr.ptr;

  LOGD ("Debug Added meta : %d %d %d %d %d %d %d %d %d %d %d %d",
      debug_meta->src_input_width,
      debug_meta->src_input_height,
      debug_meta->src_input_x,
      debug_meta->src_input_y,
      debug_meta->result_x,
      debug_meta->result_y,
      debug_meta->result_width,
      debug_meta->result_height,
      debug_meta->sw_hres,
      debug_meta->sw_vres, debug_meta->max_hres, debug_meta->max_vres);
#endif

  //create wl_buffer
  ts_info.width = GST_VIDEO_INFO_WIDTH (&info);
  ts_info.height = GST_VIDEO_INFO_HEIGHT (&info);
  ts_info.format = GST_VIDEO_INFO_FORMAT (&info);
  ts_info.bpp = tbm_surface_internal_get_bpp (ts_info.format);
  ts_info.num_planes = tbm_surface_internal_get_num_planes (ts_info.format);
  ts_info.planes[0].stride = GST_VIDEO_INFO_PLANE_STRIDE (&info, 0);
  ts_info.planes[1].stride = GST_VIDEO_INFO_PLANE_STRIDE (&info, 1);
  ts_info.planes[2].stride = GST_VIDEO_INFO_PLANE_STRIDE (&info, 2);
  ts_info.planes[0].offset = GST_VIDEO_INFO_PLANE_OFFSET (&info, 0);
  ts_info.planes[1].offset = GST_VIDEO_INFO_PLANE_OFFSET (&info, 1);
  ts_info.planes[2].offset = GST_VIDEO_INFO_PLANE_OFFSET (&info, 2);
  wlc->tsurface =
      tbm_surface_internal_create_with_bos (&ts_info, &wlc->tbm_bo, num_bo);
  wbuffer = wayland_tbm_client_create_buffer (wlc->tbm_client, wlc->tsurface);

  wl_buffer_add_listener (wbuffer, &buffer_listener, wlc);

  if (wbuffer) {
    wl_surface_attach (wlc->surface, wbuffer, 0, 0);
    wl_surface_attach (wlc->surface, wbuffer, 0, 0);
  }

  return MM_ERROR_NONE;

ERROR:

  LOGD ("Failed to send meta buf");

  return MM_ERROR_PLAYER_INTERNAL;

}

void
mm_player_wlclient_add_meta_data (mm_wl_client * wlclient, wl_meta * meta,
    GstVideoInfo * info)
{
  LOGD ("start add meta data ");

  return_if_fail (wlclient != NULL);
  mm_wl_client *wlc = NULL;
  wlc = wlclient;
  return_if_fail (meta != NULL);

  GstVideoRectangle src = { 0, };
  GstVideoRectangle res;        //dst
  GstVideoRectangle src_origin = { 0, 0, 0, 0 };
  GstVideoRectangle src_input = { 0, 0, 0, 0 };
  GstVideoRectangle dst = { 0, 0, 0, 0 };

  src.w = GST_VIDEO_INFO_WIDTH (info);
  src.h = GST_VIDEO_INFO_HEIGHT (info);

  src.x = src.y = 0;
  src_input.w = src_origin.w = src.w;   //video source width
  src_input.h = src_origin.h = src.h;   //video source height

  if (wlc->rotate_angle == DEGREE_0 || wlc->rotate_angle == DEGREE_180) {
    src.w = GST_VIDEO_INFO_WIDTH (info);
    src.h = GST_VIDEO_INFO_HEIGHT (info);
  } else {
    src.w = GST_VIDEO_INFO_HEIGHT (info);
    src.h = GST_VIDEO_INFO_WIDTH (info);
  }

  /*default res.w and  res.h */
  dst.x = wlc->rect.x;
  dst.y = wlc->rect.y;
  dst.w = wlc->rect.width;
  dst.h = wlc->rect.height;

  switch (wlc->display_geo_method) {
    case DISP_GEO_METHOD_LETTER_BOX:
      LOGD ("DISP_GEO_METHOD_LETTER_BOX");
      gst_video_sink_center_rect (src, dst, &res, TRUE);
      gst_video_sink_center_rect (dst, src, &src_input, FALSE);
      res.x += wlc->rect.x;
      res.y += wlc->rect.y;
      break;
    case DISP_GEO_METHOD_ORIGIN_SIZE_OR_LETTER_BOX:
      if (src.w > dst.w || src.h > dst.h) {
        /*LETTER BOX */
        LOGD ("DISP_GEO_METHOD_ORIGIN_SIZE_OR_LETTER_BOX -> set LETTER BOX");
        gst_video_sink_center_rect (src, dst, &res, TRUE);
        gst_video_sink_center_rect (dst, src, &src_input, FALSE);
        res.x += wlc->rect.x;
        res.y += wlc->rect.y;
      } else {
        /*ORIGIN SIZE */
        GST_INFO ("DISP_GEO_METHOD_ORIGIN_SIZE");
        gst_video_sink_center_rect (src, dst, &res, FALSE);
        gst_video_sink_center_rect (dst, src, &src_input, FALSE);
      }
      break;
    case DISP_GEO_METHOD_ORIGIN_SIZE:  //is working
      LOGD ("DISP_GEO_METHOD_ORIGIN_SIZE");
      gst_video_sink_center_rect (src, dst, &res, FALSE);
      gst_video_sink_center_rect (dst, src, &src_input, FALSE);
      break;
    case DISP_GEO_METHOD_FULL_SCREEN:  //is working
      LOGD ("DISP_GEO_METHOD_FULL_SCREEN");
      res.x = res.y = 0;
      res.w = wlc->rect.width;
      res.h = wlc->rect.height;
      break;
    case DISP_GEO_METHOD_CROPPED_FULL_SCREEN:
      LOGD ("DISP_GEO_METHOD_CROPPED_FULL_SCREEN");
      gst_video_sink_center_rect (src, dst, &res, FALSE);
      gst_video_sink_center_rect (dst, src, &src_input, FALSE);
      res.x = res.y = 0;
      res.w = dst.w;
      res.h = dst.h;
      break;
    default:
      break;
  }

  //maybe don't need to rotate screen
#if 0
  switch (wlc->rotate_angle) {
    case FLIP_NONE:
      break;
    case FLIP_VERTICAL:
      transform = WL_OUTPUT_TRANSFORM_FLIPPED;
      break;
    case FLIP_HORIZONTAL:
      transform = WL_OUTPUT_TRANSFORM_FLIPPED_180;
      break;
    case FLIP_BOTH:
      transform = WL_OUTPUT_TRANSFORM_180;
      break;
    default:
      LOGD ("Unsupported flip [%d]... set FLIP_NONE.", window->flip);
  }
#endif
  LOGD ("Result !!! => window[%d,%d %d x %d] src[%d,%d,%d x %d],dst[%d,%d,%d x %d],input[%d,%d,%d x %d],result[%d,%d,%d x %d]", wlc->rect.x, wlc->rect.y, wlc->rect.width, wlc->rect.height, src.x, src.y, src.w, src.h, dst.x, dst.y, dst.w, dst.h, src_input.x, src_input.y, src_input.w, src_input.h, res.x, res.y, res.w, res.h);

  meta->src_input_width = src_input.w;
  meta->src_input_height = src_input.h;
  meta->src_input_x = src_input.x;
  meta->src_input_y = src_input.y;
  meta->result_x = res.x;
  meta->result_y = res.y;
  meta->result_width = res.w;
  meta->result_height = res.h;
  /*in case of S/W codec, we need to set sw_hres and sw_vres value.
     in case of H/W codec, these value are not used, it can be set noise */
  meta->sw_hres = src.w;
  meta->sw_vres = src.h;
  meta->max_hres = src.w;
  meta->max_vres = src.h;

  LOGD ("Added meta : %d %d %d %d %d %d %d %d %d %d %d %d",
      meta->src_input_width,
      meta->src_input_height,
      meta->src_input_x,
      meta->src_input_y,
      meta->result_x,
      meta->result_y,
      meta->result_width,
      meta->result_height,
      meta->sw_hres, meta->sw_vres, meta->max_hres, meta->max_vres);

  return;

}


int
mm_player_wlclient_finalize (mm_wl_client * wlclient)
{
  LOGD ("start finalize wlclient");
  return_val_if_fail (wlclient != NULL, false)
  mm_wl_client *wlc = NULL;
  wlc = wlclient;

  if (wlc->tbm_bo) {
    tbm_bo_unref (wlc->tbm_bo);
    wlc->tbm_bo = NULL;
  }
  LOGD ("start finalize wlclient");
  if (wlc->tbm_client) {
    wayland_tbm_client_deinit (wlc->tbm_client);
    wlc->tbm_client = NULL;
  }
  LOGD ("start finalize wlclient");
  if (wlc->shm)
    wl_shm_destroy (wlc->shm);
  LOGD ("start finalize wlclient");
  if (wlc->shell)
    wl_shell_destroy (wlc->shell);
  LOGD ("start finalize wlclient");
  if (wlc->compositor)
    wl_compositor_destroy (wlc->compositor);
  LOGD ("start finalize wlclient");
  if (wlc->subcompositor)
    wl_subcompositor_destroy (wlc->subcompositor);
  LOGD ("start finalize wlclient");
  if (wlc->registry)
    wl_registry_destroy (wlc->registry);
  LOGD ("start finalize wlclient");
  if (wlc->tizen_policy)
    tizen_policy_destroy (wlc->tizen_policy);
  LOGD ("start finalize wlclient");
  if (wlc->tizen_video)
    tizen_video_destroy (wlc->tizen_video);
  LOGD ("start finalize wlclient");
  if (wlc->display) {
    wl_display_flush (wlc->display);
    wl_display_disconnect (wlc->display);
  }
  LOGD ("start finalize wlclient");

  return MM_ERROR_NONE;
}

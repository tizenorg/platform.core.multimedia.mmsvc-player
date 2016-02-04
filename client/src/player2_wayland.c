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
#include <string.h>
#include <dlog.h>
#include <mm_error.h>
#include <wayland-client.h>
#include <tizen-extension-client-protocol.h>

#include "player2_wayland.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TIZEN_N_PLAYER"



#define goto_if_fail(expr,label)	\
{	\
	if (!(expr)) {	\
		debug_error(" failed [%s]\n", #expr);	\
		goto label;	\
	}	\
}

void handle_resource_id(void *data, struct tizen_resource *tizen_resource, uint32_t id)
{
    unsigned int *wl_surface_id = data;

    *wl_surface_id = id;

    LOGD("[CLIENT] got parent_id(%d) from server\n", id);
}

static const struct tizen_resource_listener tz_resource_listener =
{
    handle_resource_id,
};

static void
handle_global(void *data, struct wl_registry *registry,
              uint32_t name, const char *interface, uint32_t version)
{
	return_if_fail (data != NULL);
	wl_client *wlclient = data;

	if (strcmp(interface, "tizen_surface") == 0)
    {
		LOGD("binding tizen_surface");
        wlclient->tz_surface = wl_registry_bind(registry, name, &tizen_surface_interface, version);
        return_if_fail (wlclient->tz_surface != NULL);
    }
}

static const struct wl_registry_listener registry_listener =
{
    handle_global,
};

int _wlclient_create (wl_client ** wlclient)
{
  wl_client *ptr = NULL;

  ptr = g_malloc0 (sizeof (wl_client));
  if (!ptr) {
    LOGE ("Cannot allocate memory for wlclient\n");
    goto ERROR;
  } else {
    *wlclient = ptr;
    LOGD ("Success create wlclient(%p)", *wlclient);
  }
  return MM_ERROR_NONE;

ERROR:
  *wlclient = NULL;
  return MM_ERROR_PLAYER_NO_FREE_SPACE;
}


int _wlclient_get_wl_window_wl_surface_id (wl_client * wlclient, struct wl_surface *surface, struct wl_display *display)
{
	goto_if_fail (wlclient != NULL, failed);
	goto_if_fail (surface != NULL, failed);
	goto_if_fail (display != NULL, failed);

	unsigned int wl_surface_id = 0;

	wlclient->display = display;
	goto_if_fail (wlclient->display != NULL, failed);

    wlclient->registry = wl_display_get_registry(wlclient->display);
    goto_if_fail (wlclient->registry != NULL, failed);

    wl_registry_add_listener(wlclient->registry, &registry_listener, wlclient);
    wl_display_dispatch(wlclient->display);
    wl_display_roundtrip(wlclient->display);

	/* check global objects */
	goto_if_fail (wlclient->tz_surface != NULL, failed);

	/* Get wl_surface_id which is unique in a entire systemw. */
	wlclient->tz_resource = tizen_surface_get_tizen_resource(wlclient->tz_surface, surface);
	goto_if_fail (wlclient->tz_resource != NULL, failed);

    tizen_resource_add_listener(wlclient->tz_resource, &tz_resource_listener, &wl_surface_id);
	wl_display_roundtrip(wlclient->display);
	goto_if_fail (wl_surface_id > 0, failed);

	_wlclient_finalize(wlclient);

	return wl_surface_id;

failed:
  LOGE ("Failed to get wl_surface_id");

  return 0;
}

void _wlclient_finalize (wl_client * wlclient)
{
	LOGD ("start finalize wlclient");
	return_if_fail (wlclient != NULL)

	if (wlclient->tz_surface)
		tizen_surface_destroy(wlclient->tz_surface);

	if (wlclient->tz_resource)
		tizen_resource_destroy(wlclient->tz_resource);

    /* destroy registry */
    if (wlclient->registry)
        wl_registry_destroy(wlclient->registry);
  return;
}


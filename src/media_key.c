/*
 * Copyright (c) 2011 - 2016 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <unistd.h>
#include <string.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Input.h>
#if defined(X11)
#include <Ecore_X.h>
#include <utilX.h>
#elif defined(WAYLAND)
#include <Ecore_Wayland.h>
#endif

#include <dlog.h>

#include "media_key.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "CAPI_SYSTEM_MEDIA_KEY"

struct key_map {
	char *key_str;
	media_key_e key;
};

struct key_map media_keys[] = {
	{ "XF86AudioPlay", MEDIA_KEY_PLAY },
	{ "XF86AudioStop", MEDIA_KEY_STOP },
	{ "XF86AudioPause", MEDIA_KEY_PAUSE },
	{ "XF86AudioNext", MEDIA_KEY_NEXT },
	{ "XF86AudioPrev", MEDIA_KEY_PREVIOUS },
	{ "XF86AudioRewind", MEDIA_KEY_REWIND },
	{ "XF86AudioForward", MEDIA_KEY_FASTFORWARD },
	{ "XF86AudioPlayPause", MEDIA_KEY_PLAYPAUSE },
	{ "XF86AudioMedia", MEDIA_KEY_MEDIA },
	{ NULL, MEDIA_KEY_UNKNOWN },
};

static media_key_event_cb _media_key_event_cb;
static void *_media_key_data;
static int _media_key_initialized;

static Ecore_Event_Handler *media_key_up;
static Ecore_Event_Handler *media_key_down;

#if defined(X11)
static Ecore_X_Window win;
#endif

static int _media_key_init(void)
{
	if (_media_key_initialized)
		return 0;
#if defined(X11)
	ecore_x_init(NULL);

	win = ecore_x_window_input_new(ecore_x_window_root_first_get(), 0, 0, 1, 1);
	if (!win) {
		LOGE("failed to create input window");
		return -1;
	}

	ecore_x_icccm_title_set(win, "media key receiver");
	ecore_x_netwm_name_set(win, "media key receiver");
	ecore_x_netwm_pid_set(win, getpid());
#elif defined(WAYLAND)
	ecore_wl_init(NULL);
#endif
	_media_key_initialized = 1;

	return 0;
}

static void _media_key_fini(void)
{
#if defined(X11)
	ecore_x_window_free(win);
	ecore_x_shutdown();
#elif defined(WAYLAND)
	ecore_wl_shutdown();
#endif
	_media_key_initialized = 0;
}

static void _media_key_handler(const char *key_str, media_key_e event)
{
	int i;
	media_key_e key = MEDIA_KEY_UNKNOWN;

	for (i = 0; media_keys[i].key_str; i++) {
		if (!strcmp(key_str, media_keys[i].key_str)) {
			key = media_keys[i].key;
			break;
		}
	}

	if (_media_key_event_cb && key != MEDIA_KEY_UNKNOWN)
		_media_key_event_cb(key, event, _media_key_data);
}

static Eina_Bool _media_key_press_cb(void *data, int type, void *event)
{
	Evas_Event_Key_Down *ev;

	ev = event;
	if (!ev) {
		LOGE("Invalid event object");
		return ECORE_CALLBACK_RENEW;
	}

	_media_key_handler(ev->keyname, MEDIA_KEY_STATUS_PRESSED);

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool _media_key_release_cb(void *data, int type, void *event)
{
	Evas_Event_Key_Up *ev;

	ev = event;
	if (!ev) {
		LOGE("Invalid event object");
		return ECORE_CALLBACK_RENEW;
	}

	_media_key_handler(ev->keyname, MEDIA_KEY_STATUS_RELEASED);

	return ECORE_CALLBACK_RENEW;
}

static int _grab_media_key(void)
{
	int i;
#if defined(X11)
	int ret;
#elif defined(WAYLAND)
	Eina_Bool ret;
#endif


	for (i = 0; media_keys[i].key_str; i++) {
#if defined(X11)
		ret = utilx_grab_key(ecore_x_display_get(), win,
				media_keys[i].key_str, OR_EXCLUSIVE_GRAB);
		if (ret < 0) {
			LOGE("failed to grab key: %s", media_keys[i].key_str);
			for (i = i - 1; i >= 0; i--)
				utilx_ungrab_key(ecore_x_display_get(), win,
						media_keys[i].key_str);

			return ret;
		}
#elif defined(WAYLAND)
		ret = ecore_wl_window_keygrab_set(NULL, media_keys[i].key_str,
				0, 0, 0, ECORE_WL_WINDOW_KEYGRAB_OVERRIDE_EXCLUSIVE);
		if (ret != EINA_TRUE) {
			LOGE("failed to grab key: %s", media_keys[i].key_str);
			for (i = i - 1; i >= 0; i--)
				ecore_wl_window_keygrab_unset(NULL, media_keys[i].key_str, 0, 0);

			return -1;
		}
#endif
	}

	return 0;
}

static int _ungrab_media_key(void)
{
	int i;
#if defined(X11)
	int ret;
#elif defined(WAYLAND)
	Eina_Bool ret;
#endif

	for (i = 0; media_keys[i].key_str; i++) {
#if defined(X11)
		ret = utilx_ungrab_key(ecore_x_display_get(), win,
				media_keys[i].key_str);
		if (ret)
			LOGE("failed to ungrab key: %s", media_keys[i].key_str);
#elif defined(WAYLAND)
		ret = ecore_wl_window_keygrab_unset(NULL, media_keys[i].key_str, 0, 0);
		if (ret != EINA_TRUE)
			LOGE("failed to ungrab key: %s", media_keys[i].key_str);
#endif
	}

	return 0;
}

int media_key_reserve(media_key_event_cb callback, void *user_data)
{
	int ret;

	if (callback == NULL) {
		LOGE("[%s] media_key_event_cb callback is NULL", __FUNCTION__);
		return MEDIA_KEY_ERROR_INVALID_PARAMETER;
	}

	if (!_media_key_initialized) {
		if (_media_key_init())
			return MEDIA_KEY_ERROR_OPERATION_FAILED;
	}

	ret = _grab_media_key();
	if (ret) {
		LOGE("reserve media key error [%d]", ret);
		return MEDIA_KEY_ERROR_OPERATION_FAILED;
	}

	media_key_down = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _media_key_press_cb, NULL);
	if (!media_key_down)
		LOGE("failed to register key down event handler");


	media_key_up = ecore_event_handler_add(ECORE_EVENT_KEY_UP, _media_key_release_cb, NULL);
	if (!media_key_down)
		LOGE("failed to register key up event handler");

	_media_key_event_cb = callback;
	_media_key_data = user_data;

	return MEDIA_KEY_ERROR_NONE;
}

int media_key_release(void)
{
	int ret;

	if (!_media_key_initialized) {
		LOGI("media key is not reserved");
		return MEDIA_KEY_ERROR_NONE;
	}

	ret = _ungrab_media_key();
	if (ret) {
		LOGE("release media key error [%d]", ret);
		return MEDIA_KEY_ERROR_OPERATION_FAILED;
	}

	_media_key_fini();

	if (media_key_down) {
		ecore_event_handler_del(media_key_down);
		media_key_down = NULL;
	}

	if (media_key_up) {
		ecore_event_handler_del(media_key_up);
		media_key_up = NULL;
	}

	_media_key_event_cb = NULL;
	_media_key_data = NULL;

	return MEDIA_KEY_ERROR_NONE;
}

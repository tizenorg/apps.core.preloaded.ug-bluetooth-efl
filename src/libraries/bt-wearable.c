/*
* ug-bluetooth-efl
*
* Copyright 2012 Samsung Electronics Co., Ltd
*
* Contact: Hocheol Seo <hocheol.seo@samsung.com>
*           GirishAshok Joshi <girish.joshi@samsung.com>
*           DoHyun Pyun <dh79.pyun@samsung.com>
*
* Licensed under the Flora License, Version 1.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.tizenopensource.org/license
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include "bt-debug.h"
#include "bt-main-ug.h"
#include "bt-main-view.h"
#include "bt-util.h"
#include "bt-ipc-handler.h"
#include "bt-widget.h"

#ifdef TIZEN_REDWOOD
#include <app_info.h>
#include <app_manager.h>

static bool is_enabled = false;
#define GEAR_MANAGER_APP_ID  "org.tizen.unifiedwearablemanager"

static bool __get_appinfo_cb(app_info_h app_info, void *user_data)
{
	BT_DBG("");

	char *package = NULL;
	int ret = -1;
	bool value = true;
	char *package_id = (char *)user_data;

	ret = app_info_get_package(app_info, &package);
	if(ret != APP_MANAGER_ERROR_NONE) {
		BT_ERR("Fail to get packagename");
	} else {
		if(!strcmp(package, package_id)) {
			//the game have been installed
			BT_DBG("Installed, return!");
			is_enabled = true;
			//fond, not need to search
			value = false;
		}
	}
	free(package);
	BT_DBG("");
	return value;
}

static bool __is_installed_gear_manager()
{
	is_enabled = false;
	app_info_filter_h handle = NULL;
	app_info_filter_create(&handle);
	app_info_filter_add_bool(handle, PACKAGE_INFO_PROP_APP_NODISPLAY, false);
	app_info_filter_foreach_appinfo(handle, __get_appinfo_cb, GEAR_MANAGER_APP_ID);
	app_info_filter_destroy(handle);

	BT_DBG("Install status : %d", is_enabled);
	return is_enabled;
}

static gboolean __bt_launch_tizen_app_store(void)
{
	BT_DBG("");

	char szExtraData[128] = {0, };
	app_control_h handle;
	int ret;

	app_control_create(&handle);
	if (handle == NULL) {
		BT_ERR("Service create failed");
		return FALSE;
	}

	/*set app id of Tizen store */
	ret = app_control_set_app_id(handle, "beu6y5fgnl.TizenStore");
	if (ret != APP_CONTROL_ERROR_NONE) {
		BT_ERR("app_control_set_app_id() failed");
		app_control_destroy(handle);
		return FALSE;
	}

	/*set operation id of Tizen store */
	ret = app_control_set_operation(handle, "http://tizen.org/appcontrol/operation/tizenstoreview");
	if (ret != APP_CONTROL_ERROR_NONE) {
		BT_ERR("app_control_set_operation() failed");
		app_control_destroy(handle);
		return FALSE;
	}

	snprintf(szExtraData, sizeof(szExtraData),
			"tizenstore://ProductDetail/%s",	GEAR_MANAGER_APP_ID);
	ret = app_control_add_extra_data(handle, "requestData", szExtraData);
	if (ret != APP_CONTROL_ERROR_NONE) {
		BT_ERR("app_control_add_extra_data() failed");
		app_control_destroy(handle);
		return FALSE;
	}

	ret = app_control_send_launch_request(handle, NULL,NULL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		BT_ERR("app_control_send_launch_request() failed");
		app_control_destroy(handle);
		return FALSE;
	}

	app_control_destroy(handle);
	BT_DBG("");
	return TRUE;
}

static gboolean __bt_launch_gear_manager(bt_dev_t *dev)
{
	BT_DBG("");
	app_control_h handle;
	int ret;

	ret = app_control_create(&handle);
	if (handle == NULL) {
		BT_ERR("Service create failed");
		return FALSE;
	}

	ret = app_control_set_appid(handle, GEAR_MANAGER_APP_ID);
	if (ret != APP_CONTROL_ERROR_NONE) {
		BT_ERR("app_control_set_appid() failed");
		app_control_destroy(handle);
		return FALSE;
	}

	ret = app_control_add_extra_data(handle, "BT_address", (const char *)dev->addr_str);
	if (ret != APP_CONTROL_ERROR_NONE) {
		BT_ERR("app_control_add_extra_data() failed");
		app_control_destroy(handle);
		return FALSE;
	}

	ret = app_control_add_extra_data(handle, "device_name", (const char *)dev->name);
	if (ret != APP_CONTROL_ERROR_NONE) {
		BT_ERR("app_control_add_extra_data() failed");
		app_control_destroy(handle);
		return FALSE;
	}

	/* 1 : Gear
	      2 : Wingtip */
	ret = app_control_add_extra_data(handle, "device_type", "1");
	if (ret != APP_CONTROL_ERROR_NONE) {
		BT_ERR("app_control_add_extra_data() failed");
		app_control_destroy(handle);
		return FALSE;
	}

//	app_control_set_window(handle, elm_win_xwindow_get(win));
	ret = app_control_send_launch_request(handle, NULL, NULL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		BT_ERR("app_control_send_launch_request() failed");
		app_control_destroy(handle);
		return FALSE;
	}

	app_control_destroy(handle);
	BT_DBG("");
	return TRUE;
}

gboolean _bt_handle_wearable_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	BT_DBG("");

	if (__is_installed_gear_manager()) {
		__bt_launch_gear_manager(dev);
	} else {
		__bt_launch_tizen_app_store();
	}

	return TRUE;
}
#endif


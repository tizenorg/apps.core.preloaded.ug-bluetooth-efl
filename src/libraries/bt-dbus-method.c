/*
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-bindings.h>

#include "bt-debug.h"
#include "bt-type-define.h"
#include "bt-util.h"
#include "bt-dbus-method.h"


/**********************************************************************
*                                                 Static Functions
***********************************************************************/

int __bt_get_adapter_path(DBusGConnection *GConn, char *path)
{
	FN_START;

	GError *error = NULL;
	DBusGProxy *manager_proxy = NULL;
	char *adapter_path = NULL;
	int ret = BT_UG_ERROR_NONE;
	gsize len = 0;

	retv_if(GConn == NULL, -1);
	retv_if(path == NULL, -1);

	manager_proxy = dbus_g_proxy_new_for_name(GConn, BLUEZ_DBUS_NAME, "/",
			"org.bluez.Manager");

	retv_if(manager_proxy == NULL, -1);

	dbus_g_proxy_call(manager_proxy, "DefaultAdapter", &error,
			G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &adapter_path,
			G_TYPE_INVALID);

	if (error != NULL) {
		BT_DBG("Getting DefaultAdapter failed: [%s]\n", error->message);
		g_error_free(error);
		g_object_unref(manager_proxy);
		return -1;
	}

	if (adapter_path == NULL) {
		g_object_unref(manager_proxy);
		return -1;
	}

	len = g_strlcpy(path, adapter_path, BT_ADAPTER_PATH_LEN);

	if (len >= BT_ADAPTER_PATH_LEN) {
		BT_DBG("The copied len is too large");
		ret = -1;
	}

	g_object_unref(manager_proxy);
	g_free(adapter_path);

	FN_END;
	return ret;
}


/**********************************************************************
*                                                Common Functions
***********************************************************************/

DBusGProxy *_bt_get_adapter_proxy(DBusGConnection *conn)
{
	FN_START;
	DBusGProxy *adapter = NULL;
	char adapter_path[BT_ADAPTER_PATH_LEN] = { 0 };

	retv_if(conn == NULL, NULL);

	if (__bt_get_adapter_path(conn, adapter_path) < 0) {
		BT_DBG("Could not get adapter path\n");
		return NULL;
	}

	adapter = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME,
								adapter_path, ADAPTER_INTERFACE);

	FN_END;
	return adapter;
}

void _bt_reset_environment(void)
{
	FN_START;
	DBusGProxy *proxy;
	DBusGConnection *conn;

	conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);;
	ret_if(conn == NULL);

	proxy = dbus_g_proxy_new_for_name(conn, BT_CORE_NAME,
			BT_CORE_PATH, BT_CORE_INTERFACE);

	if (dbus_g_proxy_call(proxy, "ResetAdapter", NULL,
					G_TYPE_INVALID, G_TYPE_INVALID) == FALSE) {
		 BT_ERR("Bt core call failed");
	}

	dbus_g_connection_unref(conn);

	FN_END;
}

gboolean _bt_is_profile_connected(int connected_type,
				DBusGConnection *conn,
				unsigned char *addr)
{
	FN_START;
	char *object_path = NULL;
	char addr_str[BT_ADDRESS_STR_LEN + 1] = { 0 };
	gboolean connected = FALSE;
	DBusGProxy *proxy = NULL;
	DBusGProxy *adapter = NULL;
	GError *error = NULL;
	GHashTable *hash = NULL;
	GValue *value = NULL;
	char *interface = NULL;

	retv_if(conn == NULL, FALSE);
	retv_if(addr == NULL, FALSE);

	adapter = _bt_get_adapter_proxy(conn);

	retv_if(adapter == NULL, FALSE);

	_bt_util_addr_type_to_addr_string(addr_str, addr);

	dbus_g_proxy_call(adapter, "FindDevice",
			  &error, G_TYPE_STRING, addr_str,
			  G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH,
			  &object_path, G_TYPE_INVALID);

	g_object_unref(adapter);

	if (error != NULL) {
		BT_DBG("Failed to Find device: %s\n", error->message);
		g_error_free(error);
		return FALSE;
	}

	retv_if(object_path == NULL, FALSE);

	switch (connected_type) {
	case BT_HEADSET_CONNECTED:
		interface = HEADSET_INTERFACE;
		break;
	case BT_STEREO_HEADSET_CONNECTED:
		interface = SYNK_INTERFACE;
		break;
	case BT_HID_CONNECTED:
		interface = HID_INTERFACE;
		break;
	default:
		BT_DBG("Unknown type!");
		return FALSE;
	}

	BT_DBG("Interface name: %s", interface);

	proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, object_path, interface);

	retv_if(proxy == NULL, FALSE);

	dbus_g_proxy_call(proxy, "GetProperties", &error,
				G_TYPE_INVALID,
				dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
				&hash, G_TYPE_INVALID);

	if (error != NULL) {
		BT_DBG("Failed to get properties: %s\n", error->message);
		g_error_free(error);
		g_object_unref(proxy);
		return FALSE;
	}

	if (hash != NULL) {
		value = g_hash_table_lookup(hash, "Connected");
		connected = value ? g_value_get_boolean(value) : FALSE;
	}

	g_object_unref(proxy);
	FN_END;
	return connected;
}

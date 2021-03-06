/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BT_DBUS_METHOD_H__
#define __BT_DBUS_METHOD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-bindings.h>

#define BT_ADAPTER_PATH_LEN 50

#define BLUEZ_DBUS_NAME "org.bluez"
#define MANAGER_INTERFACE "org.bluez.Manager"
#define ADAPTER_INTERFACE "org.bluez.Adapter"
#define HID_INTERFACE "org.bluez.Input"
#define HEADSET_INTERFACE "org.bluez.Headset"
#define SYNK_INTERFACE "org.bluez.AudioSink"

#define AGENT_NAME "org.bluez.frwk_agent"
#define AGENT_PATH "/org/bluez/agent/frwk_agent"
#define AGENT_INTERFACE "org.bluez.Agent"

DBusGProxy *_bt_get_adapter_proxy(DBusGConnection *conn);

gboolean _bt_is_profile_connected(int connected_type,
				DBusGConnection *conn,
				unsigned char *addr);

int _bt_get_remain_timeout(DBusGConnection *conn);

#ifdef __cplusplus
}
#endif
#endif /* __BT_DBUS_METHOD_H__ */

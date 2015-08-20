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

#ifndef __BT_NET_CONNECTION_H
#define __BT_NET_CONNECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <net_connection.h>
#include <notification.h>
#include "bt-type-define.h"

typedef struct {
	connection_profile_h profile_h;
	unsigned char *address;
} bt_net_profile_t;

int _bt_create_net_connection(void **net_connection);

int _bt_destroy_net_connection(void *net_connection);

void _bt_set_profile_state_changed_cb(void *profile, void *user_data);

void _bt_unset_profile_state_changed_cb(void *profile);

void *_bt_get_registered_net_profile(void *connection, unsigned char *address);

void *_bt_get_registered_net_profile_list(void *connection);

void *_bt_get_connected_net_profile(void *connection, unsigned char *address);

void _bt_free_net_profile_list(void *list);

int _bt_connect_net_profile(void *connection, void *profile, void *user_data);

int _bt_disconnect_net_profile(void *connection, void *profile, void *user_data);

#ifdef __cplusplus
}
#endif
#endif /* __BT_NET_CONNECTION_H */

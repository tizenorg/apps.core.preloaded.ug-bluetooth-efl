/*
 *  ug-setting-bluetooth-efl
 *
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Contact:  Hocheol Seo <hocheol.seo@samsung.com>
 *           GirishAshok Joshi <girish.joshi@samsung.com>
 *           DoHyun Pyun <dh79.pyun@samsung.com>
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
 *
 */

#ifndef __BT_NET_CONNECTION_H
#define __BT_NET_CONNECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bt-type-define.h"

int _bt_create_net_connection(void **net_connection);

int _bt_destroy_net_connection(void *net_connection);

void _bt_set_profile_state_changed_cb(void *profile, void *user_data);

void _bt_unset_profile_state_changed_cb(void *profile);

void *_bt_get_registered_net_profile(void *connection, unsigned char *address);

void *_bt_get_connected_net_profile(void *connection, unsigned char *address);

int _bt_connect_net_profile(void *connection, void *profile, void *user_data);

int _bt_disconnect_net_profile(void *connection, void *profile, void *user_data);

#ifdef __cplusplus
}
#endif
#endif /* __BT_NET_CONNECTION_H */

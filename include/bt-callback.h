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

#ifndef __BT_CALLBACK_H
#define __BT_CALLBACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth.h>

void _bt_cb_state_changed(int result,
			bt_adapter_state_e adapter_state,
			void *user_data);

void _bt_cb_discovery_state_changed(int result,
			bt_adapter_device_discovery_state_e discovery_state,
			bt_adapter_device_discovery_info_s *discovery_info,
			void *user_data);
void _bt_cb_visibility_mode_changed
	(int result, bt_adapter_visibility_mode_e visibility_mode, void *user_data);

void _bt_cb_bonding_created(int result, bt_device_info_s *device_info,
				void *user_data);

void _bt_cb_bonding_destroyed(int result, char *remote_address,
					void *user_data);

void _bt_cb_service_searched(int result, bt_device_sdp_info_s *sdp_info,
			void *user_data);

void _bt_cb_hid_state_changed(int result, bool connected,
			const char *remote_address,
			void *user_data);

void _bt_cb_audio_state_changed(int result, bool connected,
				const char *remote_address,
				bt_audio_profile_type_e type,
				void *user_data);
#ifdef TIZEN_BT_A2DP_SINK_ENABLE
void _bt_cb_a2dp_source_state_changed(int result, bool connected,
				const char *remote_address,
				bt_audio_profile_type_e type,
				void *user_data);
#endif
void _bt_cb_adapter_name_changed(char *device_name, void *user_data);

void _bt_cb_nap_state_changed(bool connected, const char *remote_address,
				const char *interface_name, void *user_data);

void _bt_cb_device_connection_state_changed(bool connected,
				bt_device_connection_info_s *conn_info,
				void *user_data);

void _bt_retry_connection_cb(void *data,
				Evas_Object *obj, void *event_info);

#ifdef __cplusplus
}
#endif
#endif				/* __BT_CALLBACK_H */

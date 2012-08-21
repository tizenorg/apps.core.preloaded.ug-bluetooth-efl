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

#ifdef __cplusplus
}
#endif
#endif				/* __BT_CALLBACK_H */

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

#include <bluetooth.h>
#include <vconf.h>
#include <aul.h>
#include <notification.h>

#include "bt-main-ug.h"
#include "bt-util.h"
#include "bt-debug.h"
#include "bt-string-define.h"
#include "bt-net-connection.h"
#include "bt-widget.h"

/**********************************************************************
*                                                Common Functions
***********************************************************************/

gboolean _bt_util_update_class_of_device_by_service_list(bt_service_class_t service_list,
						 bt_major_class_t *major_class,
						 bt_minor_class_t *minor_class)
{
	FN_START;

	retvm_if(service_list == BT_SC_NONE, FALSE,
		 "Invalid argument: service_list is NULL");

	/* set Major class */
	if (service_list & BT_SC_HFP_SERVICE_MASK ||
		service_list & BT_SC_HSP_SERVICE_MASK ||
#ifdef TIZEN_BT_A2DP_SINK_ENABLE
		service_list & BT_SC_A2DP_SOURCE_SERVICE_MASK ||
#endif
		service_list & BT_SC_A2DP_SERVICE_MASK)	/* Handsfree device */
		*major_class = BT_MAJOR_DEV_CLS_AUDIO;
	else if (service_list & BT_SC_NAP_SERVICE_MASK ||
			service_list & BT_SC_PANU_SERVICE_MASK )
		*major_class = BT_MAJOR_DEV_CLS_PHONE;

	/* set Minor class */
	if (service_list & BT_SC_HFP_SERVICE_MASK ||
			service_list & BT_SC_HSP_SERVICE_MASK)
		*minor_class = BTAPP_MIN_DEV_CLS_HEADSET_PROFILE;
	else if (service_list & BT_SC_A2DP_SERVICE_MASK)
		*minor_class = BTAPP_MIN_DEV_CLS_HEADPHONES;
	else if (service_list & BT_SC_NAP_SERVICE_MASK ||
			service_list & BT_SC_PANU_SERVICE_MASK)
		*minor_class = BTAPP_MIN_DEV_CLS_SMART_PHONE;

	BT_DBG("Updated major_class = %x, minor_class = %x", *major_class,
	       *minor_class);

	FN_END;
	return TRUE;
}

void _bt_util_set_value(const char *req, unsigned int *search_type,
			unsigned int *op_mode)
{
	FN_START;
	ret_if(req == NULL);
	ret_if(search_type == NULL);
	ret_if(op_mode == NULL);

	if (!strcasecmp(req, "send") || !strcasecmp(req, "browse")) {
		*search_type = BT_COD_SC_OBJECT_TRANSFER;
		*op_mode = BT_LAUNCH_SEND_FILE;
	} else if (!strcasecmp(req, "print")) {
		*search_type = BT_DEVICE_MAJOR_MASK_IMAGING;
		*op_mode = BT_LAUNCH_PRINT_IMAGE;
	} else if (!strcasecmp(req, "call") || !strcasecmp(req, "sound")) {
		*search_type = BT_DEVICE_MAJOR_MASK_AUDIO;
		*op_mode = BT_LAUNCH_CONNECT_HEADSET;
	} else if (!strcasecmp(req, "connect_source")) {
		*search_type = BT_DEVICE_MAJOR_MASK_AUDIO;
		*op_mode = BT_LAUNCH_CONNECT_AUDIO_SOURCE;
	} else if (!strcasecmp(req, "nfc")) {
		*search_type = BT_DEVICE_MAJOR_MASK_MISC;
		*op_mode = BT_LAUNCH_USE_NFC;
	} else if (!strcasecmp(req, "pick")) {
		*search_type = BT_DEVICE_MAJOR_MASK_MISC;
		*op_mode = BT_LAUNCH_PICK;
	} else if (!strcasecmp(req, "visibility")) {
		*search_type = BT_DEVICE_MAJOR_MASK_MISC;
		*op_mode = BT_LAUNCH_VISIBILITY;
	} else if (!strcasecmp(req, "onoff")) {
		*search_type = BT_DEVICE_MAJOR_MASK_MISC;
		*op_mode = BT_LAUNCH_ONOFF;
	} else if (!strcasecmp(req, "contact")) {
		*search_type = BT_COD_SC_OBJECT_TRANSFER;
		*op_mode = BT_LAUNCH_SHARE_CONTACT;
	} else if (!strcasecmp(req, "help")) {
		*search_type = BT_DEVICE_MAJOR_MASK_MISC;
		*op_mode = BT_LAUNCH_HELP;
	} else {
		*search_type = BT_DEVICE_MAJOR_MASK_MISC;
		*op_mode = BT_LAUNCH_NORMAL;
	}

	FN_END;

	return;
}

gboolean _bt_util_store_get_value(const char *key, bt_store_type_t store_type,
			      unsigned int size, void *value)
{
	FN_START;
	retv_if(value == NULL, FALSE);

	int ret = 0;
	int int_value = 0;
	int *intval = NULL;
	gboolean *boolean = FALSE;
	char *str = NULL;

	switch (store_type) {
	case BT_STORE_BOOLEAN:
		boolean = (gboolean *)value;
		ret = vconf_get_bool(key, &int_value);
		if (ret != 0) {
			BT_ERR("Get bool is failed");
			*boolean = FALSE;
			return FALSE;
		}
		*boolean = (int_value != FALSE);
		break;
	case BT_STORE_INT:
		intval = (int *)value;
		ret = vconf_get_int(key, intval);
		if (ret != 0) {
			BT_ERR("Get int is failed");
			*intval = 0;
			return FALSE;
		}
		break;
	case BT_STORE_STRING:
		str = vconf_get_str(key);
		if (str == NULL) {
			BT_ERR("Get string is failed");
			return FALSE;
		}
		if (size > 1)
			strncpy((char *)value, str, size - 1);

		free(str);
		break;
	default:
		BT_ERR("Unknown Store Type");
		return FALSE;
	}

	FN_END;
	return TRUE;
}

void _bt_util_set_phone_name(void)
{
	char *phone_name = NULL;
	char *ptr = NULL;

	phone_name = vconf_get_str(VCONFKEY_SETAPPL_DEVICE_NAME_STR);
	if (!phone_name)
		return;

	if (strlen(phone_name) != 0) {
                if (!g_utf8_validate(phone_name, -1, (const char **)&ptr))
                        *ptr = '\0';

		bt_adapter_set_name(phone_name);
	}

	free(phone_name);
}

int _bt_util_get_phone_name(char *phone_name, int size)
{
	FN_START;
	retv_if(phone_name == NULL, BT_UG_FAIL);

	if (_bt_util_store_get_value(VCONFKEY_SETAPPL_DEVICE_NAME_STR,
				 BT_STORE_STRING, size,
				 (void *)phone_name) < 0) {
		g_strlcpy(phone_name, BT_DEFAULT_PHONE_NAME, size);
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

char * _bt_util_get_timeout_string(int timeout)
{
	FN_START;
	char *string = NULL;
	switch (timeout) {
	case BT_ZERO:
		string = g_strdup(BT_STR_OFF);
		break;
	case BT_TWO_MINUTES:
		string = g_strdup(BT_STR_TWO_MINUTES);
		break;
	case BT_FIVE_MINUTES:
		string = g_strdup(BT_STR_FIVE_MINUTES);
		break;
	case BT_ONE_HOUR:
		string = g_strdup(BT_STR_ONE_HOUR);
		break;
	case BT_ALWAYS_ON:
		string = g_strdup(BT_STR_ALWAYS_ON);
		break;
	default:
		string = g_strdup(BT_STR_OFF);
		break;
	}

	FN_END;
	return string;
}

int _bt_util_get_timeout_value(int index)
{
	FN_START;

	int timeout;

	switch (index) {
	case 1:
		timeout = BT_ZERO;
		break;
	case 2:
		timeout = BT_TWO_MINUTES;
		break;
	case 3:
		timeout = BT_FIVE_MINUTES;
		break;
	case 4:
		timeout = BT_ONE_HOUR;
		break;
	case 5:
		timeout = BT_ALWAYS_ON;
		break;
	default:
		timeout = BT_ZERO;
		break;
	}

	FN_END;
	return timeout;
}

int _bt_util_get_timeout_index(int timeout)
{
	FN_START;

	int index = 0;

	switch (timeout) {
	case BT_ZERO:
		index = 1;
		break;
	case BT_TWO_MINUTES:
		index = 2;
		break;
	case BT_FIVE_MINUTES:
		index = 3;
		break;
	case BT_ONE_HOUR:
		index = 4;
		break;
	case BT_ALWAYS_ON:
		index = 5;
		break;
	default:
		index = 0;
		break;
	}

	BT_DBG("index: %d", index);

	FN_END;
	return index;
}

gboolean _bt_util_is_battery_low(void)
{
	FN_START;

#ifdef TIZEN_COMMON
	return FALSE;
#endif

	int value = 0;
	int charging = 0;

	if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, (void *)&charging))
		BT_ERR("Get the battery charging status fail");

	if (charging == 1)
		return FALSE;

	BT_DBG("charging: %d", charging);

	if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, (void *)&value)) {
		BT_ERR("Get the battery low status fail");
		return FALSE;
	}

	if (value <= VCONFKEY_SYSMAN_BAT_POWER_OFF)
		return TRUE;

	FN_END;
	return FALSE;
}

void _bt_util_addr_type_to_addr_string(char *address,
					       unsigned char *addr)
{
	FN_START;

	ret_if(address == NULL);
	ret_if(addr == NULL);

	snprintf(address, BT_ADDRESS_STR_LEN, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", addr[0],
		addr[1], addr[2], addr[3], addr[4], addr[5]);

	FN_END;
}

void _bt_util_addr_type_to_addr_result_string(char *address,
					       unsigned char *addr)
{
	FN_START;

	ret_if(address == NULL);
	ret_if(addr == NULL);

	snprintf(address, BT_ADDRESS_STR_LEN, "%2.2X-%2.2X-%2.2X-%2.2X-%2.2X-%2.2X", addr[0],
		addr[1], addr[2], addr[3], addr[4], addr[5]);

	FN_END;
}

void _bt_util_addr_type_to_addr_net_string(char *address,
					       unsigned char *addr)
{
	FN_START;

	ret_if(address == NULL);
	ret_if(addr == NULL);

	snprintf(address, BT_ADDRESS_STR_LEN, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X", addr[0],
		addr[1], addr[2], addr[3], addr[4], addr[5]);

	FN_END;
}

void _bt_util_addr_string_to_addr_type(unsigned char *addr,
						  const char *address)
{
	FN_START

        int i;
        char *ptr = NULL;

        if (!address || !addr)
                return;

        for (i = 0; i < BT_ADDRESS_LENGTH_MAX; i++) {
                addr[i] = strtol(address, &ptr, 16);
                if (ptr[0] != '\0') {
                        if (ptr[0] != ':') {
                                BT_ERR("Unexpected string");
                                return;
                        }
                        address = ptr + 1;
                }
        }

	FN_END;
}

void _bt_util_convert_time_to_string(unsigned int remain_time,
					char *text_display, char *text_read,
					int size_display, int size_read)
{
	FN_START;
	int minute;
	int second;

	ret_if(remain_time > BT_TIMEOUT_MAX);

	/* Get seconds */
	second = remain_time % 60;

	/* Get minutes */
	minute = remain_time / 60;

	if (size_display == BT_EXTRA_STR_LEN && text_display != NULL)
		snprintf(text_display, size_display, "%d:%02d", minute, second);

	if (size_read == BT_BUFFER_LEN && text_read != NULL) {
		char min_part[BT_BUFFER_LEN] = { 0, };
		char sec_part[BT_BUFFER_LEN] = { 0, };

		/*Set minute Text*/
		if (minute == 1)
			snprintf(min_part, BT_BUFFER_LEN, "%s",
					BT_STR_1_MINUTE);
		else if (minute > 1)
			snprintf(min_part, BT_BUFFER_LEN, "%d %s",
					minute, BT_STR_MINUTES);

		/*Set second Text*/
		if (second == 1)
			snprintf(sec_part, BT_BUFFER_LEN, "%s",
					BT_STR_1_SECOND);
		else if (second > 1)
			snprintf(sec_part, BT_BUFFER_LEN, "%d %s",
					second, BT_STR_SECONDS);

		snprintf(text_read, size_read, "%s %s", min_part, sec_part);
	}
	FN_END;
}

void _bt_util_launch_no_event(void *data, void *obj, void *event)
{
	FN_START;
	BT_DBG
	    ("End key is pressed. But there is no action to process in popup");
	FN_END;
}

void _bt_util_set_list_disabled(Evas_Object *genlist, Eina_Bool disable)
{
	FN_START;
	Elm_Object_Item *item = NULL;
	Elm_Object_Item *next = NULL;

	item = elm_genlist_first_item_get(genlist);

	while (item != NULL) {
		next = elm_genlist_item_next_get(item);
		if(item)
			elm_object_item_disabled_set(item, disable);

		_bt_update_genlist_item(item);
		item = next;
	}
	FN_END;
}

gboolean _bt_util_is_profile_connected(int connected_type, unsigned char *addr)
{
	FN_START;
	char addr_str[BT_ADDRESS_STR_LEN + 1] = { 0 };
	gboolean connected = FALSE;
	int ret = 0;
	int connected_profiles = 0x00;
	bt_profile_e profile;

	retv_if(addr == NULL, FALSE);

	_bt_util_addr_type_to_addr_string(addr_str, addr);

	BT_DBG("connected profiles: %d connected type : %d", connected_profiles,
			connected_type);

	switch (connected_type) {
	case BT_HEADSET_CONNECTED:
		profile = BT_PROFILE_HSP;
		break;
	case BT_STEREO_HEADSET_CONNECTED:
		profile = BT_PROFILE_A2DP;
		break;
	case BT_MUSIC_PLAYER_CONNECTED:
		profile = BT_PROFILE_A2DP_SINK;
		break;
	case BT_HID_CONNECTED:
		profile = BT_PROFILE_HID;
		break;
	case BT_NETWORK_CONNECTED:
		profile = BT_PROFILE_NAP;
		break;
	case BT_NETWORK_SERVER_CONNECTED:
		profile = BT_PROFILE_NAP_SERVER;
		break;
	default:
		BT_ERR("Unknown type!");
		return FALSE;
	}

	ret = bt_device_is_profile_connected(addr_str, profile,
					(bool *)&connected);

	if (ret < BT_ERROR_NONE) {
		BT_ERR("failed with [0x%04x]", ret);
		return FALSE;
	}

	FN_END;
	return connected;
}

void _bt_util_free_device_uuids(bt_dev_t *item)
{
	int i;

	ret_if(item == NULL);

	if(item->uuids) {
		for (i = 0; item->uuids[i] != NULL; i++)
			g_free(item->uuids[i]);

		g_free(item->uuids);
		item->uuids = NULL;
	}
}

void _bt_util_free_device_item(bt_dev_t *item)
{
	ret_if(item == NULL);

	_bt_util_free_device_uuids(item);

	if (item->net_profile) {
		_bt_unset_profile_state_changed_cb(item->net_profile);
		item->net_profile = NULL;
	}

	item->ugd = NULL;
	free(item);
}

gboolean _bt_util_is_space_str(const char *name_str)
{
	retv_if(name_str == NULL, FALSE);
	retv_if(*name_str == '\0', FALSE);

	while (*name_str)
	{
		if (*name_str != '\0' && *name_str != ' ')
			return FALSE;

		name_str++;
	}

	return TRUE;
}

void _bt_util_max_len_reached_cb(void *data, Evas_Object *obj,
				void *event_info)
{
	FN_START;
	int ret;
	char str[BT_STR_ACCES_INFO_MAX_LEN] = {0, };
	char *stms_str = NULL;

	stms_str = BT_STR_MAX_CHARACTER_REACHED;

	snprintf(str, sizeof(str), stms_str, DEVICE_NAME_MAX_CHARACTER);

	ret = notification_status_message_post(str);
	if (ret != NOTIFICATION_ERROR_NONE)
		BT_ERR("notification_status_message_post() ERROR [%d]", ret);

	FN_END;
}

int _bt_util_check_any_profile_connected(bt_dev_t *dev)
{
	FN_START;
	int connected=0;

	if (dev->service_list & BT_SC_HFP_SERVICE_MASK ||
		    dev->service_list & BT_SC_HSP_SERVICE_MASK ||
		    dev->service_list & BT_SC_A2DP_SERVICE_MASK) {
		connected = _bt_util_is_profile_connected(BT_HEADSET_CONNECTED,
						dev->bd_addr);
		if (!connected) {
			connected = _bt_util_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
					    	 dev->bd_addr);
		}
		if (connected)
			goto done;
	}

	if (dev->service_list & BT_SC_A2DP_SOURCE_SERVICE_MASK) {
		connected = _bt_util_is_profile_connected(BT_MUSIC_PLAYER_CONNECTED,
						dev->bd_addr);
		if (connected)
			goto done;
	}

	if (dev->service_list & BT_SC_PANU_SERVICE_MASK ||
		dev->service_list & BT_SC_NAP_SERVICE_MASK ||
		dev->service_list & BT_SC_GN_SERVICE_MASK) {
		connected = _bt_util_is_profile_connected(BT_NETWORK_CONNECTED,
						dev->bd_addr);
		if (!connected) {
			connected = _bt_util_is_profile_connected(BT_NETWORK_SERVER_CONNECTED,
				dev->bd_addr);
			}
		if (connected)
			goto done;
	}

	if (dev->service_list & BT_SC_HID_SERVICE_MASK ) {
		connected = _bt_util_is_profile_connected(BT_HID_CONNECTED,
						dev->bd_addr);

		if (connected)
			goto done;
	}
	FN_END;
done:
	return connected;
}

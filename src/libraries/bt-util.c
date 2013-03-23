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

#include <bluetooth.h>
#include <vconf.h>
#include <aul.h>

#include "bt-main-ug.h"
#include "bt-util.h"
#include "bt-debug.h"
#include "bt-string-define.h"
#include "bt-net-connection.h"

/**********************************************************************
*                                                Common Functions
***********************************************************************/

gboolean _bt_util_get_service_mask_from_uuid_list(char **uuids,
				      int no_of_service,
				      bt_service_class_t *service_mask_list)
{
	FN_START;

	int i = 0;
	unsigned int service = 0;
	char **parts = NULL;
	bt_service_class_t service_mask = 0;

	retvm_if(uuids == NULL, FALSE,
		 "Invalid argument: service_list_array is NULL\n");

	BT_DBG("no_of_service = %d\n", no_of_service);

	for (i = 0; i < no_of_service; i++) {
		parts = g_strsplit(uuids[i], "-", -1);

		if (parts == NULL || parts[0] == NULL) {
			g_strfreev(parts);
			continue;
		}

		service = g_ascii_strtoull(parts[0], NULL, 16);
		g_strfreev(parts);

		switch (service) {
		case BT_SPP_PROFILE_UUID:
			service_mask |= BT_SC_SPP_SERVICE_MASK;
			break;

		case BT_LAP_PROFILE_UUID:
			service_mask |= BT_SC_LAP_SERVICE_MASK;
			break;

		case BT_DUN_PROFILE_UUID:
			service_mask |= BT_SC_DUN_SERVICE_MASK;
			break;

		case BT_OBEX_IR_MC_SYNC_SERVICE_UUID:
			service_mask |= BT_SC_SYNC_SERVICE_MASK;
			break;

		case BT_OBEX_OBJECT_PUSH_SERVICE_UUID:
			service_mask |= BT_SC_OPP_SERVICE_MASK;
			break;

		case BT_OBEX_FILE_TRANSFER_UUID:
			service_mask |= BT_SC_FTP_SERVICE_MASK;
			break;

		case BT_HS_PROFILE_UUID:
			service_mask |= BT_SC_HSP_SERVICE_MASK;
			break;

		case BT_CTP_PROFILE_UUID:
			service_mask |= BT_SC_CTP_SERVICE_MASK;
			break;

		case BT_AUDIO_SOURCE_UUID:
			service_mask |= BT_SC_NONE;
			break;

		case BT_AUDIO_SINK_UUID:
			service_mask |= BT_SC_A2DP_SERVICE_MASK;
			break;

		case BT_VIDEO_SOURCE_UUID:
			service_mask |= BT_SC_NONE;
			break;

		case BT_VIDEO_SINK_UUID:
			service_mask |= BT_SC_NONE;
			break;

		case BT_AV_REMOTE_CONTROL_TARGET_UUID:
			service_mask |= BT_SC_NONE;
			break;

		case BT_ADVANCED_AUDIO_PROFILE_UUID:
			service_mask |= BT_SC_A2DP_SERVICE_MASK;
			break;

		case BT_AV_REMOTE_CONTROL_UUID:
			service_mask |= BT_SC_AVRCP_SERVICE_MASK;
			break;

		case BT_ICP_PROFILE_UUID:
			service_mask |= BT_SC_ICP_SERVICE_MASK;
			break;

		case BT_FAX_PROFILE_UUID:
			service_mask |= BT_SC_FAX_SERVICE_MASK;
			break;

		case BT_HEADSET_AG_SERVICE_UUID:
			service_mask |= BT_SC_NONE; /* BT_SC_HSP_SERVICE_MASK */
			break;

		case BT_PAN_PANU_PROFILE_UUID:
			service_mask |= BT_SC_PANU_SERVICE_MASK;
			break;

		case BT_PAN_NAP_PROFILE_UUID:
			service_mask |= BT_SC_NAP_SERVICE_MASK;
			break;

		case BT_PAN_GN_PROFILE_UUID:
			service_mask |= BT_SC_GN_SERVICE_MASK;
			break;

		case BT_REFERENCE_PRINTING:
			service_mask |= BT_SC_NONE;
			break;

		case BT_OBEX_IMAGING_UUID:
			service_mask |= BT_SC_NONE;
			break;

		case BT_OBEX_IMAGING_RESPONDER_UUID:
			service_mask |= BT_SC_BIP_SERVICE_MASK;
			break;

		case BT_HF_PROFILE_UUID:
			service_mask |= BT_SC_HFP_SERVICE_MASK;
			break;

		case BT_HFG_PROFILE_UUID:
			service_mask |= BT_SC_NONE;
			break;

		case BT_DIRECT_PRINTING_REFERENCE_OBJ_UUID:
			service_mask |= BT_SC_NONE;
			break;

		case BT_BASIC_PRINTING:
			service_mask |= BT_SC_NONE;
			break;

		case BT_HID_PROFILE_UUID:
			service_mask |= BT_SC_HID_SERVICE_MASK;
			break;

		case BT_SIM_ACCESS_PROFILE_UUID:
			service_mask |= BT_SC_SAP_SERVICE_MASK;
			break;

		case BT_OBEX_PBA_PROFILE_UUID:
			service_mask |= BT_SC_PBAP_SERVICE_MASK;
			break;

		case BT_OBEX_BPPS_PROFILE_UUID:
			service_mask |= BT_SC_BPP_SERVICE_MASK;
			break;

		case BT_PNP_INFORMATION_UUID:
			service_mask |= BT_SC_NONE;
			break;

		case BT_OBEX_PRINTING_STATUS_UUID:
			service_mask |= BT_SC_BPP_SERVICE_MASK;
			break;

		case BT_HCR_PROFILE_UUID:
			service_mask |= BT_SC_NONE;
			break;

		case BT_OBEX_SYNCML_TRANSFER_UUID:
			service_mask |= BT_SC_NONE;
			break;

		default:
			break;
		}

	}

	*service_mask_list = service_mask;
	BT_DBG("service_mask = %x, service_mask_lis = %x\n", service_mask,
	       service_mask_list);

	FN_END;
	return TRUE;
}

int _bt_util_parse_file_list(const char *bundle_txt, char ***filelist,
			     int number_of_files)
{
	FN_START;
	retv_if(bundle_txt == NULL || filelist == NULL, BT_UG_FAIL);

	char *token = NULL;
	char *param = NULL;
	int i = 0;

	param = (char *)bundle_txt;
	while (((token = strstr(param, "?")) != NULL) && i < number_of_files) {
		*token = '\0';
		*filelist[i] = param;
		BT_DBG("File [%d] [%s]\n", i, *filelist[i]);
		param = token + 1;
		i++;
	}
	if (i == (number_of_files - 1)) {
		*filelist[i] = param;
		BT_DBG("File [%d] [%s]\n", i, *filelist[i]);
	} else {
		BT_DBG("Not match : [%d] / [%d]\n", number_of_files, i);
		return BT_UG_FAIL;
	}

	FN_END;
	return BT_UG_ERROR_NONE;
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
	} else if (!strcasecmp(req, "nfc")) {
		*search_type = BT_DEVICE_MAJOR_MASK_MISC;
		*op_mode = BT_LAUNCH_USE_NFC;
	} else if (!strcasecmp(req, "pick")) {
		*search_type = BT_DEVICE_MAJOR_MASK_MISC;
		*op_mode = BT_LAUNCH_PICK;
	} else if (!strcasecmp(req, "visibility")) {
		*search_type = BT_DEVICE_MAJOR_MASK_MISC;
		*op_mode = BT_LAUNCH_VISIBILITY;
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
			BT_DBG("Get bool is failed\n");
			*boolean = FALSE;
			return FALSE;
		}
		*boolean = (int_value != FALSE);
		break;
	case BT_STORE_INT:
		intval = (int *)value;
		ret = vconf_get_int(key, intval);
		if (ret != 0) {
			BT_DBG("Get int is failed\n");
			*intval = 0;
			return FALSE;
		}
		break;
	case BT_STORE_STRING:
		str = vconf_get_str(key);
		if (str == NULL) {
			BT_DBG("Get string is failed\n");
			return FALSE;
		}
		if (size > 1)
			strncpy((char *)value, str, size - 1);

		free(str);
		break;
	default:
		BT_DBG("Unknown Store Type\n");
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

int _bt_util_get_timeout_string(int timeout, char *buf, int size)
{
	FN_START;
	retv_if(buf == NULL, BT_UG_FAIL);

	switch (timeout) {
	case BT_ZERO:
		g_strlcpy(buf, BT_STR_OFF, size);
		break;
	case BT_TWO_MINUTES:
		g_strlcpy(buf, BT_STR_TWO_MINUTES, size);
		break;
	case BT_FIVE_MINUTES:
		g_strlcpy(buf, BT_STR_FIVE_MINUTES, size);
		break;
	case BT_ONE_HOUR:
		g_strlcpy(buf, BT_STR_ONE_HOUR, size);
		break;
	case BT_ALWAYS_ON:
		g_strlcpy(buf, BT_STR_ALWAYS_ON, size);
		break;
	default:
		g_strlcpy(buf, BT_STR_OFF, size);
		break;
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

int _bt_util_get_timeout_value(int index)
{
	FN_START;

	int timeout;

	switch (index) {
	case 0:
		timeout = BT_ZERO;
		break;
	case 1:
		timeout = BT_TWO_MINUTES;
		break;
	case 2:
		timeout = BT_FIVE_MINUTES;
		break;
	case 3:
		timeout = BT_ONE_HOUR;
		break;
	case 4:
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
		index = 0;
		break;
	case BT_TWO_MINUTES:
		index = 1;
		break;
	case BT_FIVE_MINUTES:
		index = 2;
		break;
	case BT_ONE_HOUR:
		index = 3;
		break;
	case BT_ALWAYS_ON:
		index = 4;
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

	int value = 0;
	int charging = 0;

	if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, (void *)&charging))
		BT_DBG("Get the battery charging status fail");

	if (charging == 1)
		return FALSE;

	BT_DBG("charging: %d", charging);

	if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, (void *)&value)) {
		BT_DBG("Get the battery low status fail");
		return FALSE;
	}

	if (value <= VCONFKEY_SYSMAN_BAT_POWER_OFF)
		return TRUE;

	FN_END;
	return FALSE;
}

gboolean _bt_util_is_flight_mode(void)
{
	FN_START;

	bool mode = FALSE;

	if (vconf_get_bool(VCONFKEY_SETAPPL_FLIGHT_MODE_BOOL, (void *)&mode)) {
		BT_DBG("Get the flight mode fail");
		return FALSE;
	}

	BT_DBG("flight mode: %d", mode);

	FN_END;
	return mode;
}

void _bt_util_show_device_list(Eina_List *list)
{
	FN_START;

	int index = 0;
	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	retm_if(list == NULL, "Invalid argument: list is NULL\n");

	EINA_LIST_FOREACH(list, l, item) {
		if (item) {
			BT_DBG("count: %d", index++);
			BT_DBG("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
			       item->bd_addr[0], item->bd_addr[1],
			       item->bd_addr[2], item->bd_addr[3],
			       item->bd_addr[4], item->bd_addr[5]);
			BT_DBG("name: %s", item->name);
		}
	}

	FN_END;
}


/* status - 0 : No operation, 1 : Activate , 2 : Deactivate, 3 : Search Test*/
/* run_type - No window change, 1 : Top window, 2 : Background*/
void _bt_util_launch_bt_service(int status, int run_type)
{
	FN_START;

	bundle *kb;
	char status_val[5] = { 0, };
	char run_type_val[5] = { 0, };

	snprintf(status_val, sizeof(status_val), "%d", status);
	snprintf(run_type_val, sizeof(run_type_val),  "%d", run_type);

	BT_DBG("status: %s, run_type: %s", status_val, run_type_val);

	kb = bundle_create();
	ret_if(kb == NULL);

	bundle_add(kb, "launch-type", "setstate");
	bundle_add(kb, "status", status_val);
	bundle_add(kb, "run-type", run_type_val);

	aul_launch_app("com.samsung.bluetooth", kb);

	bundle_free(kb);

	FN_END;
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
                if (ptr != NULL) {
                        if (ptr[0] != ':') {
                                BT_DBG("Unexpected string\n");
                                return;
                        }

                        address = ptr + 1;
                }
        }

	FN_END;
}

void _bt_util_convert_time_to_string(unsigned int remain_time,
					char *buf, int size)
{
	FN_START;
	int minute;
	int second;

	ret_if(remain_time > BT_TIMEOUT_MAX);
	ret_if(size < BT_EXTRA_STR_LEN);
	ret_if(buf == NULL);

	/* Get seconds */
	second = remain_time % 60;

	/* Get minutes */
	minute = remain_time / 60;

	snprintf(buf, size, "%d:%02d", minute, second);

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
	Elm_Object_Item *item = NULL;
	Elm_Object_Item *next = NULL;

	item = elm_genlist_first_item_get(genlist);

	while (item != NULL) {
		next = elm_genlist_item_next_get(item);
		elm_object_item_disabled_set(item, disable);
		elm_genlist_item_update(item);
		item = next;
	}
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
	_bt_unset_profile_state_changed_cb(item->net_profile);

	free(item);
}


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
#include "bt-net-connection.h"
#include "bt-string-define.h"
#include "bt-callback.h"

/**********************************************************************
*                                                 Static Functions
***********************************************************************/

void __bt_cb_profile_state_changed(connection_profile_state_e state, void *user_data)
{
	FN_START;

	bt_address_t address = { { 0 } };
	bt_dev_t *dev;

	ret_if(!user_data);

	dev = (bt_dev_t *)user_data;
	bt_ug_data *ugd = dev->ugd;
	ret_if(!ugd);
	ret_if(ugd->op_status == BT_DEACTIVATING || ugd->op_status == BT_DEACTIVATED);

	BT_DBG("profile state: %d", state);

	memcpy(address.bd_addr, dev->bd_addr, BT_ADDRESS_LENGTH_MAX);

	if (state == CONNECTION_PROFILE_STATE_CONNECTED) {
		_bt_ipc_update_connected_status(dev->ugd, BT_NETWORK_CONNECTED,
					TRUE, dev->pan_connection_result, &address);
	} else if (state == CONNECTION_PROFILE_STATE_DISCONNECTED) {
		_bt_ipc_update_connected_status(dev->ugd, BT_NETWORK_CONNECTED,
				FALSE, dev->pan_connection_result, &address);
	}
	FN_END;
}

static void __bt_cb_net_opened(connection_error_e result, void* user_data)
{
	FN_START;
	bt_address_t address = { { 0 } };
	bt_dev_t *dev;

	ret_if(!user_data);

	dev = (bt_dev_t *)user_data;
	bt_ug_data *ugd = dev->ugd;
	ret_if(!ugd);

	BT_DBG("result: %d", result);

	if (result != 0) {
		dev->pan_connection_result = BT_UG_FAIL;
		memcpy(address.bd_addr, dev->bd_addr, BT_ADDRESS_LENGTH_MAX);
		_bt_ipc_update_connected_status(dev->ugd, BT_NETWORK_CONNECTED,
				FALSE, dev->pan_connection_result, &address);
	} else {
		dev->pan_connection_result = BT_UG_ERROR_NONE;
	}

	FN_END;
}

static void __bt_cb_net_closed(connection_error_e result, void* user_data)
{
	FN_START;
	bt_dev_t *dev;

	ret_if(!user_data);

	dev = (bt_dev_t *)user_data;
	bt_ug_data *ugd = dev->ugd;
	ret_if(!ugd);

	BT_DBG("result: %d", result);
	if (result != 0) {
		dev->pan_connection_result = BT_UG_FAIL;
	} else {
		dev->pan_connection_result = BT_UG_ERROR_NONE;
	}

	FN_END;
}

static void *__bt_get_net_profile_list(void *connection,
			connection_iterator_type_e type)
{
	int result;
	Eina_List *ret_list = NULL;
	gchar **split_string;
	char *profile_name = NULL;
	connection_profile_iterator_h profile_iter;
	connection_profile_h profile_h;
	connection_profile_type_e profile_type;

	retv_if(connection == NULL, NULL);

	result = connection_get_profile_iterator(connection,
				type,
				&profile_iter);
	if (result != CONNECTION_ERROR_NONE) {
		BT_ERR("Fail to get profile iterator [%d]", result);
		return NULL;
	}

	while (connection_profile_iterator_has_next(profile_iter)) {
		profile_name = NULL;
		profile_h = NULL;
		split_string = NULL;

		if (connection_profile_iterator_next(profile_iter,
					&profile_h) != CONNECTION_ERROR_NONE) {
			BT_ERR("Fail to get profile handle");
			continue;
		}

		if (connection_profile_get_type(profile_h,
					&profile_type) != CONNECTION_ERROR_NONE) {
			BT_ERR("Fail to get profile type");
			continue;
		}

		if (profile_type != CONNECTION_PROFILE_TYPE_BT)
			continue;

		if (connection_profile_get_name(profile_h,
					&profile_name) != CONNECTION_ERROR_NONE) {
			BT_ERR("Fail to get profile name");
			continue;
		}

		split_string = g_strsplit(profile_name, "_", 3);

		g_free(profile_name);

		if (g_strv_length(split_string) < 3)
			continue;

		bt_net_profile_t *profile_data =
			(bt_net_profile_t *)g_malloc0(sizeof(bt_net_profile_t));

		if (profile_data == NULL)
			continue;

		profile_data->profile_h = profile_h;
		profile_data->address = g_strdup(split_string[2]);
		ret_list = eina_list_append(ret_list, profile_data);
		g_strfreev(split_string);
	}

	FN_END;

	return ret_list;
}

static connection_profile_h __bt_get_net_profile(void *connection,
			connection_iterator_type_e type,
			unsigned char *address)
{
	int result;
	gchar **split_string;
	char net_address[BT_ADDRESS_STR_LEN + 1] = { 0 };
	char *profile_name = NULL;
	connection_profile_iterator_h profile_iter;
	connection_profile_h profile_h;
	connection_profile_type_e profile_type;

	retv_if(connection == NULL, NULL);
	retv_if(address == NULL, NULL);

	_bt_util_addr_type_to_addr_net_string(net_address, address);

	result = connection_get_profile_iterator(connection,
				type,
				&profile_iter);
	if (result != CONNECTION_ERROR_NONE) {
		BT_ERR("Fail to get profile iterator [%d]", result);
		return NULL;
	}

	while (connection_profile_iterator_has_next(profile_iter)) {
		profile_name = NULL;
		profile_h = NULL;
		split_string = NULL;

		if (connection_profile_iterator_next(profile_iter,
					&profile_h) != CONNECTION_ERROR_NONE) {
			BT_ERR("Fail to get profile handle");
			return NULL;
		}

		if (connection_profile_get_type(profile_h,
					&profile_type) != CONNECTION_ERROR_NONE) {
			BT_ERR("Fail to get profile type");
			continue;
		}

		if (profile_type != CONNECTION_PROFILE_TYPE_BT)
			continue;

		if (connection_profile_get_name(profile_h,
					&profile_name) != CONNECTION_ERROR_NONE) {
			BT_ERR("Fail to get profile name");
			return NULL;
		}

		split_string = g_strsplit(profile_name, "_", 3);

		g_free(profile_name);

		if (g_strv_length(split_string) < 3)
			continue;

		if (g_ascii_strcasecmp(split_string[2], net_address) == 0) {
			BT_DBG("matched profile");
			g_strfreev(split_string);
			return profile_h;
		}

		g_strfreev(split_string);
	}

	FN_END;

	return NULL;
}

/**********************************************************************
*                                                Common Functions
***********************************************************************/

int _bt_create_net_connection(void **net_connection)
{
	FN_START;

	int result;
	connection_h connection = NULL;

	result = connection_create(&connection);

	if (result != CONNECTION_ERROR_NONE ||
	     connection == NULL) {
		BT_ERR("connection_create() failed: %d", result);
		*net_connection = NULL;
		return BT_UG_FAIL;
	}

	*net_connection = connection;

	FN_END;
	return BT_UG_ERROR_NONE;
}

int _bt_destroy_net_connection(void *net_connection)
{
	FN_START;

	int result;

	retv_if(net_connection == NULL, BT_UG_FAIL);

	connection_unset_type_changed_cb(net_connection);
	result = connection_destroy(net_connection);

	FN_END;

	return (result == CONNECTION_ERROR_NONE) ?
		BT_UG_ERROR_NONE : BT_UG_FAIL;
}

void _bt_set_profile_state_changed_cb(void *profile, void *user_data)
{
	FN_START;

	connection_profile_h profile_clone = NULL;
	bt_dev_t *dev;

	ret_if(profile == NULL);
	ret_if(user_data == NULL);

	dev = (bt_dev_t *)user_data;

	if (connection_profile_clone(&profile_clone,
				profile) != CONNECTION_ERROR_NONE) {
		BT_ERR("Fail to clone the profile");
		return;
	}

	if (connection_profile_set_state_changed_cb(profile,
			__bt_cb_profile_state_changed,
			dev) != CONNECTION_ERROR_NONE) {
		connection_profile_destroy(profile_clone);
		return;
	}

	dev->net_profile = profile_clone;

	FN_END;
}

void _bt_unset_profile_state_changed_cb(void *profile)
{
	FN_START;

	ret_if(profile == NULL);
	connection_profile_unset_state_changed_cb(profile);
	connection_profile_destroy(profile);

	FN_END;
}

void *_bt_get_registered_net_profile(void *connection, unsigned char *address)
{
	FN_START;

	return __bt_get_net_profile(connection,
			CONNECTION_ITERATOR_TYPE_REGISTERED,
			address);
}

void *_bt_get_registered_net_profile_list(void *connection)
{
	FN_START;

	return __bt_get_net_profile_list(connection,
			CONNECTION_ITERATOR_TYPE_REGISTERED);
}

void _bt_free_net_profile_list(void *list)
{
	FN_START;
	ret_if(!list);
	Eina_List *l = NULL;
	bt_net_profile_t *profile_data = NULL;

	EINA_LIST_FOREACH(list, l, profile_data) {
		if (profile_data && profile_data->address)
			g_free(profile_data->address);
	}
	eina_list_free(list);
}

int _bt_connect_net_profile(void *connection, void *profile, void *user_data)
{
	FN_START;

	int result;
	bt_dev_t *dev;
	connection_wifi_state_e wifi_state;

	retv_if(connection == NULL, BT_UG_FAIL);
	retv_if(profile == NULL, BT_UG_FAIL);
	retv_if(user_data == NULL, BT_UG_FAIL);

	dev = (bt_dev_t *)user_data;
	result = connection_get_wifi_state(connection, &wifi_state);
	if (result != CONNECTION_ERROR_NONE) {
		BT_ERR("connection_get_wifi_state  Failed: %d", result);
		return BT_UG_FAIL;
	}

	if (wifi_state == CONNECTION_WIFI_STATE_CONNECTED) {
		bt_address_t address = { { 0 } };

		/*dislpay pop up */
		BT_INFO("wifi_state: %d", wifi_state);
		result = notification_status_message_post(BT_STR_BLUETOOTH_TETHERING_CONNECTION_ERROR);
		if (result != NOTIFICATION_ERROR_NONE)
			BT_ERR("notification_status_message_post() ERROR [%d]", result);

		/* Set PAN Connection as Disconnected to Update the Checkbox*/
		_bt_util_addr_string_to_addr_type(address.bd_addr, &dev->addr_str);
		BT_DBG("Updating Checkbox as not connected");
		dev->network_checked = dev->connected_mask & BT_NETWORK_CONNECTED;
		bt_ug_data * ugd = dev->ugd;
		if (ugd->profile_vd == NULL) {
			BT_ERR("Profile view NULL");
			return BT_UG_FAIL;
		}
		/* No need to change connect_req and ugd status as they are set only when
		  * this api is sussessfull */
		BT_DBG("As we didn't Initiated Connection only update the list");
		_bt_util_set_list_disabled(ugd->profile_vd->genlist,
				EINA_FALSE);
		return BT_UG_FAIL;
	}



	/* Fix P121126-0868 */
	/* 'dev' can be freed, if use try to unbond during connecting NAP */
	/*clone_dev = g_malloc0(sizeof(bt_dev_t));
	g_strlcpy(clone_dev->addr_str, dev->addr_str,
			BT_ADDRESS_STR_LEN + 1);
	clone_dev->ugd = dev->ugd;
	clone_dev->genlist_item = dev->genlist_item;*/

	result = connection_open_profile(connection,
				profile,
				__bt_cb_net_opened,
				dev);

	if (result != CONNECTION_ERROR_NONE) {
		BT_ERR("Connection open Failed: %d", result);
		return BT_UG_FAIL;
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

int _bt_disconnect_net_profile(void *connection, void *profile, void *user_data)
{
	FN_START;

	int result;

	retv_if(connection == NULL, BT_UG_FAIL);
	retv_if(profile == NULL, BT_UG_FAIL);

	result = connection_close_profile(connection,
				profile,
				__bt_cb_net_closed,
				user_data);

	if (result != CONNECTION_ERROR_NONE) {
		BT_ERR("Connection close Failed: %d", result);
		return BT_UG_FAIL;
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

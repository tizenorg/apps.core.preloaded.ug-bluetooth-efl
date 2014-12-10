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

#ifndef __BT_TYPE_DEFINE_H__
#define __BT_TYPE_DEFINE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <tizen_error.h>
#include <glib.h>
#include <bluetooth_type.h>

/**************************************************
*                             Constant Value
***************************************************/

#define BT_GLOBALIZATION_STR_LENGTH 256
#define BT_MAX_CHARS_IN_FTP_TITLE 12
#define BT_MAX_MENU_NAME_LEN 64
#define BT_MAX_SERVICE_LIST 9
#define BT_DEVICE_NAME_LENGTH_MAX 100 /* UX guideline */
#define BT_ADDRESS_LENGTH_MAX 6
#define BT_ADDRESS_STR_LEN 18
#define BT_FILE_NAME_LEN_MAX 255
#define BT_IMG_PATH_MAX 256
#define BT_HTML_EXTRA_TAG_LENGTH 20
#define BT_SERVICE_NAME_LENGTH 30
#define BT_SERVICE_CONTENT_LENGTH 256
#define BT_PHONE_NUM_LEN 50
#define BT_EXTRA_STR_LEN 10
#define BT_BUFFER_LEN 256
#define BT_TIMEOUT_MAX 3600
#define BT_MAX_TIMEOUT_ITEMS 5
#define BT_GLOBALIZATION_TEXT_LENGTH \
	(BT_GLOBALIZATION_STR_LENGTH+BT_EXTRA_STR_LEN)
#define BT_DISCONNECT_TEXT_LENGTH \
		((2*BT_GLOBALIZATION_STR_LENGTH)+BT_DEVICE_NAME_LENGTH_MAX)
#define BT_SERVICE_TEXT_LENGTH \
	(BT_SERVICE_CONTENT_LENGTH+BT_HTML_EXTRA_TAG_LENGTH)* \
	BT_MAX_SERVICE_LIST
#define BT_UG_SYSPOPUP_TIMEOUT_FOR_MULTIPLE_POPUPS 200

/* Timeout Value */
#define BT_SEARCH_SERVICE_TIMEOUT 5
#define BT_SELECTED_TIMEOUT 5
#define BT_DELETED_TIMEOUT 2
#define BT_VISIBILITY_TIMEOUT	1000

/* Define Error type */
#define BT_UG_FAIL -1
#define BT_UG_ERROR_NONE 0

#define BT_TWO_MINUTES 120
#define BT_FIVE_MINUTES 300
#define BT_ONE_HOUR 3600
#define BT_ZERO 0
#define BT_ALWAYS_ON -1

#define BT_RESULT_STR_MAX 256

#define BT_GENLIST_FONT_32_INC 32

/**************************************************
*                              String define
***************************************************/

#define BT_SET_FONT_SIZE \
"<font=SLP:style=Medium><font_size=%d>%s</font_size></font>"

#define BT_SET_FONT_SIZE_COLOR \
"<font=SLP:style=Medium><font_size=%d><color=%s>%s</color></font_size></font>"

/* GENLIST_TEXT_COLOR_LIST_SUB_TEXT_SETTINGS 42 137 194 255 */
#define BT_GENLIST_SUBTEXT_COLOR "#2A89C2FF"
#define BT_GENLIST_WHITE_SUBTEXT_COLOR "#FFFFFFFF"

#define BT_RESULT_SUCCESS "success"
#define BT_RESULT_FAIL "fail"

#define BT_DEFAULT_PHONE_NAME "Fraser"

#define BT_SYSPOPUP_REQUEST_NAME "app-confirm-request"
#define BT_SYSPOPUP_TWO_BUTTON_TYPE "twobtn"
#define BT_SYSPOPUP_ONE_BUTTON_TYPE "onebtn"

#define BT_VCONF_VISIBLE_TIME "file/private/libug-setting-bluetooth-efl/visibility_time"

/* AppControl Operation */
#define BT_APPCONTROL_PICK_OPERATION "http://tizen.org/appcontrol/operation/bluetooth/pick"
#define BT_APPCONTROL_VISIBILITY_OPERATION "http://tizen.org/appcontrol/operation/configure/bluetooth/visibility"

/* AppControl Output */
#define BT_APPCONTROL_ADDRESS "http://tizen.org/appcontrol/data/bluetooth/address"
#define BT_APPCONTROL_NAME "http://tizen.org/appcontrol/data/bluetooth/name"
#define BT_APPCONTROL_RSSI "http://tizen.org/appcontrol/data/bluetooth/rssi"
#define BT_APPCONTROL_IS_PAIRED "http://tizen.org/appcontrol/data/bluetooth/is_paired"
#define BT_APPCONTROL_MAJOR_CLASS "http://tizen.org/appcontrol/data/bluetooth/major_class"
#define BT_APPCONTROL_MINOR_CLASS "http://tizen.org/appcontrol/data/bluetooth/minor_class"
#define BT_APPCONTROL_SERVICE_CLASS "http://tizen.org/appcontrol/data/bluetooth/service_class"
#define BT_APPCONTROL_SERVICE_TYPE "http://tizen.org/appcontrol/data/bluetooth/service_type"
#define BT_APPCONTROL_UUID_LIST "http://tizen.org/appcontrol/data/bluetooth/uuid_list"
#define BT_APPCONTROL_VISIBILITY "http://tizen.org/appcontrol/data/bluetooth/visibility"

/* Access information */
#define BT_STR_ACCES_INFO_MAX_LEN 512
#define BT_STR_ACC_ICON "Icon"

/**************************************************
*                                  Enum type
***************************************************/

typedef enum {
	BT_CONNECTION_REQ,
	BT_PAIRING_REQ,
	BT_NONE_REQ,
} bt_confirm_req_t;

/* Visible timout value (sec)*/
typedef enum {
	BT_2MIN = 120,
	BT_5MIN = 300,
	BT_1HOUR = 3600,
	BT_NO_TIMEOUT = 0,
} bt_visible_time_t;

typedef enum {
	BT_VISIBLE_OFF = 0,
	BT_VISIBLE_ALWAYS = 1,
	BT_VISIBLE_TIME_LIMITED = 2,
} bt_visible_result_t;

typedef enum {
	BT_SEARCH_ALL_DEVICE = 0,
	BT_SEARCH_PHONE,
	BT_SEARCH_HEADSET,
	BT_SEARCH_PC,
	BT_SEARCH_HID,
} bt_search_option_t;

typedef enum {
	BT_CONNECT_MENU = 1,
	BT_SEND_MENU,
	BT_RENAME_MENU,
	BT_DELETE_MENU,
	BT_SERVICE_LIST_MENU,
} bt_menu_index_t;

typedef enum {
	BT_PAIRED_DEVICE_DISABLE,
	BT_PAIRED_DEVICE_ENABLE,
} bt_app_paired_device_status_t;

typedef enum {
	BT_STATUS_OFF         = 0x0000,
	BT_STATUS_ON          = 0x0001,
	BT_STATUS_BT_VISIBLE  = 0x0002,
	BT_STATUS_TRANSFER    = 0x0004,
} bt_status_t;

typedef enum {
	BT_LAUNCH_NORMAL = 0x00,
	BT_LAUNCH_SEND_FILE = 0x01,
	BT_LAUNCH_PRINT_IMAGE = 0x02,
	BT_LAUNCH_CONNECT_HEADSET = 0x03,
	BT_LAUNCH_USE_NFC = 0x04,
	BT_LAUNCH_PICK = 0x05,
	BT_LAUNCH_VISIBILITY = 0x06,
} bt_launch_mode_t;

typedef enum {
	BT_CONTROL_BAR_DISABLE,
	BT_CONTROL_BAR_ENABLE,
} bt_control_bar_status_t;

typedef enum {
	BT_NOCONTENT_HIDE,
	BT_NOCONTNET_SHOW,
} bt_nocontent_mode_t;

typedef enum {
	BT_STORE_BOOLEAN,
	BT_STORE_INT,
	BT_STORE_STRING,
} bt_store_type_t;

typedef enum {
	BT_ROTATE_PORTRAIT = 0,
	BT_ROTATE_LANDSCAPE,
	BT_ROTATE_PORTRAIT_UPSIDEDOWN,
	BT_ROTATE_LANDSCAPE_UPSIDEDOWN,
} bt_rotate_mode_t;

typedef enum {
	BT_ACTIVATED = 0,
	BT_ACTIVATING,
	BT_DEACTIVATED,
	BT_DEACTIVATING,
	BT_SEARCHING,
	BT_PAIRING
} bt_oper_t;

typedef enum {
	BT_UG_CREATE = 0,
	BT_UG_START,
	BT_UG_PAUSE,
	BT_UG_RESUME,
	BT_UG_DESTORY
} bt_ug_status_t;


typedef enum {
	BT_IDLE = 0,
	BT_DEV_PAIRING,
	BT_CONNECTING,
	BT_DISCONNECTING,
	BT_SERVICE_SEARCHING
} bt_dev_status_t;

typedef enum {
	BT_RUN_STATUS_NO_CHANGE = 0x00,	/* No Change BT status*/
	BT_RUN_STATUS_ACTIVATE = 0x01,	/* BT Activate*/
	BT_RUN_STATUS_DEACTIVATE = 0x02,	/* BT Deactivate*/
	BT_RUN_STATUS_SEARCH_TEST = 0x03,	/* BT Search Test*/
	BT_RUN_STATUS_TERMINATE = 0x04,	/* BT Terminate*/
	BT_RUN_STATUS_MAX = 0x05,		/* Max val*/
} bt_run_status_t;

typedef enum {
	BT_ON_CURRENTVIEW = 0x00,	/* Run BT on current view*/
	BT_ON_FOREGROUND = 0x01,	/* Run BT on foreground*/
	BT_ON_BACKGROUND = 0x02,	/* Run BT on background*/
} bt_on_t;

typedef enum {
	BT_HEADSET_CONNECTED = 0x01,
	BT_STEREO_HEADSET_CONNECTED = 0x02,
	BT_HID_CONNECTED = 0x04,
	BT_NETWORK_CONNECTED = 0x08,
} bt_connected_mask_t;

/**
 * This enum indicates  Device states.
 */

typedef enum {
	BT_DEVICE_NONE,        /** < None*/
	BT_DEVICE_PAIRED,     /** < Device Paired*/
	BT_DEVICE_CONNECTED/** <Device Connected*/
} bt_device_state_t;

typedef enum {
	BT_ITEM_NO_TYPE,
	BT_ITEM_TOP,
	BT_ITEM_CENTER,
	BT_ITEM_BOTTOM,
} bt_item_type_t;

typedef enum {
	BT_ITEM_NONE,
	BT_ITEM_NAME,
	BT_ITEM_UNPAIR,
	BT_ITEM_CALL,
	BT_ITEM_MEDIA,
	BT_ITEM_HID,
	BT_ITEM_NETWORK,
} bt_profile_view_item_type_t;

typedef enum {
	BT_DEVICE_MAJOR_MASK_MISC = 0x00,
	BT_DEVICE_MAJOR_MASK_COMPUTER = 0x0001,
	BT_DEVICE_MAJOR_MASK_PHONE = 0x0002,
	BT_DEVICE_MAJOR_MASK_LAN_ACCESS_POINT = 0x0004,
	BT_DEVICE_MAJOR_MASK_AUDIO = 0x0008,
	BT_DEVICE_MAJOR_MASK_PERIPHERAL = 0x0010,
	BT_DEVICE_MAJOR_MASK_IMAGING = 0x0020,
	BT_DEVICE_MAJOR_MASK_WEARABLE = 0x0040,
	BT_DEVICE_MAJOR_MASK_TOY = 0x0080,
	BT_DEVICE_MAJOR_MASK_HEALTH = 0x0100,
} bt_device_major_mask_t;


/*
 * Major device class (part of Class of Device)
 */
typedef enum {
	BT_MAJOR_DEV_CLS_MISC = 0x00,/**<miscellaneous */
	BT_MAJOR_DEV_CLS_COMPUTER = 0x01, /**<Computer */
	BT_MAJOR_DEV_CLS_PHONE = 0x02, /**<Phone */
	BT_MAJOR_DEV_CLS_LAN_ACCESS_POINT = 0x03,/**<LAN access point */
	BT_MAJOR_DEV_CLS_AUDIO = 0x04,/**<AudioDevice */
	BT_MAJOR_DEV_CLS_PERIPHERAL = 0x05,/**<Peripheral Device  */
	BT_MAJOR_DEV_CLS_IMAGING = 0x06,/**<Imaging Device */
	BT_MAJOR_DEV_CLS_WEARABLE = 0x07,/**<Wearable Device */
	BT_MAJOR_DEV_CLS_TOY = 0x08,/**<Toy Device */
	BT_MAJOR_DEV_CLS_HEALTH = 0x09,/**<Health Device */
	BT_MAJOR_DEV_CLS_UNCLASSIFIED = 0x1F/**<Unclassified device  */
} bt_major_class_t;

/*
 * Minoor device class (part of Class of Device)
 */
typedef enum {
	BTAPP_MIN_DEV_CLS_UNCLASSIFIED = 0x00,
	BTAPP_MIN_DEV_CLS_DESKTOP_WORKSTATION = 0x04,
	BTAPP_MIN_DEV_CLS_SERVER_CLASS_COMPUTER = 0x08,
	BTAPP_MIN_DEV_CLS_LAPTOP = 0x0C,
	BTAPP_MIN_DEV_CLS_HANDHELD_PC_OR_PDA = 0x10,
	BTAPP_MIN_DEV_CLS_PALM_SIZED_PC_OR_PDA = 0x14,
	BTAPP_MIN_DEV_CLS_WEARABLE_COMPUTER = 0x18,

	BTAPP_MIN_DEV_CLS_CELLULAR = 0x04,
	BTAPP_MIN_DEV_CLS_CORDLESS = 0x08,
	BTAPP_MIN_DEV_CLS_SMART_PHONE = 0x0C,
	BTAPP_MIN_DEV_CLS_WIRED_MODEM_OR_VOICE_GATEWAY = 0x10,
	BTAPP_MIN_DEV_CLS_COMMON_ISDN_ACCESS = 0x14,
	BTAPP_MIN_DEV_CLS_SIM_CARD_READER = 0x18,
	BTAPP_MID_DEV_CLS_PRINTER = 0x80,

	BTAPP_MIN_DEV_CLS_FULLY_AVAILABLE = 0x04,
	BTAPP_MIN_DEV_CLS_1_TO_17_PERCENT_UTILIZED = 0x20,
	BTAPP_MIN_DEV_CLS_17_TO_33_PERCENT_UTILIZED = 0x40,
	BTAPP_MIN_DEV_CLS_33_TO_50_PERCENT_UTILIZED = 0x60,
	BTAPP_MIN_DEV_CLS_50_to_67_PERCENT_UTILIZED = 0x80,
	BTAPP_MIN_DEV_CLS_67_TO_83_PERCENT_UTILIZED = 0xA0,
	BTAPP_MIN_DEV_CLS_83_TO_99_PERCENT_UTILIZED = 0xC0,
	BTAPP_MIN_DEV_CLS_NO_SERVICE_AVAILABLE = 0xE0,

	BTAPP_MIN_DEV_CLS_HEADSET_PROFILE = 0x04,
	BTAPP_MIN_DEV_CLS_HANDSFREE = 0x08,
	BTAPP_MIN_DEV_CLS_MICROPHONE = 0x10,
	BTAPP_MIN_DEV_CLS_LOUD_SPEAKER = 0x14,
	BTAPP_MIN_DEV_CLS_HEADPHONES = 0x18,
	BTAPP_MIN_DEV_CLS_PORTABLE_AUDIO = 0x1C,
	BTAPP_MIN_DEV_CLS_CAR_AUDIO = 0x20,
	BTAPP_MIN_DEV_CLS_SET_TOP_BOX = 0x24,
	BTAPP_MIN_DEV_CLS_HIFI_AUDIO_DEVICE = 0x28,
	BTAPP_MIN_DEV_CLS_VCR = 0x2C,
	BTAPP_MIN_DEV_CLS_VIDEO_CAMERA = 0x30,
	BTAPP_MIN_DEV_CLS_CAM_CORDER = 0x34,
	BTAPP_MIN_DEV_CLS_VIDEO_MONITOR = 0x38,
	BTAPP_MIN_DEV_CLS_VIDEO_DISPLAY_AND_LOUD_SPEAKER = 0x3C,
	BTAPP_MIN_DEV_CLS_VIDEO_CONFERENCING = 0x40,
	BTAPP_MIN_DEV_CLS_GAMING_OR_TOY = 0x48,

	BTAPP_MIN_DEV_CLS_KEY_BOARD = 0x40,
	BTAPP_MIN_DEV_CLS_POINTING_DEVICE = 0x80,
	BTAPP_MIN_DEV_CLS_COMBO_KEYBOARD_OR_POINTING_DEVICE = 0xC0,

	BTAPP_MIN_DEV_CLS_JOYSTICK = 0x04,
	BTAPP_MIN_DEV_CLS_GAME_PAD = 0x08,
	BTAPP_MIN_DEV_CLS_REMOTE_CONTROL = 0x0C,
	BTAPP_MIN_DEV_CLS_SENSING_DEVICE = 0x10,
	BTAPP_MIN_DEV_CLS_DIGITIZER_TABLET = 0x14,
	BTAPP_MIN_DEV_CLS_CARD_READER = 0x18,

	BTAPP_MIN_DEV_CLS_DISPLAY = 0x10,
	BTAPP_MIN_DEV_CLS_CAMERA = 0x20,
	BTAPP_MIN_DEV_CLS_SCANNER = 0x40,
	BTAPP_MIN_DEV_CLS_PRINTER = 0x80,

	BTAPP_MIN_DEV_CLS_WRIST_WATCH = 0x04,
	BTAPP_MIN_DEV_CLS_PAGER = 0x08,
	BTAPP_MIN_DEV_CLS_JACKET = 0x0C,
	BTAPP_MIN_DEV_CLS_HELMET = 0x10,
	BTAPP_MIN_DEV_CLS_GLASSES = 0x14,

	BTAPP_MIN_DEV_CLS_ROBOT = 0x04,
	BTAPP_MIN_DEV_CLS_VEHICLE = 0x08,
	BTAPP_MIN_DEV_CLS_DOLL_OR_ACTION = 0x0C,
	BTAPP_MIN_DEV_CLS_CONTROLLER = 0x10,
	BTAPP_MIN_DEV_CLS_GAME = 0x14,

	BTAPP_MIN_DEV_CLS_BLOOD_PRESSURE_MONITOR = 0x04,
	BTAPP_MIN_DEV_CLS_THERMOMETER = 0x08,
	BTAPP_MIN_DEV_CLS_WEIGHING_SCALE = 0x0C,
	BTAPP_MIN_DEV_CLS_GLUCOSE_METER = 0x10,
	BTAPP_MIN_DEV_CLS_PULSE_OXIMETER = 0x14,
	BTAPP_MIN_DEV_CLS_HEART_OR_PULSE_RATE_MONITOR = 0x18,
	BTAPP_MIN_DEV_CLS_MEDICAL_DATA_DISPLAY = 0x1C,
} bt_minor_class_t;


/*
 * Service class part of class of device returned from device discovery
 */

/**
  * This enum indicates  Service calls part  of device returned from device discovery.
  */
typedef enum {
	BT_COD_SC_ALL = 0x000000, /**< ALL*/
	BT_COD_SC_LIMITED_DISCOVERABLE_MODE = 0x002000,
	BT_COD_SC_POSITIONING = 0x010000,/**< POSITIONING*/
	BT_COD_SC_NETWORKING = 0x020000,/**< NETWORKING*/
	BT_COD_SC_RENDERING = 0x040000,/**< RENDERING*/
	BT_COD_SC_CAPTURING = 0x080000,/**< CAPTURING*/
	BT_COD_SC_OBJECT_TRANSFER = 0x100000,/**< OBJECT_TRANSFER*/
	BT_COD_SC_AUDIO = 0x200000,/**< AUDIO*/
	BT_COD_SC_TELEPHONY = 0x400000,/**< TELEPHONY*/
	BT_COD_SC_INFORMATION = 0x800000,/**< INFORMATION*/
	BT_COD_SC_UNKNOWN = 0x1FF000/**< UNKNOWN */
} bt_cod_service_class_t;

typedef enum {
	BT_SPP_PROFILE_UUID = ((unsigned short)0x1101),			/**<SPP*/
	BT_LAP_PROFILE_UUID = ((unsigned short)0x1102),			/**<LAP*/
	BT_DUN_PROFILE_UUID = ((unsigned short)0x1103),			/**<DUN*/
	BT_OBEX_IR_MC_SYNC_SERVICE_UUID = ((unsigned short)0x1104),	/**<OBEX IR MC SYNC*/
	BT_OBEX_OBJECT_PUSH_SERVICE_UUID = ((unsigned short)0x1105),	/**<OPP*/
	BT_OBEX_FILE_TRANSFER_UUID = ((unsigned short)0x1106),		/**<FTP*/
	BT_IRMC_SYNC_COMMAND_UUID = ((unsigned short)0x1107),		/**<IRMC SYNC COMMAND*/
	BT_HS_PROFILE_UUID = ((unsigned short)0x1108),			/**<HS*/
	BT_CTP_PROFILE_UUID = ((unsigned short)0x1109),			/**<CTP*/
	BT_AUDIO_SOURCE_UUID = ((unsigned short)0x110A),			/**<AUDIO SOURCE*/
	BT_AUDIO_SINK_UUID = ((unsigned short)0x110B),			/**<AUDIO SINK*/
	BT_AV_REMOTE_CONTROL_TARGET_UUID = ((unsigned short)0x110C),	/**<AV REMOTE CONTROL
										TARGET*/
	BT_ADVANCED_AUDIO_PROFILE_UUID = ((unsigned short)0x110D),	/**<A2DP*/
	BT_AV_REMOTE_CONTROL_UUID = ((unsigned short)0x110E),		/**<AV REMOTE CONTROL UUID*/
	BT_AV_REMOTE_CONTROL_CONTROLLER_UUID = ((unsigned short)0x110F),	/**<AV REMOTE CONTROLLER UUID*/
	BT_ICP_PROFILE_UUID = ((unsigned short)0x1110),			/**<ICP*/
	BT_FAX_PROFILE_UUID = ((unsigned short)0x1111),			/**<FAX*/
	BT_HEADSET_AG_SERVICE_UUID = ((unsigned short)0x1112),		/**<HS AG */
	BT_PAN_PANU_PROFILE_UUID = ((unsigned short)0x1115),		/**<PAN*/
	BT_PAN_NAP_PROFILE_UUID = ((unsigned short)0x1116),		/**<PAN*/
	BT_PAN_GN_PROFILE_UUID = ((unsigned short)0x1117),		/**<PAN*/
	BT_DIRECT_PRINTING = ((unsigned short)0x1118),
	BT_OBEX_BPPS_PROFILE_UUID = ((unsigned short)0x1118),		/**<OBEX BPPS*/ /* Will be removed */
	BT_REFERENCE_PRINTING = ((unsigned short)0x1119),
	BT_OBEX_IMAGING_UUID = ((unsigned short)0x111A),			/**<OBEX_IMAGING*/
	BT_OBEX_IMAGING_RESPONDER_UUID = ((unsigned short)0x111B),	/**<OBEX_IMAGING
										RESPONDER*/
	BT_IMAGING_AUTOMATIC_ARCHIVE_UUID = ((unsigned short)0x111C),	/**<IMAGING AUTOMATIC ARCHIVE*/
	BT_IMAGING_REFERENCED_OBJECTS_UUID = ((unsigned short)0x111D),	/**<IMAGING REFERENCED OBJECTS*/
	BT_HF_PROFILE_UUID = ((unsigned short)0x111E),			/**<HF*/
	BT_HFG_PROFILE_UUID = ((unsigned short)0x111F),			/**<HFG*/
	BT_DIRECT_PRINTING_REFERENCE_OBJ_UUID = ((unsigned short)0x1120),
									/**<DIRECT PRINTING*/
	BT_REFLECTED_UI = ((unsigned short)0x1121),		/**<REFLECTED UI*/
	BT_BASIC_PRINTING = ((unsigned short)0x1122),		/**<BASIC PRINTING*/
	BT_PRINTING_STATUS = ((unsigned short)0x1123),		/**<PRINTING  STATUS*/
	BT_OBEX_PRINTING_STATUS_UUID = ((unsigned short)0x1123),	/**<OBEX PRINTING STATUS*/ /* Will be removed */
	BT_HID_PROFILE_UUID = ((unsigned short)0x1124),		/**<HID*/
	BT_HCR_PROFILE_UUID = ((unsigned short)0x1125),		/**<HCRP*/
	BT_HCR_PRINT_UUID = ((unsigned short)0x1126),		/**<HCR PRINT*/
	BT_HCR_SCAN_UUID = ((unsigned short)0x1127),		/**<HCR SCAN*/
	BT_SIM_ACCESS_PROFILE_UUID = ((unsigned short)0x112D),	/**<SIM ACCESS PROFILE*/
	BT_PBAP_PCE_UUID = ((unsigned short)0x112E),		/**<PBAP - PCE*/
	BT_PBAP_PSE_UUID = ((unsigned short)0x112F),		/**<OBEX PBA*/
	BT_OBEX_PBA_PROFILE_UUID = ((unsigned short)0x112F),	/**<OBEX PBA*/ /* Will be removed */
	BT_OBEX_PBAP_UUID = ((unsigned short)0x1130),		/**<OBEX PBA*/
	BT_HEADSET_HS_UUID = ((unsigned short)0x1131),		/**<HEADSET HS*/
	BT_MESSAGE_ACCESS_SERVER_UUID = ((unsigned short)0x1132),/**<MESSAGE ACCESS SERVER*/
	BT_MESSAGE_NOTIFICATION_SERVER_UUID = ((unsigned short)0x1133),/**<MESSAGE NOTIFICATION SERVER*/
	BT_MESSAGE_ACCESS_PROFILE_UUID = ((unsigned short)0x1134),/**<MESSAGE ACCESS PROFILE*/
	BT_PNP_INFORMATION_UUID = ((unsigned short)0x1200),	/**<PNP*/
	BT_GENERIC_NETWORKING_UUID = ((unsigned short)0x1201),	/**<GENERIC NETWORKING*/
	BT_GENERIC_FILE_TRANSFER_UUID = ((unsigned short)0x1202),/**<GENERIC FILE TRANSFER*/
	BT_GENERIC_AUDIO_UUID = ((unsigned short)0x1203),	/**<GENERIC AUDIO*/
	BT_GENERIC_TELEPHONY_UUID = ((unsigned short)0x1204),	/**<GENERIC TELEPHONY*/
	BT_VIDEO_SOURCE_UUID = ((unsigned short)0x1303), 	/**<VEDIO SOURCE*/
	BT_VIDEO_SINK_UUID = ((unsigned short)0x1304),		/**<VEDIO SINK*/
	BT_VIDEO_DISTRIBUTION_UUID = ((unsigned short)0x1305),	/**<VEDIO DISTRIBUTION*/
	BT_HDP_UUID = ((unsigned short)0x1400),			/**<HDP*/
	BT_HDP_SOURCE_UUID = ((unsigned short)0x1401),		/**<HDP SOURCE*/
	BT_HDP_SINK_UUID = ((unsigned short)0x1402),		/**<HDP SINK*/
	BT_OBEX_SYNCML_TRANSFER_UUID = ((unsigned short)0x0000)	/**<OBEX_SYNC*/ /* Will be removed */
} bluetooth_service_uuid_list_t;


/**************************************************
*                             Struct define
***************************************************/

typedef struct {
	unsigned char bd_addr[BT_ADDRESS_LENGTH_MAX];
	char addr_str[BT_ADDRESS_STR_LEN + 1];
	char name[BT_DEVICE_NAME_LENGTH_MAX + 1];/**<  Device Name */
	bt_service_class_t service_list;  /**< type of service */
	bt_major_class_t major_class; /**< major class of the device */
	bt_minor_class_t minor_class; /**< minor class of the device */
	int authorized;    /**< authorized ? */
	bt_cod_service_class_t service_class; /**< service class of device */
	int rssi;	 /**< Received signal strength indicator */
	int connected_mask;
	int status;
	int item_type;
	int uuid_count;
	char **uuids;
	void *layout;
	void *entry;
	void *genlist_item;
	void *net_profile;
	gboolean is_bonded;
	gboolean call_checked;
	gboolean media_checked;
	gboolean hid_checked;
	gboolean network_checked;
	gboolean highlighted;
	void *ugd;
} bt_dev_t;

typedef struct {
	unsigned char bd_addr[BT_ADDRESS_LENGTH_MAX];
} bt_address_t;


/**************************************************
*                              Callback type
***************************************************/

typedef void (*bt_app_back_cb)(void *, void *, void *);


#ifdef __cplusplus
}
#endif

#endif /* __BT_TYPE_DEFINE_H__ */

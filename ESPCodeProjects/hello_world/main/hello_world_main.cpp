extern "C" void app_main(void);  // Declares app_main as a C function

#include "esp_zigbee_core.h"
#include "esp_check.h"
#include "nvs_flash.h"

#include <cstring>

static const char *TAG = "ZB_TEST";

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_ERROR_CHECK(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}


void esp_zb_app_signal_handler(esp_zb_app_signal_t* signal_struct)
{
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = (esp_zb_app_signal_type_t)(*(signal_struct->p_app_signal));

    switch (sig_type) 
    {
        case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
            ESP_LOGI(TAG, "Zigbee stack initialized");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
            break;
        case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
            if (err_status == ESP_OK) 
            {
                ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
                if (esp_zb_bdb_is_factory_new()) 
                {
                    ESP_LOGI(TAG, "Start network steering");
                    esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
                } 
                else 
                    ESP_LOGI(TAG, "Device rebooted");
            } 
            else
                ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));       // commissioning failed
            break;
        case ESP_ZB_BDB_SIGNAL_STEERING:
            if (err_status == ESP_OK) 
            {
                esp_zb_ieee_addr_t extended_pan_id;
                esp_zb_get_extended_pan_id(extended_pan_id);
                ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                        extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                        extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                        esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
            } 
            else 
            {
                ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
                esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
            }
            break;
        default:
            ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type, esp_err_to_name(err_status));
            break;
    }
}


static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void* message)
{
    switch (callback_id) 
    {
        case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        {
            esp_zb_zcl_set_attr_value_message_t* setAtrMsg = (esp_zb_zcl_set_attr_value_message_t*)message;
            ESP_LOGI(TAG, "Received message: endpoint %d, cluster 0x%x, attribute 0x%x, data size %d", setAtrMsg->info.dst_endpoint, setAtrMsg->info.cluster,
                 setAtrMsg->attribute.id, setAtrMsg->attribute.data.size);
            break;
        }
        case ESP_ZB_CORE_CMD_DEFAULT_RESP_CB_ID:
        {
            esp_zb_zcl_cmd_default_resp_message_t* msg = (esp_zb_zcl_cmd_default_resp_message_t*)message;
            ESP_LOGI(TAG, "Default response callback: dst 0x%x, status: %u", msg->info.dst_address, msg->status_code);
            break;
        }
        default:
            ESP_LOGW(TAG, "Unhandled Zigbee action 0x%x callback", callback_id);
            break;
    }
    return ESP_OK;
}


//for a good list of the possible attributes, see https://www.nxp.com/docs/en/user-guide/JN-UG-3115.pdf
esp_zb_attribute_list_t* createBasicCluster(uint8_t powerSource, const char* manufacturerName, const char* modelName)
{
    esp_zb_basic_cluster_cfg_s basicConfig = 
    {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = 4            //DC source, see note page 178
    };
    auto cluster = esp_zb_basic_cluster_create(&basicConfig);        //this creates all three mandatory attributes

    char strBuf[34];
    if (manufacturerName)
    {
        strncpy(&strBuf[1], manufacturerName, 32);
        strBuf[0] = strlen(manufacturerName);
        esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, strBuf);
    }
    if (modelName)
    {
        strncpy(&strBuf[1], modelName, 32);
        strBuf[0] = strlen(modelName);
        esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, strBuf);
    }

    return cluster;
}


esp_zb_attribute_list_t* createOnOffCluster()
{
    ESP_LOGI(TAG, "Init OnOff cluster");
    esp_zb_on_off_cluster_cfg_t sw_in_cfg = {
    .on_off = false
    };

    auto cluster = esp_zb_on_off_cluster_create(&sw_in_cfg);

    return cluster;
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    //config the radio platform
    esp_zb_platform_config_t config; 
    config.radio_config.radio_mode = ZB_RADIO_MODE_NATIVE;
    config.host_config.host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE;
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));

    //initialize Zigbee stack
    esp_zb_cfg_t zb_nwk_cfg; 
    zb_nwk_cfg.esp_zb_role = ESP_ZB_DEVICE_TYPE_ED;
    zb_nwk_cfg.install_code_policy = false;
    zb_nwk_cfg.nwk_cfg.zed_cfg.ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN;
    zb_nwk_cfg.nwk_cfg.zed_cfg.keep_alive = 3000;
    esp_zb_init(&zb_nwk_cfg);

    //create & populate cluster list
    esp_zb_cluster_list_t* cluster_list = esp_zb_zcl_cluster_list_create();        
    esp_zb_cluster_list_add_basic_cluster(cluster_list, createBasicCluster(4, "TestDevice", "ESP32H2-DevKit"), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_cluster_list_add_on_off_cluster(cluster_list, createOnOffCluster(), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    //create endpoint list and populate it
    esp_zb_ep_list_t* ep_list = esp_zb_ep_list_create();
    esp_zb_endpoint_config_t endpointConfig;
    endpointConfig.endpoint = 10;
    endpointConfig.app_profile_id = ESP_ZB_AF_HA_PROFILE_ID;
    endpointConfig.app_device_id = ESP_ZB_HA_ON_OFF_OUTPUT_DEVICE_ID;
    esp_zb_ep_list_add_ep(ep_list, cluster_list, endpointConfig);
    esp_zb_device_register(ep_list);

    //zigbee startup
    esp_zb_core_action_handler_register(zb_action_handler);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}
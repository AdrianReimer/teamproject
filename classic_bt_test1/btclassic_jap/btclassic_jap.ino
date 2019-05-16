#define CONFIG_CLASSIC_BT_ENABLED

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include "time.h"
#include "sys/time.h"

#define EXCAMPLE_DEVICE_NAME "ESP_32"

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_master = ESP_SPP_ROLE_MASTER;

static uint8_t peer_bdname_len;
static char peer_bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
static esp_bd_addr_t peer_bd_addr = {0x5C,0xBA,0x37,0xFE,0xE0,0x03}; // 適宜変更してください
static const esp_bt_inq_mode_t inq_mode = ESP_BT_INQ_MODE_GENERAL_INQUIRY;
static const uint8_t inq_len = 30;
static const uint8_t inq_num_rsps = 0;


uint16_t myhandle;


static bool get_name_from_eir(uint8_t *eir, char *bdname, uint8_t *bdname_len)
{
    uint8_t *rmt_bdname = NULL;
    uint8_t rmt_bdname_len = 0;

    if (!eir) {
        return false;
    }

    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname) {
        rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname) {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN) {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (bdname) {
            memcpy(bdname, rmt_bdname, rmt_bdname_len);
            bdname[rmt_bdname_len] = '\0';
        }
        if (bdname_len) {
            *bdname_len = rmt_bdname_len;
        }
        return true;
    }

    return false;
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
    case ESP_SPP_INIT_EVT:
        Serial.println("ESP_SPP_INIT_EVT: Starting GAP discovery");
        esp_bt_dev_set_device_name(EXCAMPLE_DEVICE_NAME);
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        esp_bt_gap_start_discovery(inq_mode, inq_len, inq_num_rsps);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        Serial.println("ESP_SPP_DISCOVERY_COMP_EVT: Connecting");
        if (param->disc_comp.status == ESP_SPP_SUCCESS) {
            //Serial.println("test: x");
            esp_spp_connect(sec_mask, role_master, param->disc_comp.scn[0], peer_bd_addr);
            //Serial.println("test: y");
        }
        break;
    case ESP_SPP_OPEN_EVT:
        Serial.println("ESP_SPP_OPEN_EVT");
        esp_spp_write(param->srv_open.handle, 14, (uint8_t*)"ATZ\rATE0\rATL0\r");
        break;
    case ESP_SPP_CLOSE_EVT:
        Serial.println("ESP_SPP_CLOSE_EVT");
        break;
    case ESP_SPP_START_EVT:
        Serial.println("ESP_SPP_START_EVT");
        break;
    case ESP_SPP_CL_INIT_EVT:
        Serial.println("ESP_SPP_CL_INIT_EVT");
        break;
    case ESP_SPP_DATA_IND_EVT:
        Serial.printf( "ESP_SPP_DATA_IND_EVT len=%d handle=%d\n", param->data_ind.len, param->data_ind.handle);
        {
          for (int i = 0; i < param->data_ind.len; i++){
              if ( (char)(param->data_ind.data[i]) == '\r' || (char)(param->data_ind.data[i]) == '\n') {
                 Serial.println("");
              }
              else {
                Serial.print( (char)(param->data_ind.data[i]) );
             }
          }
        }
        break;
    case ESP_SPP_CONG_EVT:
        Serial.print("ESP_SPP_CONG_EVT cong= ");
        Serial.println( param->cong.cong );
        break;
    case ESP_SPP_WRITE_EVT:
        myhandle = param->write.handle;
        Serial.printf( "ESP_SPP_WRITE_EVT len=%d cong=%d\n", param->write.len , param->write.cong);
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        Serial.println("ESP_SPP_SRV_OPEN_EVT");
        break;
    default:
        Serial.println("ESP_SPP_EVENT OTHER");
        break;
    }
}

static void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch(event){
    case ESP_BT_GAP_DISC_RES_EVT:
        Serial.print( "ESP_BT_GAP_DISC_RES_EVT: num = ");
        Serial.println( param->disc_res.num_prop );
        for (int i = 0; i < param->disc_res.num_prop; i++){
            if ( param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_COD && memcmp( peer_bd_addr, param->disc_res.bda, ESP_BD_ADDR_LEN ) == 0 ) {
              Serial.println( "device found." );   
              //Serial.println("test: 4");             
              esp_spp_start_discovery(peer_bd_addr);
              //Serial.println("test: 5");
              esp_bt_gap_cancel_discovery();
              //Serial.println("test: 6");
            }
            if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_COD ) {//class of device
              if ( get_name_from_eir((uint8_t*)(param->disc_res.prop[i]).val, peer_bdname, &peer_bdname_len) ) {
                //Serial.println("test: 7");
                peer_bdname[peer_bdname_len] = 0;
                Serial.print( "PEER CLASS OF DEVICE: " );             
                Serial.println( peer_bdname );             
              }
            }
            else if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_RSSI ) {
              //Serial.println("test: 8");
              Serial.print( "param->disc_res.prop[i].type: ");
              Serial.println( param->disc_res.prop[i].type );
            }
            else if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_BDNAME ) {
              //Serial.println("test: 9");
              Serial.print( "param->disc_res.prop[i].type: ");
              Serial.println( param->disc_res.prop[i].type );
            }
            else {
              //Serial.println("test: 9");
              Serial.print( "param->disc_res.prop[i].type: ");
              Serial.println( param->disc_res.prop[i].type );
            }
        }
        //Serial.println("test: 10");
        break;
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        break;
    case ESP_BT_GAP_RMT_SRVCS_EVT:
        break;
    case ESP_BT_GAP_RMT_SRVC_REC_EVT:
        break;
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        break;
        //Serial.println("test: 11");
    case ESP_BT_GAP_PIN_REQ_EVT:
        if (param->pin_req.min_16_digit) {
          Serial.println("test: 12");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
          Serial.println("test: 13");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    case ESP_BT_GAP_CFM_REQ_EVT:
      Serial.println("test: 14");
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        break;
    default:
        break;
    }
}

void setup()
{
    Serial.begin(115200);  
    Serial.print("Start\n");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (!btStart()) {
        Serial.printf( "btStart() failed\n");
        return;
    }

    esp_bluedroid_status_t bt_state = esp_bluedroid_get_status();
    if (bt_state == ESP_BLUEDROID_STATUS_UNINITIALIZED){
        if (esp_bluedroid_init()) {
            Serial.println("initialize bluedroid failed");
            return;
        }
    }

    if (bt_state != ESP_BLUEDROID_STATUS_ENABLED){
        if (esp_bluedroid_enable()) {
            Serial.println("enable bluedroid failed");
            return;
        }
    }

    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
      //Serial.println("test: 1");
        return;
    }

    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
      //Serial.println("test: 2");
        return;
    }

    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        //Serial.println("test: 3");
        return;
    }

    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
    //Serial.println("test: 33");

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    Serial.println("bluetooth initialize finished");

}


void loop()
{
  //Serial.println("test: loop");
  delay(1000);
  esp_spp_write(myhandle , 6, (uint8_t*)"01 0D\r");
  delay(1000);
  esp_spp_write(myhandle , 6, (uint8_t*)"01 0C\r" );
  delay(1000);
  esp_spp_write(myhandle , 6, (uint8_t*)"01 46\r" );
}

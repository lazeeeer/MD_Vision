
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "wifi_comms.h"

// Defines for WiFi connection and server parameters
#define WIFI_SSID       "INSERT"
#define WIFI_PASSWORD   "INSERT"
#define SERVER_IP       "INSERT"
#define SERVER_SOCKET   "INSERT"

// Defines for bits controlling wifi initialization
#define WIFI_SUCCESS        1 << 0
#define WIFI_FAILURE        1 << 1
#define TCP_SUCCESS         1 << 0
#define TCP_FAILURE         1 << 1
#define MAX_FAILURES        10     // Max failed connection attemps before tossing FAIL status


// Global Variables //
static const char *TAG = "WIFI";
static EventGroupHandle_t wifi_event_group;     // group bits to contain status bits for wifi connection
static int s_retry_num = 0;                     //retry tracker



// ==== WiFi Connection Functions (Not for data processing) ======================= //

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    // base case where we want to connect to a wifi network(sta)
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // we start trying to connect to wifi
        ESP_LOGI(TAG, "Connecting to AP...");
        esp_wifi_connect();

    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // start trying to reconnect upon failures until max failures reached
        if (s_retry_num < MAX_FAILURES)
        {
            // try to reconnect again while tracking num of failed conenctions
            ESP_LOGI(TAG, "Reconnecting to AP...");
            esp_wifi_connect();
            s_retry_num++;

        }else {
            // set the failture bit HIGH in event group bit register
            xEventGroupSetBits(wifi_event_group, WIFI_FAILURE);
        }
    }
}


// Event handler for IP events
static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    //
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        //ESP_LOGI(TAG, "STA IP: ", IPSTR, IP2STR(&event->ip_info.ip) );
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_SUCCESS);
    }

}


esp_err_t connect_wifi()
{
    int status = WIFI_FAILURE;

    // init all the needed wifi shid //

    ESP_ERROR_CHECK(esp_netif_init());                  // init network interface
    ESP_ERROR_CHECK(esp_event_loop_create_default());   // init default event loop

    // create wifi station in wifi driver
    esp_netif_create_default_wifi_sta();

    // setup the wifi station with default parameters
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    // event loop shid //

    // taking static event group var and initing it 
    wifi_event_group = xEventGroupCreate();

    esp_event_handler_instance_t wifi_handler_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        &wifi_handler_event_instance
    ));

    esp_event_handler_instance_t got_ip_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &ip_event_handler,
        NULL,
        &got_ip_event_instance
    ));


    // Start WiFi Driver shid //

    wifi_config_t wifi_config = {
        
        // insert all paramets for wifi connection here
        .sta = {
            .ssid = "MY SSID",
            .password = "MY PASSWORD",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    // set the wifi controller to be a station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // set the wifi config
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // start the wifi driver
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "STA initialization complete");


    // Now waiting to event handles ... // 

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
    
        WIFI_SUCCESS | WIFI_FAILURE,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    // xEventGroupWaitBits returns bits before the call returned so we see what event acc happened
    if (bits & WIFI_SUCCESS) {
        ESP_LOGI(TAG, "Connected to ap");
        status = WIFI_SUCCESS;
    } else if (bits & WIFI_FAILURE) {
        ESP_LOGI(TAG, "Failed to connect to ap");
        status = WIFI_FAILURE;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        status = WIFI_FAILURE;
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler_event_instance));
    vEventGroupDelete(wifi_event_group);

    return status;

}


// TODO: STEVEN MAKE THIS 
// THIS IS THE FUNCTION TO CALL TO REPEATEDLT ON COMMAND I THINK
esp_err_t connect_tcp_server()
{

    return ESP_OK;
}


// functino for calling to and initing wifi connection using functions above
// this function is to be called from main during the POST tests
void init_wifi_comms()
{
    esp_err_t status = WIFI_FAILURE;

    // initializing flash storage - NOT SURE IF THIS IS NEEDED
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    // connect to wireless AP
    status = connect_wifi();
    if (WIFI_SUCCESS != status)
    {
        ESP_LOGI(TAG, "Failed to associate to AP, dying...");
        return;
    }

}
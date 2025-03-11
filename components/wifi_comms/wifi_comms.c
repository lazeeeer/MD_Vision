// standard includes
#include <stdio.h>
#include <string.h>

// esp system includes
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_netif.h"

#include "esp_camera.h"
#include "GUI_drivers.h"
#include "esp_timer.h"

#include "cJSON.h"

// C http libraries
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

// library for parsing the JSONs from HTTP
#include "jsmn.h"

// custom header file
#include "wifi_comms.h"


// ==== Defines needed for code =============================

// Defines for WiFi connection and server parameters
#define WIFI_SSID       "Danijela"
#define WIFI_PASSWORD   "Gumdrop1"

#define SERVER_IP       "INSERT"
#define SERVER_SOCKET   "INSERT"

// Defines for bits controlling wifi initialization
#define WIFI_SUCCESS        1 << 0
#define WIFI_FAILURE        1 << 1
#define TCP_SUCCESS         1 << 0
#define TCP_FAILURE         1 << 1
#define MAX_FAILURES        10     // Max failed connection attemps before tossing FAIL status


// define for button assignment and variables
#define WIFI_BUTTON         33


// Global Variables //
static const char *TAG = "WIFI_COMMS";
static EventGroupHandle_t wifi_event_group;     // group bits to contain status bits for wifi connection
static int s_retry_num = 0;                     //retry tracker
#define MAX_HTTP_OUTPUT_BUFFER 128
static char response_buffer[MAX_HTTP_OUTPUT_BUFFER];



// JSON Example to test pasring HTTP response info
const char *json_string = "{"
    "\"patient_name\": \"John Doe\","
    "\"check_in_date\": \"2025-01-01\","
    "\"last_check_in_time\": \"14:30:00\","
    "\"current_doctor\": \"Dr. Smith\""
"}";


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


    // Start WiFi Driver stuff //
    wifi_config_t wifi_config = {
        
        // insert all paramets for wifi connection here
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
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


// ==== Function to call from wrapper to init wifi comms ============================

// functino for calling to and initing wifi connection using functions above
// this function is to be called from main during the POST tests
esp_err_t init_wifi_comms()
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
        return ESP_FAIL;
    }
    else {
        return ESP_OK;  // case where is WAS able to connect
    }

}



// ==== Function calls for data processing and server interaction =======================

//event handler for when we make a request to a server as a client
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {

    if (evt->event_id == HTTP_EVENT_ON_DATA) {

        // making sure out HTTP buffer isnt overflowed
        if (evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
            //printf("%s\n", (char *)evt->data);
            strncpy(response_buffer, (char *)evt->data, evt->data_len);
            response_buffer[evt->data_len] = '\0';  // Null-terminate the buffer
        }
    }
    return ESP_OK;
}



// function to parse the JSON string we would get from the HTTP response
void parse_json( const char * jsonString)
{

    // passinng the json to the parser and checking for any issues
    cJSON *root = cJSON_Parse(jsonString);
    if (!root)
    {
        printf("Error before: %s\n", cJSON_GetErrorPtr());
    }

    // extracting a "device field"
    cJSON *first_name = cJSON_GetObjectItem(root, "f_name");
    if ( cJSON_IsString(first_name) ) {
        printf("name is: %s\n", first_name->valuestring);
        write_to_disp(first_name->valuestring);
        printf("REACHED after messagekj\n");
        vTaskDelay(pdMS_TO_TICKS(3000));
        display_clear_msg_text();
    }

    // clearning the cJSON root used to parse before completing
    cJSON_Delete(root);
}



// simple function that takes a URL to ping as a test
esp_err_t http_ping_server(const char* url) {

    int64_t start_time = esp_timer_get_time();  // start time

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
    };

    // config and send the client request
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    int64_t stop_time = esp_timer_get_time();   // stopping and measuring time after client request

    if (err == ESP_OK) {
        printf("HTTP GET Status = %d, content_length = %lld \n", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
        printf("Elapsed time was: %lld microseconds\n", start_time-stop_time);
    } else {
        printf("HTTP GET request failed: %s\n", esp_err_to_name(err));
        return ESP_FAIL;
    }

    // Clean up
    esp_http_client_cleanup(client);
    return ESP_OK;
}



// function to use in order to send an image to the server and get JSON patient data back
esp_err_t send_image_to_server( camera_fb_t *fb )
{
    esp_http_client_config_t config = {
        .url = "http://10.0.0.73:5000/upload_image", // Flash server URL for image upload
        .event_handler = _http_event_handler,
        .method = HTTP_METHOD_POST,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // filling header information for client request
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_header(client, "Content-Disposition", "form-data; name=\"file\"; filename=\"image.jpg\"");
    esp_http_client_set_post_field( client, (const char*)fb->buf, fb->len );

    esp_err_t err = esp_http_client_perform(client);    // sending the image

    // checking client resposne
    if ( err == ESP_OK )
    {
        int status_code = esp_http_client_get_status_code(client);
        printf("returned status code: %d\n", status_code);

        // readiong JSON
        int content_length = esp_http_client_get_content_length(client);
        if (content_length > 0)
        {   
            printf("%s\n", response_buffer);
            parse_json(response_buffer);
        }
    }
    else {
        printf("HTTP POST failed...\n");
    }


    // clean up before returning
    esp_http_client_cleanup(client);
    return err;
}










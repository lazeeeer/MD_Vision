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
#define WIFI_SSID       "SLIM-PC"
#define WIFI_PASSWORD   "12345678"

#define SERVER_IP       "INSERT"
#define SERVER_SOCKET   "INSERT"

// Defines for bits controlling wifi initialization
#define WIFI_SUCCESS        1 << 0
#define WIFI_FAILURE        1 << 1
#define TCP_SUCCESS         1 << 0
#define TCP_FAILURE         1 << 1
#define MAX_FAILURES        10     // Max failed connection attemps before tossing FAIL status


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


    // Start WiFi Driver shid //
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


// event handler for when we make a request to a server as a client
// esp_err_t _http_event_handler(esp_http_client_event_t *evt) {

//     if (evt->event_id == HTTP_EVENT_ON_DATA) {

//         // making sure out HTTP buffer isnt overflowed
//         if (evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
//             printf("%s\n", (char *)evt->data);
//             strncpy(response_buffer, (char *)evt->data, evt->data_len);
//             response_buffer[evt->data_len] = '\0';  // Null-terminate the buffer
//         }
//     }
//     return ESP_OK;
// }



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
    cJSON *name = cJSON_GetObjectItem(root, "patient_name");
    if ( cJSON_IsString(name) ) {
        printf("name is: %s\n", name->valuestring);
    }

    // extracting a "device field"
    cJSON *date = cJSON_GetObjectItem(root, "check_in_date");
    if ( cJSON_IsString(date) ) {
        printf("date is: %s\n", date->valuestring);
    }

    // extracting a "device field"
    cJSON *checkIn = cJSON_GetObjectItem(root, "last_check_in_time");
    if ( cJSON_IsString(checkIn) ) {
        printf("checkIn is: %s\n", checkIn->valuestring);
    }

    // extracting a "device field"
    cJSON *doctor = cJSON_GetObjectItem(root, "current_doctor");
    if ( cJSON_IsString(doctor) ) {
        printf("doctor is: %s\n", doctor->valuestring);
    }

    // clearning the cJSON root used to parse before completing
    cJSON_Delete(root);
}


// function call for testing HTTP client request to our Flask server
// void test_http_request() {
    
//     // condfiguring the client struct for WHERE we want to connect to 
//     esp_http_client_config_t config = {
//         .url = "http://172.20.10.3:5000/ping",
//         //.event_handler = _http_event_handler,
//         .auth_type = HTTP_AUTH_TYPE_NONE,
//     };
//     esp_http_client_handle_t client = esp_http_client_init(&config);

//     // sending the request as a client
//     // when request is made, our event handler will fill the response buffer global variable with the response
//     esp_err_t err = esp_http_client_perform(client);
//     if (err == ESP_OK) {
//         printf("REACHED1\n");
//         printf("%s\n", response_buffer);
//         printf("HTTP GET Status = %d, content_length = %lld\n", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
//         printf("REACHED2\n");

//         // passing the response buffer to the JSON parser for output
//         parse_json(response_buffer);

//     } else {
//         // something went wrong, print the err using esp_err_to_name()
//         printf("HTTP GET request failed: %s\n", esp_err_to_name(err));
//     }

//     // freeing memmory of the client struct
//     esp_http_client_cleanup(client);
// }




// Callback to handle the received data
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            printf("HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                printf("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
                printf("Received data: %s", (char *)evt->data);
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

// void test_http_request() {
//     esp_http_client_config_t config = {
//         .url = "http://172.20.10.3:5000/ping", // Replace with your Flask server's IP address
//         .event_handler = _http_event_handler,
//     };

//     esp_http_client_handle_t client = esp_http_client_init(&config);

//     // Perform HTTP GET request
//     esp_err_t err = esp_http_client_perform(client);

//     if (err == ESP_OK) {
//         printf("HTTP GET Status = %d, content_length = %lld", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
//     } else {
//         printf("HTTP GET request failed: %s", esp_err_to_name(err));
//     }

//     // Clean up
//     esp_http_client_cleanup(client);
// }



esp_err_t send_image_to_server( camera_fb_t *fb )
{
    esp_http_client_config_t config = {
        .url = "http://172.20.10.3:5000/upload_image", // Flash server URL for image upload
        .method = HTTP_METHOD_POST,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // filling header information for client request
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_post_field( client, (const char*)fb->buf, fb->len );

    esp_err_t err = esp_http_client_perform(client);    // sending the image

    // checking client resposne
    if ( err == ESP_OK )
    {
        int status_code = esp_http_client_get_status_code(client);
        printf("returned status code: %d\n", status_code);
    }
    else {
        printf("HTTP POST failed...\n");
    }

    // clean up before returning
    esp_http_client_cleanup(client);
    return err;
}






// TODO: STEVEN MAKE THIS 
// THIS IS THE FUNCTION TO CALL TO REPEATEDLT ON COMMAND I THINK
esp_err_t connect_tcp_server()
{

    return ESP_OK;
}


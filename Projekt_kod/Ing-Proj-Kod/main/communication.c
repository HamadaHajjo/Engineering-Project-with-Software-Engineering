// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mdf_common.h"
#include "mwifi.h"
#include "driver/uart.h"
#include "communication.h"
#include "statemachine.h"


// #define MEMORY_DEBUG

#define BUF_SIZE (1024)

#define MAX_NODES 20  
#define BUTTON1_GPIO GPIO_NUM_25
#define BUTTON2_GPIO GPIO_NUM_26
#define BUTTON3_GPIO GPIO_NUM_27
#define LED_MESSAGE_GPIO GPIO_NUM_14
#define LED_ROOT_GPIO GPIO_NUM_13

static int g_sockfd    = -1;
static const char *TAG = "communication";
static esp_netif_t *netif_sta = NULL;

static uint8_t routing_table[MAX_NODES][MWIFI_ADDR_LEN];
static int routing_table_size = 0;

static uint8_t parent_addr[MWIFI_ADDR_LEN]; 
static uint8_t root_addr[MWIFI_ADDR_LEN];
static uint8_t self_addr[MWIFI_ADDR_LEN];

static uint8_t broadcast_addr[MWIFI_ADDR_LEN] = MWIFI_ADDR_BROADCAST;



void gpio_init() {
    
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_MESSAGE_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_config);
    gpio_set_level(LED_MESSAGE_GPIO, 0);  

    
    gpio_config_t led_root_config = {
        .pin_bit_mask = (1ULL << LED_ROOT_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_root_config);
    gpio_set_level(LED_ROOT_GPIO, 0);  

    
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON1_GPIO) | (1ULL << BUTTON2_GPIO) | (1ULL << BUTTON3_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,  
    };
    gpio_config(&button_config);

    
    MDF_LOGI("GPIO init completed: LEDs configured as output, buttons configured as input.");
}




void broadcastTick(void *args){
    MDF_LOGW("IN broadcastTic");
    mdf_err_t ret = MDF_OK;
    size_t size   = 0;
    mwifi_data_type_t data_type = {0x0};
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "command", "Tick_root");

    char *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    size = strlen(data);

    mwifi_root_write(broadcast_addr, 1, &data_type, data, size, true);
}

void toggle_message_led(void *args){
    gpio_set_level(LED_MESSAGE_GPIO, 1);
    vTaskDelay(1000 / portTICK_RATE_MS);  
    gpio_set_level(LED_MESSAGE_GPIO, 0);  
}


/**
 * @brief Create a tcp client
 */
static int socket_tcp_client_create(const char *ip, uint16_t port)
{
    MDF_PARAM_CHECK(ip);

    MDF_LOGI("Create a tcp client, ip: %s, port: %d", ip, port);

    mdf_err_t ret = ESP_OK;
    int sockfd    = -1;
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(ip),
    };

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    MDF_ERROR_GOTO(sockfd < 0, ERR_EXIT, "socket create, sockfd: %d", sockfd);

    ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "socket connect, ret: %d, ip: %s, port: %d",
                   ret, ip, port);

    return sockfd;

ERR_EXIT:

    if (sockfd != -1) {
        close(sockfd);
    }

    return -1;
}



void tcp_client_write_task(cJSON *message)
{
    mdf_err_t ret = MDF_OK;
    char *data    = MDF_CALLOC(1, MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0x0};

    MDF_LOGI("TCP client write task is running");

    /*
    if (g_sockfd == -1) {
        vTaskDelay(500 / portTICK_RATE_MS);
        continue;
    }

    
    size = MWIFI_PAYLOAD_LEN - 1;
    memset(data, 0, MWIFI_PAYLOAD_LEN);
    ret = mwifi_root_read(src_addr, &data_type, data, &size, portMAX_DELAY);
    MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_root_read", mdf_err_to_name(ret));
    */

    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "message", cJSON_Duplicate(message, true));;

    char *json_str = cJSON_PrintUnformatted(json);

    MDF_LOGD("TCP write, size: %d, data: %s", size, data);
    size = strlen(json_str);

    
    char *json_with_newline = malloc(size + 2); // +1 för \n och +1 för \0
    sprintf(json_with_newline, "%s\n", json_str);

    ret = write(g_sockfd, json_with_newline, strlen(json_with_newline));
    if (ret <= 0) {
        MDF_LOGW("Failed to write to socket: %s", strerror(errno));
    } else {
        MDF_LOGW("Sent message: %s", json_with_newline);
    }
    //MDF_ERROR_CONTINUE(ret <= 0, "<%s> TCP write", strerror(errno));
    MDF_LOGW("ret: %x", ret);

    free(json_str);
    cJSON_Delete(json);
    

    MDF_LOGI("TCP client write task is exit");

    //close(g_sockfd);
    //g_sockfd = -1;
    MDF_FREE(data);

    //vTaskDelete(NULL);
}

static void new_node_connected_server(uint8_t *new_node_addr){
    mdf_err_t ret = MDF_OK;
    mwifi_data_type_t data_type = {0x0};

    
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        MDF_LOGE("Failed to create JSON object");
        return;
    }

    
    char formatted_string[128];
    snprintf(formatted_string, sizeof(formatted_string), MACSTR, MAC2STR(new_node_addr));
    cJSON_AddStringToObject(json, "node_id", formatted_string);
    cJSON_AddStringToObject(json, "command", "CREATE_FIREMAN");
    tcp_client_write_task(json);

    
    char *json_str = cJSON_PrintUnformatted(json);
    if (!json_str) {
        MDF_LOGE("Failed to print JSON object");
        cJSON_Delete(json);
        return;
    }

    
    MDF_LOGD("TCP write, data: %s", json_str);
    //tcp_client_write_task(json_str);

    
    cJSON_Delete(json);
    MDF_FREE(json_str);
}

void new_node_connected(uint8_t *new_node_addr){
    MDF_LOGW("IN NEW_NODE_CONNECTED");
    mdf_err_t ret = MDF_OK;
    mdf_err_t ret_get_macs = MDF_OK;
    mdf_err_t ret_root_mac = MDF_OK;
    size_t size   = 0;
    size_t size_get_macs   = 0;
    size_t size_root_addr = 0;
    mwifi_data_type_t data_type = {0x0};
    mwifi_data_type_t data_type_get_macs = {0x0};
    mwifi_data_type_t data_type_root_mac = {0x0};
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "command", "new_node_connected");

    
    char mac_str[18] = {0};
    snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(new_node_addr));
    cJSON_AddStringToObject(json, "mac", mac_str);

    char *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    size = strlen(data);

    MDF_LOGW("IN NEW_NODE_CONNECTED, BEFORE IF-STATEMENT");
    if(esp_mesh_is_root()){
        MDF_LOGW("IN NEW_NODE_CONNECTED, IN IF-STATEMENT");
        for(int i = 1; i < routing_table_size; i++){
            MDF_LOGW("IN NEW_NODE_CONNECTED, IN FOR-LOOP, BEFORE IF-STATEMENT");
            if(memcmp(routing_table[i], new_node_addr, MWIFI_ADDR_LEN) != 0 && memcmp(routing_table[i], root_addr, MWIFI_ADDR_LEN) != 0){
                MDF_LOGW("IN NEW_NODE_CONNECTED, IN FOR-LOOP, IN IF-STATEMENT");
                ret = mwifi_root_write(routing_table[i], 1, &data_type, data, size, true);

                cJSON *json_get_macs = cJSON_CreateObject();
                cJSON_AddStringToObject(json_get_macs, "command", "new_node_connected");
                char get_macs_str[18] = {0};
                snprintf(get_macs_str, sizeof(get_macs_str), MACSTR, MAC2STR(routing_table[i]));
                cJSON_AddStringToObject(json_get_macs, "mac", get_macs_str);
                char *get_macs_data = cJSON_PrintUnformatted(json_get_macs);
                cJSON_Delete(json_get_macs);
                size_get_macs = strlen(get_macs_data);
                ret_get_macs = mwifi_root_write(new_node_addr, 1, &data_type_get_macs, get_macs_data, size_get_macs, true);
                

                if (ret != MDF_OK) {
                    MDF_LOGE("Failed to broadcast new node connected message, ret: %s", mdf_err_to_name(ret));
                } else {
                    MDF_LOGW("Sent message: %s, to " MACSTR, data, MAC2STR(routing_table[i]));
                }

                if (ret_get_macs != MDF_OK) {
                    MDF_LOGE("Failed to broadcast new node connected message, ret: %s", mdf_err_to_name(ret));
                } else {
                    MDF_LOGW("Sent message: %s, to " MACSTR, data, MAC2STR(routing_table[i]));
                }
            }
        }

        cJSON *json_root_addr = cJSON_CreateObject();
        cJSON_AddStringToObject(json_root_addr, "command", "root_address");
        char root_addr_str[18] = {0};
        snprintf(root_addr_str, sizeof(root_addr_str), MACSTR, MAC2STR(self_addr));
        cJSON_AddStringToObject(json_root_addr, "root_mac", root_addr_str);
        char *root_addr_data = cJSON_PrintUnformatted(json_root_addr);
        cJSON_Delete(json_root_addr);
        size_root_addr = strlen(root_addr_data);
        ret_root_mac = mwifi_root_write(new_node_addr, 1, &data_type_root_mac, root_addr_data, size_root_addr, true);

    } else {
        ret = mwifi_write(parent_addr,&data_type, data, size, true);
        if (ret == MDF_OK){
            MDF_LOGW("Sent message to parent node: %s", data);
        }else{
            MDF_LOGE("Failed to broadcast new node connected message, ret: %s", mdf_err_to_name(ret));
        }
    }
    
    MDF_FREE(data);
}


void tcp_client_read_task(void *arg)
{
    mdf_err_t ret                     = MDF_OK;
    char *data                        = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size                       = MWIFI_PAYLOAD_LEN;
    cJSON *json_root                  = NULL;

    MDF_LOGI("TCP client read task is running");

    while (true) { 
        if (!mwifi_is_connected()) {
            MDF_LOGW("Mesh is disconnected, waiting...");
            vTaskDelay(1000 / portTICK_RATE_MS); 
            continue;
        }

        if (g_sockfd == -1) {
            g_sockfd = socket_tcp_client_create(CONFIG_SERVER_IP, CONFIG_SERVER_PORT);
            new_node_connected_server(self_addr);
            if (g_sockfd == -1) {
                MDF_LOGW("Failed to connect to server, retrying...");
                vTaskDelay(2000 / portTICK_RATE_MS); 
                continue;
            }
        }

        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = read(g_sockfd, data, size - 1); 
        MDF_LOGD("TCP read, %d, size: %d, data: %s", g_sockfd, size, data);

        if (ret <= 0) {
            MDF_LOGW("<%s> TCP read failed, closing socket", strerror(errno));
            close(g_sockfd);
            g_sockfd = -1;
            continue; 
        }

        data[ret] = '\0'; 
        json_root = cJSON_Parse(data);
        if (!json_root) {
            MDF_LOGW("Failed to parse JSON: %s", data);
            continue;
        }

        
        cJSON *command = cJSON_GetObjectItem(json_root, "command");
        if (strcmp(command->valuestring, "Tick") == 0) {
            MDF_LOGI("Server confirmation: %s", command->valuestring);
            broadcastTick(NULL);
            set_tick();
        }else if (strcmp(command->valuestring, "update_pos") == 0){
            cJSON *mac = cJSON_GetObjectItem(json_root, "fireman");
            uint8_t dest_mac[MWIFI_ADDR_LEN] = {0};
            char mac_str[18] = {0};
            strncpy(mac_str, mac->valuestring, sizeof(mac_str) - 1);
            mac_str[18] = '\0';

            unsigned int mac_tmp[MWIFI_ADDR_LEN] = {0};
            sscanf(mac_str, MACSTR, &mac_tmp[0], &mac_tmp[1], &mac_tmp[2],
                &mac_tmp[3], &mac_tmp[4], &mac_tmp[5]);

            
            for (int i = 0; i < MWIFI_ADDR_LEN; i++) {
                dest_mac[i] = (uint8_t)mac_tmp[i];
            }

            cJSON *xCor = cJSON_GetObjectItem(json_root, "xCor");
            cJSON *yCor = cJSON_GetObjectItem(json_root, "yCor");
            int x = xCor->valueint;
            int y = yCor->valueint;

            if(memcmp(root_addr, dest_mac, MWIFI_ADDR_LEN) == 0){
                MDF_LOGW("I if-sats för att sätta egen position");
                set_fireman_pos(x, y);
            }else{
                MDF_LOGW("I else-sats för att skicka annans position");
                size_t size_pos   = 0;
                mwifi_data_type_t data_type_pos = {0x0};
                mdf_err_t ret_pos                     = MDF_OK;
                cJSON *update_pos = cJSON_CreateObject();
                cJSON_AddStringToObject(update_pos, "command", "update_pos");
                cJSON_AddNumberToObject(update_pos, "xCor", x);
                cJSON_AddNumberToObject(update_pos, "yCor", y);
                char *data_pos = cJSON_PrintUnformatted(update_pos);
                cJSON_Delete(update_pos);

                size_pos = strlen(data_pos);

                ret_pos = mwifi_root_write(dest_mac, 1, &data_type_pos, data_pos, size_pos, true);
                if(ret_pos != MDF_OK){
                    MDF_LOGW("Positionsmeddelande skickades ej");
                }else{
                    MDF_LOGW("Positionsmeddelande skickades!!");
                }
                MDF_FREE(data_pos);
            }
        } else {
            MDF_LOGW("Unexpected JSON format: %s", data);
        }

        
        cJSON_Delete(json_root);
    }

    
    MDF_LOGI("TCP client read task is exiting");
    close(g_sockfd);
    g_sockfd = -1;
    MDF_FREE(data);
    vTaskDelete(NULL);
}



static bool check_if_contains_addr(uint8_t *new_node_addr){
    for(int i = 0; i < routing_table_size; i++){
        if(memcmp(routing_table[i], new_node_addr, MWIFI_ADDR_LEN) == 0){
            return true;
        }
    }

    return false;
}


static void node_write_task(int target_index){
    mdf_err_t ret = MDF_OK;
    mdf_err_t ret_log = MDF_OK;
    size_t size   = 0;
    size_t size_log =0;
    mwifi_data_type_t data_type = {0x0};
    mwifi_data_type_t data_type_log = {0x0};

    MDF_LOGI("Node write task is running");


    //size = sprintf(data, "toggle_led");

    cJSON *json = cJSON_CreateObject();
    cJSON *json_log = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "command", "toggle_led");
    cJSON_AddStringToObject(json_log, "command", "log_message");

    char formatted_string[128];
    snprintf(formatted_string, sizeof(formatted_string), "Command 'toggle_led' sent from: " MACSTR " to: " MACSTR, MAC2STR(self_addr), MAC2STR(routing_table[target_index]));
    cJSON_AddStringToObject(json_log, "message_to_log", formatted_string);
    char *data_log = cJSON_PrintUnformatted(json_log);
    cJSON_Delete(json_log);
    size_log = strlen(data_log);

    

    char *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    size = strlen(data);

    ret = mwifi_write(routing_table[target_index], &data_type, data, size, true);
    if(ret == MDF_OK){
        MDF_LOGW("Meddelande skickat till: " MACSTR, MAC2STR(routing_table[target_index]));

        if(memcmp(routing_table[target_index], root_addr, MWIFI_ADDR_LEN) != 0){
            ret_log = mwifi_write(root_addr, &data_type_log, data_log, size_log, true);

            if(ret_log == MDF_OK){
                MDF_LOGW("Meddelandet HAR skickats till root-noden för att loggas");
            }else{
                MDF_LOGW("Meddelandet har INTE skickats till root-noden för att loggas");
            }
        }
    }

    if(ret != MDF_OK){
        MDF_LOGI("mwifi_write, ret: %x", ret);
    }

    vTaskDelay(1000 / portTICK_RATE_MS);
    

    MDF_LOGW("Node write task is exit");

    MDF_FREE(data);
    MDF_FREE(data_log);
    //vTaskDelete(NULL);
}



static void new_node_connected_root(){
    vTaskDelay(1000/portTICK_RATE_MS);
    new_node_connected(self_addr);
}

static void node_read_task(void *arg){
    mdf_err_t ret = MDF_OK;
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    MDF_LOGI("Node read task is running");

    for (;;) {
        if (!mwifi_is_connected()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        cJSON *json = cJSON_Parse(data);

        if (!json) {
            MDF_LOGW("Failed to parse JSON data: %s", data);
            return;
        }

        cJSON *command = cJSON_GetObjectItem(json, "command");



        if(strcmp(command->valuestring, "toggle_led") == 0){
            toggle_message_led(NULL);
            if(esp_mesh_is_root()){
                MDF_LOGW("I node_read_task, innan tcp_write");
                tcp_client_write_task(json);
            }
        }else if(strcmp(command->valuestring, "new_node_connected") == 0){

            uint8_t new_mac[MWIFI_ADDR_LEN] = {0};

            cJSON *mac = cJSON_GetObjectItem(json, "mac");

            unsigned int mac_tmp[MWIFI_ADDR_LEN] = {0};
            sscanf(mac->valuestring, MACSTR, &mac_tmp[0], &mac_tmp[1], &mac_tmp[2],
                &mac_tmp[3], &mac_tmp[4], &mac_tmp[5]);

            
            for (int i = 0; i < MWIFI_ADDR_LEN; i++) {
                new_mac[i] = (uint8_t)mac_tmp[i];
            }

            MDF_LOGW("New node connected: MAC = " MACSTR, MAC2STR(new_mac));
            int index = routing_table_size;

            bool contains = check_if_contains_addr(new_mac);

            if (index >= MAX_NODES) {
                MDF_LOGW("Routing table is full");
            } else if(contains){
                MDF_LOGW("Routing table already contains MAC: " MACSTR, MAC2STR(new_mac)); 
            } else {
                memcpy(routing_table[index], new_mac, MWIFI_ADDR_LEN);
                routing_table_size++;
            }

            if(esp_mesh_is_root()){
                new_node_connected(new_mac);
                new_node_connected_server(new_mac);
            }

        } else if(strcmp(command->valuestring, "root_address") == 0){
            uint8_t root_mac[MWIFI_ADDR_LEN] = {0};

            cJSON *mac = cJSON_GetObjectItem(json, "root_mac");

            unsigned int mac_tmp[MWIFI_ADDR_LEN] = {0};
            sscanf(mac->valuestring, MACSTR, &mac_tmp[0], &mac_tmp[1], &mac_tmp[2],
                &mac_tmp[3], &mac_tmp[4], &mac_tmp[5]);

        
            for (int i = 0; i < MWIFI_ADDR_LEN; i++) {
                root_mac[i] = (uint8_t)mac_tmp[i];
            }

            memcpy(root_addr, root_mac, MWIFI_ADDR_LEN);

            MDF_LOGW("Stored root address: " MACSTR, MAC2STR(root_mac));
        }else if(strcmp(command->valuestring, "log_message") == 0){
            cJSON *log_message = cJSON_GetObjectItem(json, "log_message");
            tcp_client_write_task(json);
        }else if(strcmp(command->valuestring, "Tick_root") == 0){
            MDF_LOGI("Tic from root: %s", command->valuestring);
            set_tick();
        }else if (strcmp(command->valuestring, "update_pos") == 0){
            cJSON *xCor = cJSON_GetObjectItem(json, "xCor");
            cJSON *yCor = cJSON_GetObjectItem(json, "yCor");
            int x = xCor->valueint;
            int y = yCor->valueint;
            set_fireman_pos(x, y);
        }

        cJSON_Delete(json);

        MDF_ERROR_CONTINUE(ret != MDF_OK, "mwifi_read, ret: %x", ret);
        MDF_LOGI("Node receive, addr: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
    }

    MDF_LOGW("Node read task is exit");

    MDF_FREE(data);
    vTaskDelete(NULL);
}

static void root_write_task(int target_index)
{
    mdf_err_t ret = MDF_OK;
    mdf_err_t ret_log = MDF_OK;
    size_t size   = 0;
    size_t size_log =0;
    mwifi_data_type_t data_type = {0x0};
    mwifi_data_type_t data_type_log = {0x0};

    MDF_LOGI("Root write task is running");

    //size = sprintf(data, "toggle_led");
    //dest = routing_table[target_index];

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "command", "toggle_led");
    cJSON *json_log = cJSON_CreateObject();
    cJSON_AddStringToObject(json_log, "command", "log_message");

    char formatted_string[128];
    snprintf(formatted_string, sizeof(formatted_string), "Command 'toggle_led' sent from: " MACSTR " to: " MACSTR, MAC2STR(self_addr), MAC2STR(routing_table[target_index]));
    cJSON_AddStringToObject(json_log, "message_to_log", formatted_string);
    tcp_client_write_task(json_log);
    cJSON_Delete(json_log);

    char *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    size = strlen(data);

    ret = mwifi_root_write(routing_table[target_index], 1, &data_type, data, size, true);
    if(ret != MDF_OK){
        MDF_LOGI("mwifi_root_write, ret: %x", ret);
    }

    vTaskDelay(1000 / portTICK_RATE_MS);
    

    MDF_LOGW("Root write task is exit");

    MDF_FREE(data);
    //vTaskDelete(NULL);
}

static void root_read_task(void *arg){
    mdf_err_t ret = MDF_OK;
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    MDF_LOGI("Root read task is running");

    for (;;) {
        /*
        if (!mwifi_is_connected()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }
        */
        MDF_LOGW("KOMMER JAG IN I FOR-LOOPEN IN ROOT_READ_TASK?");
        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);

        MDF_LOGW("Mottagit meddelande från: " MACSTR, MAC2STR(src_addr));
        cJSON *json = cJSON_Parse(data);

        if (!json) {
            MDF_LOGW("Failed to parse JSON data: %s", data);
            return;
        }else {
            MDF_LOGW("Data parsed: %s", data);
        }

        cJSON *command = cJSON_GetObjectItem(json, "command");

        if(strcmp(command->valuestring, "toggle_led") == 0){
            toggle_message_led(NULL);
            MDF_LOGW("Efter toggle_led, innan tcp_write");
            tcp_client_write_task(json);
        }else if(strcmp(command->valuestring, "new_node_connected") == 0){
            uint8_t new_mac[MWIFI_ADDR_LEN] = {0};
            cJSON *mac = cJSON_GetObjectItem(json, "mac");

            unsigned int mac_tmp[MWIFI_ADDR_LEN] = {0};
            sscanf(mac->valuestring, MACSTR, &mac_tmp[0], &mac_tmp[1], &mac_tmp[2],
                &mac_tmp[3], &mac_tmp[4], &mac_tmp[5]);

            
            for (int i = 0; i < MWIFI_ADDR_LEN; i++) {
                new_mac[i] = (uint8_t)mac_tmp[i];
            }

            MDF_LOGI("New node connected: MAC = " MACSTR, MAC2STR(new_mac));
            int index = routing_table_size;
            
            if (index >= MAX_NODES) {
                MDF_LOGW("Routing table is full, cannot add new MAC address");
            } else {
                //memcpy(routing_table[index], new_mac, MWIFI_ADDR_LEN);
                //routing_table_size++;
                new_node_connected(new_mac);
            }
        }

        cJSON_Delete(json);

        MDF_ERROR_CONTINUE(ret != MDF_OK, "mwifi_read, ret: %x", ret);
        MDF_LOGI("Node receive, addr: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
    }

    MDF_LOGW("Root read task is exit");

    MDF_FREE(data);
    vTaskDelete(NULL);
}


void button_task(void *arg){
    MDF_LOGI("In button_task");

    while(1){
        int target_index = -1;
        if (gpio_get_level(BUTTON1_GPIO) == 0) {
            target_index = 0;
            MDF_LOGW("In button 1, target_index: %d", target_index);
        } else if (gpio_get_level(BUTTON2_GPIO) == 0) {
            target_index = 1;
            MDF_LOGW("In button 2, target_index: %d", target_index);
        } else if (gpio_get_level(BUTTON3_GPIO) == 0) {
            target_index = 2;
            MDF_LOGW("In button 3, target_index: %d", target_index);
        }

        if(target_index > -1){
            if(esp_mesh_is_root()){
                root_write_task(target_index);
            }else{
                node_write_task(target_index);
            }
        }

        vTaskDelay(1000 / portTICK_RATE_MS);

    }
    MDF_LOGW("button_task exit");
    vTaskDelete(NULL);
}

/**
 * @brief Timed printing system information
 */
static void print_system_info_timercb(void *timer)
{
    uint8_t primary                 = 0;
    wifi_second_chan_t second       = 0;
    mesh_addr_t parent_bssid        = {0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    wifi_sta_list_t wifi_sta_list   = {0x0};

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    esp_wifi_get_channel(&primary, &second);
    esp_mesh_get_parent_bssid(&parent_bssid);

    MDF_LOGI("System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
             ", parent rssi: %d, node num: %d, free heap: %u", primary,
             esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
             mwifi_get_parent_rssi(), esp_mesh_get_total_node_num(), esp_get_free_heap_size());

    for (int i = 0; i < wifi_sta_list.num; i++) {
        MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
    }
    
    for(int i = 0; i < routing_table_size; i++){
        MDF_LOGI("MAC: " MACSTR, MAC2STR(routing_table[i]));  
    }

    MDF_LOGI("Root-node MAC: " MACSTR, MAC2STR(root_addr));

#ifdef MEMORY_DEBUG

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    mdf_mem_print_heap();
    mdf_mem_print_record();
    mdf_mem_print_task();
#endif /**< MEMORY_DEBUG */
}

static mdf_err_t wifi_init()
{
    mdf_err_t ret          = nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    MDF_ERROR_ASSERT(ret);

    MDF_ERROR_ASSERT(esp_netif_init());
    MDF_ERROR_ASSERT(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, NULL));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());

    return MDF_OK;
}

/**
 * @brief All module events will be sent to this task in esp-mdf
 *
 * @Note:
 *     1. Do not block or lengthy operations in the callback function.
 *     2. Do not consume a lot of memory in the callback function.
 *        The task memory of the callback function is only 4KB.
 */
static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: %d", event);

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");

            if (esp_mesh_is_root()) {

                MDF_LOGI("Startar DHCP-klienten för root-noden.");
                MDF_ERROR_ASSERT(esp_netif_dhcpc_start(netif_sta));

                
            }


            if (esp_mesh_is_root()) {
                MDF_LOGI("Detta är root-noden, tänder lysdioden");
                gpio_set_level(LED_ROOT_GPIO, 1); 

                

                esp_wifi_get_mac(ESP_IF_WIFI_STA, self_addr);
                esp_wifi_get_mac(ESP_IF_WIFI_STA, root_addr);
                
                
            } else {
                MDF_LOGI("Detta är en vanlig nod, släcker root-lysdioden");
                gpio_set_level(LED_ROOT_GPIO, 0);  
                
                mesh_addr_t parent_bssid = {0};
                esp_mesh_get_parent_bssid(&parent_bssid);
                memcpy(routing_table[routing_table_size], parent_bssid.addr, MWIFI_ADDR_LEN);
                memcpy(parent_addr, parent_bssid.addr, MWIFI_ADDR_LEN);
                routing_table_size++;
                MDF_LOGI("Parent MAC added to routing table: " MACSTR, MAC2STR(parent_bssid.addr));

                uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
                esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
                memcpy(routing_table[routing_table_size], sta_mac, MWIFI_ADDR_LEN);
                routing_table_size++;
                new_node_connected(sta_mac);
                MDF_LOGI("Added own MAC to routing table: " MACSTR, MAC2STR(sta_mac));
            }

            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");
            
            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            MDF_LOGI("total_num: %d", esp_mesh_get_total_node_num());

            break;

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {

            MDF_LOGW("Root obtains the IP address. It is posted by LwIP stack automatically");
            
            MDF_LOGI("Detta är root-noden, tänder lysdioden");
            gpio_set_level(LED_ROOT_GPIO, 1);  

            uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
            esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
            if(!check_if_contains_addr(sta_mac)){
                memcpy(routing_table[routing_table_size], sta_mac, MWIFI_ADDR_LEN);
                routing_table_size++;
            }
            
            MDF_LOGI("Added own MAC to routing table: " MACSTR, MAC2STR(sta_mac));

            /*
            xTaskCreate(tcp_client_write_task, "tcp_client_write_task", 4 * 1024,
                        NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
            */
            xTaskCreate(tcp_client_read_task, "tcp_server_read", 4 * 1024,
                        NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
            break;
        }

        default:
            break;
    }

    return MDF_OK;
}

void comSetUp()
{
    esp_mesh_set_self_organized(true, true);

    mwifi_init_config_t cfg   = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config     = {
        .router_ssid     = CONFIG_ROUTER_SSID,
        .router_password = CONFIG_ROUTER_PASSWORD,
        .channel = 0,
        .mesh_id         = CONFIG_MESH_ID,
        .mesh_password   = CONFIG_MESH_PASSWORD,
        .mesh_type = MESH_IDLE,
    };



    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief Initialize wifi mesh.
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&config));
    MDF_ERROR_ASSERT(mwifi_start());


    gpio_init();


    if (esp_mesh_is_root()) {
        MDF_LOGI("Detta är root-noden, tänder lysdioden");
        gpio_set_level(LED_ROOT_GPIO, 1); 

        uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
        esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
        memcpy(routing_table[routing_table_size], sta_mac, MWIFI_ADDR_LEN);
        routing_table_size++;

        MDF_LOGW("Lägger till egen mac på index: %d. MAC: " MACSTR, routing_table_size-1, MAC2STR(sta_mac));

        
    }

    esp_wifi_get_mac(ESP_IF_WIFI_STA, self_addr);



    if(esp_mesh_is_root()){
        xTaskCreate(root_read_task, "root_read_task", 4 * 1024, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);

    }else {
        xTaskCreate(node_read_task, "node_read_task", 4 * 1024,
                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    }
    
    xTaskCreate(button_task, "button_task", 4 * 1024, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);

    TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS,
                                       true, NULL, print_system_info_timercb);
    xTimerStart(timer, 0);
}

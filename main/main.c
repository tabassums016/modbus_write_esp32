/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
   WIFI=SSID:"Aktarali",PASS:"9819256348",DHCP:"1"
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 1024;
static const char *TAG = "MODBUS";

#define TXD_PIN (GPIO_NUM_13)
#define RXD_PIN (GPIO_NUM_12)

// char query[1][17]={{ 0x01, 0x10, 0x00, 0x00, 0X00, 0X04, 0x08, 0x46, 0x0F, 0xFF, 0x5C, 0x00, 0x00, 0x23, 0xFF}}; // write multiple register
//
char query[]; //= {0x01, 0x10, 0x00, 0x00, 0X00, 0X04, 0x08, 0x46, 0x0F, 0xFF, 0x5C, 0x00, 0x00, 0x23, 0xFF};

// char query[]={ 0x01, 0x06, 0x00, 0x01, 0X23, 0XFF}; // write single register
// char query[1][8]={{ 0x01, 0x06, 0x00, 0x01, 0X23, 0XFF}}; // write single register
//  char query[1][11]={{ 0x01, 0x0F, 0x00, 0x00, 0X00, 0X0A, 0X02, 0XFF, 0X03}};  //write multiple coil

void write_modbus_cases(int func_code, int datatype, int register_count, char write_values[]);
char *Add_CRC(char query[], int len);
void sendData(char query[], int len);

void init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void datatype_parser(uint16_t* write_values,uint8_t *modbus_received, int register_count) //++ Function to Parse Response according to Data Types
{
    uint8_t datatype_int=12;
    // int index;
    // int data_field = 0;
    // int query_ix=0;
    // uint16_t write_data [100];
    // char slave_data[50];
    // while (data_field != strlen(write_values))
    // {
    //     index = 0;
    //     memset(slave_data,0,sizeof(slave_data));
    //     while (write_values[data_field] != ',') //++ Parsing of Slave ID
    //     {
    //         slave_data[index] = write_values[data_field];
    //         data_field++;
    //         index++;
    //     }
    //     write_data[query_ix]=atoi(slave_data);
    //     query_ix++;
    //     data_field++;
    // }

    switch (datatype_int)
    {
    
   

    case 12:
    {
        // int bytecount=register_count;
        // INT AB
        int index = 7;
        uint16_t mbdata; 
        int test;
        for (int i = 0; i < register_count; i++)
        {
            mbdata = write_values[i];
            printf("\nquery arr value %d",mbdata);
            printf("\nquery arr 0 %d",*((char *)&mbdata + 0) & 0xFF);
            printf("\nquery arr 1 %d",*((char *)&mbdata + 1) & 0xFF);
            test=*((char *)&mbdata + 0) & 0xFF ;
            query[index+1]=test;
            printf("\nquery data 0: %d",test);
            test=*((char *)&mbdata + 1) & 0xFF ;
            query[index]=test;
            printf("\nquery data 1:%d",test);
            printf("\nquery data %d= %d %d",i, query[index],query[index+1]);

            // query[index+1]=*((char *)&mbdata + 0) & 0xFF ;
            // query[index]=*((char *)&mbdata + 1) & 0xFF ;

            index=index+2;
            
        }
        break;
    }

    case 13:
    {
        // INT BA
        int16_t slave_response[register_count];
        int index = 3;
        int16_t mbdata = 0;

        for (int i = 0; i < register_count; i++)
        {
            *((char *)&mbdata + 0) = modbus_received[index];
            *((char *)&mbdata + 1) = modbus_received[index + 1];

            index = index + 2;
            slave_response[i] = mbdata;
        }

        break;
    }

    case 14:
    {
        // UINT AB
        uint16_t slave_response[register_count];
        int index = 3;
        uint16_t mbdata = 0;

        for (int i = 0; i < register_count; i++)
        {
            *((char *)&mbdata + 0) = modbus_received[index + 1];
            *((char *)&mbdata + 1) = modbus_received[index];

            index = index + 2;
            slave_response[i] = mbdata;
        }
        break;
    }

    case 15:
    {
        // UINT BA
        uint16_t slave_response[register_count];
        int index = 3;
        uint16_t mbdata = 0;

        for (int i = 0; i < register_count; i++)
        {
            *((char *)&mbdata + 0) = modbus_received[index];
            *((char *)&mbdata + 1) = modbus_received[index + 1];

            index = index + 2;
            slave_response[i] = mbdata;
        }
        break;
    }

    default:
    {
        // sprintf(mqtt_string, "{\"data\":[\"ERROR\"]}");
        break;
    }
    }
}
void query_parser()
{

    char query_str[] = "{1,16,104,4,8,12,266,77,99,8}";

    int slave_int = 0;        //++ Variable to store Slave Id as Integer
    int function_int = 0;     //++ Variable to store Function Code as Integer
    int addressH_int = 0;     //++ Variable to store MSB Address as Integer
    int addressL_int = 0;     //++ Variable to store LSB Address as Integer
    int registerH_int = 0;    //++ Variable to store MSB Register as Integer
    int registerL_int = 1;    //++ Variable to store LSB Register as Integer
    uint8_t datatype_int = 0; //++ Variable to store data_field as Integer
    int query_param_ix = 0;   //++ Variable to store query parameter insex
    int bytecount_int = 2;
    int index = 0; //++ Indices to navigate the string
    int data_field = 1;

    char slave_data[5]; //++ Arrays to stored parsed data from the entire string
    char function_data[5];
    char address_data[10];

    char register_data[10];
    char byte_count[10];
    char data_mode[5];
    char q_count[5];
    char write_data[100];

    memset(slave_data, 0, sizeof(slave_data)); //++ Clears every array
    memset(function_data, 0, sizeof(function_data));
    memset(address_data, 0, sizeof(address_data));

    memset(register_data, 0, sizeof(register_data));
    memset(byte_count, 0, sizeof(byte_count));
    memset(data_mode, 0, sizeof(data_mode));
    memset(q_count, 0, sizeof(q_count));
    memset(write_data, 0, sizeof(write_data));

    while (query_str[data_field] != ',') //++ Parsing of Slave ID
    {
        slave_data[index] = query_str[data_field];
        data_field++;
        index++;
    }
    slave_int = atoi(slave_data);
    query[query_param_ix] = slave_int;
    query_param_ix++;

    data_field++;
    index = 0;
    while (query_str[data_field] != ',') //++ Parsing of Function Code
    {
        function_data[index] = query_str[data_field];
        data_field++;
        index++;
    }
    function_int = atoi(function_data);
    query[query_param_ix] = function_int;
    query_param_ix++;

    data_field++;
    index = 0;
    while (query_str[data_field] != ',') //++ Parsing of MSD Address
    {
        address_data[index] = query_str[data_field];
        data_field++;
        index++;
    }
    addressH_int = atoi(address_data) / 256;
    addressL_int = atoi(address_data) % 256;
    query[query_param_ix] = addressH_int;
    query_param_ix++;
    query[query_param_ix] = addressL_int;
    query_param_ix++;

    if (function_int == 15 || function_int == 16) // No of register required only in  write multiple coil (15) and multiple register (16)
    {
        data_field++;
        index = 0;
        while (query_str[data_field] != ',') //++ Parsing of MSB Register
        {
            register_data[index] = query_str[data_field];
            data_field++;
            index++;
        }
        registerH_int = atoi(register_data) / 256;
        registerL_int = atoi(register_data) % 256;
        query[query_param_ix] = registerH_int;
        query_param_ix++;
        query[query_param_ix] = registerL_int;
        query_param_ix++;

        data_field++;
        index = 0;
        while (query_str[data_field] != ',') //++ Parsing of MSB Register
        {
            byte_count[index] = query_str[data_field];
            data_field++;
            index++;
        }
        bytecount_int = atoi(byte_count);
        query[query_param_ix] = bytecount_int;
        query_param_ix++;
    }

    data_field++;
    index = 0;
    while (query_str[data_field] != ',') //++ Parsing datatype
    {
        data_mode[index] = query_str[data_field];
        data_field++;
        index++;
    }
    datatype_int = atoi(data_mode);

    data_field++;
    index = 0;

    while (query_str[data_field] != '}') //++ Parsing of data to write
    {
        write_data[index] = query_str[data_field];
        data_field++;
        index++;
    }
    query_param_ix++;
    ESP_LOGI(TAG, "WRITE DATA = %s\n", write_data);
    for (int i = 0; i < 7; i++)
    {
        // send_arr[0][i] = query[i];
        printf("%02X  ", query[i]);
    }
    write_modbus_cases(function_int, datatype_int,registerL_int, write_data);
    printf("\n After data addition:\n");
    for (int i = 0; i < 15; i++)
    {
        // send_arr[0][i] = query[i];
        printf("%02X  ", query[i]);
    }
    // querycount_int = atoi(q_count);
    printf("reg data in nvs= %d\n", atoi(address_data));
    printf("reg address= %x %x\n", addressH_int, addressL_int);
    char *final_query = malloc(50);
    // char send_arr[1][len];
    final_query = Add_CRC(query, 15);
        // len=len+2;
        sendData(final_query, 17);
}

void write_modbus_cases(int func_code, int datatype, int register_count, char write_values[])
{
    printf("\n Length of write values =%d",strlen(write_values));
    printf("\nFunc code: %d",func_code);
    switch (func_code)
    {
    case 5: // single coil
    {
        int query_ix = 4;
        if (atoi(write_values) == 1)
        {
            query[query_ix] = 0xFF;
            query[query_ix + 1] = 0x00;
        }
        else if (atoi(write_values) == 0)
        {
            query[query_ix] = 0x00;
            query[query_ix + 1] = 0x00;
        }
    }
    break;

    // case 15: // multiple coil
    // {
    // }
    // break;

    case 6: // single register
    {
        uint16_t write_data [100];
        write_data[0]=atoi(write_values);
        
        datatype_parser(write_data,NULL,register_count);
    }
    break;

    case 16: // multiple register
    {
        int index;
    int data_field = 0;
    int query_ix=0;
    // uint16_t write_data [100];
    uint16_t* write_data=malloc(100);
    char slave_data[50];
    ESP_LOGI("PARSER","Length of write values =%d",strlen(write_values));
    while (data_field <= strlen(write_values))
    {
        index = 0;
        memset(slave_data,0,sizeof(slave_data));
        while (write_values[data_field] != ','&& data_field != strlen(write_values)) 
        {
            slave_data[index] = write_values[data_field];
            data_field++;
            //  ESP_LOGI("PARSER","data_field:%d",data_field);
            index++;
        }
        write_data[query_ix]=atoi(slave_data);
        ESP_LOGI("PARSER","data %d:%d",query_ix,write_data[query_ix]);
        query_ix++;
        data_field++;
    }
     datatype_parser(write_data,NULL,register_count);
    

    }
    break;

    default:
        break;
    }
}

char *Add_CRC(char query[], int len) //++ Funtion to Add CRC Check for every query
{

    uint16_t crc = 0xFFFF;
    // int len=sizeof(query);
    printf("\naddcrc len=%d", len);
    char *query_crc = malloc(50);
    for (int pos = 0; pos < len; pos++)
    {
        crc ^= (uint16_t)query[pos]; // XOR byte into least sig. byte of crc

        for (int i = 8; i != 0; i--)
        { // Loop over each bit
            if ((crc & 0x0001) != 0)
            {              // If the LSB is set
                crc >>= 1; // Shift right and XOR 0xA001
                crc ^= 0xA001;
            }
            else           // Else LSB is not set
                crc >>= 1; // Just shift right
        }
    }
    // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
    query[len] = *((char *)&crc + 0);     //&crc+0; //*((char*)&crc+0);
    query[len + 1] = *((char *)&crc + 1); //&crc+1; //*((char*)&crc+1);
                                          // printf("\nbefore crc return=%s",query[5]);
    for (int i = 0; i < len + 2; i++)
    {
        query_crc[i] = query[i];
    }
    return query_crc;
}

void sendData(char query[], int len) //++ Function to send Modbus Queries
{

    // const int len = sizeof(query); //++ Sends Queries one by one
    // char query_send[1][len];
    // memset(query_send, 0, strlen(query_send));
    printf("\n\nQyery sent:");
    for (int i = 0; i < len; i++)
    {
        // query_send[0][i] = query[i];
        printf(" %X", query[i]);
    }
    const int txBytes = uart_write_bytes(UART_NUM_1, query, len);
    ESP_LOGI("SEND","\nSending query %d bytes", txBytes);
    // printf("\nquery_rcv=%d", query_send[0][5]);
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    int len = 0;
    char *final_query = malloc(50);
    char send_arr[1][len];
    // query_parser();
    // len = sizeof(query);

    // for (int i = 0; i < 6; i++)
    // {
    //     // send_arr[0][i] = query[i];
    //     printf("%02X  ", query[i]);
    // }
    while (1)
    {

        // final_query = Add_CRC(send_arr[0], len);
        // // len=len+2;
        // sendData(final_query, len + 2);
        query_parser();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    while (1)
    {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0)
        {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

void app_main(void)
{
    init();
//    query_parser();
     xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
     xTaskCreate(tx_task, "uart_tx_task", 2048 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
}

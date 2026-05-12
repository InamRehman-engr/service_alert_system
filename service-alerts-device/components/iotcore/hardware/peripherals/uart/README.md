# UART

## Usage

Example

```C
#include <stdio.h>
#include "uart_driver.h"

int bytes_written = 0;
int bytes_read = 0;

const int baud_rate = 115200;
const int uart_data_bits = UART_DATA_8_BITS;
const int uart_parity = UART_PARITY_DISABLE;
const int uart_stop_bits = UART_STOP_BITS_1;
const int hw_flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
const int uart_clk = UART_SCLK_DEFAULT;
const int uart_port = UART_NUM_2;
const int tx = 17;
const int rx = 16;
const int cts = -1;
const int rts = -1;
const int recv_buffer = 1024;
const int send_buffer = 1024;

#define TAG "UART-Example"

void app_main(void)
{
    printf("\nStart\n");
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = uart_data_bits,
        .parity = uart_parity,
        .stop_bits = uart_stop_bits,
        .flow_ctrl = hw_flow_ctrl,
        .source_clk = uart_clk,
    };

    uart_pin_config_t pinconfig;
    pinconfig.TXD = tx;
    pinconfig.RXD = rx;
    pinconfig.CTS = cts;
    pinconfig.RTS = rts;


    uart_peripheral uart_2;
    uart_init(&uart_2, uart_port, &uart_config, pinconfig, send_buffer, recv_buffer);

    uint8_t send_data = 'A';
    uint8_t recv_data;
    ESP_LOGI(TAG, "Sending Data: %c", send_data);

    uart_2.uart_send_single_byte(uart_port, &send_data);
    uart_2.uart_receive_single_byte(uart_port, &recv_data, 20);
    
    ESP_LOGI(TAG, "Received Data: %c", recv_data);

    uart_2.uart_deinit(uart_port);
}
```

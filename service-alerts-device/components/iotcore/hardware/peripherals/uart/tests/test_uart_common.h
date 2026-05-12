#include "uart_driver.h"

// UART Configs
const int baud_rate = 115200;
const int uart_data_bits = UART_DATA_8_BITS;
const int uart_parity = UART_PARITY_DISABLE;
const int uart_stop_bits = UART_STOP_BITS_1;
const int hw_flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
const int uart_clk = UART_SCLK_DEFAULT;

// UART Pins
const int tx = 17;
const int rx = 16;
const int cts = -1;
const int rts = -1;

const int uart_port = UART_NUM_2;
const int recv_buffer = 1024;
const int send_buffer = 1024;

uart_config_t uart_config = {
    .baud_rate = baud_rate,
    .data_bits = uart_data_bits,
    .parity = uart_parity,
    .stop_bits = uart_stop_bits,
    .flow_ctrl = hw_flow_ctrl,
    .source_clk = uart_clk,
};

uart_pin_config_t pinconfig = {
    .TXD = tx,
    .RXD = rx,
    .CTS = cts,
    .RTS = rts,
};

uart_peripheral uart_func; // Struct containing uart functions

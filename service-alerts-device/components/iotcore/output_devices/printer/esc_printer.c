#include "esc_printer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <string.h>
#define TAG "ESC_PRINTER"

// Global variables for UART
static uart_port_t g_uart_num;
static int g_tx_pin;
static int g_rx_pin;
static int g_baud_rate;

// UART initialization
void escPrinter_initialize(uart_port_t uart_num, int tx_pin, int rx_pin,
                           int baud_rate) {
  g_uart_num = uart_num;
  g_tx_pin = tx_pin;
  g_rx_pin = rx_pin;
  g_baud_rate = baud_rate;

  const uart_config_t uart_config = {
      .baud_rate = g_baud_rate,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };
  uart_driver_install(g_uart_num, 1024 * 2, 0, 0, NULL, 0);
  uart_param_config(g_uart_num, &uart_config);
  uart_set_pin(g_uart_num, g_tx_pin, g_rx_pin, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);
}

void escPrinter_printerInit(void) {
  char initData[] = {ESC, 0x40}; // ESC @ (Initialize Printer)
  escPrinter_sendData(TAG, initData, sizeof(initData));
}

void escPrinter_feed(int data) {
  while (data--)
    escPrinter_sendData(TAG, (char[]){LF}, 1); // Line Feed
}

int escPrinter_sendData(const char *logName, const char *data, int len) {
  int txBytes = uart_write_bytes(g_uart_num, data, len);
  ESP_LOGI(logName, "Sent %d bytes", len);
  return txBytes;
}

// Alignment methods
void escPrinter_justify(void) {
  // Align center by default
  char command[] = {ESC, 0x61, ALIGN_CENTER}; // ESC a (alignment: center)
  escPrinter_sendData(TAG, command, sizeof(command));
}

void escPrinter_alignLeft(void) {
  char command[] = {ESC, 0x61, ALIGN_LEFT}; // ESC a (alignment: left)
  escPrinter_sendData(TAG, command, sizeof(command));
}

void escPrinter_alignRight(void) {
  char command[] = {ESC, 0x61, ALIGN_RIGHT}; // ESC a (alignment: right)
  escPrinter_sendData(TAG, command, sizeof(command));
}

// Print styles
void escPrinter_printLine(const char *data) {
  escPrinter_sendData(TAG, data, strlen(data));
  escPrinter_sendData(TAG, (char[]){LF}, 1); // Line Feed
}

void escPrinter_printBlock(const char *data, int lines) {
  escPrinter_sendData(TAG, data, strlen(data));
  char command[] = {ESC, 0x64, (char)lines}; // ESC d (feed n lines)
  escPrinter_sendData(TAG, command, sizeof(command));
}

void escPrinter_printHeading(const char *data) {
  char command[] = {GS, 0x21, STYLE_HEADING}; // GS ! (bold/heading)
  escPrinter_sendData(TAG, command, sizeof(command));
  escPrinter_printLine(data);
  command[2] = STYLE_NORMAL; // Reset to normal style
  escPrinter_sendData(TAG, command, sizeof(command));
}

void escPrinter_printUnderline(const char *data) {
  char command[] = {ESC, 0x2D, STYLE_UNDERLINE}; // ESC - (underline)
  escPrinter_sendData(TAG, command, sizeof(command));
  escPrinter_printLine(data);
  command[2] = STYLE_NORMAL; // Reset to normal style
  escPrinter_sendData(TAG, command, sizeof(command));
}

void escPrinter_printBold(const char *data) {
  char command[] = {ESC, 0x45, STYLE_BOLD}; // ESC E (bold)
  escPrinter_sendData(TAG, command, sizeof(command));
  escPrinter_printLine(data);
  command[2] = STYLE_NORMAL; // Reset to normal style
  escPrinter_sendData(TAG, command, sizeof(command));
}

void escPrinter_printBarcode(barcode_mode_t mode, const char *data) {
  escPrinter_sendData(TAG, (char[]){LF}, 1); // Line Feed
  char barcodeCommand[] = {GS, 0x6B, (char)mode,
                           (char)strlen(data)}; // GS k (barcode)
  escPrinter_sendData(TAG, barcodeCommand, sizeof(barcodeCommand));
  escPrinter_sendData(TAG, data, strlen(data));
  escPrinter_sendData(TAG, (char[]){LF}, 1); // Line Feed
}

void escPrinter_setFont(font_t font) {
  char command[] = {ESC, 0x4D, (char)font}; // ESC M (select font)
  escPrinter_sendData(TAG, command, sizeof(command));
}

void escPrinter_setSize(int height, int width) {
  height = height % 8;
  width = width % 8;
  char comm = (width % 16);
  comm = comm | ((height % 16) << 4);
  char command[] = {GS, 0x21, comm}; // GS ! (set character size)
  escPrinter_sendData(TAG, command, sizeof(command));
}

void escPrinter_cutFull(void) {
  escPrinter_sendData(TAG, (char[]){LF}, 1); // Line Feed
  char cutCommand[] = {ESC, CUT};            // ESC m (cut paper)
  escPrinter_sendData(TAG, cutCommand, sizeof(cutCommand));
  escPrinter_sendData(TAG, (char[]){LF}, 1); // Line Feed
}

void escPrinter_checkPaperStatus() {
  uint8_t response[1];
  char command[] = {GS, 0x72, 0x01};                  // GS r n
  escPrinter_sendData(TAG, command, sizeof(command)); // Line Feed
  int status = uart_read_bytes(g_uart_num, response, sizeof(response),
                               pdMS_TO_TICKS(7000)); // Block for 7 seconds
  if (status == 0)
    ESP_LOGE(TAG, "Error, printer did not respond to check status command");
  else {
    response[0] = response[0] & 0x0F;
    if (!(response[0] & 0x00))
      ESP_LOGI(TAG, "Paper Roll Present With Abundance");
    if (response[0] & 0x03)
      ESP_LOGI(TAG, "Paper Roll Near End");
    if (response[0] & 0x0C)
      ESP_LOGI(TAG, "Paper Roll Not Present");
  }
}
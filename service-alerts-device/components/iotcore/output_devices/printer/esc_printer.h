#ifndef ESC_PRINTER_H
#define ESC_PRINTER_H

#include "driver/uart.h"
// Enum for alignment
typedef enum {
  ALIGN_LEFT = 0x00,
  ALIGN_CENTER = 0x01,
  ALIGN_RIGHT = 0x02
} alignment_t;

// Enum for print styles
typedef enum {
  STYLE_NORMAL = 0x00,
  STYLE_BOLD = 0x01,
  STYLE_UNDERLINE = 0x01,
  STYLE_HEADING = 0x04
} print_style_t;

// Enum for barcode types
typedef enum { BARCODE_CODE39 = 0x45, BARCODE_ITF = 0x46 } barcode_mode_t;

// Enum for font type
typedef enum { FONT_A = 0x00, FONT_B = 0x01 } font_t;

// Enum for ESC/POS command sequences
typedef enum {
  ESC = 0x1B, // ESC
  GS = 0x1D,  // GS
  DLE = 0x10, // DLE
  STX = 0x02, // STX
  LF = 0x0A,  // LF (Line Feed)
  CR = 0x0D,  // CR (Carriage Return)
  CUT = 0x6D  // Cut Paper
} esc_pos_command_t;

// Declare the function prototypes
void escPrinter_initialize(uart_port_t uart_num, int tx_pin, int rx_pin,
                           int baud_rate);
void escPrinter_printerInit(void);
void escPrinter_feed(int data);
int escPrinter_sendData(const char *logName, const char *data, int len);
void escPrinter_justify(void);
void escPrinter_alignLeft(void);
void escPrinter_alignRight(void);
void escPrinter_printLine(const char *data);
void escPrinter_printBlock(const char *data, int lines);
void escPrinter_printHeading(const char *data);
void escPrinter_printUnderline(const char *data);
void escPrinter_printBold(const char *data);
void escPrinter_printBarcode(barcode_mode_t mode, const char *data);
void escPrinter_setFont(font_t data);
void escPrinter_setSize(int height, int width);
void escPrinter_cutFull(void);
void escPrinter_checkPaperStatus(void);
#endif // ESC_PRINTER_H

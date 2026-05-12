# **ESCPrinter Library for ESP-IDF**

## **Overview**

The `ESCPrinter` library provides an  implementation for controlling ESC/POS-compatible thermal printers using ESP-IDF.

---

## **Features**

- Print plain text with alignment options (left, right, center).
- Print bold or underlined text.
- Print barcode.
- Paper cutting/
- Easily extendable for other printer models via inheritance.


### **Methods**
#### Initialization:
```cpp
initialize(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate)
printerInit()
```

Call after creating an object
### Check Status Methods
```cpp
escPrinter_checkStatus()
```
### Alignment/ Layout Methods

#### Justify:
```cpp
escPrinter_justify()
```

Justifies all lines printed after this function call

#### alignLeft:
```cpp
escPrinter_alignLeft()
```
Left aligns all lines printed after this function call

#### alignRight:
```cpp
escPrinter_alignRight()
```
Right aligns all lines printed after this function call

#### Feed:
```cpp
escPrinter_feed(int data)
```
Feeds n lines of paper

#### Set Size:
```cpp
escPrinter_setSize(int height, int width)
```
Sets the size of printed characters as defined by height and width **(0-7)** where **0** corresponds to original size

#### Set Font:
```cpp
escPrinter_setFont(font_t data)
```
Selects font A or B depending on the argument. 
FONT_A: font A
FONT_B: font B

### Printing Methods

#### Print Line:
```cpp
escPrinter_printLine(const char* data)
```
Prints a line of data

#### Print Line Bold:
```cpp
escPrinter_printBold(const char* data)
```
Prints a bold line of data

#### Print underlined line:
```cpp
escPrinter_printUnderline(const char* data)
```
Prints an underlined line of data

#### Print barcode:
```cpp
escPrinter_printBarcode(barcode_t mode, const char* data)
```
Prints a barcode from the string specified in the format specified
BARCODE_CODE39: Code 39 barcode
BARCODE_ITF : ITF format barcode

#### Cut:
```cpp
escPrinter_cutFull()
```
Cuts the paper

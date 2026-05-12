#ifndef PPSI262_H
#define PPSI262_H

// #About the Library

// I2C Operations
// WRITING Operation
//<SLAVE ADDR>|R/W --> ACK <REG-8bits> -->ACK<DATA1> ....//

// n =3 bytes

// READING OPERATION
//<SLAVE ADDR>|R/W --> ACK <REG-8bits> --> <SLAVE ADDR>|R/W --->READOUT TIME
// WINDOW //

// n =3 bytes  three bytes are used for the read and write operation

/* Note:, after the host sends the slave address
during a write operation, the PPSI262 pulls the I2C_DAT line low (shown as ACK)
-if the slave address matches 5Ah(when SEN is high) or 5Bh (when SEN is low).

Similarly, the host pulls the I2C_DAT line high (shown as NACK) as an
acknowledgment of a successfully completed read operation involving three bytes
of data.
*/

// #I2C specific defines

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_SCL_IO 25        /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 26        /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_0    /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000   /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_TIMEOUT_MS 1000
#define I2C_MASTER_READ 1
#define I2C_MASTER_WRITE 0
k
#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0
#define ACK_VAL 0x0
#define NACK_VAL 0x1
#define PPSI262_I2C_ADDR 0x38
#define READ_WRITE_BYTE_NUMBER                                                 \
  3 /* Value of n Three bytes are sent and recieved according to the           \
       documentation */
#define I2C_SLAVEADDR_SENPIN_HIGH 0x5A
    // #define I2C_SLAVEADDR_SENPIN_LOW            0x5B /*Enable This option
    // once relevant to design */ and comment the line above it
    // @TODO add this in Menuconfig   for SENPIN
    // PPSI262 has 22 bit representation from the photodiodes
    // 24 bit data from the ADC can be read in 2's complement form
    // Meaning to say that 3 bytes as mentioned earlier
    // -------------------------------------------------

    // Variables :
    int *heartbeatrate;
// Program Flow to obtain values from the sensor
//  1-
//  2-
//  3-

//@ Register Addresses for Accessing data
//@TODO
//@ End of Register Address Book
esp_err_t writeCommandBytes(const i2c_port_t i2c_num, const uint8_t reg,
                            const uint8_t *i2c_command, const size_t nbytes);

esp_err_t writeCommandBytes(const i2c_port_t i2c_num, const uint8_t reg,
                            const uint8_t *i2c_command, const size_t nbytes);

// @TODO build a 2's complement Decoder
static uint32_t twos_complement_adc_value(uint32_t reada_complement_dcvalue);

#endif
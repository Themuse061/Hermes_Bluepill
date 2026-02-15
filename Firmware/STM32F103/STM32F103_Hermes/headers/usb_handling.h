#include <stdint.h>

void hermes_USB_initialization();
void USB_poll_update();

/** Write a packet
 * @param buf pointer to user data to write
 * @param len # of bytes
 * @return 0 if failed, len if successful
 */
uint16_t USB_send_data(void *buf, uint16_t len);

/** Read a packet
 * @param buf user buffer that will receive data
 * @param len # of bytes
 * @return Actual # of bytes read
 */
uint16_t hermes_USB_recieve_data(void *buf, uint16_t len);

// weak void USB_recieve_interrupt() exists
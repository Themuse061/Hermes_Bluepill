#include <stdlib.h>
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/cm3/nvic.h>
#include <debug_leds.h>
#define MAX_TRANSFER_SIZE 1024
static uint8_t big_rx_buffer[MAX_TRANSFER_SIZE];
static uint32_t big_rx_idx = 0;

#define USE_INTERRUPT 1 // 1 or 0

// Global variables
volatile bool tx_just_sent = 0;
volatile bool rx_just_recieved = 0;

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

usbd_device *usbd_dev;

uint16_t hermes_USB_recieve_data(void *buf, uint16_t len)
{

	return usbd_ep_read_packet(usbd_dev, 0x01, buf, len);
}

uint16_t USB_send_data(void *buf, uint16_t len)
{
	debug_led_usb_busy(1);
	uint8_t *buf_ptr = (uint8_t *)buf;
	int data_to_be_sent = len;
	int sent_data = 0;

	while (data_to_be_sent)
	{

		if (data_to_be_sent > 64)
		{

			// send the data
			tx_just_sent = 0;
			sent_data += usbd_ep_write_packet(usbd_dev, 0x82, buf_ptr, 64);
			while (tx_just_sent == 0)
			{
				// wait for usb to send the data
			}

			// update Pointers
			buf_ptr += 64;
			data_to_be_sent -= 64;
		}
		else
		{

			// send the data
			tx_just_sent = 0;
			sent_data += usbd_ep_write_packet(usbd_dev, 0x82, buf_ptr, data_to_be_sent);
			while (tx_just_sent == 0)
			{
				// wait for usb to send the data
			}

			// Update pointers
			buf_ptr += data_to_be_sent;
			data_to_be_sent = 0;
		}
	}

	// check if we need to write zero length packet
	// Only trigger if we hit the exact boundary of the USB packet size
	if (len > 0 && (len % 64 == 0))
	{
		tx_just_sent = 0;
		// Send the "End of Message" signal
		usbd_ep_write_packet(usbd_dev, 0x82, NULL, 0);
		while (tx_just_sent == 0)
		{
			// wait for usb to send the data
		}
	}
	debug_led_usb_busy(0);
	return sent_data;
}

/** Called when USB writes to the device
 */
// weak recieve IT for echoing the data
void __attribute__((weak)) USB_recieve_interrupt(uint8_t *recieve_buffer, int len)
{

	USB_send_data(recieve_buffer, len);
}

void __attribute__((weak)) USB_transmit_interrupt()
{
}

/* --- USB DESCRIPTORS BEGIN --- */
static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5740,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x83,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
															   .bLength = USB_DT_ENDPOINT_SIZE,
															   .bDescriptorType = USB_DT_ENDPOINT,
															   .bEndpointAddress = 0x01,
															   .bmAttributes = USB_ENDPOINT_ATTR_BULK,
															   .wMaxPacketSize = 64,
															   .bInterval = 1,
														   },
														   {
															   .bLength = USB_DT_ENDPOINT_SIZE,
															   .bDescriptorType = USB_DT_ENDPOINT,
															   .bEndpointAddress = 0x82,
															   .bmAttributes = USB_ENDPOINT_ATTR_BULK,
															   .wMaxPacketSize = 64,
															   .bInterval = 1,
														   }};

static const struct
{
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength = sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	},
};

static const struct usb_interface_descriptor comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 0,
	.endpoint = comm_endp,
	.extra = &cdcacm_functional_descriptors,
	.extralen = sizeof(cdcacm_functional_descriptors),
}};

static const struct usb_interface_descriptor data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,
	.endpoint = data_endp,
}};

static const struct usb_interface ifaces[] = {{
												  .num_altsetting = 1,
												  .altsetting = comm_iface,
											  },
											  {
												  .num_altsetting = 1,
												  .altsetting = data_iface,
											  }};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 2,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,
	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Black Sphere TechnologieC",
	"CDC-ACM DemC",
	"DEMS",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

/* --- USB DESCRIPTORS END --- */

/* --- HELPER FUNCTIONS --- */

// Global structure to catch the data Windows sends (prevents stack corruption)
static struct usb_cdc_line_coding cdc_line_coding;

static enum usbd_request_return_codes cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
															 uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)usbd_dev;

	switch (req->bRequest)
	{
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		// Windows sends this to toggle DTR/RTS.
		// We just say "Okay" and do nothing.
		// The original code had complex logic here that often fails on Clones.
		return USBD_REQ_HANDLED;

	case USB_CDC_REQ_SET_LINE_CODING:
		// Windows sends 7 bytes of data (Baud rate, etc.)
		if (*len < sizeof(struct usb_cdc_line_coding))
			return USBD_REQ_NOTSUPP;

		// We accept the data, but we don't actually use it.
		// Point the buffer to our global struct so the stack has somewhere to put the data.
		*buf = (uint8_t *)&cdc_line_coding;
		return USBD_REQ_HANDLED;

	case USB_CDC_REQ_GET_LINE_CODING:
		// Windows asks "What is your baud rate?"
		// We send back whatever it sent us last time, or defaults.
		*buf = (uint8_t *)&cdc_line_coding;
		*len = sizeof(struct usb_cdc_line_coding);
		return USBD_REQ_HANDLED;
	}
	return USBD_REQ_NOTSUPP;
}

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
	(void)ep;
	uint8_t temp_pkt[64];

	// 1. Grab the current packet from hardware
	int len = usbd_ep_read_packet(usbd_dev, 0x01, temp_pkt, 64);

	if (len > 0)
	{
		// 2. Copy into the accumulator
		if (big_rx_idx + len <= MAX_TRANSFER_SIZE)
		{
			for (int i = 0; i < len; i++)
			{
				big_rx_buffer[big_rx_idx++] = temp_pkt[i];
			}
		}
	}

	// 3. Is this the end of the message? (Short packet logic)
	if (len < 64)
	{
		// --- THIS IS THE MOMENT MAIN IS STOPPED ---

		// Call your processing function with the full buffer
		USB_recieve_interrupt(big_rx_buffer, big_rx_idx);

		// Reset for the next message
		big_rx_idx = 0;

		// --- AFTER THIS, CONTROL RETURNS TO MAIN ---
	}
}

static void cdcacm_data_tx_cb(usbd_device *usbd_dev, uint8_t ep)
{
	(void)ep;
	(void)usbd_dev;

	tx_just_sent = 1;
	USB_transmit_interrupt();
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;
	(void)usbd_dev;

	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_tx_cb);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(
		usbd_dev,
		USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
		USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		cdcacm_control_request);
}

void hermes_USB_initialization()
{

	int i;
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
	rcc_periph_clock_enable(RCC_GPIOA);

	// 2. LED Setup (Bluepill usually uses PC13)
	// Note: If your LED is PC11, change this back to GPIO11
	gpio_set(GPIOC, GPIO13);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
				  GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

	// 3. USB RE-ENUMERATION / SOFT DISCONNECT (Crucial for Bluepill)
	// We pull PA12 (USB D+) low for a short time to fool the PC
	// into thinking we unplugged the USB cable.
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
				  GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
	gpio_clear(GPIOA, GPIO12);

	// Hold low for a bit (~800,000 cycles is roughly a few milliseconds)
	for (i = 0; i < 800000; i++)
	{
		__asm__("nop");
	}

	// 4. Initialize USB
	// PA11/PA12 are automatically taken over by the USB peripheral here
	usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config,
						 usb_strings, 3,
						 usbd_control_buffer, sizeof(usbd_control_buffer));

	usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);

	if (USE_INTERRUPT)
	{
		nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
	}
}

void USB_poll_update()
{
	usbd_poll(usbd_dev);
}

void usb_lp_can_rx0_isr(void)
{
	usbd_poll(usbd_dev);
}
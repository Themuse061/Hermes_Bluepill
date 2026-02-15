
#include "Usb_driver.h"
#include <libserialport.h>
#include <stdio.h>
#include <stdlib.h>

// Internal handle to the serial port
static struct sp_port *port = NULL;

// Internal handle to event set for waiting
static struct sp_event_set *event_set = NULL;

int hermes_USB_init(const char *port_name, int baud_rate)
{
    if (port != NULL)
    {
        // Already initialized
        return -1;
    }

    enum sp_return ret;

    ret = sp_get_port_by_name(port_name, &port);
    if (ret != SP_OK)
        return ret;

    ret = sp_open(port, SP_MODE_READ_WRITE);
    if (ret != SP_OK)
    {
        sp_free_port(port);
        port = NULL;
        return ret;
    }

    sp_set_baudrate(port, baud_rate);
    sp_set_bits(port, 8);
    sp_set_parity(port, SP_PARITY_NONE);
    sp_set_stopbits(port, 1);
    // sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);

    return 0;
}

void hermes_USB_deinit(void)
{
    if (event_set != NULL)
    {
        sp_free_event_set(event_set);
        event_set = NULL;
    }

    if (port != NULL)
    {
        sp_close(port);
        sp_free_port(port);
        port = NULL;
    }
}

int hermes_USB_send(const unsigned char *data, int length)
{
    if (port == NULL)
        return -1;
    // Timeout 0 means wait indefinitely
    return sp_blocking_write(port, data, length, 0);
}

int hermes_USB_recieve(unsigned char *buffer, int length, unsigned int timeout_ms)
{
    if (port == NULL)
        return -1;
    return sp_blocking_read(port, buffer, length, timeout_ms);
}

int hermes_USB_check_recieve_buffer(void)
{
    if (port == NULL)
        return -1;
    return sp_input_waiting(port);
}

int hermes_USB_wait_for_recieve(unsigned int max_delay_ms)
{
    if (port == NULL)
        return -1;

    // Allocate event set if not already done
    // We only need to do this once unless the port changes
    if (event_set == NULL)
    {
        if (sp_new_event_set(&event_set) != SP_OK)
        {
            return -1;
        }
        if (sp_add_port_events(event_set, port, SP_EVENT_RX_READY) != SP_OK)
        {
            sp_free_event_set(event_set);
            event_set = NULL;
            return -1;
        }
    }

    // Wait for event
    return sp_wait(event_set, max_delay_ms);
}
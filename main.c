/**
 * Copyright (c) 2022 FIF orientering.
 * 
 * Serial buffer
 * Buffers serial data on UART0, to facilitate interfacing a transmitter without handshake to a slow receiver
 * All characters are relayed as  received.
 * The CTS input is respected stopping the Tx until released 
 */

#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "fifo.h"

/// \tag::SerialBuffer[]

#define UART_ID uart0
#define BAUD_RATE 38400
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

static int chars_rxed, chars_txed = 0;
static char outMsg[1024];
#define QUEUE_SIZE 10*1024

uint8_t rx_ch, tx_ch;
const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  

int main() {

    printf("Started Serial Buffer\n");
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    // Allocate the queue buffer
    queue_t queue = {0, 0, QUEUE_SIZE, malloc(sizeof(uint8_t*) * QUEUE_SIZE)};

    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 2400);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    gpio_set_function(18, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS only (the buffer is always ready to receive)
    uart_set_hw_flow(UART_ID, true, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // OK, all set up.
    uart_puts(UART_ID, "\nSerialBuffer started\n");
    while (1) {
        // Polled Rx
        if (uart_is_readable(UART_ID)) {
            rx_ch = uart_getc(UART_ID);
            // Push into FIFO buffer
            queue_write(&queue, (void*)rx_ch);
            chars_rxed++;
            gpio_put(LED_PIN, 1);
        }
        // // Polled Tx
        if (uart_is_writable(UART_ID) && (queue.head != queue.tail)) {
            uint8_t tx_ch = (uint8_t)queue_read(&queue);
            uart_putc(UART_ID, tx_ch);
            chars_txed++;
            gpio_put(LED_PIN, 0);
        }
    }
}

/// \end:SerialBuffer[]

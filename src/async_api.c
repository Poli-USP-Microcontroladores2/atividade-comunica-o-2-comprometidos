/*
 * Polling-based TX/RX cycle for FRDM board
 *
 * Transmit "Cassoli carregado" por 5 segundos
 * depois recebe dados por 5 segundos, repetindo o ciclo.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
#define MSG_SIZE 32

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* Recebe caracteres e imprime no console (polling RX) */
void poll_receive(uint32_t duration_ms)
{
    uint8_t c;
    int64_t start = k_uptime_get();
    while (k_uptime_get() - start < duration_ms) {
        if (uart_poll_in(uart_dev, &c) == 0) {
            if (c != '\r') {  // ignora carriage return
                uart_poll_out(uart_dev, c); // ecoa de volta
            }
        }
    }
}

/* Transmite mensagem repetidamente por duration_ms */
void poll_transmit(uint32_t duration_ms)
{
    const char *msg = "Cassoli carregado\r\n";
    int64_t start = k_uptime_get();
    size_t msg_len = strlen(msg);

    while (k_uptime_get() - start < duration_ms) {
        for (size_t i = 0; i < msg_len; i++) {
            uart_poll_out(uart_dev, msg[i]);
        }
        k_msleep(200); // pequeno delay entre mensagens
    }
}

void main(void)
{
    if (!device_is_ready(uart_dev)) {
        printk("UART device not found!\n");
        return;
    }

    while (1) {
        printk("=== Ciclo TX: 5 segundos transmitindo ===\n");
        poll_transmit(5000);

        printk("=== Ciclo RX: 5 segundos recebendo ===\n");
        poll_receive(5000);
    }
}
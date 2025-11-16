/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 * (Modificado por Gemini para ignorar mensagens no modo TX)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <string.h>

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

#define MSG_SIZE 32

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 * (Sem modificações aqui)
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	while (uart_fifo_read(uart_dev, &c, 1) == 1) {

		if (c == '\n' || c == '\r') {
			if (rx_buf_pos > 0) {
				rx_buf[rx_buf_pos] = '\0';
				k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
				rx_buf_pos = 0;
			}
		
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
	}
}

/*
 * Print a null-terminated string character by character to the UART interface
 * (Sem modificações aqui)
 */
void print_uart(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

/*
 * Função main MODIFICADA para limpar a fila no início do ciclo RX
 */
int main(void)
{
	char tx_buf[MSG_SIZE];

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return 0;
	}

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return 0;
	}
	
	/* Habilita a interrupção de RX (apenas uma vez) */
	uart_irq_rx_enable(uart_dev);
	
	/* Loop principal cíclico RX/TX */
	while (1) {
		
		/* --- ESTADO 1: MODO RX (5 segundos) --- */
		
		/* **NOVA LINHA:** Limpa (purga) a fila de qualquer mensagem
		 * que foi recebida durante o modo TX anterior. */
		k_msgq_purge(&uart_msgq);
		
		print_uart("Modo RX: Acumulando mensagens por 5s...\r\n");
		
		// A ISR 'serial_cb' está sempre rodando em segundo plano.
		// Dormimos para deixar a fila (agora limpa) acumular.
		k_sleep(K_SECONDS(5));

		
		/* --- ESTADO 2: MODO TX (5 segundos) --- */
		print_uart("Modo TX: Esvaziando fila... (Próximo RX em 5s)\r\n");

		// Esvazia e imprime tudo que foi coletado no Modo RX
		while (k_msgq_get(&uart_msgq, &tx_buf, K_NO_WAIT) == 0) {
			print_uart("Eco: ");
			print_uart(tx_buf);
			print_uart("\r\n");
		}

		// Pausa por 5s. A ISR continua recebendo dados
		// e enchendo a fila (esses dados serão "lixo"
		// e limpos pelo 'purge' no próximo ciclo).
		k_sleep(K_SECONDS(5));

		print_uart("--- Reiniciando ciclo ---\r\n");
	}
	
	return 0; // Inacessível
}
# PSI-Microcontroladores2-Aula10
Atividade: Comunicação UART

# Projeto UART – Atividade em Duplas (Echo Bot + Async API)

## 1. Informações Gerais

* Dupla:

  * Integrante 1: Filipe Cassoli
  * Integrante 2: Henrique Santiago

* Objetivo: implementar, testar e documentar aplicações de comunicação UART baseadas nos exemplos oficiais “echo_bot” e “async_api”, utilizando desenvolvimento orientado a testes, diagramas de sequência D2 e registro de evidências.

---

# 2. Estrutura Esperada do Repositório

```
README.md
src/

docs/
  evidence/
  sequence-diagrams/

```

---

# 3. Etapa 1 – Echo Bot (UART Polling/Interrupt)

---

## 3.1 Descrição do Funcionamento

Descrever aqui de forma textual o comportamento esperado baseado no exemplo oficial.
Link usado como referência:
[https://docs.zephyrproject.org/latest/samples/drivers/uart/echo_bot/README.html](https://docs.zephyrproject.org/latest/samples/drivers/uart/echo_bot/README.html)

---

## **Descrição do Comportamento Esperado – UART Echo Bot**

O *UART Echo Bot* é um exemplo simples que demonstra o uso do driver UART para comunicação serial. O programa atua como um “bot” que recebe dados digitados pelo usuário via console UART e devolve exatamente o mesmo conteúdo após o usuário pressionar a tecla *Enter*.


## 🧭 **Visão Geral do Comportamento**

O programa inicializa a UART padrão do Zephyr (geralmente a mesma usada pelo console/shell) e passa a funcionar como um **bot de eco via serial**.
Ele aguarda o usuário digitar uma linha de texto (finalizada com *Enter*), e então envia de volta a mesma linha, precedida da palavra **“Echo:”**.

Durante o funcionamento:

* A **recepção** dos caracteres ocorre **de forma assíncrona**, via **interrupções**.
* O **envio** da resposta é feito **por polling** (síncrono), caractere a caractere.
* O programa fica rodando indefinidamente, repetindo o ciclo de leitura → eco → espera por nova entrada.


## ⚙️ **Fluxo de Execução Esperado**

### **1️⃣ Inicialização**

1. O código obtém o *device handle* da UART configurada como `zephyr_shell_uart` no *Device Tree*.
2. Ele verifica se o dispositivo está pronto com `device_is_ready()`.

   * Se não estiver, exibe a mensagem de erro:

     ```
     UART device not found!
     ```
3. Configura a UART para operação **interrompida**, registrando a função `serial_cb` como *callback* para tratar os dados recebidos.
4. Habilita a recepção por interrupção (`uart_irq_rx_enable()`).
5. Envia duas mensagens de boas-vindas pela UART:

   ```
   Hello! I'm your echo bot.
   Tell me something and press enter:
   ```


### **2️⃣ Recepção de dados (Interrupção via `serial_cb`)**

A função `serial_cb()` é chamada automaticamente sempre que a UART recebe dados.

Comportamento detalhado:

* Lê cada caractere recebido via `uart_fifo_read()`.
* Armazena os caracteres no buffer `rx_buf[]`.
* Quando detecta um *fim de linha* (`\n` ou `\r`), considera que a mensagem terminou:

  * Adiciona um terminador nulo (`\0`) ao final da string.
  * Copia a linha completa para a **fila de mensagens (`k_msgq`)**.
  * Zera o índice do buffer (`rx_buf_pos = 0`) para começar a próxima linha.
* Se o buffer encher antes do *Enter*, os caracteres excedentes são descartados.
* Se a fila estiver cheia (10 mensagens pendentes), novas mensagens são descartadas silenciosamente.


### **3️⃣ Fila de mensagens (`k_msgq`)**

A `k_msgq` é uma fila do Zephyr usada para comunicação entre a *interrupt callback* e a *thread principal* (`main()`).

* Capacidade: **10 mensagens**
* Tamanho de cada mensagem: **32 bytes**
* Alinhamento: **4 bytes**

Ela permite que a função principal espere por mensagens novas **sem bloquear o recebimento de interrupções**.


### **4️⃣ Loop principal (`main`)**

A função `main()` entra em um loop infinito:

```c
while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
    print_uart("Echo: ");
    print_uart(tx_buf);
    print_uart("\r\n");
}
```

Comportamento esperado:

1. O código aguarda indefinidamente (`K_FOREVER`) por uma nova linha de texto na fila (`uart_msgq`).
2. Quando uma linha chega:

   * Escreve `"Echo: "`
   * Escreve a linha recebida (`tx_buf`)
   * Finaliza com quebra de linha `\r\n`
3. Repete o ciclo para a próxima entrada.


### **5️⃣ Envio de dados (`print_uart`)**

A função `print_uart()` envia cada caractere da string informada usando `uart_poll_out()` — um método **bloqueante**, mas simples.

Ela é usada:

* Para exibir as mensagens de boas-vindas
* Para enviar o eco de volta ao usuário


## 💬 **Exemplo de Interação Esperada (via terminal serial)**

```
Hello! I'm your echo bot.
Tell me something and press enter:
Type e.g. "Hi there!" and hit enter!
```

Usuário digita:

```
Hi there!
```

Bot responde:

```
Echo: Hi there!
```

Usuário digita outra linha:

```
Zephyr is cool
```

Bot responde:

```
Echo: Zephyr is cool
```

O ciclo continua indefinidamente.


## ⚠️ **Tratamento de Casos Especiais**

| Situação                            | Comportamento esperado                        |
| ----------------------------------- | --------------------------------------------- |
| Linha muito longa (> 31 caracteres) | Caracteres excedentes são descartados         |
| Linha vazia (apenas *Enter*)        | Gera eco: `Echo:`                             |
| Fila cheia (10 mensagens pendentes) | Mensagens novas são ignoradas                 |
| UART não pronta                     | Mensagem de erro no console e fim da execução |
| Erro ao configurar interrupção      | Exibe mensagem explicativa e encerra          |


## 🧩 **Resumo funcional**

| Função         | Papel                                                         |
| -------------- | ------------------------------------------------------------- |
| `serial_cb()`  | ISR da UART: lê caracteres e envia mensagens completas à fila |
| `print_uart()` | Envia texto para o terminal, caractere a caractere            |
| `main()`       | Inicializa UART, exibe mensagens e ecoa entrada recebida      |

---

## **3.2 Casos de Teste Planejados (TDD)** – *UART Echo Bot (Zephyr)*


### **CT1 – Eco básico**

| Item                       | Descrição                                                                                                                                                                                      |
| -------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Usuário digita `Hello` e pressiona *Enter* (`\r` ou `\n`).                                                                                                                                     |
| **Saída esperada:**        | `Echo: Hello`                                                                                                                                                                                  |
| **Critério de Aceitação:** | O texto deve ser ecoado exatamente como digitado, com o prefixo “Echo: ” e apenas após o *Enter* ser recebido (fim da linha detectado). O sistema deve permanecer pronto para próxima entrada. |


### **CT2 – Linha vazia**

| Item                       | Descrição                                                                                                                                                           |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Usuário pressiona *Enter* sem digitar nenhum caractere.                                                                                                             |
| **Saída esperada:**        | `Echo:` *(linha vazia após o prefixo)*                                                                                                                              |
| **Critério de Aceitação:** | O sistema não deve travar nem gerar erro. Deve ecoar uma linha vazia, demonstrando que o *callback* e a fila (`k_msgq`) tratam corretamente mensagens sem conteúdo. |


### **CT3 – Linha longa (acima de 31 caracteres)**

| Item                       | Descrição                                                                                                                                      |
| -------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Usuário digita uma linha com mais de 31 caracteres e pressiona *Enter*.                                                                        |
| **Saída esperada:**        | Apenas os primeiros 31 caracteres são ecoados (restante truncado). Exemplo: `Echo: <primeiros 31 caracteres>`                                  |
| **Critério de Aceitação:** | O sistema deve descartar caracteres excedentes sem travar, conforme lógica `rx_buf_pos < sizeof(rx_buf)-1`. Nenhum erro ou reset deve ocorrer. |


### **CT4 – Caracteres especiais**

| Item                       | Descrição                                                                                                             |
| -------------------------- | --------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Usuário digita: `!@#$%&*()_+-=[]{};:'",.<>/?\|` e pressiona *Enter*.                                                  |
| **Saída esperada:**        | `Echo: !@#$%&*()_+-=[]{};:'",.<>/?\|`                                                                                 |
| **Critério de Aceitação:** | Todos os caracteres devem ser transmitidos e recebidos sem alteração. Nenhum símbolo deve ser perdido ou substituído. |


### **CT5 – Caracteres não ASCII (UTF-8)**

| Item                       | Descrição                                                                                                                                                                |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Entrada:**               | Usuário digita `Olá, você está bem? äöüñç` e pressiona *Enter*.                                                                                                          |
| **Saída esperada:**        | `Echo: Olá, você está bem? äöüñç` *(ou comportamento definido caso UART não suporte UTF-8)*                                                                              |
| **Critério de Aceitação:** | Se o hardware/UART suportar UTF-8, os caracteres devem ser ecoados corretamente. Caso contrário, caracteres multibyte podem ser omitidos, mas o sistema não deve travar. |


### **CT6 – Múltiplas linhas seguidas**

| Item                       | Descrição                                                                                                                             |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Usuário envia diversas linhas consecutivas: `A` + Enter, `B` + Enter, `C` + Enter, etc.                                               |
| **Saída esperada:**        | Cada linha é ecoada individualmente, ex.: `Echo: A`, `Echo: B`, `Echo: C`...                                                          |
| **Critério de Aceitação:** | O sistema deve processar todas as mensagens na ordem correta, sem perder ou misturar linhas. A fila `k_msgq` deve manter a sequência. |


### **CT7 – Alta taxa de entrada de caracteres**

| Item                       | Descrição                                                                                                                                                                                                                                   |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Script envia várias linhas rapidamente, com pouco tempo entre elas.                                                                                                                                                                         |
| **Saída esperada:**        | Cada linha deve ser ecoada corretamente, mesmo em alta taxa de transmissão.                                                                                                                                                                 |
| **Critério de Aceitação:** | O ISR (`serial_cb`) deve conseguir lidar com o fluxo sem perda de dados. Caso a fila (`k_msgq`) encha (10 mensagens), o programa deve continuar funcional e descartar silenciosamente mensagens excedentes. Nenhum travamento deve ocorrer. |


### **CT8 – Reset durante digitação**

| Item                       | Descrição                                                                                                                                              |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Entrada:**               | Usuário digita parte de uma mensagem, sem pressionar *Enter*, e o dispositivo é reiniciado.                                                            |
| **Saída esperada:**        | Após reiniciar, o sistema exibe novamente as mensagens iniciais:                                                                                       |
|                            | `Hello! I'm your echo bot.`<br>`Tell me something and press enter:`                                                                                    |
| **Critério de Aceitação:** | O buffer de recepção (`rx_buf_pos`) deve ser reiniciado. Nenhum dado parcial anterior deve ser ecoado. O sistema deve voltar ao estado inicial normal. |


### 🧾 **Resumo**

| Categoria            | Casos                   |
| -------------------- | ----------------------- |
| Funcionamento normal | CT1, CT2, CT3, CT4, CT6 |
| Robustez e limites   | CT5, CT7, CT10          |
| Resiliência e erro   | CT8                     |

---

## 3.3 Implementação

* Arquivo(s) modificados:
* Justificativa das alterações:

## 3.4 Evidências de Funcionamento

Salvar evidências em `docs/evidence/echo_bot/`.

Exemplo de referência no README:

```
[Link para o log CT1](docs/evidence/echo_bot/ct1_output.txt)
```

Adicionar aqui pequenos trechos ilustrativos:

```
Hello! I'm your echo bot. Tell me something and press enter:
Echo: Hello World!
```

## 3.5 Diagramas de Sequência D2

Vide material de apoio: https://d2lang.com/tour/sequence-diagrams/

Adicionar arquivos (diagrama completo e o código-base para geração do diagrama) em `docs/sequence-diagrams/`.

---

# 4. Etapa 2 – Async API (Transmissão/Recepção Assíncrona)

## 4.1 Descrição do Funcionamento

Descrever o comportamento esperado de forma textual, especialmente com a alternância TX/RX.
Link usado como referência:
[https://docs.zephyrproject.org/latest/samples/drivers/uart/async_api/README.html](https://docs.zephyrproject.org/latest/samples/drivers/uart/async_api/README.html)

## 4.2 Casos de Teste Planejados (TDD)

### CT1 – Transmissão de pacotes a cada 5s

### CT2 – Recepção

### CT3 – Verificação de timing dos 5s

(Adicionar mais casos se necessário.)

## 4.3 Implementação

* Arquivos modificados:
* Motivos/Justificativas:

## 4.4 Evidências de Funcionamento

Salvar em `docs/evidence/async_api/`.

Exemplo:

```
Loop 0:
Sending 3 packets (packet size: 5)
Packet: 0
Packet: 1
Packet: 2
```

Ou:

```
RX is now enabled
UART callback: RX_RDY
Data (HEX): 48 65 6C 6C 6F
Data (ASCII): Hello
```

## 4.5 Diagramas de Sequência D2

Vide material de referência: https://d2lang.com/tour/sequence-diagrams/

Adicionar arquivos (diagrama completo e o código-base para geração do diagrama) em `docs/sequence-diagrams/`.

---

# 5. Conclusões da Dupla

* O que deu certo:
* O que foi mais desafiador:

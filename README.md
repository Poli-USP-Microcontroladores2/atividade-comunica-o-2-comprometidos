# PSI-Microcontroladores2-Aula10
Atividade: Comunicação UART

# Projeto UART – Atividade em Duplas (Echo Bot + Async API)

## 1. Informações Gerais

* Dupla:

  * Filipe Cassoli - 5806752
  * Henrique Santiago - 16872729

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

### **Descrição do Comportamento Esperado – UART Echo Bot**

O *UART Echo Bot* é um exemplo simples que demonstra o uso do driver UART para comunicação serial. O programa atua como um “bot” que recebe dados digitados pelo usuário via console UART e devolve exatamente o mesmo conteúdo após o usuário pressionar a tecla *Enter*.


### **Visão Geral do Comportamento**

O programa inicializa a UART padrão do Zephyr (geralmente a mesma usada pelo console/shell) e passa a funcionar como um **bot de eco via serial**.
Ele aguarda o usuário digitar uma linha de texto (finalizada com *Enter*), e então envia de volta a mesma linha, precedida da palavra **“Echo:”**.

Durante o funcionamento:

* A **recepção** dos caracteres ocorre **de forma assíncrona**, via **interrupções**.
* O **envio** da resposta é feito **por polling** (síncrono), caractere a caractere.
* O programa fica rodando indefinidamente, repetindo o ciclo de leitura → eco → espera por nova entrada.


### **Fluxo de Execução Esperado**

#### **1 - Inicialização**

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


#### **2 - Recepção de dados (Interrupção via `serial_cb`)**

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


#### **3 - Fila de mensagens (`k_msgq`)**

A `k_msgq` é uma fila do Zephyr usada para comunicação entre a *interrupt callback* e a *thread principal* (`main()`).

* Capacidade: **10 mensagens**
* Tamanho de cada mensagem: **32 bytes**
* Alinhamento: **4 bytes**

Ela permite que a função principal espere por mensagens novas **sem bloquear o recebimento de interrupções**.


#### **4 - Loop principal (`main`)**

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


#### **5 - Envio de dados (`print_uart`)**

A função `print_uart()` envia cada caractere da string informada usando `uart_poll_out()` — um método **bloqueante**, mas simples.

Ela é usada:

* Para exibir as mensagens de boas-vindas
* Para enviar o eco de volta ao usuário


### **Exemplo de Interação Esperada (via terminal serial)**

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


### **Tratamento de Casos Especiais**

| Situação                            | Comportamento esperado                        |
| ----------------------------------- | --------------------------------------------- |
| Linha muito longa (> 31 caracteres) | Caracteres excedentes são descartados         |
| Linha vazia (apenas *Enter*)        | Gera eco: `Echo:`                             |
| Fila cheia (10 mensagens pendentes) | Mensagens novas são ignoradas                 |
| UART não pronta                     | Mensagem de erro no console e fim da execução |
| Erro ao configurar interrupção      | Exibe mensagem explicativa e encerra          |


### **Resumo funcional**

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


### **Resumo**

| Categoria            | Casos                   |
| -------------------- | ----------------------- |
| Funcionamento normal | CT1, CT2, CT3, CT4, CT6 |
| Robustez e limites   | CT5, CT7, CT10          |
| Resiliência e erro   | CT8                     |

---

## 3.3 Implementação

Não foi realizada nenhuma alteração no código-fonte do *Echo Bot UART*, já que o exemplo utilizado faz parte dos **samples oficiais do Zephyr Project** e já vem pronto para uso. A implementação foi executada exatamente conforme disponibilizada em `samples/drivers/uart/echo_bot`.

Em vez de executar o código pelo PlatformIO no VSCode, seguiu-se o procedimento oficial descrito no tutorial **“Getting Started Guide”** da documentação do Zephyr.
Esse guia fornece as instruções necessárias para **instalar o ambiente de desenvolvimento, configurar o SDK e o gerenciador de builds (west)**, bem como **compilar, gravar e executar aplicações de exemplo em placas de desenvolvimento compatíveis**, como a **FRDM-KL25Z**.


### **Etapas do processo (baseadas no Getting Started Guide)**

1. **Configuração do ambiente:**

   * Foi configurado um ambiente de desenvolvimento Python virtual (`.venv`) dentro da pasta `zephyrproject`, utilizando:

     ```powershell
     python -m venv zephyrproject\.venv
     zephyrproject\.venv\Scripts\Activate.ps1
     ```
   * Com o ambiente ativo, instalou-se o gerenciador de projetos Zephyr:

     ```powershell
     pip install west
     ```

2. **Obtenção do código-fonte do Zephyr:**

   * O Zephyr foi inicializado e clonado com seus módulos:

     ```powershell
     west init zephyrproject
     cd zephyrproject
     west update
     west zephyr-export
     ```
   * Foram instaladas as dependências Python do Zephyr:

     ```powershell
     west packages pip --install
     ```

3. **Instalação do SDK:**

   * O Zephyr SDK foi instalado usando o próprio comando do *west*, que inclui as toolchains necessárias (compilador, assembler e linker):

     ```powershell
     cd zephyr
     west sdk install
     ```

4. **Compilação do exemplo Echo Bot:**

   * O projeto foi compilado para a placa **FRDM-KL25Z**, utilizando o comando:

     ```powershell
     west build -p always -b frdm_kl25z samples/drivers/uart/echo_bot
     ```
   * O parâmetro `-p always` força uma compilação limpa (*pristine build*), garantindo que não haja resíduos de builds anteriores.

5. **Gravação (flash) do firmware:**

   * Com a placa conectada via USB e o **LinkServer** instalado, o código foi gravado na placa:

     ```powershell
     west flash
     ```
   * Esse processo compila o binário, identifica automaticamente a interface de programação e transfere o firmware para a placa.

6. **Execução e monitoramento serial:**

   * Após o upload, o dispositivo inicia automaticamente e exibe a mensagem de boas-vindas:

     ```
     Hello! I'm your echo bot.
     Tell me something and press enter:
     ```

   * A comunicação UART foi então monitorada por meio do um terminal serial do VSCode, configurado com:

     * Porta: COM3
     * Baud rate: 115200 bps
     * 8 data bits, sem paridade, 1 stop bit (8N1)

   * Ao enviar qualquer texto seguido de **Enter**, o dispositivo responde com o eco:

     ```
     Echo: <mensagem digitada>
     ```


### **Resumo do comportamento**

O *Echo Bot UART* utiliza a API de interrupção da UART para **receber dados de forma assíncrona** e a API de polling para **enviar os dados de volta ao console**.
Cada linha digitada e finalizada com *Enter* é armazenada em uma fila (`k_msgq`) e posteriormente reenviada pelo firmware, simulando o comportamento de um "bot" que repete o que o usuário digita.


### **Conclusão**

O exemplo foi executado com sucesso seguindo o procedimento do **Getting Started Guide**, sem necessidade de alterações no código.
O processo demonstrou corretamente o funcionamento da comunicação UART no Zephyr, com envio e recepção de mensagens de texto através da placa FRDM-KL25Z.

---

## 3.4 Evidências de Funcionamento

Todas as evidências disponíveis em [docs/evidence/etapa1_echobot_uart_pollingInterrupt/](docs/evidence/etapa1_echobot_uart_pollingInterrupt/).

---

### Evidências CT1 – Eco básico

![Imagem log CT1 – Eco básico](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct1_eco_básico/log_ct1.PNG)

[Link para log CT1 – Eco básico](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct1_eco_básico/log_ct1.txt)

---

### Evidências CT2 – Linha vazia

![Imagem log CT2 – Linha vazia](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct2_linha_vazia/log_ct2.PNG)

[Link para log CT2 – Linha vazia](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct2_linha_vazia/log_ct2.txt)

---

### Evidências CT3 – Linha longa (acima de 31 caracteres)

![Imagem CT3 – Linha longa (acima de 31 caracteres)](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct3_linha_longa_acima_de_31_caracteres/log_ct3.PNG)

[Link para log CT3 – Linha longa (acima de 31 caracteres)](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct3_linha_longa_acima_de_31_caracteres/log_ct3.txt)

---

### Evidências CT4 – Caracteres especiais

![Imagem CT4 – Caracteres especiais](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct4_caracteres_especiais/log_ct4.PNG)

[Link para log CT4 – Caracteres especiais](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct4_caracteres_especiais/log_ct4.txt)

---

### Evidências CT5 – Caracteres não ASCII (UTF-8)

![Imagem CT5 – Caracteres não ASCII (UTF-8)](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct5_caracteres_nao_ascii_utf_8/log_ct5.PNG)

[Link para log CT5 – Caracteres não ASCII (UTF-8)](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct5_caracteres_nao_ascii_utf_8/log_ct5.txt)

---

### Evidências CT6 – Múltiplas linhas seguidas

![Imagem CT6 – Múltiplas linhas seguidas](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct6_multiplas_linhas_seguidas/log_ct6.PNG)

[Link para log CT6 – Múltiplas linhas seguidas](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct6_multiplas_linhas_seguidas/log_ct6.txt)

---

### Evidências CT7 – Alta taxa de entrada de caracteres

![Imagem CT7 – Alta taxa de entrada de caracteres](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct7_alta_taxa_de_entrada_de_caracteres/log_ct7.PNG)

[Link para script python utilizado para alta taxa de entrada](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct7_alta_taxa_de_entrada_de_caracteres/script_ct7.py)

[Link para log CT7 – Alta taxa de entrada de caracteres](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct7_alta_taxa_de_entrada_de_caracteres/log_ct7.txt)

---

### Evidências CT8 – Reset durante digitação

![Imagem CT8 – Reset durante digitação](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct8_reset_durante_digitacao/log_ct8.PNG)

[Link para log CT8 – Reset durante digitação](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct8_reset_durante_digitacao/log_ct8.txt)

---

## 3.5 Diagramas de Sequência D2

Diagrama completo e código base disponíveis em [docs/sequence-diagrams/etapa1_echobot_uart_pollingInterrupt/](docs/sequence-diagrams/etapa1_echobot_uart_pollingInterrupt/)

### **Código base D2**

```
shape: sequence_diagram

# Echo Bot UART - Diagrama de Sequência

App -> UART_Driver: "uart_irq_callback_user_data_set(serial_cb)"
UART_Driver -> UART_Hardware: "Registra callback de interrupção"

App -> UART_Driver: "uart_irq_rx_enable()"
UART_Driver -> UART_Hardware: "Habilita RX"

App -> UART_Hardware: "print_uart('Hello! I\\'m your echo bot.')"
App -> UART_Hardware: "print_uart('Tell me something and press enter:')"

loop "aguarda entrada do usuário"
    UART_Hardware -> UART_Driver: "caracteres recebidos via IRQ"
    UART_Driver -> UART_Driver: "serial_cb() processa cada caractere"
    alt "fim de linha detectado (\\r ou \\n)"
        UART_Driver -> k_msgq: "k_msgq_put(rx_buf)"
        k_msgq -> App: "linha pronta para eco"
        App -> UART_Hardware: "print_uart('Echo: ')"
        App -> UART_Hardware: "print_uart(linha)"
        App -> UART_Hardware: "print_uart('\\r\\n')"
    else "linha não finalizada"
        UART_Driver -> UART_Driver: "acumula caractere em rx_buf"
    end
end
```
### **Diagrama**

![svg Diagrama](docs/sequence-diagrams/etapa1_echobot_uart_pollingInterrupt/echo_bot.svg)

---

# 4. Etapa 2 – Async API (Transmissão/Recepção Assíncrona)

## 4.1 Descrição do Funcionamento

Descrever o comportamento esperado de forma textual, especialmente com a alternância TX/RX.
Link usado como referência:
[https://docs.zephyrproject.org/latest/samples/drivers/uart/async_api/README.html](https://docs.zephyrproject.org/latest/samples/drivers/uart/async_api/README.html)


### Descrição textual do comportamento esperado (ciclo TX/RX)

O programa implementa um ciclo contínuo que alterna entre períodos de recepção (RX) e períodos de transmissão (TX) da UART, cada um com duração de 5 segundos. O objetivo é verificar se esse ciclo funciona corretamente quando não existem chamadas a `printk()` dentro do loop principal, evitando interferência com `uart_poll_out()`, que também utiliza o periférico UART.


#### 1. Inicialização

* O código obtém o dispositivo UART definido por `zephyr_shell_uart`.
* Verifica se o dispositivo está pronto.
* Não é configurado nenhum mecanismo de interrupção; toda a comunicação ocorre em modo polling (para RX e TX).


#### 2. Loop principal (executado indefinidamente)

Dentro do `while (1)`, duas fases acontecem sequencialmente:


### Etapa 1 — Recepção (RX) por 5 segundos

##### Comportamento da função `poll_receive()`

Durante 5 segundos:

1. O programa chama repetidamente `uart_poll_in()` para tentar ler um byte.
2. Se um caractere for recebido:

   * Se ele não for `'\r'`, é transmitido de volta usando `uart_poll_out()`, funcionando como um echo simples.
3. A função chama `k_msleep(1)` a cada iteração, para:

   * evitar uso excessivo da CPU,
   * permitir escalonamento de outras threads pelo RTOS,
   * manter uma temporização mais estável.

##### Resultado esperado

* Qualquer caractere enviado pela outra ponta da UART é imediatamente devolvido (ecoado).
* Se nada for recebido durante os 5 segundos, nada é transmitido.


### Etapa 2 — Transmissão (TX) por 5 segundos

##### Comportamento da função `poll_transmit()`

Durante 5 segundos:

1. A função transmite repetidamente a mensagem:

   ```
   Cassoli carregado\r\n
   ```
2. A transmissão é feita caractere por caractere usando `uart_poll_out()`.
3. Após cada envio completo, a função espera 200 ms antes de transmitir novamente.

##### Resultado esperado

* Durante todo o período de TX, a UART envia a mensagem "Cassoli carregado" várias vezes por segundo.
* O intervalo entre cada mensagem é de aproximadamente 200 ms.


### Alternância do ciclo

O comportamento total é o seguinte:

1. Recepção por 5 segundos
   – A UART fica lendo caracteres e ecoa tudo o que chega.

2. Transmissão por 5 segundos
   – A UART envia repetidamente a mensagem "Cassoli carregado".

3. Retorno ao estado de recepção
   – O ciclo se repete continuamente.


### Objetivo do teste

O objetivo é garantir que a alternância RX/TX funciona corretamente quando:

* Nenhuma chamada a `printk()` ocorre dentro do loop principal.
* Apenas `uart_poll_in()` e `uart_poll_out()` acessam o periférico UART.
* Evita-se conflito com `printk()`, que também utiliza o mesmo hardware UART.

Em placas como a FRDM, o console UART é compartilhado internamente. Portanto, `printk()` pode interferir com `uart_poll_out()`, causando falhas ou travamentos. Ao remover `printk()` do loop principal, o teste verifica se o comportamento do ciclo fica estável.


### Resumo geral

| Intervalo (segundos) | Modo | Comportamento                           |
| -------------------- | ---- | --------------------------------------- |
| 0–5                  | RX   | Echo simples do que chega               |
| 5–10                 | TX   | Envia "Cassoli carregado" repetidamente |
| 10–15                | RX   | Retorna ao modo echo                    |
| 15–20                | TX   | Envia novamente a mensagem              |
| ...                  | ...  | Continua alternando indefinidamente     |


---

## 4.2 Casos de Teste Planejados (TDD)


### **CT1 – Transmissão de pacotes a cada 5 segundos**

| Item                       | Descrição                                                                                                                                    |
| -------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | O sistema inicia a execução normalmente. Nenhum dado é enviado pela UART externa.                                                            |
| **Saída esperada:**        | A cada 5 segundos, o log exibe mensagens no formato `Loop X: Sending N packets`, seguidas por `Loop X: Packet: Y`.                           |
| **Critério de Aceitação:** | O intervalo entre ciclos é aproximadamente 5 segundos (±0,5s). Cada pacote é transmitido sem erro. Nenhum travamento ou erro de UART ocorre. |


### **CT2 – Recepção de dados (RX habilitado)**

| Item                       | Descrição                                                                                                                                    |
| -------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Durante o período em que o log mostra “RX is now enabled”, o usuário envia caracteres ou strings via terminal serial.                        |
| **Saída esperada:**        | O log exibe eventos `RX_RDY` com *hexdumps* dos dados recebidos.                                                                             |
| **Critério de Aceitação:** | Todos os bytes enviados devem aparecer nos *logs* sem perda. Caso RX esteja desativado, nada é recebido. Nenhum erro de buffer deve ocorrer. |


### **CT3 – Verificação de temporização de 5s**

| Item                       | Descrição                                                                                                                                              |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Entrada:**               | Monitorar o log de saída do sistema por pelo menos 3 iterações consecutivas.                                                                           |
| **Saída esperada:**        | A diferença de tempo entre as mensagens “Loop X” e “Loop X+1” é de aproximadamente 5 segundos.                                                         |
| **Critério de Aceitação:** | O temporizador `k_sleep(K_SECONDS(5))` deve ser respeitado. Tolerância de ±0,5 segundos. O sistema não deve adiantar nem atrasar de forma perceptível. |


### **CT4 – Fila de transmissão cheia (EBUSY)**

| Item                       | Descrição                                                                                                                                                                     |
| -------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Reduzir o tempo de espera (para testes) ou aumentar o número de pacotes transmitidos (`LOOP_ITER_MAX_TX`) de forma que novas transmissões ocorram antes da anterior terminar. |
| **Saída esperada:**        | Quando a UART estiver ocupada, o log exibe `Queuing buffer <ptr>`. Após o evento `TX_DONE`, os buffers enfileirados são enviados automaticamente.                             |
| **Critério de Aceitação:** | Nenhum pacote é perdido. A fila `k_fifo` é processada corretamente após cada `TX_DONE`. O sistema continua operando normalmente, sem travar nem perder dados.                 |


### **CT5 – Alternância de recepção (RX ON/OFF)**

| Item                       | Descrição                                                                                                                      |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| **Entrada:**               | Observar a execução contínua do código. A cada iteração do loop principal, o RX deve alternar entre habilitado e desabilitado. |
| **Saída esperada:**        | O log mostra alternadamente “RX is now enabled” e “RX is now disabled” a cada ciclo.                                           |
| **Critério de Aceitação:** | A alternância ocorre corretamente a cada 5 segundos. Nenhum erro ou exceção ocorre durante a ativação ou desativação do RX.    |


### **CT6 – Double buffering de recepção**

| Item                       | Descrição                                                                                                                     |
| -------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Enviar dados continuamente durante o período RX habilitado (por exemplo, via script Python).                                  |
| **Saída esperada:**        | O log exibe alternância entre `Providing buffer index 0` e `Providing buffer index 1` em eventos `UART_RX_BUF_REQUEST`.       |
| **Critério de Aceitação:** | O sistema alterna corretamente entre os dois buffers (`async_rx_buffer[2]`). Nenhum dado é perdido durante a troca de buffer. |


### **CT7 – Alta taxa de entrada de caracteres**

| Item                       | Descrição                                                                                                                                                                                    |
| -------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Um script envia dados rapidamente (múltiplas linhas ou bytes por segundo) durante o período RX habilitado.                                                                                   |
| **Saída esperada:**        | O sistema processa e exibe os eventos `RX_RDY` normalmente até o limite do buffer. Excedentes são descartados de forma silenciosa.                                                           |
| **Critério de Aceitação:** | O sistema não trava nem reinicia. O ISR (`uart_callback`) deve lidar com o fluxo sem falhas. Mensagens excedentes podem ser ignoradas, mas a aplicação deve permanecer estável e responsiva. |


### **CT8 – Execução contínua e estabilidade**

| Item                       | Descrição                                                                                                |
| -------------------------- | -------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Deixar o sistema executando continuamente por 10 minutos (ou mais) com TX/RX ativos.                     |
| **Saída esperada:**        | O sistema mantém alternância RX/TX e gera logs regulares. Nenhum erro crítico (`ERR`) é registrado.      |
| **Critério de Aceitação:** | A aplicação permanece funcional por todo o período. Sem travamentos, reinicializações ou falhas de UART. |

---


## 4.3 Implementação

A implementação seguiu o mesmo procedimento adotado na atividade anterior [Item 3.3](#33-implementação). 

Difererindo no comando de compilação, no qual foi utilizado:

```powershell
west build -p always -b frdm_kl25z samples/drivers/uart/async_api
```

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

Diagrama completo e código base disponíveis em [docs/sequence-diagrams/etapa2_asyncapi_transmissão_recepção_assíncrona/](docs/sequence-diagrams/etapa2_asyncapi_transmissão_recepção_assíncrona/)

### **Código base D2**

```
shape: sequence_diagram

main -> UART: "poll_receive(5000)"
note over UART: "Lê caracteres via uart_poll_in() por 5s"

UART -> UART: "Se caractere recebido"
UART -> main: "Entrega caractere"
main -> UART: "uart_poll_out() ecoa o caractere"

main -> UART: "poll_transmit(5000)"
note over main: "Envia 'Cassoli carregado' por 5s"

main -> UART: "uart_poll_out() repetido"
note over UART: "Mensagem transmitida a cada ~200ms"

main -> main: "Loop reinicia (while 1)"
```

### **Diagrama**

![svg Diagrama](docs/sequence-diagrams/etapa2_asyncapi_transmissão_recepção_assíncrona/async_api.svg)

---

# 5. Conclusões da Dupla

* O que deu certo:
* O que foi mais desafiador:

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

### **Descrição do Comportamento Esperado – UART Echo Bot**

O *UART Echo Bot* é um exemplo simples que demonstra o uso do driver UART para comunicação serial. O programa atua como um “bot” que recebe dados digitados pelo usuário via console UART e devolve exatamente o mesmo conteúdo após o usuário pressionar a tecla *Enter*.


### 🧭 **Visão Geral do Comportamento**

O programa inicializa a UART padrão do Zephyr (geralmente a mesma usada pelo console/shell) e passa a funcionar como um **bot de eco via serial**.
Ele aguarda o usuário digitar uma linha de texto (finalizada com *Enter*), e então envia de volta a mesma linha, precedida da palavra **“Echo:”**.

Durante o funcionamento:

* A **recepção** dos caracteres ocorre **de forma assíncrona**, via **interrupções**.
* O **envio** da resposta é feito **por polling** (síncrono), caractere a caractere.
* O programa fica rodando indefinidamente, repetindo o ciclo de leitura → eco → espera por nova entrada.


### ⚙️ **Fluxo de Execução Esperado**

#### **1️⃣ Inicialização**

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


#### **2️⃣ Recepção de dados (Interrupção via `serial_cb`)**

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


#### **3️⃣ Fila de mensagens (`k_msgq`)**

A `k_msgq` é uma fila do Zephyr usada para comunicação entre a *interrupt callback* e a *thread principal* (`main()`).

* Capacidade: **10 mensagens**
* Tamanho de cada mensagem: **32 bytes**
* Alinhamento: **4 bytes**

Ela permite que a função principal espere por mensagens novas **sem bloquear o recebimento de interrupções**.


#### **4️⃣ Loop principal (`main`)**

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


#### **5️⃣ Envio de dados (`print_uart`)**

A função `print_uart()` envia cada caractere da string informada usando `uart_poll_out()` — um método **bloqueante**, mas simples.

Ela é usada:

* Para exibir as mensagens de boas-vindas
* Para enviar o eco de volta ao usuário


### 💬 **Exemplo de Interação Esperada (via terminal serial)**

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


### ⚠️ **Tratamento de Casos Especiais**

| Situação                            | Comportamento esperado                        |
| ----------------------------------- | --------------------------------------------- |
| Linha muito longa (> 31 caracteres) | Caracteres excedentes são descartados         |
| Linha vazia (apenas *Enter*)        | Gera eco: `Echo:`                             |
| Fila cheia (10 mensagens pendentes) | Mensagens novas são ignoradas                 |
| UART não pronta                     | Mensagem de erro no console e fim da execução |
| Erro ao configurar interrupção      | Exibe mensagem explicativa e encerra          |


### 🧩 **Resumo funcional**

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

Não foi realizada nenhuma alteração no código-fonte do *Echo Bot UART*, já que o exemplo utilizado faz parte dos **samples oficiais do Zephyr Project** e já vem pronto para uso. A implementação foi executada exatamente conforme disponibilizada em `samples/drivers/uart/echo_bot`.

Em vez de executar o código pelo PlatformIO no VSCode, seguiu-se o procedimento oficial descrito no tutorial **“Getting Started Guide”** da documentação do Zephyr.
Esse guia fornece as instruções necessárias para **instalar o ambiente de desenvolvimento, configurar o SDK e o gerenciador de builds (west)**, bem como **compilar, gravar e executar aplicações de exemplo em placas de desenvolvimento compatíveis**, como a **FRDM-KL25Z**.


### 🧩 **Etapas do processo (baseadas no Getting Started Guide)**

1. **Configuração do ambiente:**

   * Foi configurado um ambiente de desenvolvimento Python virtual (`.venv`) dentro da pasta `zephyrproject`, utilizando:

     ```powershell
     python -m venv zephyrproject\.venv
     zephyrproject\.venv\Scripts\activate.bat
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
     west flash --runner=linkserver
     ```
   * Esse processo compila o binário, identifica automaticamente a interface de programação e transfere o firmware para a placa.

6. **Execução e monitoramento serial:**

   * Após o upload, o dispositivo inicia automaticamente e exibe a mensagem de boas-vindas:

     ```
     Hello! I'm your echo bot.
     Tell me something and press enter:
     ```

   * A comunicação UART foi então monitorada por meio de um terminal serial (como PuTTY, Tera Term ou o VSCode Serial Monitor), configurado com:

     * Porta: COMx (geralmente COM3 ou COM4)
     * Baud rate: 115200 bps
     * 8 data bits, sem paridade, 1 stop bit (8N1)

   * Ao enviar qualquer texto seguido de **Enter**, o dispositivo responde com o eco:

     ```
     Echo: <mensagem digitada>
     ```


### 💡 **Resumo do comportamento**

O *Echo Bot UART* utiliza a API de interrupção da UART para **receber dados de forma assíncrona** e a API de polling para **enviar os dados de volta ao console**.
Cada linha digitada e finalizada com *Enter* é armazenada em uma fila (`k_msgq`) e posteriormente reenviada pelo firmware, simulando o comportamento de um "bot" que repete o que o usuário digita.


### ✅ **Conclusão**

O exemplo foi executado com sucesso seguindo o procedimento do **Getting Started Guide**, sem necessidade de alterações no código.
O processo demonstrou corretamente o funcionamento da comunicação UART no Zephyr, com envio e recepção de mensagens de texto através da placa FRDM-KL25Z.

---

## 3.4 Evidências de Funcionamento

Todas as evidências disponíveis em `docs/evidence/etapa1_echobot_uart_pollingInterrupt/`.

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

[Link para CT7 – Alta taxa de entrada de caracteres](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct7_alta_taxa_de_entrada_de_caracteres/log_ct7.txt)

---

### Evidências CT8 – Reset durante digitação

![Imagem CT8 – Reset durante digitação](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct8_reset_durante_digitacao/log_ct8.PNG)

[Link para log CT8 – Reset durante digitação](docs/evidence/etapa1_echobot_uart_pollingInterrupt/ct8_reset_durante_digitacao/log_ct8.txt)

---

## 3.5 Diagramas de Sequência D2

Vide material de apoio: https://d2lang.com/tour/sequence-diagrams/

Adicionar arquivos (diagrama completo e o código-base para geração do diagrama) em `docs/sequence-diagrams/`.

---

# 4. Etapa 2 – Async API (Transmissão/Recepção Assíncrona)

## 4.1 Descrição do Funcionamento

Descrever o comportamento esperado de forma textual, especialmente com a alternância TX/RX.
Link usado como referência:
[https://docs.zephyrproject.org/latest/samples/drivers/uart/async_api/README.html](https://docs.zephyrproject.org/latest/samples/drivers/uart/async_api/README.html)

---

O código implementa uma aplicação exemplo que demonstra o uso da **API Assíncrona de UART (Async UART API)** do **Zephyr RTOS**. Essa API permite realizar **transmissão (TX)** e **recepção (RX)** de dados pela UART de forma **não bloqueante**, utilizando **interrupções** e **eventos** em vez de chamadas síncronas. Dessa forma, o microcontrolador pode continuar executando outras tarefas enquanto as operações de comunicação são processadas em segundo plano.

O funcionamento do programa é dividido em **duas partes principais**: o *callback* de eventos da UART, responsável por lidar com as interrupções de TX e RX, e o laço principal (*main loop*), que realiza transmissões periódicas e alterna o estado de recepção.


### **1. Inicialização da UART e do Callback**

O código seleciona o dispositivo UART padrão definido no *Device Tree* (`DT_CHOSEN(zephyr_shell_uart)`) e registra uma função de *callback* responsável por tratar todos os eventos gerados pelo periférico.
Esse *callback* (`uart_callback`) é chamado automaticamente sempre que ocorre um evento de transmissão concluída, buffer liberado, dados recebidos, entre outros.

O registro é feito com:

```c
uart_callback_set(uart_dev, uart_callback, (void *)uart_dev);
```


### **2. Envio Assíncrono de Dados (Transmissão - TX)**

No *loop principal*, o programa aguarda 5 segundos entre cada iteração e, em seguida, gera um número aleatório de pacotes (de 1 a 4).
Cada pacote contém uma pequena mensagem de texto com o número do loop e do pacote, por exemplo:

```
Loop 3: Packet 1
```

Esses pacotes são armazenados em buffers da pool `tx_pool` e transmitidos usando a função **`uart_tx()`**, que inicia a transmissão de forma assíncrona.
Caso a UART esteja ocupada no momento do envio, o pacote é colocado em uma fila (`k_fifo tx_queue`) e será transmitido automaticamente quando o evento `UART_TX_DONE` indicar que o canal está livre.

Ao final de cada transmissão, o evento `UART_TX_DONE` é acionado dentro do *callback*, liberando o buffer e enviando o próximo pacote da fila, se houver.
Esse mecanismo garante que a transmissão ocorra de forma **contínua e não bloqueante**, sem perda de dados e sem travar a execução principal do sistema.


### **3. Recepção Assíncrona de Dados (RX)**

A recepção é configurada para operar de forma assíncrona e alterna entre dois buffers (`async_rx_buffer[2][RX_CHUNK_LEN]`), implementando o conceito de **duplo buffer**.
Quando o driver UART precisa de um novo buffer para armazenar os dados recebidos, ele aciona o evento `UART_RX_BUF_REQUEST`, e o código responde com:

```c
uart_rx_buf_rsp(dev, async_rx_buffer[async_rx_buffer_idx], sizeof(async_rx_buffer[0]));
```

Isso permite que a recepção continue sem interrupções enquanto o outro buffer é processado.

Os dados recebidos são reportados no evento `UART_RX_RDY`, no qual o programa exibe o conteúdo recebido em formato hexadecimal no log:

```c
LOG_HEXDUMP_INF(evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len, "RX_RDY");
```


### **4. Alternância entre TX e RX**

Uma característica central desse exemplo é a **alternância entre os modos de transmissão e recepção**.
A cada iteração do *loop principal*, após enviar os pacotes, o código alterna o estado da UART:

* Se a recepção estava habilitada, ela é desativada com `uart_rx_disable(uart_dev);`
* Caso contrário, ela é habilitada novamente com `uart_rx_enable(uart_dev, async_rx_buffer[0], RX_CHUNK_LEN, 100);`

Essa alternância periódica demonstra o uso dinâmico da API assíncrona, simulando cenários onde o dispositivo precisa **alternar entre enviar e receber dados** em momentos diferentes, mantendo a flexibilidade e o controle total da comunicação serial.


### **5. Comportamento Esperado**

Durante a execução, o programa deve:

* A cada 5 segundos, **enviar de 1 a 4 mensagens UART** formatadas como “Loop X: Packet Y”;
* Alternar o estado da recepção a cada ciclo (ativar/desativar RX);
* Exibir logs informando transmissões, ativações e dados recebidos;
* Manter a **fila de transmissão e buffers RX operando corretamente**, sem travamentos ou perda de dados.


### **6. Conclusão**

Esse exemplo demonstra o uso prático da **API Assíncrona da UART no Zephyr**, evidenciando como é possível implementar uma comunicação **full-duplex, eficiente e não bloqueante** entre sistemas embarcados.
O mecanismo de **eventos e buffers duplos** garante que transmissões e recepções possam ocorrer de forma simultânea, controlada e segura, sem a necessidade de *polling* ou espera ativa.
A alternância periódica entre TX e RX reforça o uso flexível da UART, útil em aplicações como sistemas de telemetria, gateways de comunicação e protocolos seriais bidirecionais.

---

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

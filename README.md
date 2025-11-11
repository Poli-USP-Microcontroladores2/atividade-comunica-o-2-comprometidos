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

## 3.1 Descrição do Funcionamento

Descrever aqui de forma textual o comportamento esperado baseado no exemplo oficial.
Link usado como referência:
[https://docs.zephyrproject.org/latest/samples/drivers/uart/echo_bot/README.html](https://docs.zephyrproject.org/latest/samples/drivers/uart/echo_bot/README.html)

## **Descrição do Comportamento Esperado – UART Echo Bot**

O *UART Echo Bot* é um exemplo simples que demonstra o uso do driver UART para comunicação serial. O programa atua como um “bot” que recebe dados digitados pelo usuário via console UART e devolve exatamente o mesmo conteúdo após o usuário pressionar a tecla *Enter*.

### **Funcionamento Geral**

1. Ao iniciar, o sistema exibe uma mensagem de boas-vindas via UART, orientando o usuário a digitar algum texto.
2. O usuário digita uma sequência de caracteres no console UART.
3. O programa armazena os caracteres recebidos.

   * A recepção é feita por meio de **interrupções** (interrupt-driven), permitindo que a thread principal continue disponível para outras tarefas enquanto aguarda novos dados.
4. Quando o usuário pressiona *Enter* (fim da linha), o sistema envia de volta a linha recebida.

   * O envio é realizado usando a **API de polling**, ou seja, os caracteres são transmitidos de forma síncrona até concluir o envio.
5. Após o eco da mensagem, o sistema volta a aguardar novos dados, repetindo o ciclo.

### **Principais Características da Implementação**

* **Recepção por interrupção**: evita bloqueio da thread enquanto aguarda dados, permitindo potencial processamento paralelo.
* **Transmissão por polling**: simples e direta para enviar dados de volta ao usuário.
* **Compatível com a maioria das placas**: utiliza a UART padrão normalmente usada pelo *Zephyr shell*.
* **Comportamento contínuo**: o bot permanece ativo, ecoando cada nova entrada enviada pelo usuário após *Enter*.

### **Exemplo de Interação Esperada**

```
Hello! I'm your echo bot.
Tell me something and press enter:
Type e.g. "Hi there!" and hit enter!

Echo: Hi there!
```

O usuário digita uma mensagem (ex.: *Hi there!*), pressiona *Enter*, e o bot responde com a mesma mensagem antecedida por *"Echo:"*.


## 3.2 Casos de Teste Planejados (TDD)

### CT1 – Eco básico

* Entrada:
* Saída esperada:
* Critério de Aceitação:

### CT2 – Linha vazia

### CT3 – Linha longa

(Adicionar mais casos se necessário.)

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

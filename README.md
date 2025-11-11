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


### **3.2 Casos de Teste Planejados (TDD)**

#### **CT1 – Eco básico**

| Item                       | Descrição                                                                                    |
| -------------------------- | -------------------------------------------------------------------------------------------- |
| **Entrada:**               | Usuário digita `Hello` e pressiona *Enter*.                                                  |
| **Saída esperada:**        | `Echo: Hello`                                                                                |
| **Critério de Aceitação:** | O texto ecoado deve ser idêntico ao digitado, com o prefixo “Echo: ”, e apenas após *Enter*. |

---

#### **CT2 – Linha vazia**

| Item                       | Descrição                                                                                                                                               |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Usuário pressiona *Enter* sem digitar nenhum caractere.                                                                                                 |
| **Saída esperada:**        | `Echo:` *(linha vazia após o prefixo)*                                                                                                                  |
| **Critério de Aceitação:** | O sistema não deve travar nem gerar erro ao receber uma linha vazia. A linha vazia deve ser ecoada (ou comportamento conforme requisito, se diferente). |

---

#### **CT3 – Linha longa**

| Item                       | Descrição                                                                                                                                                          |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Entrada:**               | Usuário digita uma linha com o tamanho máximo suportado pelo buffer (ex.: 128 caracteres) e pressiona *Enter*.                                                     |
| **Saída esperada:**        | A linha completa deve ser ecoada com o prefixo `Echo:` sem truncamento ou corrupção.                                                                               |
| **Critério de Aceitação:** | O sistema deve ecoar todos os caracteres dentro do limite. Se excedido, deve adotar o comportamento definido (ex.: truncar, descartar excedente). Não pode travar. |

---

#### **CT4 – Caracteres especiais**

| Item                       | Descrição                                                                                                                 |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Usuário digita: `!@#$%&*()_+-=[]{};:'",.<>/?\|` e pressiona *Enter*.                                                      |
| **Saída esperada:**        | `Echo: !@#$%&*()_+-=[]{};:'",.<>/?\|`                                                                                     |
| **Critério de Aceitação:** | Todos os caracteres especiais devem ser recebidos e ecoados de forma idêntica, sem alterações, remoções ou substituições. |

---

#### **CT5 – Caracteres não ASCII (UTF-8)**

| Item                       | Descrição                                                                                                                                                                   |
| -------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Usuário digita: `Olá, você está bem? äöüñç` e pressiona *Enter*.                                                                                                            |
| **Saída esperada:**        | `Echo: Olá, você está bem? äöüñç`                                                                                                                                           |
| **Critério de Aceitação:** | O sistema deve manter a integridade dos caracteres acentuados ou multibyte. Se o driver não suportar UTF-8, o comportamento esperado deve ser documentado. Não deve travar. |

---

#### **CT6 – Múltiplas linhas seguidas**

| Item                       | Descrição                                                                                                                          |
| -------------------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Usuário envia 5 linhas seguidas, ex.: `A` + Enter, `B` + Enter, `C` + Enter...                                                     |
| **Saída esperada:**        | O sistema deve ecoar cada linha imediatamente após cada *Enter* (`Echo: A`, `Echo: B`, etc.).                                      |
| **Critério de Aceitação:** | O sistema deve processar e ecoar cada linha com sucesso, sem perder mensagens e sem necessidade de reinicialização entre entradas. |

---

#### **CT7 – Alta taxa de entrada de caracteres**

| Item                       | Descrição                                                                                                                                                                                     |
| -------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | O usuário ou script envia caracteres rapidamente, com pouco intervalo entre eles.                                                                                                             |
| **Saída esperada:**        | Todas as entradas devem ser corretamente recebidas e ecoadas assim que cada *Enter* for recebido.                                                                                             |
| **Critério de Aceitação:** | O sistema não pode perder caracteres devido ao recebimento por interrupção. Caso o buffer fique cheio, o comportamento deve seguir o especificado (ex.: aviso, truncamento). Não pode travar. |

---

#### **CT8 – Reset durante digitação**

| Item                       | Descrição                                                                                                                        |
| -------------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | Usuário digita parte de uma frase, sem pressionar *Enter*, e o dispositivo é reiniciado.                                         |
| **Saída esperada:**        | Após reiniciar, o sistema deve exibir novamente a mensagem inicial de boas-vindas. O texto parcial anterior não deve ser ecoado. |
| **Critério de Aceitação:** | O buffer deve ser reinicializado após reset e o bot deve retornar ao estado inicial sem comportamento inesperado.                |

---

#### **CT9 – Erro de UART / ruído na linha**

| Item                       | Descrição                                                                                                                                          |
| -------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entrada:**               | São inseridos erros simulados na transmissão (ex.: byte inválido, paridade incorreta, ou ruídos na linha).                                         |
| **Saída esperada:**        | O sistema deve manter estabilidade. Pode ignorar caracteres inválidos, substituí-los por placeholder ou tratá-los conforme configuração do driver. |
| **Critério de Aceitação:** | O sistema não deve travar nem reiniciar devido a erros de UART. Deve lidar com erros da forma definida e continuar operacional.                    |


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

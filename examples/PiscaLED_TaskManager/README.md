# Instrumentação Industrial I
## Aula Prática 03: Gerenciamento de Tarefas e Evolução Arquitetural em Sistemas Embarcados com ESP32

O gerenciamento de tarefas em sistemas embarcados é essencial para garantir que diferentes funcionalidades sejam executadas de forma eficiente e previsível, especialmente em projetos complexos. Este repositório explora a evolução de quatro arquiteturas distintas, implementadas com ESP32, analisando os prós e contras de cada abordagem.

---

## 📂 Sumário
1. [Introdução](#introdução)
2. [Evolução Arquitetural](#evolução-arquitetural)
   - [Código 1: Gerenciamento Rudimentar](#código-1-gerenciamento-rudimentar)
   - [Código 2: Introdução de Fila e Timer](#código-2-introdução-de-fila-e-timer)
   - [Código 3: Estruturação com Estruturas de Dados](#código-3-estruturação-com-estruturas-de-dados)
   - [Código 4: Generalização com Abstrações](#código-4-generalização-com-abstrações)
3. [Conclusão](#conclusão)
4. [Links dos Códigos](#links-dos-códigos)

---

## Introdução

O gerenciamento eficiente de tarefas em sistemas embarcados é uma habilidade fundamental para desenvolvedores que lidam com hardware de tempo real. Este repositório apresenta quatro implementações que mostram diferentes níveis de complexidade e abstração, desde um controle rudimentar até arquiteturas escaláveis e reutilizáveis.

---

## Evolução Arquitetural

### Código 1: Gerenciamento Rudimentar
**Descrição:** A gestão de tarefas é feita de forma direta, utilizando contadores manuais no loop principal para alternar entre tarefas.

**Prós:**
- Simplicidade na implementação.
- Controle direto sem dependências adicionais.

**Contras:**
- Baixa escalabilidade.
- Sobrecarga no loop principal.
- Dificuldade em gerenciar múltiplas tarefas simultaneamente.

**Código:** [esp1.cpp](./src/esp1.cpp)

---

### Código 2: Introdução de Fila e Timer
**Descrição:** Neste estágio, filas e timers são introduzidos para melhor gerenciamento das tarefas. Um sistema de filas do FreeRTOS é utilizado para comunicação entre tarefas.

**Prós:**
- Melhor gerenciamento do fluxo de dados.
- Utilização de timers para precisão no controle de tempo.
- Redução de complexidade no loop principal.

**Contras:**
- Aumento da complexidade na configuração inicial.
- Possível overhead de desempenho dependendo do número de tarefas.

**Código:** [esp2.cpp](./src/esp2.cpp)

---

### Código 3: Estruturação com Estruturas de Dados
**Descrição:** A arquitetura evolui com a introdução de estruturas de dados, permitindo a parametrização de tarefas e maior organização do código.

**Prós:**
- Código mais legível e organizado.
- Escalabilidade melhorada.
- Parametrização facilita a reutilização de partes do código.

**Contras:**
- Necessidade de maior atenção na manipulação das estruturas.
- Depuração pode ser mais complexa.

**Código:** [esp3.cpp](./src/esp3.cpp)

---

### Código 4: Generalização com Abstrações
**Descrição:** Neste último estágio, uma abstração completa é implementada, permitindo que o código suporte novas funcionalidades e configurações de forma simples.

**Prós:**
- Alta escalabilidade.
- Extensibilidade com mínimo esforço.
- Separação clara de responsabilidades.

**Contras:**
- Complexidade inicial na implementação.
- Maior curva de aprendizado para novos desenvolvedores no projeto.

**Código:** [esp4.cpp](./src/esp4.cpp)

---

## Tarefa a ser executada:

- Execute cada um dos códigos,
- Crie um novo arquivo **esp5.cpp** na pasta **./src**,
- Copie o código **esp4.cpp** no arquivo **esp5.cpp**,
- Faça um função que lê um valor na entrada analogica **def_pin_ADC1** e escreve na serial
- Faça as ligações necessárias e veja se o código esta funcionando corretamente usando o **teleplot**.

## Conclusão

A evolução apresentada neste repositório demonstra como a introdução de abstrações, estruturas de dados e boas práticas pode transformar um projeto rudimentar em uma aplicação escalável e de fácil manutenção. Cada abordagem possui seu espaço de aplicação, dependendo dos requisitos e das restrições do sistema embarcado.

---

## Links dos Códigos
- [Código 1: Gerenciamento Rudimentar](./src/esp1.cpp)
- [Código 2: Introdução de Fila e Timer](./src/esp2.cpp)
- [Código 3: Estruturação com Estruturas de Dados](./src/esp3.cpp)
- [Código 4: Generalização com Abstrações](./src/esp4.cpp)

# IIkit

**Versão:** 0.0.1  
**Autor:** josuemoraisgh

## Descrição

IIkit é uma biblioteca destinada à configuração e gerenciamento de módulos ESP8266/ESP32 para instrumentação. Ela facilita a configuração de credenciais WiFi e a definição de parâmetros customizados em tempo de execução por meio de um portal cativo, além de incluir utilitários e funções auxiliares para diversas tarefas comuns em aplicações IoT.

## Recursos

- **Portal Cativo para Configuração WiFi:** Permite que o dispositivo crie um ponto de acesso para que o usuário insira as credenciais de rede e configure parâmetros customizados.
- **Atualizações Over-The-Air (OTA):** Suporte para atualização remota do firmware.
- **Integração com Sensores e Dispositivos:** Funções auxiliares para leitura de sensores (por exemplo, ADS1115) e controle de displays.
- **Gerenciamento de Tarefas e Filas:** Implementa estruturas para agendamento e execução de tarefas de forma cooperativa.
- **Suporte para Sistema de Arquivos SPIFFS:** Permite armazenamento e leitura de dados na memória flash do dispositivo.

## Instalação

### PlatformIO

Adicione a seguinte linha no arquivo `platformio.ini` do seu projeto:

```ini
lib_deps =
  https://github.com/josuemoraisgh/IIkit.git

```
### Arduino IDE


1. Faça o download do repositório [IIkit](https://github.com/josuemoraisgh/IIkit.git) como arquivo ZIP.
2. No Arduino IDE, vá em **Sketch > Incluir Biblioteca > Adicionar Biblioteca .ZIP...** e selecione o arquivo baixado.

---

## Estrutura do Repositório

Abaixo, uma explicação de cada arquivo e diretório do projeto:

### Arquivos Principais da Biblioteca

- **IIKit.h / IIKit.cpp**  
  Arquivos centrais da biblioteca.  
  - **IIKit.h:** Declara as classes, funções e constantes necessárias para configurar e gerenciar os módulos ESP com suporte para portal cativo e outros recursos.  
  - **IIKit.cpp:** Contém as implementações das funções declaradas, centralizando a lógica de configuração e comunicação com o dispositivo.

- **IIKitmini.h / IIKitmini.cpp**  
  Versões simplificadas dos arquivos principais, indicadas para aplicações que exigem menor consumo de memória ou funcionalidades reduzidas.  
  - **IIKitmini.h:** Declara uma interface mais enxuta para as operações básicas.  
  - **IIKitmini.cpp:** Implementa essa interface, mantendo apenas o essencial para a configuração básica.

- **OTA.h**  
  Define as funções e macros necessárias para realizar atualizações Over-The-Air (OTA), permitindo a atualização remota do firmware do dispositivo sem a necessidade de conexão física.

- **ads1115_c.h**  
  Fornece a interface para o conversor analógico-digital ADS1115. Inclui funções para configurar e ler valores analógicos do chip ADS1115, útil para medições precisas em projetos com sensores.

- **asyncDelay.h**  
  Utilitário para gerenciamento de atrasos de forma assíncrona. Permite que o sistema execute outras tarefas enquanto aguarda um intervalo de tempo, melhorando a responsividade em aplicações multitarefa.

- **dinDebounce.h**  
  Contém funções para debouncing de entradas digitais. Essencial para evitar leituras falsas em botões e sinais digitais, garantindo que apenas transições válidas sejam processadas.

- **display_c.h**  
  Fornece funções para controle de displays (LCD, OLED, etc.), facilitando a exibição de informações e o status do dispositivo em tempo real.

- **hart_c.h**  
  Provavelmente contém funções relacionadas a sinais de "heartbeat" (sinal de vida) ou gerenciamento de tempo crítico, assegurando que o sistema opere de forma estável e confiável.

- **jqueue.h**  
  Implementa uma estrutura de fila (queue) para o gerenciamento de tarefas ou mensagens, permitindo a organização e o processamento sequencial ou prioritário de eventos no sistema.

- **jtask.h**  
  Define a estrutura e as funções para o gerenciamento de tarefas, possibilitando o agendamento e a execução de múltiplas tarefas de forma cooperativa ou concorrente, otimizando a eficiência do sistema.

- **spiffs.h**  
  Interface para o sistema de arquivos SPIFFS (SPI Flash File System). Permite que a aplicação armazene e recupere dados na memória flash interna do dispositivo.

- **wifimanager_c.h**  
  Gerencia a configuração da rede WiFi, incluindo a criação e o controle do portal cativo onde os usuários podem inserir e alterar as credenciais de conexão.

- **wserial_c.h**  
  Fornece funções para comunicação serial, facilitando a depuração e a troca de informações entre o dispositivo e outros dispositivos seriais ou um computador.

---

### Arquivos de Metadados e Configuração

- **library.properties**  
  Contém informações essenciais sobre a biblioteca, como nome, versão, autor, descrição, categoria, URL do repositório e arquiteturas suportadas (ESP8266 e ESP32). Esse arquivo é utilizado por ferramentas de gerenciamento de bibliotecas para identificar e integrar a biblioteca corretamente.

- **library.json**  
  Arquivo de metadados similar ao *library.properties*, que pode ser utilizado por outras ferramentas ou IDEs para facilitar a integração e atualização da biblioteca.

---

### Diretórios Adicionais

- **examples/PiscaLED_TaskManager/**  
  Contém um exemplo prático de utilização da IIkit. Neste exemplo, o projeto demonstra como gerenciar a piscada de um LED juntamente com o agendamento de tarefas, exemplificando o uso do gerenciador de tarefas e outras funcionalidades da biblioteca.

- **other/WiFiManager-2.0.17/**  
  Diretório que inclui uma versão do WiFiManager. Esse componente pode ser integrado à IIkit para melhorar a gestão das conexões WiFi e a implementação do portal cativo. Pode ser customizado conforme as necessidades do projeto.

---

### Contribuições

Contribuições são sempre bem-vindas! Para colaborar com o projeto, siga estas orientações:

1. **Abra uma Issue:** Se identificar algum problema ou tiver sugestões de melhoria.
2. **Fork o Repositório:** Crie uma cópia do repositório e implemente suas alterações.
3. **Envie um Pull Request:** Após testar as modificações, envie um pull request para que sua contribuição seja avaliada.

---

### Licença

Este projeto está licenciado sob a [Licença MIT](LICENSE).

---

## Contato

Desenvolvido por [josuemoraisgh](https://github.com/josuemoraisgh).

---

Esperamos que este repositório facilite o desenvolvimento dos seus projetos de instrumentação com ESP. Se tiver dúvidas, sugestões ou encontrar algum problema, sinta-se à vontade para abrir uma issue ou entrar em contato.


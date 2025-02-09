/**
 * @file jtask.cpp
 * @brief Biblioteca para gerenciamento de tarefas sem utilização de interrupções de timer e sem uso de fila.
 *
 * Este arquivo implementa a estrutura para configurar, agendar e executar tarefas
 * utilizando a função micros() no loop principal, dispensando tanto a interrupção de timer quanto a fila.
 */

#include "Arduino.h"

#ifndef NUMTASKS
  /**
   * @brief Número máximo de tarefas que podem ser registradas.
   */
  #define NUMTASKS 2
#endif

/**
 * @brief Índice para rastrear o número de tarefas registradas.
 */
uint8_t jtaskIndex = 0;

/**
 * @struct TaskConfig_t
 * @brief Estrutura para configuração das tarefas.
 *
 * @param lastExec Instante (em microssegundos) da última execução.
 * @param period Período da tarefa (em microssegundos).
 * @param task Ponteiro para a função que representa a tarefa.
 */
struct TaskConfig_t {
  unsigned long lastExec;
  unsigned long period;
  void (*task)();
};

/**
 * @brief Array que armazena as configurações das tarefas.
 */
TaskConfig_t jtaskStruct[NUMTASKS];

/**
 * @brief Inicializa o sistema de tarefas.
 *
 * Esta função pode ser utilizada para futuras inicializações, se necessário.
 */
void jtaskSetup() {
  // Nenhuma inicialização específica é necessária neste modelo.
}

/**
 * @brief Registra uma nova tarefa para execução periódica.
 *
 * @param task Ponteiro para a função da tarefa.
 * @param period Período da tarefa (em microssegundos).
 * @return true se a tarefa foi registrada com sucesso, false se o número máximo foi atingido.
 *
 * Ao registrar a tarefa, o instante atual é armazenado para controle do período.
 */
bool jtaskAttachFunc(void (*task)(), unsigned long period) {
  if (jtaskIndex >= NUMTASKS) return false;  // Verifica se já atingiu o máximo de tarefas
  
  jtaskStruct[jtaskIndex].lastExec = micros();
  jtaskStruct[jtaskIndex].period   = period;
  jtaskStruct[jtaskIndex].task     = task;
  jtaskIndex++;
  return true;
}

/**
 * @brief Atualiza os tempos de execução e executa as tarefas se o período for atingido.
 *
 * Esta função deve ser chamada periodicamente no loop principal do programa.
 * Para cada tarefa registrada, é verificado se o tempo atual menos o instante da última execução
 * é maior ou igual ao período configurado. Se sim, a tarefa é executada imediatamente e o tempo é atualizado.
 */
void jtaskLoop() {
  unsigned long currentMicros = micros();
  for (uint8_t i = 0; i < jtaskIndex; i++) {
    if (currentMicros - jtaskStruct[i].lastExec >= jtaskStruct[i].period) {
      jtaskStruct[i].lastExec = currentMicros;
      jtaskStruct[i].task();
    }
  }
}
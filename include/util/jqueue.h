/**
 * @file jqueue.h
 * @brief Biblioteca para manipulação de filas genéricas no Arduino.
 *
 * Este arquivo implementa uma estrutura de fila genérica com suporte a diferentes tipos de dados,
 * permitindo o uso em ambientes com interrupções, como ISRs (Interrupt Service Routines).
 */

#ifndef __JQUEUE_H
#define __JQUEUE_H

#include <Arduino.h>
#include <stdlib.h>

#ifndef MAXLENGTHJQUEUE
/**
 * @brief Define o tamanho máximo do buffer da fila.
 */
#define MAXLENGTHJQUEUE 5
#endif

/**
 * @struct jQueue_t
 * @brief Estrutura que representa uma fila genérica.
 */
typedef struct {
    void *buffer[MAXLENGTHJQUEUE]; ///< Buffer para armazenar endereços de itens na fila.
    uint8_t head; ///< Índice do próximo item a ser lido.
    uint8_t tail; ///< Índice do próximo espaço disponível para escrita.
    uint8_t count; ///< Número atual de itens na fila.
} jQueue_t;

/**
 * @brief Inicializa uma fila genérica.
 *
 * @param queue Ponteiro para a estrutura da fila.
 */
void jQueueInit(jQueue_t *queue) {
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

/**
 * @brief Adiciona um item à fila a partir de uma ISR.
 *
 * @param queue Ponteiro para a estrutura da fila.
 * @param item Ponteiro para o item a ser adicionado.
 * @return true se o item foi adicionado com sucesso, false caso a fila esteja cheia.
 */
bool jQueueSendFromISR(jQueue_t *queue, void *item) {
    if (queue->count == MAXLENGTHJQUEUE) return false; // Verifica se a fila está cheia.
    //noInterrupts();
    queue->buffer[queue->tail] = item;
    queue->tail = (queue->tail + 1) % MAXLENGTHJQUEUE;
    queue->count++;
    //interrupts();
    return true;
}

/**
 * @brief Remove um item da fila e o retorna via ponteiro.
 *
 * @param queue Ponteiro para a estrutura da fila.
 * @param item Ponteiro para armazenar o item removido.
 * @return true se o item foi removido com sucesso, false caso a fila esteja vazia.
 */
bool jQueueReceive(jQueue_t *queue, void **item) {
    if (queue->count == 0) return false; // Verifica se a fila está vazia.
    //noInterrupts();
    *item = queue->buffer[queue->head];
    queue->head = (queue->head + 1) % MAXLENGTHJQUEUE;
    queue->count--;
    //interrupts();
    return true;
}

/**
 * @brief Verifica se a fila está vazia.
 * @param queue Ponteiro para a estrutura da fila.
 * @return true se a fila estiver vazia, false caso contrário.
 */
bool jQueueIsEmpty(const jQueue_t *queue) {
    return queue->count == 0;
}

/**
 * @brief Verifica se a fila está cheia.
 * @param queue Ponteiro para a estrutura da fila.
 * @return true se a fila estiver cheia, false caso contrário.
 */
bool jQueueIsFull(const jQueue_t *queue) {
    return queue->count == MAXLENGTHJQUEUE;
}

/**
 * @brief Retorna o número de itens atualmente na fila.
 * @param queue Ponteiro para a estrutura da fila.
 * @return Número de itens na fila.
 */
uint8_t jQueueSize(const jQueue_t *queue) {
    return queue->count;
}

#endif

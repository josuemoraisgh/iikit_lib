#ifndef DIGITAL_IN_DEBOUNCE_H
#define DIGITAL_IN_DEBOUNCE_H

#include "Arduino.h"

/**
 * @brief Classe para leitura digital com debounce utilizando callback.
 *
 * Permite configurar um pino digital com debounce, sem uso de interrupções.
 * Sempre que o estado estável do pino mudar, a função callback é invocada,
 * recebendo o novo status (true para HIGH, false para LOW).
 */
class DigitalINDebounce
{
public:
  // Define o tipo da função callback: recebe o novo estado do pino (bool)
  typedef void (*CallbackFunc)(bool newState);
  /**
   * @brief Construtor da classe.
   * @param pin Número do pino a ser lido.
   * @param debounceDelay Tempo de debounce em milissegundos (padrão: 50ms).
   * @param callback Função callback a ser chamada quando houver mudança de estado.
   * @param mode Modo do pino (INPUT, INPUT_PULLUP, etc).*
   */
  DigitalINDebounce(uint8_t pin = 0, unsigned long debounceDelay = 50, CallbackFunc callback = nullptr, uint8_t mode = INPUT_PULLDOWN)
  {
    setup(pin, debounceDelay, callback, mode);
  }
  /**
   * @brief Inicializa o pino caso o construtor usado foi DigitalDebounce().
   * @param pin Número do pino a ser lido.
   * @param debounceDelay Tempo de debounce em milissegundos (padrão: 50ms).
   * @param callback Função callback a ser chamada quando houver mudança de estado.
   * @param mode Modo do pino (INPUT, INPUT_PULLUP, etc).*
   */
  void setup(uint8_t pin, unsigned long debounceDelay = 50, CallbackFunc callback = nullptr, uint8_t mode = INPUT_PULLDOWN)
  {
    _pin = pin;
    _debounceDelay = debounceDelay;
    _lastDebounceTime = 0;
    _callback = callback;
    pinMode(_pin, mode);
    _currentState = digitalRead(_pin);
    _stableState = _currentState;
  }
  /**
   * @brief Atualiza a leitura e processa o debounce.
   *
   * Deve ser chamada periodicamente (por exemplo, no loop) para ler o pino.
   * Quando o estado estável mudar, a função callback é chamada.
   */
  void update()
  {
    // Lê o pino
    bool reading = digitalRead(_pin);

    // Se houve mudança na leitura instantânea, reseta o timer de debounce
    if (reading != _currentState)
    {
      _lastDebounceTime = millis();
      _currentState = reading;
    }

    // Se o tempo decorrido for maior ou igual ao debounceDelay, confirma o novo estado
    if ((millis() - _lastDebounceTime) >= _debounceDelay)
    {
      if (_stableState != _currentState)
      {
        _stableState = _currentState;
        // Chama a callback se ela estiver definida
        if (_callback != nullptr)
        {
          _callback(_stableState);
        }
      }
    }
  }
  /**
   * @brief Retorna o estado estável atual do pino.
   * @return true se o pino estiver em nível HIGH, false se estiver em LOW.
   */
  bool pinValue()
  {
    return _stableState;
  }
  /**
   * @brief Define ou altera a função callback.
   * @param callback Função callback que recebe o novo estado do pino.
   */
  void setCallback(CallbackFunc callback)
  {
    _callback = callback;
  }

private:
  uint8_t _pin;                    // Número do pino de leitura
  unsigned long _debounceDelay;    // Tempo de debounce em milissegundos
  bool _currentState;              // Última leitura instantânea do pino
  bool _stableState;               // Estado estável após debounce
  unsigned long _lastDebounceTime; // Último instante de mudança detectada
  CallbackFunc _callback;          // Função callback para notificar mudança de estado
};

#endif // DIGITAL_IN_DEBOUNCE_H
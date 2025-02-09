#include <Arduino.h>
#include <stdint.h>

/**
 * @brief Classe para gerenciamento assíncrono de intervalos de tempo usando millis().
 * 
 * A AsyncDelay_c permite verificar se um intervalo de tempo pré-definido já foi atingido
 * sem bloquear a execução do código. Uma vez expirado, o intervalo pode se repetir
 * automaticamente, ou pode ser reiniciado manualmente. Ela não depende de RTOS, portanto
 * é compatível com plataformas Arduino clássicas (como o UNO) e ESP32 com Arduino Core.
 */
class AsyncDelay_c {
protected:
    uint32_t _expires  = 0;   ///< Armazena o tempo, em ms (fornecido por millis()), quando o intervalo expira.
    uint32_t _interval = 0;   ///< Duração do intervalo em milissegundos.

public:
    /**
     * @brief Construtor que inicializa o intervalo e define o tempo de expiração.
     * @param interval Duração do intervalo em milissegundos.
     */
    AsyncDelay_c(const uint32_t interval);

    /**
     * @brief Reinicia o intervalo com um novo valor, redefinindo o tempo de expiração.
     * @param interval Nova duração do intervalo em milissegundos.
     */
    void restart(const uint32_t interval);

    /**
     * @brief Verifica se o intervalo já expirou.
     * 
     * Se o intervalo tiver expirado, chama automaticamente repeat() para agendar o próximo.
     * @return true se o intervalo expirou, false caso contrário.
     */
    bool isExpired(void);

    /**
     * @brief Ajusta o tempo de expiração para o próximo intervalo, mantendo o mesmo valor de _interval.
     */
    void repeat(void);
};

inline AsyncDelay_c::AsyncDelay_c(const uint32_t interval) {
    restart(interval);
}

inline void AsyncDelay_c::restart(const uint32_t interval) {
    _interval = interval;
    _expires = millis() + _interval;
}

inline bool AsyncDelay_c::isExpired(void) {
    // Verifica se o tempo atual menos o tempo de expiração é >= 0, tratando o caso de overflow.
    // O cast para int32_t assegura uma comparação correta mesmo após overflow do millis().
    bool expirou = ((int32_t)(millis() - _expires) >= 0);

    if (expirou) {
        repeat(); // Se expirou, agendar o próximo intervalo automaticamente.
    }

    return expirou;
}

inline void AsyncDelay_c::repeat(void) {
    // Ao simplesmente somar _interval, criamos um novo ponto de expiração no futuro.
    _expires += _interval;
}
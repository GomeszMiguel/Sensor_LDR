/**
 * @file sensor_ldr.cpp
 * @brief Implementação de leitura de luminosidade utilizando um sensor LDR em um sistema Linux embarcado.
 * @author Miguel Gomes
 * @date 03 de Setembro de 2025
 * 
 * Este programa lê valores do ADC exportados no sysfs e converte a resistência do LDR
 * em uma estimativa percentual de luminosidade (0% = escuro, 100% = claro).
 */

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <cmath>

/**
 * @class SensorLDR
 * @brief Classe para leitura e cálculo de luminosidade a partir de um LDR.
 *
 * A classe realiza a leitura de valores crus do ADC, calcula a resistência do LDR
 * e mapeia para uma escala percentual de luminosidade.
 */
class SensorLDR {
private:
    std::string path;       /**< Caminho do arquivo sysfs do ADC. */
    float R_CLARO = 146*1e3;  /**< Resistência aproximada do LDR em ambiente claro (ohms). */
    float R_ESCURO = 5*1e6;   /**< Resistência aproximada do LDR em ambiente escuro (ohms). */
    const float ADC_MAX = 4095.0; /**< Valor máximo do ADC (12 bits). */
    const float R_FIXO = 10000.0; /**< Resistência fixa usada no divisor resistivo (ohms). */

public:
    /**
     * @brief Construtor da classe SensorLDR.
     * @param adcPath Caminho para o arquivo sysfs do ADC (ex.: `/sys/bus/iio/devices/...`).
     */
    SensorLDR(const std::string& adcPath) {
        path = adcPath;
    }

    /**
     * @brief Lê o valor cru do ADC.
     * @return Valor inteiro lido diretamente do ADC.
     */
    int lerValor() {
        std::ifstream file(path);
        int valor;
        file >> valor;
        return valor;
    }

    /**
     * @brief Calcula a luminosidade em percentual com base no valor lido do ADC.
     *
     * O cálculo utiliza a relação logarítmica entre a resistência do LDR e a intensidade luminosa.
     *
     * @return Luminosidade percentual (0 a 100).
     */
    int lerLuminosidadePercentual() {
        int valor = lerValor();
        float r_ldr = R_FIXO*(ADC_MAX - valor)/valor;
        float log_r_ldr = log(r_ldr);
        float log_r_claro = log(R_CLARO);
        float log_r_escuro = log(R_ESCURO);

        if (log_r_ldr > log_r_escuro) {
            return 0;
        }
        if (log_r_ldr < log_r_claro) {
            return 100;
        }
        float porcentagem = 100.0 * (log_r_escuro - log_r_ldr) / (log_r_escuro - log_r_claro);
        return static_cast<int>(porcentagem);
    }
};

/**
 * @brief Função principal.
 *
 * Cria um objeto SensorLDR, lê continuamente a luminosidade e imprime na saída padrão.
 *
 * @return 0 em caso de execução normal (embora nunca alcance o return devido ao loop).
 */
int main() {
    SensorLDR ldr("/sys/bus/iio/devices/iio:device0/in_voltage13_raw");

    while (true) {
        int val = ldr.lerLuminosidadePercentual();
        std::cout << "Luminosidade: " << val << "%" << std::endl;
        sleep(1);
    }

    return 0;
}

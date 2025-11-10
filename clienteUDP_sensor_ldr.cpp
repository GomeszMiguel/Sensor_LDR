/**
 * @file clienteUDP_sensor_ldr.cpp
 * @brief Implementação de leitura de luminosidade utilizando um sensor LDR em um sistema Linux embarcado.
 * * @author Miguel Gomes
 * @date 22 de Outubro de 2025
 * * @details Este programa lê valores do ADC exportados no sysfs e converte a resistência do LDR
 * em uma estimativa percentual de luminosidade (0% = escuro, 100% = claro).
 * O valor lido é então enviado via protocolo UDP (datagrama) para o servidor
 * rodando no endereço SERVER_IP (Host Windows/WSL) na porta 8080.
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <cmath>
#include <sys/socket.h> // Funções de socket (POSIX)
#include <arpa/inet.h> // Funções de conversão de endereço (inet_pton)
#include <string> // Necessário para std::string e std::to_string

using namespace std;

/** @def SERVER_IP
 * @brief Endereço IP do servidor (Host Windows/WSL).
 */
#define SERVER_IP "192.168.42.10" 

/** @def PORT
 * @brief Porta UDP do servidor.
 */
#define PORT 8080

/** @def BUFFER_SIZE
 * @brief Tamanho máximo do buffer de comunicação.
 */
#define BUFFER_SIZE 1024

/**
 * @class SensorLDR
 * @brief Classe para leitura e cálculo de luminosidade a partir de um LDR.
 *
 * @details A classe realiza a leitura de valores crus do ADC, calcula a resistência do LDR
 * e mapeia para uma escala percentual de luminosidade, utilizando a lei de potência
 * que relaciona a resistência com a intensidade luminosa (logarítmica).
 */
class SensorLDR {
private:
    /**< Caminho do arquivo sysfs do ADC (ex.: `/sys/bus/iio/devices/.../in_voltage13_raw`). */
    std::string path; 
    
    /**< Resistência aproximada do LDR em ambiente claro (ohms). */
    float R_CLARO = 146*1e3; 
    
    /**< Resistência aproximada do LDR em ambiente escuro (ohms). */
    float R_ESCURO = 5*1e6; 
    
    /**< Valor máximo do ADC (12 bits). */
    const float ADC_MAX = 4095.0; 
    
    /**< Resistência fixa usada no divisor resistivo (ohms). */
    const float R_FIXO = 10000.0; 

public:
    /**
     * @brief Construtor da classe SensorLDR.
     * @param adcPath Caminho para o arquivo sysfs do ADC.
     */
    SensorLDR(const std::string& adcPath) {
        path = adcPath;
    }

    /**
     * @brief Lê o valor cru do ADC.
     *
     * @details A leitura é feita diretamente do arquivo de dispositivo (sysfs) configurado no path.
     *
     * @return Valor inteiro lido diretamente do ADC.
     */
    int lerValor() {
        std::ifstream file(path);
        int valor = 0;
        if (file.is_open()) {
            file >> valor;
            file.close();
        } else {
            // Em um sistema embarcado real, aqui deve haver um tratamento de erro mais robusto.
            cerr << "Erro: Nao foi possivel abrir o arquivo ADC em " << path << endl;
        }
        return valor;
    }

    /**
     * @brief Calcula a luminosidade em percentual com base no valor lido do ADC.
     *
     * @details O cálculo utiliza a fórmula do divisor de tensão para obter a R_LDR,
     * e então mapeia a resistência (em escala logarítmica) para uma porcentagem (0-100).
     *
     * @return Luminosidade percentual (0 a 100).
     */
    int lerLuminosidadePercentual() {
        int valor = lerValor();
        // Fórmula do divisor de tensão: R_LDR = R_FIXO * (ADC_MAX - V_LDR) / V_LDR
        // Onde V_LDR é valor lido do ADC (ADC_MAX é a tensão de referência)
        float r_ldr = R_FIXO * (ADC_MAX - valor) / valor;
        
        float log_r_ldr = log(r_ldr);
        float log_r_claro = log(R_CLARO);
        float log_r_escuro = log(R_ESCURO);

        // Limita o valor para 0%
        if (log_r_ldr > log_r_escuro) {
            return 0;
        }
        // Limita o valor para 100%
        if (log_r_ldr < log_r_claro) {
            return 100;
        }
        
        // Mapeamento linear na escala logarítmica
        float porcentagem = 100.0 * (log_r_escuro - log_r_ldr) / (log_r_escuro - log_r_claro);
        
        return static_cast<int>(porcentagem);
    }
};

/**
 * @brief Função principal.
 *
 * @details Configura o socket UDP para enviar dados para o servidor 192.168.42.10:8080.
 * Cria um objeto SensorLDR, lê continuamente a luminosidade, converte o valor inteiro
 * para string e envia o datagrama para o servidor a cada segundo.
 *
 * @return 0 em caso de execução normal.
 */
int main() {
    // Inicializa o sensor LDR com o caminho do arquivo ADC no sysfs da placa
    SensorLDR ldr("/sys/bus/iio/devices/iio:device0/in_voltage13_raw");

    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    // 1. Criar o Socket
    // AF_INET: Endereços IPv4
    // SOCK_DGRAM: Protocolo UDP (datagrama, sem conexão)
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Erro ao criar o socket UDP do cliente");
        return -1;
    }

    // Limpar e configurar o endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); // Converte a porta (8080) para a ordem de bytes de rede

    // Converter endereço IP de string para formato binário
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Endereco IP invalido/nao suportado");
        close(client_socket);
        return -1;
    }
    
    cout << "Socket UDP criado com sucesso." << endl;

    /**
     * @brief Loop principal de leitura e envio.
     * @details O loop executa leituras e envios a cada 1 segundo.
     */
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);

        // O valor lido do sensor é um INT
        int val = ldr.lerLuminosidadePercentual();

        // Converte o valor inteiro (int) para uma string C++ (std::string)
        string val_str = to_string(val);

        // Obtém o ponteiro C-style (const char*) da string para uso na função sendto()
        const char *message = val_str.c_str(); 
    
        // Calcula o tamanho da mensagem (apenas o tamanho da string, sem o terminador nulo)
        size_t message_len = val_str.length();

        /**
         * @brief Envia o datagrama UDP.
         * @details O UDP é um protocolo sem conexão e não confiável; a chegada do pacote
         * não é garantida pelo protocolo e é gerenciada pela aplicação (se necessário).
         * O uso do UDP prioriza a baixa latência de dados de status em tempo real.
         * @param client_socket O descritor do socket.
         * @param message O ponteiro para os dados a serem enviados.
         * @param message_len O tamanho dos dados.
         * @param 0 Flags (geralmente 0 para UDP).
         * @param server_addr O endereço de destino.
         * @param sizeof(server_addr) O tamanho da estrutura de endereço.
         */
        ssize_t bytes_sent = sendto(client_socket, message, message_len, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));

        if (bytes_sent == -1) {
            perror("Erro ao enviar datagrama");
        } 
        else{
            cout << "Datagrama enviado (" << bytes_sent << " bytes) para " << SERVER_IP << ":" << PORT << endl;
            cout << "Luminosidade: " << message << "%" << std::endl;
            sleep(1); // Espera 1 segundo antes da próxima leitura/envio
        }
    }
    // O loop é infinito, o código abaixo só seria executado em caso de interrupção
    // 4. Fechar o Socket
    close(client_socket);
    cout << "Socket fechado. Cliente UDP encerrado." << endl;
    return 0;
}
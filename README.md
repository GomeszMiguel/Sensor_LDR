# Monitoramento de Carga com Sensor de Luminosidade (LDR) na STM32MP1

### Descrição Geral do Projeto

Este projeto implementa o **Módulo Sensor de Luminosidade** como parte de uma missão de Monitoramento Inteligente de Carga para o Exército Brasileiro. O objetivo é detectar a violação de um compartimento de carga sensível através da medição da luminosidade ambiente. Uma mudança súbita nos níveis de luz (de escuro para claro) indica uma possível abertura não autorizada, acionando um alerta no sistema central.

A solução foi desenvolvida para a placa **STM32MP1 DK1** e utiliza um sensor LDR (Resistor Dependente de Luz) para a aquisição de dados. O software foi implementado em C++ e encapsulado em uma classe para garantir a modularidade e facilitar a integração com outros sensores do sistema.

### Dependências Necessárias

Para compilar e gerar a documentação deste projeto, os seguintes pacotes são necessários:

* **Compilação:**
    * Um ambiente de compilação cruzada (cross-compilation) para ARM, como o `arm-linux-gnueabihf-g++`.
* **Documentação:**
    * **Doxygen:** Ferramenta para geração de documentação a partir do código-fonte.
    * **Graphviz (dot):** Ferramenta para a geração de diagramas e gráficos na documentação.

### Estrutura do Código e Explicação

O código-fonte está estruturado de forma modular para promover o encapsulamento e a clareza.

* **`SensorLDR` (Classe Principal):**
    * **Descrição:** Esta classe é responsável por toda a interação com o hardware do sensor LDR. Ela abstrai os detalhes de baixo nível da leitura do ADC (Conversor Analógico-Digital) via `sysfs` no Linux embarcado.
    * **Métodos Principais:**
        * `lerValor()`: Realiza a leitura do valor cru do ADC associado ao sensor.
        * *(Adicione outros métodos importantes aqui, por exemplo: `converterParaLux()` ou `verificarViolacao()`)*.

### Instruções de Compilação e Execução

Siga as etapas abaixo para compilar e executar o módulo do sensor.

1.  **Clonar o Repositório:**
    ```bash
    git clone [URL_DO_SEU_REPOSITORIO_AQUI]
    cd [NOME_DO_SEU_REPOSITORIO]
    ```

2.  **Compilar o Código (Cross-compilation):**
    Execute o comando abaixo no seu ambiente de desenvolvimento para compilar o código para a arquitetura da placa STM32MP1.
    *(Este é um exemplo, ajuste o comando conforme seu toolchain)*
    ```bash
    arm-linux-gnueabihf-g++ -o sensor_ldr main.cpp SensorLDR.cpp
    ```

3.  **Executar na Placa STM32MP1:**
    1.  Transfira o arquivo executável `sensor_ldr` para a placa (via `scp`, `sshfs` ou cartão SD).
    2.  No terminal da placa, dê permissão de execução ao arquivo:
        ```bash
        chmod +x sensor_ldr
        ```
    3.  Execute o programa:
        ```bash
        ./sensor_ldr
        ```
    O programa começará a exibir as leituras de luminosidade no terminal.

### Geração da Documentação

Todo o código é documentado usando o padrão Doxygen. Para gerar a documentação completa em HTML:

1.  Certifique-se de que o Doxygen e o Graphviz estão instalados.
2.  Na raiz do projeto, execute o comando:
    ```bash
    doxygen
    ```
3.  A documentação será gerada em uma nova pasta `html/`. Abra o arquivo `index.html` em um navegador para explorar as classes, métodos e diagramas.

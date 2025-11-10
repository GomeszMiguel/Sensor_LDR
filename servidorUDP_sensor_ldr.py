"""
@file servidorUDP_sensor_ldr.py
@brief Aplicação GUI de Monitoramento em Tempo Real para Sensor LDR via UDP.

Este script Python implementa um monitor gráfico em tempo real para dados
de luminosidade (em porcentagem) recebidos através de pacotes UDP.
Ele usa Tkinter para a interface gráfica e Matplotlib para o gráfico de histórico.
A principal característica é a função iniciar_servidor_udp, que recebe uma
string numérica via UDP e a transforma em um objeto JSON enriquecido
para processamento na GUI.

@author Gabriel Belizario
@date 09 Novembro de 2025
"""

import tkinter as tk
from tkinter import font
import socket
import threading
import json
import queue
from datetime import datetime
from collections import deque

# Importa as bibliotecas do Matplotlib
import matplotlib
matplotlib.use("TkAgg")
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

# --- Configurações ---
## @def UDP_IP
# O endereço IP local para o servidor UDP.
UDP_IP = "192.168.42.10" 
## @def UDP_PORT
# A porta UDP para escuta. Deve corresponder à porta configurada no transmissor C++.
UDP_PORT = 8080
## @def HISTORICO_MAX_PONTOS
# Número máximo de pontos de dados a serem mantidos e exibidos no gráfico.
HISTORICO_MAX_PONTOS = 60

# Fila para comunicação entre threads (servidor -> GUI)
## @var dados_fila
# Fila thread-safe para armazenar os dicionários/objetos JSON recebidos.
dados_fila = queue.Queue()
## @var dados_grafico
# Deque para armazenar o histórico de valores para plotagem, com tamanho máximo.
dados_grafico = deque(maxlen=HISTORICO_MAX_PONTOS)

def iniciar_servidor_udp():
    """
    @brief Inicia um servidor UDP em um thread separado para receber dados.

    Escuta por pacotes UDP contendo uma string numérica. Converte a string
    para um número inteiro e a *enriquece* com metadados (ID e unidade)
    em um formato de dicionário Python (JSON). O dicionário enriquecido
    é então colocado na fila de dados para ser processado pela thread principal (GUI).
    O ID do sensor é fixo como "LDR_KY-018" e a unidade como "%".
    
    @note Assume-se que os pacotes UDP contêm apenas a representação string de um inteiro.
    @return Não retorna. Executa em loop infinito.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print(f"Servidor UDP (modo string) escutando em {UDP_IP}:{UDP_PORT}...")

    while True:
        try:
            # 1. Recebe o datagrama (bytes)
            data, addr = sock.recvfrom(1024) 
            
            # 2. Converte bytes para string e remove espaços em branco (ex: "75")
            mensagem_string = data.decode('utf-8').strip()
            
            # 3. Converte a string (ex: "75") para um número inteiro
            valor_percentual = int(mensagem_string)
            
            # 4. TRANSFORMAÇÃO PARA JSON (Dicionário Python)
            dados_json_enriquecidos = {
                "id": "LDR_KY-018",        # Adiciona o ID do grupo
                "valor": valor_percentual,
                "unidade": "%"             # Adiciona a unidade
            }
            
            # 5. Coloca o objeto JSON (dicionário) na fila
            dados_fila.put(dados_json_enriquecidos)
            
        except (ValueError, UnicodeDecodeError):
            # Erro se a mensagem não for um número (ex: "ola")
            print(f"Erro: Pacote recebido não é um número válido: {data}")
        except Exception as e:
            print(f"Erro no servidor UDP: {e}")

class App(tk.Tk):
    """@class App
@brief Classe principal que define a Interface Gráfica do Usuário (GUI).

Gerencia a janela principal, exibe o valor atual, o status de alerta,
o gráfico de histórico e a funcionalidade de salvar log.
Utiliza o método after() do Tkinter para processar a fila de dados de forma assíncrona.
"""
    def __init__(self):
        """
        @brief Construtor da classe App.
        Inicializa a janela, variáveis de controle e chama a criação de widgets.
        """
        super().__init__()
        self.title("Monitor de Carga - Sensor LDR")
        self.geometry("700x500")
        
        ## @var valor_atual
        # Variável Tkinter para exibir o valor de luminosidade atual.
        self.valor_atual = tk.StringVar(value="-- %")
        ## @var status_atual
        # Variável Tkinter para exibir a mensagem de status/alerta.
        self.status_atual = tk.StringVar(value="Aguardando dados...")
        
        self.criar_widgets()
        self.processar_fila_dados()

    def criar_widgets(self):
        """
        @brief Configura e empacota todos os componentes da interface.
        Cria os rótulos, o frame do Matplotlib e o botão de salvar log.
        """
        # ... (Detalhes da criação de widgets omitidos para brevidade Doxygen)

        # Frame do Gráfico
        frame_grafico = tk.Frame(self)
        frame_grafico.pack(fill=tk.BOTH, expand=True, padx=10)

        ## @var fig
        # Figura do Matplotlib para o gráfico.
        self.fig = Figure(figsize=(6, 3), dpi=100)
        ## @var ax
        # Eixo da figura para plotagem.
        self.ax = self.fig.add_subplot(111)
        self.ax.set_title("Histórico de Luminosidade (Últimos 60s)")
        self.ax.set_ylabel("Luminosidade (%)")
        self.ax.set_ylim(0, 100)
        self.ax.grid()
        ## @var linha_grafico
        # Objeto de linha usado para atualizar o plot de forma eficiente.
        self.linha_grafico, = self.ax.plot([], [])
        
        ## @var canvas
        # Canvas Tkinter que contém a figura do Matplotlib.
        self.canvas = FigureCanvasTkAgg(self.fig, master=frame_grafico)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        # ... (Fim da criação de widgets)

    def processar_fila_dados(self):
        """
        @brief Verifica a fila de comunicação em busca de novos dados e atualiza a GUI.

        Utiliza get_nowait() para verificar a fila sem bloquear. Se um dicionário
        de dados for encontrado, a função atualiza o rótulo de valor, a mensagem
        de status (alerta) e adiciona o ponto ao gráfico. Chama a si mesma
        a cada 100ms via self.after().
        """
        try:
            dados = dados_fila.get_nowait() 
            
            valor = dados.get('valor', 0)
            
            # 1. Atualiza o Valor Atual
            # ...

            # 2. Atualiza o Alerta Visual
            # ...
            
            # 3. Atualiza o Histórico Gráfico
            dados_grafico.append(valor)
            self.atualizar_grafico()

        except queue.Empty:
            # Não havia dados na fila, apenas continua
            pass
        finally:
            self.after(100, self.processar_fila_dados)
            
    def atualizar_grafico(self):
        """
        @brief Atualiza os dados (eixos X e Y) do gráfico Matplotlib.
        O eixo Y é o histórico de dados_grafico. O eixo X é o índice da lista.
        """
        self.linha_grafico.set_ydata(list(dados_grafico))
        self.linha_grafico.set_xdata(range(len(dados_grafico)))
        self.ax.set_xlim(0, HISTORICO_MAX_PONTOS - 1)
        self.canvas.draw()
        
    def salvar_log(self):
        """
        @brief Salva o valor atual de luminosidade em um arquivo CSV (ldr_log.csv).
        Inclui um timestamp para cada registro e um cabeçalho se o arquivo for novo.
        """
        valor_log = self.valor_atual.get().split(' ')[0]
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        linha = f"{timestamp},{valor_log}\n"
        
        try:
            with open("ldr_log.csv", "a") as f:
                if f.tell() == 0:
                    f.write("timestamp,luminosidade_percent\n")
                f.write(linha)
            self.status_atual.set(f"Log salvo com sucesso às {timestamp}")
            self.label_status.config(fg="blue")
        except Exception as e:
            self.status_atual.set(f"Erro ao salvar log: {e}")
            self.label_status.config(fg="red")

# --- Ponto de Entrada Principal ---
if __name__ == "__main__":
    # 1. Instala a dependência (se ainda não o fez)
    print("Verifique se você instalou o Matplotlib: pip install matplotlib")

    # 2. Inicia o thread do servidor UDP (com a nova lógica)
    servidor_thread = threading.Thread(target=iniciar_servidor_udp, daemon=True)
    servidor_thread.start()

    # 3. Inicia a aplicação GUI
    app = App()
    app.mainloop()
import serial
import csv

# ================= CONFIGURAÇÕES =================
PORTA_SERIAL = 'COM4'
BAUD_RATE = 115200
ARQUIVO_CSV = 'dados_cinematicos.csv'

CABECALHO = [
    'tempo_ms', 'ax', 'ay', 'az',
    'vx', 'vy', 'vz',
    'px', 'py', 'pz'
]
# =================================================

def iniciar_leitura():
    conexao = None

    try:
        conexao = serial.Serial(PORTA_SERIAL, BAUD_RATE, timeout=1)

        print(f"Conectado na porta {PORTA_SERIAL}.")
        print(f"Gravando em {ARQUIVO_CSV}.")
        print("Pressione Ctrl+C para encerrar.")

        with open(ARQUIVO_CSV, mode='w', newline='', encoding='utf-8') as arquivo:
            escritor_csv = csv.writer(arquivo)
            cabecalho_lido = False

            while True:
                linha = conexao.readline().decode(
                    'utf-8', errors='replace'
                ).strip()

                if not linha:
                    continue

                dados = linha.split(',')

                # Identificar o cabeçalho
                if not cabecalho_lido:
                    if dados == CABECALHO:
                        escritor_csv.writerow(dados)
                        arquivo.flush()
                        cabecalho_lido = True
                        print("Cabeçalho identificado!")
                    continue

                # Aceitar somente linhas com 10 campos numéricos
                if len(dados) == 10:
                    try:
                        [float(valor) for valor in dados]
                    except ValueError:
                        continue

                    escritor_csv.writerow(dados)
                    arquivo.flush()
                    print(f"Dado salvo: {dados}")

    except serial.SerialException as e:
        print(f"Erro na comunicação serial: {e}")

    except KeyboardInterrupt:
        print("\nLeitura encerrada pelo usuário.")

    finally:
        if conexao is not None and conexao.is_open:
            conexao.close()
            print("Porta serial fechada.")


if __name__ == "__main__":
    iniciar_leitura()

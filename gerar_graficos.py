
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# ==================================================
# 1. CONFIGURAÇÕES
# ==================================================

ARQUIVO_CSV = 'dados_cinematicos.csv'
ARQUIVO_GRAFICO = 'graficos_atividade2.png'

colunas_esperadas = [
    'tempo_ms',
    'ax', 'ay', 'az',
    'vx', 'vy', 'vz',
    'px', 'py', 'pz'
]

# ==================================================
# 2. CARREGAR E VALIDAR OS DADOS
# ==================================================

try:
    df = pd.read_csv(ARQUIVO_CSV)

    # Remove espaços dos nomes das colunas
    df.columns = df.columns.str.strip()

    # Verifica se todas as colunas necessárias existem
    colunas_ausentes = [
        coluna for coluna in colunas_esperadas
        if coluna not in df.columns
    ]

    if colunas_ausentes:
        raise ValueError(
            f"Colunas ausentes no CSV: {colunas_ausentes}"
        )

    # Mantém somente as colunas esperadas
    df = df[colunas_esperadas].copy()

    # Converte os valores para números
    for coluna in colunas_esperadas:
        df[coluna] = pd.to_numeric(
            df[coluna], errors='coerce'
        )

    # Remove linhas com valores inválidos
    df = df.dropna()

    if df.empty:
        raise ValueError("O CSV não possui amostras válidas.")

except FileNotFoundError:
    print(f"Erro: arquivo '{ARQUIVO_CSV}' não encontrado.")
    exit()

except ValueError as erro:
    print(f"Erro nos dados: {erro}")
    exit()

# ==================================================
# 3. PREPARAR O VETOR DE TEMPO
# ==================================================

# O firmware envia o tempo em milissegundos.
# Convertendo para segundos:
tempo = df['tempo_ms'] / 1000.0

# ==================================================
# 4. CONFIGURAR OS GRÁFICOS
# ==================================================

fig, axs = plt.subplots(
    3, 1,
    figsize=(12, 12),
    sharex=True
)

# ==================================================
# 5. GRÁFICO DE ACELERAÇÃO
# ==================================================

axs[0].plot(tempo, df['ax'], label='Eixo X')
axs[0].plot(tempo, df['ay'], label='Eixo Y')
axs[0].plot(tempo, df['az'], label='Eixo Z')

axs[0].set_title('Aceleração ao longo do tempo')
axs[0].set_ylabel('Aceleração (m/s²)')
axs[0].legend()
axs[0].grid(True, linestyle='--', alpha=0.7)

# ==================================================
# 6. GRÁFICO DE VELOCIDADE
# ==================================================

axs[1].plot(tempo, df['vx'], label='Eixo X')
axs[1].plot(tempo, df['vy'], label='Eixo Y')
axs[1].plot(tempo, df['vz'], label='Eixo Z')

axs[1].set_title('Velocidade estimada ao longo do tempo')
axs[1].set_ylabel('Velocidade (m/s)')
axs[1].legend()
axs[1].grid(True, linestyle='--', alpha=0.7)

# ==================================================
# 7. GRÁFICO DE POSIÇÃO
# ==================================================

axs[2].plot(tempo, df['px'], label='Eixo X')
axs[2].plot(tempo, df['py'], label='Eixo Y')
axs[2].plot(tempo, df['pz'], label='Eixo Z')

axs[2].set_title('Posição estimada ao longo do tempo')
axs[2].set_ylabel('Posição (m)')
axs[2].set_xlabel('Tempo (s)')
axs[2].legend()
axs[2].grid(True, linestyle='--', alpha=0.7)

# ==================================================
# 8. SALVAR E EXIBIR
# ==================================================

plt.tight_layout()

plt.savefig(
    ARQUIVO_GRAFICO,
    dpi=300
)

print(f"Gráfico salvo com sucesso: {ARQUIVO_GRAFICO}")
print(f"Quantidade de amostras: {len(df)}")
print(f"Duração da captura: {tempo.iloc[-1] - tempo.iloc[0]:.2f} s")

plt.show()

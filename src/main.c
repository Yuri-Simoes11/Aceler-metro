#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdint.h>

#define INTERVALO_MS 150

#define NUM_AMOSTRAS_CALIBRACAO 200

#define STACK_AQUISICAO 1024
#define STACK_INTEGRACAO 1024
#define STACK_SERIAL 2048

#define PRIORIDADE_AQUISICAO 4
#define PRIORIDADE_INTEGRACAO 5
#define PRIORIDADE_SERIAL 6

static const struct device *const accel =
    DEVICE_DT_GET(DT_NODELABEL(mma8451q));

// Dados enviados pela thread de aquisicao
struct amostra {
    int64_t tempo_ms;

    double ax;
    double ay;
    double az;
};

// Dados enviados pela thread de integracao
struct resultado {
    int64_t tempo_ms;

    double ax, ay, az;
    double vx, vy, vz;
    double px, py, pz;
};

// Definição das Mensagens
// Fila entre aquisicao e integracao
K_MSGQ_DEFINE(fila_amostras, sizeof(struct amostra), 8, 4);

// Fila entre integracao e serial
K_MSGQ_DEFINE(fila_resultados, sizeof(struct resultado), 8, 4);

// Função Auxiliar para ler o acelerômetro
static int ler_acelerometro(double *ax, double *ay, double *az) {
    struct sensor_value x, y, z;
    int ret;

    ret = sensor_sample_fetch(accel);

    if (ret != 0) {
        return ret;
    }

    ret = sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &x);

    if (ret != 0) {
        return ret;
    }

    ret = sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &y);

    if (ret != 0) {
        return ret;
    }

    ret = sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &z);

    if (ret != 0) {
        return ret;
    }

    *ax = sensor_value_to_double(&x);
    *ay = sensor_value_to_double(&y);
    *az = sensor_value_to_double(&z);

    return 0;
}

// Definição das Threads (Sem inicialização)
K_THREAD_STACK_DEFINE(stack_aquisicao, STACK_AQUISICAO);
K_THREAD_STACK_DEFINE(stack_integracao, STACK_INTEGRACAO);
K_THREAD_STACK_DEFINE(stack_serial, STACK_SERIAL);

static struct k_thread thread_aquisicao;
static struct k_thread thread_integracao;
static struct k_thread thread_serial;

// =====================================================
// THREAD 1: AQUISICAO
// =====================================================

void thread_aquisicao_func(void *a, void *b, void *c) {
    double ax, ay, az;

    double soma_x = 0.0;
    double soma_y = 0.0;
    double soma_z = 0.0;

    double offset_x, offset_y, offset_z;

    int contador = 0;

    struct amostra dado;

    // ---------------------------------------------
    // CALIBRACAO INICIAL
    // Placa deve permanecer parada!
    // ---------------------------------------------

    while (contador < NUM_AMOSTRAS_CALIBRACAO) {

        if (ler_acelerometro(&ax, &ay, &az) == 0) {

            soma_x += ax;
            soma_y += ay;
            soma_z += az;

            contador++;
        }

        k_sleep(K_MSEC(INTERVALO_MS));
    }

    // Calcular aceleracao media de repouso
    offset_x = soma_x / NUM_AMOSTRAS_CALIBRACAO;
    offset_y = soma_y / NUM_AMOSTRAS_CALIBRACAO;
    offset_z = soma_z / NUM_AMOSTRAS_CALIBRACAO;

    // Inicio da contagem de tempo apos calibracao
    int64_t inicio_ms = k_uptime_get();

    // ---------------------------------------------
    // AQUISICAO CONTINUA
    // ---------------------------------------------

    while (1) {

        if (ler_acelerometro(&ax, &ay, &az) == 0) {

            dado.tempo_ms = k_uptime_get() - inicio_ms;

            // Compensacao do repouso
            dado.ax = ax - offset_x;
            dado.ay = ay - offset_y;
            dado.az = az - offset_z;

            // Enviar amostra para a fila
            k_msgq_put(&fila_amostras,
                       &dado,
                       K_FOREVER);
        }

        k_sleep(K_MSEC(INTERVALO_MS));
    }
}

// =====================================================
// THREAD 2: INTEGRACAO NUMERICA
// =====================================================

void thread_integracao_func(void *a, void *b, void *c) {
    struct amostra dado;

    struct resultado saida;

    bool primeira_amostra = true;

    int64_t tempo_anterior = 0;

    double ax_anterior = 0.0;
    double ay_anterior = 0.0;
    double az_anterior = 0.0;

    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;

    double px = 0.0;
    double py = 0.0;
    double pz = 0.0;

    double dt;

    while (1) {

        // Esperar uma nova amostra
        k_msgq_get(&fila_amostras, &dado, K_FOREVER);

        if (primeira_amostra) {

            tempo_anterior = dado.tempo_ms;

            ax_anterior = dado.ax;
            ay_anterior = dado.ay;
            az_anterior = dado.az;

            primeira_amostra = false;

        } else {

            // Intervalo real entre amostras, em segundos
            dt = (dado.tempo_ms - tempo_anterior) / 1000.0;

            if (dt <= 0.0) {
                continue;
            }

            // Guardar velocidades anteriores
            double vx_anterior = vx;
            double vy_anterior = vy;
            double vz_anterior = vz;

            // Integracao da aceleracao: velocidade
            vx += ((dado.ax + ax_anterior) / 2.0) * dt;
            vy += ((dado.ay + ay_anterior) / 2.0) * dt;
            vz += ((dado.az + az_anterior) / 2.0) * dt;

            // Integracao da velocidade: posicao
            px += ((vx + vx_anterior) / 2.0) * dt;
            py += ((vy + vy_anterior) / 2.0) * dt;
            pz += ((vz + vz_anterior) / 2.0) * dt;

            // Atualizar valores anteriores
            tempo_anterior = dado.tempo_ms;

            ax_anterior = dado.ax;
            ay_anterior = dado.ay;
            az_anterior = dado.az;
        }

        // Preparar resultado
        saida.tempo_ms = dado.tempo_ms;

        saida.ax = dado.ax;
        saida.ay = dado.ay;
        saida.az = dado.az;

        saida.vx = vx;
        saida.vy = vy;
        saida.vz = vz;

        saida.px = px;
        saida.py = py;
        saida.pz = pz;

        // Enviar resultado para a thread serial
        k_msgq_put(&fila_resultados,
                   &saida,
                   K_FOREVER);
    }
}

// =====================================================
// THREAD 3: COMUNICACAO SERIAL
// =====================================================

void thread_serial_func(void *a, void *b, void *c) {
    struct resultado dado;

    // Cabecalho CSV
    printk("tempo_ms,ax,ay,az,vx,vy,vz,px,py,pz\n");

    while (1) {

        // Esperar resultado da integracao
        k_msgq_get(&fila_resultados,
                   &dado,
                   K_FOREVER);

        // Enviar dados ao computador
        printk("%lld,%.6f,%.6f,%.6f,"
               "%.6f,%.6f,%.6f,"
               "%.6f,%.6f,%.6f\n",

               (long long)dado.tempo_ms,

               dado.ax, dado.ay, dado.az,

               dado.vx, dado.vy, dado.vz,

               dado.px, dado.py, dado.pz);
    }
}

void main(void) {
    printk("\n");
    printk("========================================\n");
    printk(" FRDM-KL25Z - MMA8451Q - Zephyr Threads\n");
    printk("========================================\n");

    if (!device_is_ready(accel)) {
        printk("ERRO: Acelerometro nao esta pronto!\n");
        return;
    }

    printk("Acelerometro inicializado!\n");
    printk("Intervalo solicitado: %d ms\n", INTERVALO_MS);
    printk("Iniciando threads...\n");

    // Thread de aquisicao
    k_thread_create(&thread_aquisicao,
                    stack_aquisicao,
                    K_THREAD_STACK_SIZEOF(stack_aquisicao),
                    thread_aquisicao_func,
                    NULL, NULL, NULL,
                    PRIORIDADE_AQUISICAO,
                    0,
                    K_NO_WAIT);

    // Thread de integracao
    k_thread_create(&thread_integracao,
                    stack_integracao,
                    K_THREAD_STACK_SIZEOF(stack_integracao),
                    thread_integracao_func,
                    NULL, NULL, NULL,
                    PRIORIDADE_INTEGRACAO,
                    0,
                    K_NO_WAIT);

    // Thread de comunicacao serial
    k_thread_create(&thread_serial,
                    stack_serial,
                    K_THREAD_STACK_SIZEOF(stack_serial),
                    thread_serial_func,
                    NULL, NULL, NULL,
                    PRIORIDADE_SERIAL,
                    0,
                    K_NO_WAIT);

    // A thread principal permanece suspensa
    while (1) {
        k_sleep(K_FOREVER);
    }
}

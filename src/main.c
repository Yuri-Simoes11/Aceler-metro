
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdint.h>

#define INTERVALO_MS 50

#define NUM_AMOSTRAS_CALIBRACAO 40

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

void main(void)
{
    struct sensor_value accel_x, accel_y, accel_z;
    int ret;

    int64_t inicio_ms;
    int64_t tempo_ms;

    double ax, ay, az;

    printk("\n");
    printk("========================================\n");
    printk("  FRDM-KL25Z - Acelerometro MMA8451Q\n");
    printk("========================================\n");

    if (!device_is_ready(accel)) {
        printk("ERRO: Acelerometro nao esta pronto!\n");
        return;
    }

    printk("Acelerometro inicializado com sucesso!\n");
    printk("Intervalo solicitado: %d ms\n\n", INTERVALO_MS);

    k_sleep(K_MSEC(1000));

    inicio_ms = k_uptime_get();

    while (1) {

        // 1. Solicitar uma nova amostra
        ret = sensor_sample_fetch(accel);

        if (ret != 0) {
            printk("Erro na leitura do sensor: %d\n", ret);
            k_sleep(K_MSEC(INTERVALO_MS));
            continue;
        }

        // 2. Obter os valores dos tres eixos
        ret = sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &accel_x);
        if (ret != 0) {
            printk("Erro ao obter eixo X: %d\n", ret);
            continue;
        }

        ret = sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &accel_y);
        if (ret != 0) {
            printk("Erro ao obter eixo Y: %d\n", ret);
            continue;
        }

        ret = sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &accel_z);
        if (ret != 0) {
            printk("Erro ao obter eixo Z: %d\n", ret);
            continue;
        }

        // 3. Converter para m/s², preservando os sinais
        ax = sensor_value_to_double(&accel_x);
        ay = sensor_value_to_double(&accel_y);
        az = sensor_value_to_double(&accel_z);

        // 4. Obter o tempo real desde o inicio da aquisicao
        tempo_ms = k_uptime_get() - inicio_ms;

        // 5. Enviar os dados pela serial
        printk("T: %lld, X: %.6f, Y: %.6f, Z: %.6f\n",
               (long long)tempo_ms, ax, ay, az);

        // 6. Aguardar antes da proxima amostra
        k_sleep(K_MSEC(INTERVALO_MS));
    }
}

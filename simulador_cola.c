#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <string.h>

typedef struct {
    int *buffer;
    int capacidad;
    int frente;
    int final;
    int contador;
    int productores_activos;
    pthread_mutex_t mutex;
    pthread_cond_t no_lleno;
    pthread_cond_t no_vacio;
    pthread_cond_t productores_terminaron;
    FILE *log;
} ColaCircularMonitor;

typedef struct {
    ColaCircularMonitor *cola;
    int tiempo_espera;
} ConsumidorArgs;

void redimensionarCola(ColaCircularMonitor *cola, int nueva_capacidad) {
    int *nuevo_buffer = (int *)malloc(nueva_capacidad * sizeof(int));
    for (int i = 0; i < cola->contador; i++) {
        nuevo_buffer[i] = cola->buffer[(cola->frente + i) % cola->capacidad];
    }
    free(cola->buffer);
    cola->buffer = nuevo_buffer;
    cola->capacidad = nueva_capacidad;
    cola->frente = 0;
    cola->final = cola->contador - 1;

    fprintf(cola->log, "Cambio de tamaño de la cola: nueva capacidad = %d\n", nueva_capacidad);
    fflush(cola->log);
}

void inicializarCola(ColaCircularMonitor *cola, int capacidad, int num_productores) {
    cola->capacidad = capacidad;
    cola->frente = 0;
    cola->final = -1;
    cola->contador = 0;
    cola->productores_activos = num_productores;
    cola->buffer = (int *)malloc(capacidad * sizeof(int));
    pthread_mutex_init(&cola->mutex, NULL);
    pthread_cond_init(&cola->no_lleno, NULL);
    pthread_cond_init(&cola->no_vacio, NULL);
    pthread_cond_init(&cola->productores_terminaron, NULL);
    cola->log = fopen("log.txt", "w");
    if (cola->log == NULL) {
        perror("Error al abrir el archivo log");
        exit(1);
    }
    fprintf(cola->log, "Inicio de la ejecución\n");
}

void producir(ColaCircularMonitor *cola, int item) {
    pthread_mutex_lock(&cola->mutex);

    if (cola->contador == cola->capacidad) {
        fprintf(cola->log, "Cola llena. Duplicando capacidad a %d.\n", cola->capacidad * 2);
        fflush(cola->log);
        redimensionarCola(cola, cola->capacidad * 2);
    }

    cola->final = (cola->final + 1) % cola->capacidad;
    cola->buffer[cola->final] = item;
    cola->contador++;

    fprintf(cola->log, "Producido: %d, elementos en cola: %d\n", item, cola->contador);
    fflush(cola->log);

    pthread_cond_signal(&cola->no_vacio);
    pthread_mutex_unlock(&cola->mutex);
}

void finalizarProductor(ColaCircularMonitor *cola) {
    pthread_mutex_lock(&cola->mutex);
    cola->productores_activos--;

    if (cola->productores_activos == 0) {
        pthread_cond_broadcast(&cola->productores_terminaron);
    }
    pthread_mutex_unlock(&cola->mutex);
}

int consumir(ColaCircularMonitor *cola, int tiempo_espera) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += tiempo_espera;

    pthread_mutex_lock(&cola->mutex);

    while (cola->contador == 0 && cola->productores_activos > 0) {
        if (pthread_cond_timedwait(&cola->no_vacio, &cola->mutex, &ts) == ETIMEDOUT) {
            pthread_mutex_unlock(&cola->mutex);
            return -1;
        }
    }

    if (cola->contador == 0 && cola->productores_activos == 0) {
        pthread_mutex_unlock(&cola->mutex);
        return -1;
    }

    int item = cola->buffer[cola->frente];
    cola->frente = (cola->frente + 1) % cola->capacidad;
    cola->contador--;

    if (cola->contador > 0 && cola->contador <= cola->capacidad / 4 && cola->capacidad > 1) {
        fprintf(cola->log, "Reduciendo capacidad de la cola a %d.\n", cola->capacidad / 2);
        fflush(cola->log);
        redimensionarCola(cola, cola->capacidad / 2);
    }

    fprintf(cola->log, "Consumido: %d, elementos en cola: %d\n", item, cola->contador);
    fflush(cola->log);

    pthread_cond_signal(&cola->no_lleno);
    pthread_mutex_unlock(&cola->mutex);
    return item;
}

void destruirCola(ColaCircularMonitor *cola) {
    fprintf(cola->log, "Fin de la ejecución\n");
    fclose(cola->log);
    free(cola->buffer);
    pthread_mutex_destroy(&cola->mutex);
    pthread_cond_destroy(&cola->no_lleno);
    pthread_cond_destroy(&cola->no_vacio);
    pthread_cond_destroy(&cola->productores_terminaron);
}

void *productor(void *arg) {
    ColaCircularMonitor *cola = (ColaCircularMonitor *)arg;
    for (int i = 0; i < 5; i++) {
        producir(cola, i);
        usleep(100000);
    }
    finalizarProductor(cola);
    return NULL;
}

void *consumidor(void *arg) {
    ConsumidorArgs *args = (ConsumidorArgs *)arg;
    ColaCircularMonitor *cola = args->cola;
    int tiempo_espera = args->tiempo_espera;

    int item;
    while ((item = consumir(cola, tiempo_espera)) != -1) {
        usleep(150000);
    }
    fprintf(cola->log, "Consumidor finaliza por falta de items o tiempo de espera agotado.\n");
    fflush(cola->log);
    return NULL;
}

int main(int argc, char *argv[]) {
    int num_productores = 0, num_consumidores = 0, tamano_inicial = 0, tiempo_espera = 0;
    
    // Procesamiento de los argumentos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            num_productores = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-c") == 0) {
            num_consumidores = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0) {
            tamano_inicial = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0) {
            tiempo_espera = atoi(argv[++i]);
        }
    }

    if (num_productores <= 0 || num_consumidores <= 0 || tamano_inicial <= 0 || tiempo_espera <= 0) {
        fprintf(stderr, "Uso: %s -p <num_productores> -c <num_consumidores> -s <tamano_inicial> -t <tiempo_espera>\n", argv[0]);
        return 1;
    }

    ColaCircularMonitor cola;
    inicializarCola(&cola, tamano_inicial, num_productores);

    pthread_t productores[num_productores], consumidores[num_consumidores];
    ConsumidorArgs args = {&cola, tiempo_espera};

    for (int i = 0; i < num_productores; i++) {
        pthread_create(&productores[i], NULL, productor, (void *)&cola);
    }

    for (int i = 0; i < num_consumidores; i++) {
        pthread_create(&consumidores[i], NULL, consumidor, (void *)&args);
    }

    for (int i = 0; i < num_productores; i++) {
        pthread_join(productores[i], NULL);
    }

    for (int i = 0; i < num_consumidores; i++) {
        pthread_join(consumidores[i], NULL);
    }

    destruirCola(&cola);
    return 0;
}

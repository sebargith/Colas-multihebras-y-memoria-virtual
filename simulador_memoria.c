#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define TAMANO_HASH 10 // Tamaño de la tabla hash, puedes ajustarlo según las necesidades

// Estructura para representar una página en la memoria virtual
typedef struct Pagina {
    int numero_pagina;
    struct Pagina *siguiente;
} Pagina;

// Estructura para la tabla hash de páginas
typedef struct TablaHash {
    Pagina *entradas[TAMANO_HASH];
} TablaHash;

// Estructura para representar un marco de memoria
typedef struct {
    int numero_pagina;
    int ultimo_uso; // Usado para LRU
    int referencia; // Usado para Reloj
} Marco;

// Función de hash
int funcion_hash(int numero_pagina) {
    return numero_pagina % TAMANO_HASH;
}

// Crear la tabla hash
TablaHash *crear_tabla_hash() {
    TablaHash *tabla = (TablaHash *)malloc(sizeof(TablaHash));
    for (int i = 0; i < TAMANO_HASH; i++) {
        tabla->entradas[i] = NULL;
    }
    return tabla;
}

// Función para leer referencias de un archivo
int *leer_referencias(const char *filename, int *num_referencias) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error al abrir el archivo de referencias");
        exit(1);
    }

    int *referencias = (int *)malloc(100 * sizeof(int)); // Tamaño inicial
    *num_referencias = 0;

    while (fscanf(file, "%d", &referencias[*num_referencias]) == 1) {
        (*num_referencias)++;
        if (*num_referencias % 100 == 0) { // Redimensionar si es necesario
            referencias = realloc(referencias, (*num_referencias + 100) * sizeof(int));
        }
    }

    fclose(file);
    return referencias;
}

// Algoritmo FIFO de reemplazo de páginas
int reemplazo_fifo(int *fifo_indice, int num_marcos) {
    int reemplazo = *fifo_indice;
    *fifo_indice = (*fifo_indice + 1) % num_marcos;
    return reemplazo;
}

// Algoritmo LRU de reemplazo de páginas
int reemplazo_lru(Marco *marcos, int num_marcos) {
    int pagina_a_reemplazar = 0;
    int min_ultimo_uso = marcos[0].ultimo_uso;

    for (int i = 1; i < num_marcos; i++) {
        if (marcos[i].ultimo_uso < min_ultimo_uso) {
            min_ultimo_uso = marcos[i].ultimo_uso;
            pagina_a_reemplazar = i;
        }
    }
    return pagina_a_reemplazar;
}

// Algoritmo de Reloj para reemplazo de páginas
int reemplazo_reloj(Marco *marcos, int num_marcos, int *reloj_puntero) {
    while (true) {
        if (marcos[*reloj_puntero].referencia == 0) {
            int reemplazo = *reloj_puntero;
            *reloj_puntero = (*reloj_puntero + 1) % num_marcos;
            return reemplazo;
        }
        marcos[*reloj_puntero].referencia = 0;
        *reloj_puntero = (*reloj_puntero + 1) % num_marcos;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Uso: %s -m <num_marcos> -a <algoritmo> -f <archivo_referencias>\n", argv[0]);
        return 1;
    }

    int num_marcos = 0;
    char algoritmo[10];
    char archivo_referencias[50];
    int contador_fallos = 0; // Contador de fallos de página

    // Procesar los argumentos de la línea de comandos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            num_marcos = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-a") == 0) {
            strcpy(algoritmo, argv[++i]);
        } else if (strcmp(argv[i], "-f") == 0) {
            strcpy(archivo_referencias, argv[++i]);
        }
    }

    if (num_marcos <= 0) {
        fprintf(stderr, "El número de marcos debe ser mayor que 0\n");
        return 1;
    }

    // Leer referencias del archivo
    int num_referencias;
    int *referencias = leer_referencias(archivo_referencias, &num_referencias);

    // Inicializar marcos y variables de algoritmo
    Marco *marcos = (Marco *)malloc(num_marcos * sizeof(Marco));
    for (int i = 0; i < num_marcos; i++) {
        marcos[i].numero_pagina = -1; // Marco vacío al inicio
        marcos[i].ultimo_uso = 0;
        marcos[i].referencia = 0;
    }

    int fifo_indice = 0;
    int reloj_puntero = 0;
    int contador_uso = 0; // Para LRU

    // Simulación de reemplazo de páginas
    for (int i = 0; i < num_referencias; i++) {
        int pagina_actual = referencias[i];
        int encontrado = 0;

        // Verificar si la página ya está en memoria
        for (int j = 0; j < num_marcos; j++) {
            if (marcos[j].numero_pagina == pagina_actual) {
                encontrado = 1;
                marcos[j].ultimo_uso = contador_uso++; // Actualizar para LRU
                marcos[j].referencia = 1; // Actualizar para Reloj
                break;
            }
        }

        // Si la página no está en memoria, ocurre un fallo de página
        if (!encontrado) {
            contador_fallos++;

            int indice_reemplazo;
            if (strcmp(algoritmo, "FIFO") == 0) {
                indice_reemplazo = reemplazo_fifo(&fifo_indice, num_marcos);
            } else if (strcmp(algoritmo, "LRU") == 0) {
                indice_reemplazo = reemplazo_lru(marcos, num_marcos);
            } else if (strcmp(algoritmo, "Clock") == 0) {
                indice_reemplazo = reemplazo_reloj(marcos, num_marcos, &reloj_puntero);
            } else {
                fprintf(stderr, "Algoritmo de reemplazo no reconocido: %s\n", algoritmo);
                free(referencias);
                free(marcos);
                return 1;
            }

            // Reemplazar la página en el marco seleccionado
            marcos[indice_reemplazo].numero_pagina = pagina_actual;
            marcos[indice_reemplazo].ultimo_uso = contador_uso++;
            marcos[indice_reemplazo].referencia = 1;

            printf("Fallo de página: Página %d cargada en el marco %d\n", pagina_actual, indice_reemplazo);
        }
    }

    printf("Total de fallos de página: %d\n", contador_fallos);

    // Liberar memoria
    free(referencias);
    free(marcos);
    return 0;
}

# **Colas Multihebras y Memoria Virtual**
### Tarea 2 de Sistemas Operativos

---

### **Integrantes**
- **Sebastián Rosas**  
- **Nicolás Ricciardi**  
- **Enzo Levancini**  
- **Matías Cruces**  

---

## **Requisitos**
- Compilador de C compatible con POSIX.
- Sistema Operativo Linux.
- Archivo de referencias para la Parte 2 (simulador de memoria).

---

## **Instrucciones de Compilación**


gcc simulador_cola.c -o simulador_cola

gcc simulador_memoria.c -o simulador_memoria

---

## **Uso**

**Simulador de cola**

**./simulador_cola -p <num_productores> -c <num_consumidores> -s <tamano_inicial> -t <tiempo_espera>**

-p: Número de hilos productores.

-c: Número de hilos consumidores.

-s: Tamaño inicial de la cola circular.

-t: Tiempo de espera máximo para los consumidores (en segundos).

---

**Simulador de memoria:**

**./simulador_memoria -m <num_marcos> -a <algoritmo> -f <archivo_referencias>**

-m: Número de marcos de memoria disponibles.

-a: Algoritmo de reemplazo de páginas a utilizar:

   FIFO
   
   LRU
   
   Clock
   
-f: Ruta al archivo de texto con las referencias de páginas.


---



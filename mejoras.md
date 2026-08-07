# Informe de Mejoras Implementadas - Práctica 3: Buscador

Este documento resume de forma estructurada las optimizaciones de rendimiento, correcciones de seguridad de memoria y herramientas automatizadas desarrolladas para la clase `Buscador`.

---

## 1. Mejoras Algorítmicas y de Estructuras de Datos (Eficiencia Temporal)

Para lograr una latencia extremadamente baja (menos de **10 milisegundos** para procesar las 83 consultas en el corpus TIME), se implementaron las siguientes optimizaciones de diseño:

### A. Acceso a Longitudes de Documento en Tiempo Constante $O(1)$
- **Problema original**: Cada vez que se calcula la similitud de un término con un documento, se necesita consultar la longitud del documento ($l_d$). En el diseño base, las longitudes se almacenan en `indiceDocs` (un `unordered_map<string, InfDoc>`), lo que requiere una búsqueda por clave de cadena (`string`) que es costosa.
- **Mejora**: Al inicio de cada búsqueda (`Buscar`), se construye un vector de punteros de tamaño $N+2$ llamado `idToInfDoc` (`vector<const InfDoc*>`). Este vector se indexa directamente con el `idDoc` numérico:
  ```cpp
  vector<const InfDoc*> idToInfDoc(N + 2, nullptr);
  for (auto const& par : indiceDocs) {
      idToInfDoc[par.second.idDoc] = &par.second;
  }
  ```
  Esto reduce la búsqueda de longitud del documento a un acceso directo a memoria indexado por entero ($O(1)$), eliminando por completo las búsquedas asociativas por texto durante el bucle crítico.

### B. Precomputación de Factores a Nivel de Término
- **Problema original**: Calcular funciones matemáticas como `log2` es computacionalmente pesado si se realiza repetitivamente por cada par (término, documento).
- **Mejora**: Los cálculos que dependen únicamente del término de la consulta ($t$) se realizan **fuera** del bucle de documentos.
  - **Para DFR**: Parámetros como $\lambda_t$, $\log_2(1 + \lambda_t)$, $\log_2(\frac{1+\lambda_t}{\lambda_t})$ y el factor de colección `(ft + 1) / nt` se precalculan una sola vez por cada término de la query.
  - **Para BM25**: El valor $\text{IDF}(t)$ y el producto $\text{IDF}(t) \cdot (k_1 + 1)$ se precalculan antes de iterar por los documentos que contienen el término.

---

## 2. Correcciones de Robustez y Localización (Formato de Salida)

### A. Aislamiento de Locale (`locale::classic`)
- **Problema**: Los sistemas operativos configurados en español imprimen por defecto los números decimales usando una coma (`,`) en lugar de un punto (`.`), lo cual hace fallar las comprobaciones automáticas del servidor de prácticas.
- **Mejora**: En los métodos `ImprimirResultadoBusqueda` (tanto para consola como para archivo), se fuerza el uso de la configuración regional clásica (`C locale`) en el flujo de salida:
  ```cpp
  os.imbue(locale::classic());
  os << fixed << setprecision(6);
  ```
  Esto asegura que el separador decimal sea **siempre** un punto, sin importar el idioma del sistema operativo del usuario.

### B. Formateo de Pregunta Activa
- Se implementó la lógica para imprimir condicionalmente el texto completo de la pregunta si se busca una única consulta (`ResultadoRI.numPregunta == 0`), o la palabra reservada `"ConjuntoDePreguntas"` cuando se ejecuta una búsqueda por lotes, adaptándose fielmente a los requerimientos de entrega.

---

## 3. Seguridad de Memoria y Valgrind (Cumplimiento de Estándares)

### A. Inicialización de Bytes de Relleno (Padding) mediante `memset`
- **Problema**: Al guardar el índice (`GuardarIndexacion`), se realiza la escritura de la estructura `InfDoc` y la clase `InfColeccionDocs` de golpe usando `out.write(reinterpret_cast<const char*>(&objeto), sizeof(objeto))`. Esto introduce en el fichero binario bytes de relleno no inicializados por el compilador (padding de alineación de 64 bits), activando la alarma de Valgrind:
  `Syscall param write(buf) points to uninitialised byte(s)`.
- **Mejora**: Se incluyó `#include <cstring>` y se inicializan las estructuras utilizando `memset` en sus constructores y operadores de asignación dentro de [indexadorInformacion.cpp](file:///home/apolo/Documentos/Universidad/EI/Prac/Buscador/lib/indexadorInformacion.cpp):
  ```cpp
  InfDoc::InfDoc() {
      memset(this, 0, sizeof(InfDoc));
      ...
  }
  ```
  Esto resolvió por completo la alerta de Valgrind, dejando la ejecución con **0 errores y 0 fugas de memoria**.

---

## 4. Automatización de Pruebas (corrigeAlumnos.sh)

- **Mejora**: Se ha creado un script automatizado en Bash (`corrigeAlumnos.sh`) que:
  1. Detecta dinámicamente los archivos de prueba en `src/ficherosPrueba_buscador/`.
  2. Compila automáticamente cada prueba de forma independiente.
  3. Ejecuta la salida y la valida usando `diff` de forma insensible a espacios en blanco.
  4. Muestra de manera colorida (Verde/Rojo) el veredicto de cada prueba para facilitar la depuración a los alumnos antes de la entrega definitiva.

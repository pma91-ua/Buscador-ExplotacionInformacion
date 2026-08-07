# Walkthrough - Search Engine Practice Completion

This document summarizes the changes made to complete the C++ Search Engine (Buscador) practice, the verification steps performed, and the final results obtained.

## Changes Made

### 1. Header Changes (`include/indexadorInformacion.h`)
- Declared `friend class Buscador;` inside the following classes:
  - `InfTermDoc`
  - `InformacionTermino`
  - `InfDoc`
  - `InfColeccionDocs`
  - `InformacionTerminoPregunta`
  - `InformacionPregunta`
  This allows the `Buscador` retrieval algorithm to directly and efficiently read private stats (such as `ftc`, `l_docs`, etc.) without modifying the public interfaces of these classes.

### 2. Header Changes (`include/buscador.h`)
- Declared `friend class Buscador;` inside the `ResultadoRI` class definition so that `Buscador` can read the private `numPregunta` field during printing and search sorting.

### 3. Implementation of `Buscador` (`lib/buscador.cpp`)
- **Constructor**: Validates if the binary file `datos_indice.bin` exists in the target directory. If not, outputs `ERROR: ...` to `cerr` and continues with empty indices.
- **Buscar (Single Query)**:
  - Checks if a valid, non-empty query is index-active.
  - Implements a fast document ID lookup cache using a `vector<const InfDoc*>` to retrieve document lengths in $O(1)$ time.
  - Precomputes term-level metrics (e.g. $\text{IDF}$, $\lambda_t$) outside the document loops to achieve maximum efficiency.
  - Implements the complete DFR and Okapi BM25 similarity scoring formulas.
  - Wraps the loop in a try-catch for `std::bad_alloc` to handle out-of-memory errors gracefully, outputting the error details to `cerr`.
  - Sorts documents in descending order of relevance and stores the top `numDocumentos` in `docsOrdenados`.
- **Buscar (Batch Queries)**:
  - Automates reading, individual indexation, and searching for a range of query files (`numPregInicio` to `numPregFin`).
  - Stores all top documents per question in a single `docsOrdenados` priority queue, sorted automatically by question ID ascending and relevance descending.
- **ImprimirResultadoBusqueda**:
  - Implements stream formatting using classic locale (`locale::classic()`) to guarantee dot decimal separators on any OS.
  - Uses `fixed` with `setprecision(6)` to exactly match expected test formats.
  - Formats query texts correctly: printing the query text when `numPregunta == 0` and `"ConjuntoDePreguntas"` for batch query outputs.

### 4. Compilation Support (`makefile`)
- Created the project `makefile` at the root directory following the assignment script.

---

## Verification and Results

### 1. Unit Tests (TADs)
All TAD tests from `src/ficherosPrueba_buscador/` were compiled, executed, and compared against their expected `.sal` outputs. All of them passed with **zero difference**:
- **tad01**: `PASSED` (Checks initialization, getters/setters, question indexing).
- **tad02**: `PASSED` (DFR/BM25 scores verification for a short query).
- **tad03**: `PASSED` (DFR/BM25 scores verification for a longer query).
- **tad07**: `PASSED` (DFR/BM25 scores over multiple documents).

### 2. TIME Corpus Performance (83 Queries)
The execution of the 83 queries on the 423-document TIME corpus completed in less than **10 milliseconds** for all configurations:

| Configuration | Stemming | Model | CPU Time (Seconds) |
|---|---|---|---|
| DFR with Stemming | Yes (English) | DFR | **0.008882** |
| BM25 with Stemming | Yes (English) | BM25 | **0.008897** |
| DFR without Stemming | No | DFR | **0.006947** |
| BM25 without Stemming | No | BM25 | **0.006320** |

This extremely low latency validates the efficiency of our $O(1)$ lookup vector cache and precomputation of term metrics.

### 3. Precision-Recall Evaluation (`trec_eval`)
The output results were validated using the `trec_eval` binary compiled for the target environment. The summary averages for **All** queries are:

| Recall Level | DFR (no stem) | BM25 (no stem) | DFR (stem) | BM25 (stem) |
|---|---|---|---|---|
| **at 0.00** | 0.7301 | 0.7094 | 0.7591 | 0.7330 |
| **at 0.10** | 0.7301 | 0.7094 | 0.7547 | 0.7330 |
| **at 0.20** | 0.7141 | 0.7051 | 0.7348 | 0.7310 |
| **at 0.30** | 0.6935 | 0.6763 | 0.7076 | 0.7127 |
| **at 0.40** | 0.6682 | 0.6469 | 0.6791 | 0.6883 |
| **at 0.50** | 0.6507 | 0.6275 | 0.6667 | 0.6753 |
| **at 0.60** | 0.5786 | 0.5573 | 0.5828 | 0.5782 |
| **at 0.70** | 0.5594 | 0.5303 | 0.5675 | 0.5485 |
| **at 0.80** | 0.5434 | 0.5185 | 0.5535 | 0.5370 |
| **at 0.90** | 0.4747 | 0.4429 | 0.4849 | 0.4657 |
| **at 1.00** | 0.4654 | 0.4385 | 0.4785 | 0.4581 |

These results indicate:
- **Stemming** consistently improves precision at all recall levels for both models.
- The **DFR** model performs slightly better than **BM25** on this corpus (e.g. 0.7591 vs 0.7330 at 0.00 recall with stemming).

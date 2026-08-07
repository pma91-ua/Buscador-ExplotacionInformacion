// Pablo Mira Amante 50504059S
#include "buscador.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <algorithm>

// =========================================================================
// IMPLEMENTACIa“N DE LA CLASE ResultadoRI
// =========================================================================

ResultadoRI::ResultadoRI(const double& kvSimilitud, const long int& kidDoc, const int& np) {
    vSimilitud = kvSimilitud;
    idDoc = kidDoc;
    numPregunta = np;
}

double ResultadoRI::VSimilitud() const {
    return vSimilitud;
}

long int ResultadoRI::IdDoc() const {
    return idDoc;
}

// Determina como ordena la priority_queue.
bool ResultadoRI::operator<(const ResultadoRI& lhs) const {
    if (numPregunta == lhs.numPregunta) {
        return (vSimilitud < lhs.vSimilitud); // Devuelve true si 'this' es MENOR, los MAYORES quedan arriba
    }
    return (numPregunta > lhs.numPregunta); // En caso de distinta pregunta, ordena por el numero de esta
}

ostream& operator<<(ostream &os, const ResultadoRI &res) {
    os << res.vSimilitud << "\t\t" << res.idDoc << "\t" << res.numPregunta << "\n";
    return os;
}


// =========================================================================
// 2. IMPLEMENTACION DE LA CLASE Buscador
// =========================================================================

// Constructor Privado por defecto (obligatorio por el enunciado)
Buscador::Buscador() : IndexadorHash() {
    formSimilitud = 0;
    c = 2.0;
    k1 = 1.2;
    b = 0.75;
}

// Constructor Principal
Buscador::Buscador(const string& directorioIndexacion, const int& f) 
    : IndexadorHash(directorioIndexacion) { // Hereda e inicializa la clase base
    
    formSimilitud = f;
    c = 2.0;
    k1 = 1.2;
    b = 0.75;

    // Comprobar si la indexacion existe en el directorio
    string archivo = directorioIndexacion + "/datos_indice.bin";
    ifstream in(archivo, ios::binary);
    if (!in) {
        cerr << "ERROR: No se pudo recuperar la indexacion desde el directorio: " << directorioIndexacion << "\n";
    } else {
        in.close();
    }
}

// Constructor de copia
Buscador::Buscador(const Buscador& busc) 
    : IndexadorHash(busc) { // Copia de la parte del Indexador
    
    formSimilitud = busc.formSimilitud; 
    c = busc.c;
    k1 = busc.k1;
    b = busc.b;
    docsOrdenados = busc.docsOrdenados;
}

// Destructor
Buscador::~Buscador() {
    // Al no usar punteros dinamicos manuales en buscador, se deja vaci­o
}

// Operador de asignacion
Buscador& Buscador::operator=(const Buscador& busc) {
    if (this != &busc) {
        IndexadorHash::operator=(busc); // Asignacion de la clase base
        formSimilitud = busc.formSimilitud;
        c = busc.c;
        k1 = busc.k1;
        b = busc.b;
        docsOrdenados = busc.docsOrdenados;
    }
    return *this;
}

// =========================================================================
// METODOS DE CONFIGURACION (GETTERS Y SETTERS)
// =========================================================================

int Buscador::DevolverFormulaSimilitud() const {
    return formSimilitud;
}

bool Buscador::CambiarFormulaSimilitud(const int& f) {
    if (f == 0 || f == 1) { // Validamos que sea DFR (0) o BM25 (1)
        formSimilitud = f;
        return true;
    }
    return false;
}

void Buscador::CambiarParametrosDFR(const double& kc) {
    c = kc;
}

double Buscador::DevolverParametrosDFR() const {
    return c;
}

void Buscador::CambiarParametrosBM25(const double& kk1, const double& kb) {
    k1 = kk1;
    b = kb;
}

void Buscador::DevolverParametrosBM25(double& kk1, double& kb) const {
    kk1 = k1; // Se devuelven por referencia
    kb = b;
}

// =========================================================================
// 4. METODOS PRINCIPALES DE BUSQUEDA
// =========================================================================

bool Buscador::Buscar(const int& numDocumentos) {
    // 1. Limpieza y validación
    // Vaciar la cola de resultados
    while(!docsOrdenados.empty()) {
        docsOrdenados.pop();
    }

    // Comprobar si hay pregunta y obtenerla
    string preg;
    if (!DevuelvePregunta(preg) || preg.empty() || indicePregunta.empty()) {
        return false; // No hay pregunta o esta vacia
    }

    int N = informacionColeccionDocs.numDocs;
    if (N == 0) {
        return false;
    }

    int k = infPregunta.numTotalPalSinParada;
    if (k == 0) {
        return false;
    }

    // 2. Generacion de cache de acceso directo
    unordered_map<long int, double> scoresDocumentos;
    string currentTerm = "";
    long int currentDocId = -1;

    try {
        // Mapeo rapido de ID de documento a InfDoc
        vector<const InfDoc*> idToInfDoc(N + 2, nullptr);
        for (auto const& par : indiceDocs) {
            idToInfDoc[par.second.idDoc] = &par.second;
        }

        double avr_ld = (double)informacionColeccionDocs.numTotalPalSinParada / N;

        // 4. Precomputaciones a nivel de termino
        for (auto const& parPreg : indicePregunta) {
            currentTerm = parPreg.first;
            
            auto itColeccion = indice.find(currentTerm);
            if (itColeccion != indice.end()) { // Si el termino existe en la coleccion
                int ft = itColeccion->second.ftc;
                int nt = itColeccion->second.l_docs.size();

                if (nt == 0) continue;

                // variables para aplicar los diferentes algoritmos
                double wt_q = 0.0;
                double idf = 0.0;
                double idf_times_k1_plus_1 = 0.0;
                double lambda_t = 0.0;
                double log2_1_lambda = 0.0;
                double log2_ratio = 0.0;
                double term_factor = 0.0;

                if (formSimilitud == 0) {
                    wt_q = (double)parPreg.second.ft / k;
                    lambda_t = (double)ft / N;
                    log2_1_lambda = log2(1.0 + lambda_t);
                    log2_ratio = log2((1.0 + lambda_t) / lambda_t);
                    term_factor = (double)(ft + 1) / nt;
                } else if (formSimilitud == 1) {
                    double num_idf = N - nt + 0.5;
                    double den_idf = nt + 0.5;
                    idf = log2(num_idf / den_idf);
                    idf_times_k1_plus_1 = idf * (k1 + 1.0);
                }

                // 4. Evaluacion de los documentos
                for (auto const& docPair : itColeccion->second.l_docs) {
                    currentDocId = docPair.first;
                    int ft_d = docPair.second.ft;

                    // Obtener longitud del documento
                    int ld = 0;
                    if (currentDocId >= 0 && currentDocId < (int)idToInfDoc.size() && idToInfDoc[currentDocId] != nullptr) {
                        ld = idToInfDoc[currentDocId]->numPalSinParada;
                    }

                    if (ld <= 0) continue;

                    double scoreTermino = 0.0;
                    if (formSimilitud == 0) {
                        // Formula DFR
                        double f_td_star = ft_d * log2(1.0 + c * avr_ld / ld);
                        double w_td = (log2_1_lambda + f_td_star * log2_ratio) * (term_factor / (f_td_star + 1.0));
                        scoreTermino = wt_q * w_td;
                    } else if (formSimilitud == 1) {
                        // Formula BM25
                        double K = k1 * (1.0 - b + b * (double)ld / avr_ld);
                        scoreTermino = idf_times_k1_plus_1 * ft_d / (ft_d + K);
                    }

                    scoresDocumentos[currentDocId] += scoreTermino;
                }
            }
        }
    } catch (const std::bad_alloc& e) {
        cerr << "ERROR: Falta de memoria en la busqueda. Termino: " << currentTerm << ", Documento ID: " << currentDocId << "\n";
        return false;
    }

    // 5. Ordenacion de los resultados
    vector<ResultadoRI> resVec;
    resVec.reserve(scoresDocumentos.size());
    for (auto const& par : scoresDocumentos) {
        resVec.push_back(ResultadoRI(par.second, par.first, 0));
    }

    sort(resVec.begin(), resVec.end(), [](const ResultadoRI& a, const ResultadoRI& b) {
        return b < a; // b < a nos devuelve descendente por relevancia
    });

    // 6. Volcado final
    int limit = min((int)resVec.size(), numDocumentos);
    for (int i = 0; i < limit; i++) {
        docsOrdenados.push(resVec[i]);
    }

    return true; 
}

bool Buscador::Buscar(const string& dirPreguntas, const int& numDocumentos, const int& numPregInicio, const int& numPregFin) {
    // 1. Limpieza y optimizacion global
    while(!docsOrdenados.empty()) {
        docsOrdenados.pop();
    }

    int N = informacionColeccionDocs.numDocs;
    if (N == 0) {
        return false;
    }

    // Mapeo rapido de ID de documento a InfDoc
    vector<const InfDoc*> idToInfDoc(N + 2, nullptr);
    for (auto const& par : indiceDocs) {
        idToInfDoc[par.second.idDoc] = &par.second;
    }

    double avr_ld = (double)informacionColeccionDocs.numTotalPalSinParada / N;

    // 2. Bucle iterativo de lectura e indexacion dinamica
    for (int i = numPregInicio; i <= numPregFin; i++) {
        string path = dirPreguntas + "/" + to_string(i) + ".txt";
        ifstream file(path);
        if (!file.is_open()) {
            cerr << "ERROR: No existe el documento a indexar: " << path << "\n";
            return false;
        }

        string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();

        // Indexar pregunta individual
        if (!IndexarPregunta(content)) {
            continue; // Si esta vaci­a o no tiene palabras con contenido, se ignora
        }

        int k = infPregunta.numTotalPalSinParada;
        if (k == 0) continue;

        unordered_map<long int, double> scoresDocumentos;
        string currentTerm = "";
        long int currentDocId = -1;

        // 3. Evaluacion matematica y acumulacion local de puntuaciones
        try {
            for (auto const& parPreg : indicePregunta) {
                currentTerm = parPreg.first;
                
                auto itColeccion = indice.find(currentTerm);
                if (itColeccion != indice.end()) {
                    int ft = itColeccion->second.ftc;
                    int nt = itColeccion->second.l_docs.size();

                    if (nt == 0) continue;

                    double wt_q = 0.0;
                    double idf = 0.0;
                    double idf_times_k1_plus_1 = 0.0;
                    double lambda_t = 0.0;
                    double log2_1_lambda = 0.0;
                    double log2_ratio = 0.0;
                    double term_factor = 0.0;

                    if (formSimilitud == 0) {
                        wt_q = (double)parPreg.second.ft / k;
                        lambda_t = (double)ft / N;
                        log2_1_lambda = log2(1.0 + lambda_t);
                        log2_ratio = log2((1.0 + lambda_t) / lambda_t);
                        term_factor = (double)(ft + 1) / nt;
                    } else if (formSimilitud == 1) {
                        double num_idf = N - nt + 0.5;
                        double den_idf = nt + 0.5;
                        idf = log2(num_idf / den_idf);
                        idf_times_k1_plus_1 = idf * (k1 + 1.0);
                    }

                    for (auto const& docPair : itColeccion->second.l_docs) {
                        currentDocId = docPair.first;
                        int ft_d = docPair.second.ft;

                        int ld = 0;
                        if (currentDocId >= 0 && currentDocId < (int)idToInfDoc.size() && idToInfDoc[currentDocId] != nullptr) {
                            ld = idToInfDoc[currentDocId]->numPalSinParada;
                        }

                        if (ld <= 0) continue;

                        double scoreTermino = 0.0;
                        if (formSimilitud == 0) {
                            double f_td_star = ft_d * log2(1.0 + c * avr_ld / ld);
                            double w_td = (log2_1_lambda + f_td_star * log2_ratio) * (term_factor / (f_td_star + 1.0));
                            scoreTermino = wt_q * w_td;
                        } else if (formSimilitud == 1) {
                            double K = k1 * (1.0 - b + b * (double)ld / avr_ld);
                            scoreTermino = idf_times_k1_plus_1 * ft_d / (ft_d + K);
                        }

                        scoresDocumentos[currentDocId] += scoreTermino;
                    }
                }
            }
        } catch (const std::bad_alloc& e) {
            cerr << "ERROR: Falta de memoria en la busqueda. Termino: " << currentTerm << ", Pregunta: " << i << ", Documento ID: " << currentDocId << "\n";
            return false;
        }

        // 4. Ordenacion local de los resultados
        vector<ResultadoRI> resVec;
        resVec.reserve(scoresDocumentos.size());
        for (auto const& par : scoresDocumentos) {
            resVec.push_back(ResultadoRI(par.second, par.first, i));
        }

        sort(resVec.begin(), resVec.end(), [](const ResultadoRI& a, const ResultadoRI& b) {
            return b < a;
        });

        // 5. Volvado final
        int limit = min((int)resVec.size(), numDocumentos);
        for (int j = 0; j < limit; j++) {
            docsOrdenados.push(resVec[j]);
        }
    }

    return true;
}

// Funcion auxiliar estatica para extraer el nombre del fichero sin camino ni extension
static string getDocName(const string& fullPath) {
    size_t lastSlash = fullPath.find_last_of("/\\");
    size_t lastDot = fullPath.find_last_of('.');
    
    size_t start = (lastSlash == string::npos) ? 0 : lastSlash + 1;
    size_t end = (lastDot == string::npos || lastDot < start) ? fullPath.length() : lastDot;
    
    return fullPath.substr(start, end - start);
}

void Buscador::ImprimirResultadoBusqueda(const int& numDocumentos) const {
    priority_queue<ResultadoRI> copyQueue = docsOrdenados;
    
    // Mapeo rapido de ID a ruta de documento
    vector<string> idToDocPath(informacionColeccionDocs.numDocs + 2);
    for (const auto& par : indiceDocs) {
        idToDocPath[par.second.idDoc] = par.first;
    }
    
    int currentPreg = -1;
    int printedForCurrentPreg = 0;
    string formulaName = (formSimilitud == 0) ? "DFR" : "BM25";
    
    // Forzar el locale classic para usar punto en los decimales
    ostream& os = cout;
    auto oldLocale = os.imbue(locale::classic());
    auto oldFlags = os.flags();
    
    os << fixed << setprecision(6);
    
    while (!copyQueue.empty()) {
        ResultadoRI res = copyQueue.top();
        copyQueue.pop();
        
        if (res.numPregunta != currentPreg) {
            currentPreg = res.numPregunta;
            printedForCurrentPreg = 0;
        }
        
        if (printedForCurrentPreg < numDocumentos) {
            string docPath = "";
            if (res.idDoc >= 0 && res.idDoc < (int)idToDocPath.size()) {
                docPath = idToDocPath[res.idDoc];
            }
            string docName = getDocName(docPath);
            
            string pregText = (res.numPregunta == 0) ? pregunta : "ConjuntoDePreguntas";
            
            os << res.numPregunta << " " 
               << formulaName << " " 
               << docName << " " 
               << printedForCurrentPreg << " " 
               << res.vSimilitud << " " 
               << pregText << "\n";
               
            printedForCurrentPreg++;
        }
    }
    
    os.imbue(oldLocale);
    os.flags(oldFlags);
}

bool Buscador::ImprimirResultadoBusqueda(const int& numDocumentos, const string& nombreFichero) const {
    ofstream os(nombreFichero);
    if (!os.is_open()) {
        return false;
    }
    
    priority_queue<ResultadoRI> copyQueue = docsOrdenados;
    
    vector<string> idToDocPath(informacionColeccionDocs.numDocs + 2);
    for (const auto& par : indiceDocs) {
        idToDocPath[par.second.idDoc] = par.first;
    }
    
    int currentPreg = -1;
    int printedForCurrentPreg = 0;
    string formulaName = (formSimilitud == 0) ? "DFR" : "BM25";
    
    os.imbue(locale::classic());
    os << fixed << setprecision(6);
    
    while (!copyQueue.empty()) {
        ResultadoRI res = copyQueue.top();
        copyQueue.pop();
        
        if (res.numPregunta != currentPreg) {
            currentPreg = res.numPregunta;
            printedForCurrentPreg = 0;
        }
        
        if (printedForCurrentPreg < numDocumentos) {
            string docPath = "";
            if (res.idDoc >= 0 && res.idDoc < (int)idToDocPath.size()) {
                docPath = idToDocPath[res.idDoc];
            }
            string docName = getDocName(docPath);
            
            string pregText = (res.numPregunta == 0) ? pregunta : "ConjuntoDePreguntas";
            
            os << res.numPregunta << " " 
               << formulaName << " " 
               << docName << " " 
               << printedForCurrentPreg << " " 
               << res.vSimilitud << " " 
               << pregText << "\n";
               
            printedForCurrentPreg++;
        }
    }
    
    os.close();
    return true;
}
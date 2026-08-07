//Pablo Mira Amante 50504059S
#include "indexadorHash.h"
#include <fstream>
#include <filesystem>
#include <sys/stat.h>
#include <sstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


// ==========================================
// Constructores y Destructor
// ==========================================

// Constructor por defecto
IndexadorHash::IndexadorHash() {
    pregunta = "";
    ficheroStopWords = "";
    directorioIndice = "";
    tipoStemmer = 0;
    almacenarPosTerm = false;
}

// Constructor principal
IndexadorHash::IndexadorHash(const string& fichStopWords, const string& delimitadores,
                             const bool& detectComp, const bool& minuscSinAcentos, 
                             const string& dirIndice, const int& tStemmer, const bool& almPosTerm) 
    : tok(delimitadores, detectComp, minuscSinAcentos)
{
    ficheroStopWords = fichStopWords;
    directorioIndice = dirIndice;
    tipoStemmer = tStemmer;
    almacenarPosTerm = almPosTerm;
    pregunta = "";

    // Mantener las tablas Hash rápidas y sin colisiones
    indice.max_load_factor(0.5);
    indiceDocs.max_load_factor(0.5);
    stopWords.max_load_factor(0.5);
    
    // Abrimos el archivo de palabras de parada
    ifstream archStopWords(ficheroStopWords);
    if (archStopWords.is_open()) {
        string palabra;
        stemmerPorter stemmer;

        while (getline(archStopWords, palabra)) {
            // Limpieza básica: quitar el retorno de carro (\r) si el archivo viene de Windows
            if (!palabra.empty() && palabra.back() == '\r') {
                palabra.pop_back();
            }

            if (!palabra.empty()) {
                stopWords.insert(palabra);
            }
        }
        archStopWords.close();
    } else {
        cerr << "ERROR: No se pudo abrir el fichero de palabras de parada: " << ficheroStopWords << '\n';
    }
}

// Constructor desde directorio (Recuperar Indexación)
IndexadorHash::IndexadorHash(const string& directorioIndexacion) {
    RecuperarIndexacion(directorioIndexacion);
}

// Constructor de copia
IndexadorHash::IndexadorHash(const IndexadorHash& other) {
    *this = other;
}

// Destructor
IndexadorHash::~IndexadorHash() {
    indice.clear();
    indiceDocs.clear();
    indicePregunta.clear();
    stopWords.clear();
}

// Operador de asignación
IndexadorHash& IndexadorHash::operator=(const IndexadorHash& other) {
    if (this != &other) {
        indice = other.indice;
        indiceDocs = other.indiceDocs;
        informacionColeccionDocs = other.informacionColeccionDocs;
        pregunta = other.pregunta;
        indicePregunta = other.indicePregunta;
        infPregunta = other.infPregunta;
        stopWords = other.stopWords;
        ficheroStopWords = other.ficheroStopWords;
        tok = other.tok;
        directorioIndice = other.directorioIndice;
        tipoStemmer = other.tipoStemmer;
        almacenarPosTerm = other.almacenarPosTerm;
    }
    return *this;
}

// ==========================================
// Métodos de Indexación Principales
// ==========================================

bool IndexadorHash::Indexar(const string& ficheroDocumentos) {
    if (indice.empty()) {
        indice.reserve(200000);
        indiceDocs.reserve(10000);
    }

    ifstream listaFicheros(ficheroDocumentos);
    if (!listaFicheros.is_open()) {
        cerr << "ERROR: No se pudo abrir el fichero con la lista de documentos: " << ficheroDocumentos << '\n';
        return false;
    }

    string nomDoc;
    stemmerPorter stemmer;

    while (getline(listaFicheros, nomDoc)) {
        if (!nomDoc.empty() && nomDoc.back() == '\r') nomDoc.pop_back();
        if (nomDoc.empty()) continue;

        // 1. Abrir el fichero
        int fd = open(nomDoc.c_str(), O_RDONLY);
        if (fd == -1) {
            cerr << "ERROR: No existe el documento a indexar: " << nomDoc << '\n';
            continue; // Caso "documento no existe": avisar y seguir, return true al final
        }

        struct stat fileInfo;
        if (fstat(fd, &fileInfo) != 0) {
            cerr << "ERROR: No se pudo obtener informacion del documento: " << nomDoc << '\n';
            close(fd);
            continue;
        }

        long int fechaSistema = fileInfo.st_mtime;
        int tamBytesDoc = fileInfo.st_size;
        int idActual = indiceDocs.size() + 1;

        // 2. Comprobar si ya estaba indexado
        auto itDoc = indiceDocs.find(nomDoc);
        if (itDoc != indiceDocs.end()) {
            long int fechaIndexada = itDoc->second.getFechaModificacion().sys_time;

            if (fechaSistema > fechaIndexada) {
                // El fichero en disco es MÁS RECIENTE que el indexado: reindexar
                cerr << "ERROR: El documento " << nomDoc 
                     << " ya estaba indexado y ha sido modificado, se reindexara.\n";
                idActual = itDoc->second.getIdDoc(); // Conservar el mismo idDoc
                BorraDoc(nomDoc);
            } else {
                // Mismo fichero o más antiguo: avisar pero NO reindexar
                cerr << "ERROR: El documento " << nomDoc 
                     << " ya estaba indexado y no ha sido modificado.\n";
                close(fd);
                continue;
            }
        }

        // 3. Leer el contenido con mmap
        string textoDocumento;
        if (tamBytesDoc > 0) {
            char* file_in_memory = static_cast<char*>(
                mmap(nullptr, tamBytesDoc, PROT_READ, MAP_PRIVATE, fd, 0)
            );
            if (file_in_memory != MAP_FAILED) {
                textoDocumento.assign(file_in_memory, tamBytesDoc);
                munmap(file_in_memory, tamBytesDoc);
            }
        }
        close(fd);

        // 4. Tokenizar
        list<string> tokensDoc;
        tok.Tokenizar(textoDocumento, tokensDoc);

        // 5. Preparar el nuevo InfDoc
        InfDoc nuevoDoc;
        nuevoDoc.setIdDoc(idActual);
        nuevoDoc.setTamBytes(tamBytesDoc);
        Fecha f; f.sys_time = fechaSistema;
        nuevoDoc.setFechaModificacion(f);

        int docNumPal = 0;
        int docNumPalSinParada = 0;
        int docNumPalDiferentes = 0;
        int posicionPalabra = 0;

        // 6. Procesar cada token
        for (string& token : tokensDoc) {
            docNumPal++;

            if (stopWords.find(token) != stopWords.end()) {
                posicionPalabra++;
                continue; // Es StopWord
            }

            if (tipoStemmer > 0) {
                stemmer.stemmer(token, tipoStemmer);
            }

            docNumPalSinParada++;

            InformacionTermino& infoTermGlobal = indice[token];
            if (infoTermGlobal.esNuevoEnDocumento(idActual)) {
                docNumPalDiferentes++;
            }

            infoTermGlobal.incrementarFtc();

            InfTermDoc& infoTermDelDoc = infoTermGlobal.obtenerOcrearInfTermDoc(idActual);
            infoTermDelDoc.incrementarFt();

            if (almacenarPosTerm) {
                infoTermDelDoc.addPosicion(posicionPalabra);
            }
            posicionPalabra++;
        }

        // 7. Guardar Estadísticas
        nuevoDoc.setNumPal(docNumPal);
        nuevoDoc.setNumPalSinParada(docNumPalSinParada);
        nuevoDoc.setNumPalDiferentes(docNumPalDiferentes);
        indiceDocs[nomDoc] = nuevoDoc;

        informacionColeccionDocs.addNumDocs(1);
        informacionColeccionDocs.addTamBytes(tamBytesDoc);
        informacionColeccionDocs.addTotalPal(docNumPal);
        informacionColeccionDocs.addTotalPalSinParada(docNumPalSinParada);
        informacionColeccionDocs.setTotalPalDiferentes(indice.size());
    }

    listaFicheros.close();
    return true; // Los casos de doc repetido/no existente no son error grave
}

bool IndexadorHash::IndexarDirectorio(const string& dirAIndexar) {
    if (!filesystem::exists(dirAIndexar) || !filesystem::is_directory(dirAIndexar)) {
        cerr << "ERROR: El directorio indicado no existe o no es valido: " << dirAIndexar << '\n';
        return false;
    }

    // 1. Creamos un archivo temporal para guardar la lista de rutas
    string nombreFicheroTemp = "temp_lista_dir.txt";
    ofstream tempFile(nombreFicheroTemp);
    if (!tempFile.is_open()) return false;

    // 2. Recorremos el directorio y apuntamos cada archivo
    for (const auto& entry : filesystem::recursive_directory_iterator(dirAIndexar)) {
        if (filesystem::is_regular_file(entry.status())) {
            tempFile << entry.path().string() << "\n";
        }
    }
    tempFile.close();

    // 3. Llamamos a tu método Indexar, que ya sabe qué hacer con una lista de archivos
    bool exito = Indexar(nombreFicheroTemp);

    // 4. Borramos el archivo temporal para no dejar basura en el disco
    remove(nombreFicheroTemp.c_str());

    return exito;
}

bool IndexadorHash::GuardarIndexacion() const {
    if (directorioIndice.empty()) return false;
    mkdir(directorioIndice.c_str(), 0777);

    string archivo = directorioIndice + "/datos_indice.bin";
    ofstream out(archivo, ios::binary);
    if (!out) return false;

    // Buffer de 1 MB para reducir llamadas al disco
    char bufferEscritura[1 << 20];
    out.rdbuf()->pubsetbuf(bufferEscritura, sizeof(bufferEscritura));

    auto writeString = [&](const string& s) {
        uint32_t len = s.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
        out.write(s.data(), len);
    };

    auto writeInt = [&](int v) {
        out.write(reinterpret_cast<const char*>(&v), sizeof(int));
    };

    auto writeUInt32 = [&](uint32_t v) {
        out.write(reinterpret_cast<const char*>(&v), sizeof(uint32_t));
    };

    // 1. Configuración
    writeString(ficheroStopWords);
    writeString(tok.DelimitadoresPalabra());
    writeInt(tipoStemmer);
    bool casos  = tok.CasosEspeciales();
    bool minusc = tok.PasarAminuscSinAcentos();
    out.write(reinterpret_cast<const char*>(&almacenarPosTerm), sizeof(bool));
    out.write(reinterpret_cast<const char*>(&casos),            sizeof(bool));
    out.write(reinterpret_cast<const char*>(&minusc),           sizeof(bool));

    // 2. InfColeccionDocs de golpe
    // Los 5 campos son ints contiguos en memoria, se escriben en una sola llamada
    out.write(reinterpret_cast<const char*>(&informacionColeccionDocs), sizeof(InfColeccionDocs));

    // 3. indiceDocs
    writeUInt32(static_cast<uint32_t>(indiceDocs.size()));
    for (const auto& par : indiceDocs) {
        writeString(par.first); // ruta del documento
        // InfDoc contiene solo ints y un long int contiguos: una sola llamada
        out.write(reinterpret_cast<const char*>(&par.second), sizeof(InfDoc));
    }

    // 4. Índice de términos
    writeUInt32(static_cast<uint32_t>(indice.size()));
    for (const auto& par : indice) {
        writeString(par.first); // término
        writeInt(par.second.ftc);

        writeUInt32(static_cast<uint32_t>(par.second.l_docs.size()));
        for (const auto& doc : par.second.l_docs) {
            writeInt(doc.first); // idDoc
            writeInt(doc.second.ft);

            writeUInt32(static_cast<uint32_t>(doc.second.posTerm.size()));
            // El vector de posiciones es memoria contigua: se vuelca de un golpe
            if (!doc.second.posTerm.empty()) {
                out.write(reinterpret_cast<const char*>(doc.second.posTerm.data()),
                          doc.second.posTerm.size() * sizeof(int));
            }
        }
    }

    out.close();
    return true;
}

bool IndexadorHash::RecuperarIndexacion(const string& directorioIndexacion) {
    VaciarIndiceDocs();
    VaciarIndicePreg();
    stopWords.clear();

    string archivo = directorioIndexacion + "/datos_indice.bin";
    ifstream in(archivo, ios::binary);
    if (!in) return false;

    // Buffer de 1 MB para reducir llamadas al disco
    char bufferLectura[1 << 20];
    in.rdbuf()->pubsetbuf(bufferLectura, sizeof(bufferLectura));

    auto readString = [&](string& s) {
        uint32_t len;
        in.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));
        s.resize(len);
        in.read(&s[0], len);
    };

    auto readInt = [&](int& v) {
        in.read(reinterpret_cast<char*>(&v), sizeof(int));
    };

    auto readUInt32 = [&](uint32_t& v) {
        in.read(reinterpret_cast<char*>(&v), sizeof(uint32_t));
    };

    // 1. Configuración
    readString(ficheroStopWords);
    string delim;
    readString(delim);
    readInt(tipoStemmer);
    bool casos, minusc;
    in.read(reinterpret_cast<char*>(&almacenarPosTerm), sizeof(bool));
    in.read(reinterpret_cast<char*>(&casos),            sizeof(bool));
    in.read(reinterpret_cast<char*>(&minusc),           sizeof(bool));
    tok.DelimitadoresPalabra(delim);
    tok.CasosEspeciales(casos);
    tok.PasarAminuscSinAcentos(minusc);

    // 2. Recargar stop-words desde el fichero original
    ifstream archSW(ficheroStopWords);
    if (archSW.is_open()) {
        string palabra;
        while (getline(archSW, palabra)) {
            if (!palabra.empty() && palabra.back() == '\r') palabra.pop_back();
            if (!palabra.empty()) stopWords.insert(palabra);
        }
        archSW.close();
    }

    // 3. InfColeccionDocs de golpe
    in.read(reinterpret_cast<char*>(&informacionColeccionDocs), sizeof(InfColeccionDocs));

    // 4. indiceDocs
    uint32_t numDocs;
    readUInt32(numDocs);
    indiceDocs.reserve(numDocs);
    for (uint32_t i = 0; i < numDocs; i++) {
        string nomDoc;
        readString(nomDoc);
        InfDoc doc;
        in.read(reinterpret_cast<char*>(&doc), sizeof(InfDoc));
        indiceDocs.emplace(move(nomDoc), move(doc));
    }

    // 5. Índice de términos
    uint32_t numTerminos;
    readUInt32(numTerminos);
    indice.reserve(numTerminos);
    for (uint32_t i = 0; i < numTerminos; i++) {
        string termino;
        readString(termino);
        InformacionTermino infTerm;
        readInt(infTerm.ftc);

        uint32_t numLDocs;
        readUInt32(numLDocs);
        infTerm.l_docs.reserve(numLDocs);
        for (uint32_t j = 0; j < numLDocs; j++) {
            int idDoc;
            InfTermDoc infTermDoc;
            readInt(idDoc);
            readInt(infTermDoc.ft);

            uint32_t numPos;
            readUInt32(numPos);
            infTermDoc.posTerm.resize(numPos);
            // El vector es memoria contigua: se lee de un golpe
            if (numPos > 0) {
                in.read(reinterpret_cast<char*>(infTermDoc.posTerm.data()),
                        numPos * sizeof(int));
            }
            infTerm.l_docs.push_back({idDoc, move(infTermDoc)});
        }
        indice.emplace(move(termino), move(infTerm));
    }

    in.close();
    directorioIndice = directorioIndexacion;
    return true;
}

// ==========================================
// Métodos de Preguntas
// ==========================================

bool IndexadorHash::IndexarPregunta(const string& preg) {
    // 1. Vaciar cualquier pregunta que se haya buscado antes
    VaciarIndicePreg();
    pregunta = preg;

    // 2. Extraer los tokens de la pregunta
    list<string> tokensPregunta;
    tok.Tokenizar(pregunta, tokensPregunta);

    stemmerPorter stemmer;
    int numPalabras = 0;
    int numPalSinParada = 0;
    int posicionActual = 0;

    // 3. Procesar palabra por palabra
    for (string& token : tokensPregunta) {
        numPalabras++;

        // Comprobar si es palabra de parada
        if (stopWords.find(token) != stopWords.end()) {
            posicionActual++; // Sumamos posiciÃ³n aunque sea stopword (lo exige el guion)
            continue;
        }

        //Aplicamos stemmer
        if (tipoStemmer > 0) {
            stemmer.stemmer(token, tipoStemmer);
        }

        // Es una palabra válida (con contenido)
        numPalSinParada++;

        // 4. Actualizar el í­ndice de la pregunta (InformacionTerminoPregunta)
        InformacionTerminoPregunta& infoTerm = indicePregunta[token];
        infoTerm.incrementarFt();
        
        if (almacenarPosTerm) {
            infoTerm.addPosicion(posicionActual);
        }

        posicionActual++;
    }

    if (indicePregunta.empty()) {
        cerr << "ERROR: La pregunta no contiene ningun termino con contenido." << '\n';
        return false; // Devolvamos false si la pregunta no tiene ningún término útil
    }

    // 5. Rellenar las estadÃ­sticas de la pregunta
    infPregunta.setNumTotalPal(numPalabras);
    infPregunta.setNumTotalPalSinParada(numPalSinParada);
    infPregunta.setNumTotalPalDiferentes(indicePregunta.size());

    return true;
}

bool IndexadorHash::DevuelvePregunta(string& preg) const {
    if (indicePregunta.empty()) return false;
    preg = pregunta;
    return true;
}

bool IndexadorHash::DevuelvePregunta(const string& word, InformacionTerminoPregunta& inf) const {
    list<string> tokens;
    tok.Tokenizar(word, tokens);
    if (tokens.empty()) return false;
    
    string palabraBuscada = tokens.front();

    auto it = indicePregunta.find(palabraBuscada);
    if (it != indicePregunta.end()) {
        inf = it->second;
        return true;
    }
    inf = InformacionTerminoPregunta();
    return false;
}

bool IndexadorHash::DevuelvePregunta(InformacionPregunta& inf) const {
    if (indicePregunta.empty()) {
        inf = InformacionPregunta();
        return false;
    }
    inf = infPregunta;
    return true;
}

// ==========================================
// Consultas e Impresión
// ==========================================

bool IndexadorHash::Devuelve(const string& word, InformacionTermino& inf) const {
    list<string> tokens;
    tok.Tokenizar(word, tokens);
    if (tokens.empty()) return false;
    
    string palabraBuscada = tokens.front();

    auto it = indice.find(palabraBuscada);
    if (it != indice.end()) {
        inf = it->second;
        return true;
    }
    inf = InformacionTermino();
    return false;
}

bool IndexadorHash::Devuelve(const string& word, const string& nomDoc, InfTermDoc& InfDocParam) const {
    list<string> tokens;
    tok.Tokenizar(word, tokens);
    if (tokens.empty()) return false;
    
    string palabraBuscada = tokens.front();

    auto itWord = indice.find(palabraBuscada);
    if (itWord != indice.end()) {
        auto itDoc = indiceDocs.find(nomDoc);
        if (itDoc != indiceDocs.end()) {
            int idDocumento = itDoc->second.getIdDoc(); 
            if (itWord->second.tieneDocumento(idDocumento, InfDocParam)) {
                return true;
            }
        }
    }
    InfDocParam = InfTermDoc();
    return false; 
}

bool IndexadorHash::Existe(const string& word) const {
    // 1. tokenizamos, normalizando todo
    list<string> tokens;
    tok.Tokenizar(word, tokens);
    
    // Si la palabra era un delimitador o un espacio, la lista estará vacía
    if (tokens.empty()) return false;
    
    string palabraBuscada = tokens.front();

    // 2. Buscar
    return indice.find(palabraBuscada) != indice.end();
}

bool IndexadorHash::BorraDoc(const string& nomDoc) {
    // 1. Comprobar si el documento realmente existe en nuestro í­ndice
    auto itDoc = indiceDocs.find(nomDoc);
    if (itDoc == indiceDocs.end()) {
        return false; // No está
    }

    // 2. Rescatar la información del documento antes de borrarlo
    int idDocBorrar = itDoc->second.getIdDoc();
    int tamBytesBorrar = itDoc->second.getTamBytes(); 
    int numPalBorrar = itDoc->second.getNumPal();     
    int numPalSinParadaBorrar = itDoc->second.getNumPalSinParada();

    // 3. Recorrer el í­ndice de términos para eliminar la presencia de este documento
    // Como vamos a borrar elementos del propio 'indice', usamos un bucle con iteradores cuidadoso
    for (auto itTerm = indice.begin(); itTerm != indice.end(); ) {
        
        if (itTerm->second.eliminarDocumento(idDocBorrar)) {
            
            // Si al eliminar el doc, este término se queda con 0 apariciones en la colección
            if (itTerm->second.getNumDocs() == 0) {
                // Lo borramos del í­ndice general y avanzamos el iterador
                itTerm = indice.erase(itTerm); 
                continue; 
            }
        }
        ++itTerm; // Avanzamos al siguiente término
    }

    // 4. Actualizar las estadÃ­sticas globales de la colección (restando)
    informacionColeccionDocs.addNumDocs(-1);
    informacionColeccionDocs.addTamBytes(-tamBytesBorrar);
    informacionColeccionDocs.addTotalPal(-numPalBorrar);
    informacionColeccionDocs.addTotalPalSinParada(-numPalSinParadaBorrar);
    informacionColeccionDocs.setTotalPalDiferentes(indice.size()); 

    // 5. Finalmente, eliminar el documento de indiceDocs
    indiceDocs.erase(itDoc);

    return true;
}

// ==========================================
// Getters y auxiliares
// ==========================================

void IndexadorHash::VaciarIndiceDocs() {
    indice.clear();
    indiceDocs.clear();
    informacionColeccionDocs = InfColeccionDocs();
}

void IndexadorHash::VaciarIndicePreg() {
    indicePregunta.clear();
    infPregunta = InformacionPregunta();
    pregunta = "";
}

int IndexadorHash::NumPalIndexadas() const {
    return indice.size();
}

string IndexadorHash::DevolverFichPalParada() const {
    return ficheroStopWords;
}

int IndexadorHash::NumPalParada() const {
    return stopWords.size();
}

string IndexadorHash::DevolverDelimitadores() const {
    return tok.DelimitadoresPalabra(); 
}

bool IndexadorHash::DevolverCasosEspeciales() const {
    return tok.CasosEspeciales();
}

bool IndexadorHash::DevolverPasarAminuscSinAcentos() const {
    return tok.PasarAminuscSinAcentos();
}

bool IndexadorHash::DevolverAlmacenarPosTerm() const {
    return almacenarPosTerm;
}

string IndexadorHash::DevolverDirIndice() const {
    return directorioIndice;
}

int IndexadorHash::DevolverTipoStemming() const {
    return tipoStemmer;
}

// ==========================================
// Listados por Pantalla
// ==========================================

void IndexadorHash::ListarPalParada() const {
    for (const auto& word : stopWords) {
        cout << word << '\n';
    }
}

void IndexadorHash::ListarInfColeccDocs() const {
    cout << informacionColeccionDocs << '\n';
}

void IndexadorHash::ListarTerminos() const {
    for (auto it = indice.begin(); it != indice.end(); ++it) {
        cout << it->first << '\t' << it->second << '\n';
    }
}

bool IndexadorHash::ListarTerminos(const string& nomDoc) const {
    // 1. Comprobamos si el documento existe
    auto itDoc = indiceDocs.find(nomDoc);
    if (itDoc == indiceDocs.end()) {
        return false; // No existe
    }

    // 2. Extraemos su ID
    int idDocBuscado = itDoc->second.getIdDoc();
    InfTermDoc dummy; // Variable auxiliar para usar 'tieneDocumento'

    // 3. Recorremos TODOS los tÃ©rminos del Ã­ndice general
    for (auto it = indice.begin(); it != indice.end(); ++it) {
        // Si el tÃ©rmino aparece en este documento, lo imprimimos
        if (it->second.tieneDocumento(idDocBuscado, dummy)) {
            cout << it->first << '\t' << it->second << '\n';
        }
    }
    return true;
}

void IndexadorHash::ListarDocs() const {
    for (auto it = indiceDocs.begin(); it != indiceDocs.end(); ++it) {
        cout << it->first << '\t' << it->second << '\n';
    }
}

bool IndexadorHash::ListarDocs(const string& nomDoc) const {
    auto it = indiceDocs.find(nomDoc);
    if (it != indiceDocs.end()) {
        cout << it->first << '\t' << it->second << '\n';
        return true;
    }
    return false;
}

void IndexadorHash::ImprimirIndexacionPregunta() {
    cout << "Pregunta indexada: " << pregunta << '\n';
    cout << "Terminos indexados en la pregunta: " << '\n';
    for (auto it = indicePregunta.begin(); it != indicePregunta.end(); ++it) {
        cout << it->first << '\t' << it->second << '\n';
    }
    cout << "Informacion de la pregunta: " << infPregunta << '\n';
}

void IndexadorHash::ImprimirPregunta() {
    cout << "Pregunta indexada: " << pregunta << '\n';
    cout << "Informacion de la pregunta: " << infPregunta << '\n';
}

void IndexadorHash::ImprimirIndexacion() const {
    cout << "Terminos indexados: " << '\n';
    ListarTerminos();
    cout << "Documentos indexados: " << '\n';
    ListarDocs();
}
// PABLO MIRA AMANTE 50504059S
#include "tokenizador.h"

// --------------------------------------------------------------------------
// CONSTRUCTORES Y DESTRUCTOR
// --------------------------------------------------------------------------

struct GeneradorTablaNormalizacion {
    unsigned char tabla[256];
    
    GeneradorTablaNormalizacion() {
        // 1. Por defecto, cada caracter se mapea a si mismo
        for (int i = 0; i < 256; ++i) {
            tabla[i] = (unsigned char)i;
        }
        
        // 2. mayusculas a minusculas (A-Z -> a-z)
        for (int i = 'A'; i <= 'Z'; ++i) {
            tabla[i] = (unsigned char)(i + 32);
        }
        
        // 3. Acentos y caracteres especiales en ISO-8859-1
        // 'a'
        tabla[0xC1] = tabla[0xC0] = tabla[0xC4] = tabla[0xC2] = 0x61;
        tabla[0xE1] = tabla[0xE0] = tabla[0xE4] = tabla[0xE2] = 0x61;
        // 'e'
        tabla[0xC9] = tabla[0xC8] = tabla[0xCB] = tabla[0xCA] = 0x65;
        tabla[0xE9] = tabla[0xE8] = tabla[0xEB] = tabla[0xEA] = 0x65;
        // 'i'
        tabla[0xCD] = tabla[0xCC] = tabla[0xCF] = tabla[0xCE] = 0x69;
        tabla[0xED] = tabla[0xEC] = tabla[0xEF] = tabla[0xEE] = 0x69;
        // 'o'
        tabla[0xD3] = tabla[0xD2] = tabla[0xD6] = tabla[0xD4] = 0x6F;
        tabla[0xF3] = tabla[0xF2] = tabla[0xF6] = tabla[0xF4] = 0x6F;
        // 'u'
        tabla[0xDA] = tabla[0xD9] = tabla[0xDC] = tabla[0xDB] = 0x75;
        tabla[0xFA] = tabla[0xF9] = tabla[0xFC] = tabla[0xFB] = 0x75;
        // 'ñ' (mayuscula -> minuscula)
        tabla[0xD1] = 0xF1;
        // 'c' (mayï¿½scula y minuscula -> c)
        tabla[0xC7] = tabla[0xE7] = 0x63;
    }
};

static const GeneradorTablaNormalizacion NormalizadorGlobal;

Tokenizador::Tokenizador(const string& delimitadoresPalabra, const bool& kcasosEspeciales, const bool& minuscSinAcentos) {
    // Inicializa delimiters, casosEspeciales y pasarAminuscSinAcentos
    this->casosEspeciales = kcasosEspeciales;
    this->pasarAminuscSinAcentos = minuscSinAcentos;
    this->delimiters = "";

    for (const char &c : delimitadoresPalabra) { 
        if (this->delimiters.find(c) == string::npos) {
            this->delimiters += c;
        }
    }
    ActualizarMatrizDelimitadores();
}

Tokenizador::Tokenizador(const Tokenizador& t) {
    // Constructor de copia
    this->delimiters = t.delimiters; 
    this->casosEspeciales = t.casosEspeciales;
    this->pasarAminuscSinAcentos = t.pasarAminuscSinAcentos;
    ActualizarMatrizDelimitadores();
}

Tokenizador::Tokenizador(){
    this->delimiters=",;:.-/+*\\ '\"{}[]()<> ! ?&#=\t@";
    this->casosEspeciales = true;
    this->pasarAminuscSinAcentos = false;
    ActualizarMatrizDelimitadores();
}
Tokenizador::~Tokenizador() {
    // Destructor: 
    this->delimiters = "";
}

// --------------------------------------------------------------------------
// Mï¿½TODOS AUXILIARES
// --------------------------------------------------------------------------

string Tokenizador::Normalizar(const string& str) const {
    string aux = str;
    
    // Modificamos el string "en el sitio" usando la tabla precalculada
    for (size_t i = 0; i < aux.length(); ++i) {
        aux[i] = NormalizadorGlobal.tabla[(unsigned char)aux[i]];
    }
    
    return aux;
}

void Tokenizador::TokenizarAStream(const string& str, ostream& os) const {
    string cadenaNormalizada;
    const string* pCadena = &str;
    
    if (pasarAminuscSinAcentos) {
        cadenaNormalizada = Normalizar(str);
        pCadena = &cadenaNormalizada;
    } 

    const string& cadena = *pCadena;

    size_t i = 0;
    size_t lastPos = 0;

    while (i < cadena.length()) {
        // 1. Saltar delimitadores iniciales
        while (i < cadena.length() && matrizDelims[(unsigned char)cadena[i]]) {
            i++;
        }
        if (i >= cadena.length()) break;

        // 2. Lï¿½gica de retroceso para decimales
        if (casosEspeciales && i > 0) {
            char prevChar = cadena[i-1];
            if ((prevChar == '.' || prevChar == ',') && matrizDelims[(unsigned char)prevChar]) {
                size_t nextD_dummy;
                bool addZero_dummy;
                size_t pos_anterior = i - 1;
                
                if (EsDecimal(cadena, pos_anterior, nextD_dummy, addZero_dummy)) {
                    i--; 
                }
            }
        }

        // 3. Encontrar delimitador base
        lastPos = i;
        while (lastPos < cadena.length() && !matrizDelims[(unsigned char)cadena[lastPos]]) {
            lastPos++;
        }

        // 4. BUCLE DE LOOKAHEAD (Heurï¿½sticas)
        if (casosEspeciales) {
            while (lastPos < cadena.length()) {
                char delim = cadena[lastPos];
                bool esCasoEspecial = false;
                bool addZero = false;

                if (delim == ':' && (lastPos == i + 3 || lastPos == i + 4 || lastPos == i + 5)) {
                    if (EsURL(cadena, lastPos, i, lastPos)) esCasoEspecial = true;
                }
                else if ((delim == '.' || delim == ',') && EsDecimal(cadena, lastPos, lastPos, addZero)) {
                    esCasoEspecial = true;
                }
                else if (delim == '@' && EsEmail(cadena, lastPos, i, lastPos)) {
                    esCasoEspecial = true;
                    break;
                }
                else if (delim == '.' && EsAcronimo(cadena, lastPos, i, lastPos)) {
                    esCasoEspecial = true;
                    break;
                }
                else if (delim == '-' && EsMultipalabra(cadena, lastPos)) {
                    esCasoEspecial = true;
                    lastPos++;
                    while (lastPos < cadena.length() && !matrizDelims[(unsigned char)cadena[lastPos]]) {
                        lastPos++;
                    }
                }
                    
                if (!esCasoEspecial) break;
            }
        }

        // 5. Ajuste final para acrï¿½nimos
        if (casosEspeciales && matrizDelims[(unsigned char)'.']) {
            if (lastPos > i + 1 && cadena[lastPos - 1] == '.') {
                bool tieneOtroPunto = false;
                for (size_t j = i; j < lastPos - 1; ++j) {
                    if (cadena[j] == '.') {
                        tieneOtroPunto = true;
                        break;
                    }
                }
                if (tieneOtroPunto) lastPos--;
            }
        }

        // 6. EXTRACCION DIRECTA A DISCO
        size_t len = lastPos - i;

        if (casosEspeciales && len > 1 && (cadena[i] == '.' || cadena[i] == ',') && es_digito(cadena[i+1])) {
            os << '0';
            os.write(cadena.c_str() + i, len);
            os << '\n';
        } else {
            os.write(cadena.c_str() + i, len);
            os << '\n';
        }

        i = lastPos;
    }
}

bool Tokenizador::EsURL(const string& str, size_t& pos, size_t start, size_t& nextDelim) const {
    // Delimitadores: Comienza por "http:", "https:" o "ftp:"
    string prefijo = "";
    
    if (str.length() - start >= 5 && str.compare(start, 5, "http:") == 0) {
        prefijo = "http:";
    } else if (str.length() - start >= 6 && str.compare(start, 6, "https:") == 0) {
        prefijo = "https:";
    } else if (str.length() - start >= 4 && str.compare(start, 4, "ftp:") == 0) {
        prefijo = "ftp:";
    } else {
        return false; // No empieza por un esquema valido
    }

    unsigned char delimTrigger = (unsigned char)str[pos];
    if (pos >= start + prefijo.length() && !EsPermitidoURL(delimTrigger)) {
        return false;
    }

    // 3. Escanear DESPUï¿½S del prefijo para validar el resto
    size_t k = start + prefijo.length(); 
    
    while (k < str.length()) {
        char c = str[k];
        
        if (es_espacio(c)) break;
        
        if (matrizDelims[(unsigned char)c] && !EsPermitidoURL(c)) {
            break;
        }
        k++;
    }
    
    // Validar que hay algo despuï¿½s del prefijo
    if (k == start + prefijo.length()) return false;

    nextDelim = k;
    return true;
}

bool Tokenizador::EsDecimal(const string& str, size_t& pos, size_t& nextDelim, bool& addZero) const {
    // Delimitadores: . ,
    
    char delim = str[pos];
    if (delim != '.' && delim != ',') return false;

    // Chequeo de "varios seguidos"
    if (pos + 1 < str.length()) {
        char next = str[pos+1];
        if (next == '.' || next == ',') return false;
    }

    bool inicioToken = false;
    if (pos == 0) inicioToken = true;
    else {
        if (es_espacio(str[pos-1]) || matrizDelims[(unsigned char)str[pos-1]]) {
            inicioToken = true;
        }
    }

    if (!inicioToken && !es_digito(str[pos-1])) {
        return false;
    }

    if (pos + 1 >= str.length() || !es_digito(str[pos+1])) {
        return false;
    }

    size_t k = pos + 1;
    while (k < str.length()) {
        char c = str[k];
        
        if (es_digito(c)) {
            k++;
        } else if (c == '.' || c == ',') {
            if (k + 1 < str.length() && es_digito(str[k+1])) {
                k++;
            } else {
                if (!es_espacio(c) && !matrizDelims[(unsigned char)c]){
                    return false;
                }
                
                //Para casos con vocales: 10.a.10
                if (k + 1 < str.length() && es_letra(str[k+1])) {
                    return false; 
                }

                break;
            }
        } else {
            if (matrizDelims[(unsigned char)c]) {
                break;
            } else {
                return false;
            }
        }
    }
    
    nextDelim = k;
    return true;
}

bool Tokenizador::EsEmail(const string& str, size_t& pos, size_t start, size_t& nextDelim) const {
    // Delimitador: @

    // 1. Validaciones bï¿½sicas del @
    if (pos == start) return false;
    if (pos + 1 >= str.length()) return false;
    
    if (matrizDelims[(unsigned char)str[pos-1]]) return false;

    // 2. Escanear hacia adelante para encontrar el final del Email
    size_t k = pos + 1;
    bool tieneOtraArroba = false;
    string permitidosEmail = ".-_";

    while (k < str.length()) {
        char c = str[k];
        
        if (es_espacio(c)) break;

        if (c == '@') {
            tieneOtraArroba = true;
            break;
        }

        if (matrizDelims[(unsigned char)c]) {
            if (EsPermitidoEmail(c)) {
                if (k + 1 < str.length()) {
                    unsigned char next = (unsigned char)str[k+1];
                    if (es_espacio(next) || matrizDelims[next]) {
                        break;
                    }
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        k++;
    }

    if (tieneOtraArroba) return false;

    bool tieneLetra = false;
    for (size_t j = start; j < k; ++j) {
        if (es_letra(str[j])) {
            tieneLetra = true;
            break;
        }
    }

    if (!tieneLetra) return false;

    nextDelim = k;
    return true;
}

bool Tokenizador::EsAcronimo(const string& str, size_t& pos, size_t start, size_t& nextDelim) const {
    // Delimitadores: .

    if (pos == start) return false; // No empieza por punto
    if (pos + 1 >= str.length()) return false;
    
    // Regla: No .. seguidos ni espacio tras el punto
    if (str[pos+1] == '.' || es_espacio(str[pos+1])) return false;

    size_t k = pos + 1; 
    while (k < str.length()) {
        char c = str[k];

        if (es_espacio(c)) break;

        if (matrizDelims[(unsigned char)c]) {
            if (c == '.') {
                if (k + 1 >= str.length()) { k++; break; }
                
                char next = str[k+1];
                if (next == '.' || matrizDelims[(unsigned char)next]) {
                    k++;
                    break; 
                }
            } else {
                break;
            }
        }
        k++;
    }

    nextDelim = k;
    return true;
}

bool Tokenizador::EsMultipalabra(const string& str, size_t& pos) const {
    // Delimitador: -

    if (pos == 0) return false;
    if (matrizDelims[(unsigned char)str[pos-1]]) return false;

    if (pos + 1 >= str.length()) return false;
    if (matrizDelims[(unsigned char)str[pos+1]]) return false;
    
    return true;
}

// --------------------------------------------------------------------------
// Mï¿½TODOS DE TOKENIZACIï¿½N
// --------------------------------------------------------------------------

void Tokenizador::Tokenizar(const string& str, list<string>& tokens) const {
    tokens.clear();
    
    string cadenaNormalizada;
    const string* pCadena = &str;
    
    if (pasarAminuscSinAcentos) {
        cadenaNormalizada = Normalizar(str);
        pCadena = &cadenaNormalizada;
    } 

    const string& cadena = *pCadena;

    size_t i = 0;
    size_t lastPos = 0;

    while (i < cadena.length()) {
        // Saltar delimitadores iniciales
        while (i < cadena.length() && matrizDelims[(unsigned char)cadena[i]]) {
            i++;
        }
        if (i >= cadena.length()) break;

        // Lï¿½gica de retroceso para decimales (solo si casosEspeciales estï¿½ activo)
        if (casosEspeciales && i > 0) {
            char prevChar = cadena[i-1];
            if ((prevChar == '.' || prevChar == ',') && matrizDelims[(unsigned char)prevChar]) {
                size_t nextD_dummy;
                bool addZero_dummy;
                size_t pos_anterior = i - 1;
                
                if (EsDecimal(cadena, pos_anterior, nextD_dummy, addZero_dummy)) {
                    i--; 
                }
            }
        }

        // Encontrar el final del token (primer delimitador)
        lastPos = i;
        while (lastPos < cadena.length() && !matrizDelims[(unsigned char)cadena[lastPos]]) {
            lastPos++;
        }

        // BUCLE DE LOOKAHEAD (CASOS ESPECIALES) - Solo si estï¿½ activado
        if (casosEspeciales) {
            while (lastPos < cadena.length()) {
                char delim = cadena[lastPos];
                bool esCasoEspecial = false;
                bool addZero = false;

                /* antes
                if (EsURL(cadena, lastPos, i, lastPos)) {
                    esCasoEspecial = true;
                }
                */

                if (delim == ':' && (lastPos == i + 3 || lastPos == i + 4 || lastPos == i + 5)) {
                    if (EsURL(cadena, lastPos, i, lastPos)) {
                        esCasoEspecial = true;
                    }
                }

                else if ((delim == '.' || delim == ',') && EsDecimal(cadena, lastPos, lastPos, addZero)) {
                    esCasoEspecial = true;
                }
                else if (delim == '@' && EsEmail(cadena, lastPos, i, lastPos)) {
                    esCasoEspecial = true;
                    break;
                }
                else if (delim == '.' && EsAcronimo(cadena, lastPos, i, lastPos)) {
                    esCasoEspecial = true;
                    break;
                }
                else if (delim == '-' && EsMultipalabra(cadena, lastPos)) {
                    esCasoEspecial = true;
                    lastPos++;
                    while (lastPos < cadena.length() && !matrizDelims[(unsigned char)cadena[lastPos]]) {
                        lastPos++;
                    }
                }
                    
                if (!esCasoEspecial) {
                    break;
                }
            }
        }

        if (casosEspeciales && matrizDelims[(unsigned char)'.']) {
            if (lastPos > i + 1 && cadena[lastPos - 1] == '.') {
                // Comprobamos si hay otro punto antes en este mismo token
                bool tieneOtroPunto = false;
                for (size_t j = i; j < lastPos - 1; ++j) {
                    if (cadena[j] == '.') {
                        tieneOtroPunto = true;
                        break;
                    }
                }
                if (tieneOtroPunto) {
                    lastPos--; // Reducimos la longitud lï¿½gicamente
                }
            }
        }

        size_t len = lastPos - i;

        if (casosEspeciales && len > 1 && (cadena[i] == '.' || cadena[i] == ',') && es_digito(cadena[i+1])) {
            string tokenConCero = "0";
            tokenConCero.reserve(len + 1); 
            tokenConCero.append(cadena, i, len);            
            tokens.push_back(std::move(tokenConCero));
        } else {
            tokens.emplace_back(cadena, i, len);
        }
        i = lastPos;  // Continuar desde el delimitador encontrado
    }
}
bool Tokenizador::Tokenizar(const string& i, const string& f) const {
    // Tokeniza fichero i -> fichero f
    ifstream entrada(i.c_str());
    if (!entrada) {
        cerr << "ERROR: No existe el archivo: " << i << endl; 
        return false;
    }

    ofstream salida(f.c_str());
    if (!salida) {
        cerr << "ERROR: No se pudo crear el archivo: " << f << endl;
        entrada.close();
        return false;
    }

    //Reserva de un mayor buffer
    char bufferEntrada[65536]; // 64 KB
    char bufferSalida[65536];  // 64 KB
    entrada.rdbuf()->pubsetbuf(bufferEntrada, sizeof(bufferEntrada));
    salida.rdbuf()->pubsetbuf(bufferSalida, sizeof(bufferSalida));

    string linea;
    
    // Lectura lï¿½nea a lï¿½nea
    while (getline(entrada, linea)) {
        TokenizarAStream(linea, salida); 
    }

    entrada.close();
    salida.close();
    return true;
}

bool Tokenizador::Tokenizar(const string& i) const {
    // Tokeniza fichero i -> fichero i.tk
    string nombreSalida = i + ".tk";
    
    return Tokenizar(i, nombreSalida);
}

bool Tokenizador::TokenizarListaFicheros(const string& i) const {
    ifstream lista(i.c_str());
    if (!lista) {
        cerr << "ERROR: No existe el archivo de lista: " << i << endl;
        return false;
    }

    string nombreFichero;
    bool todoCorrecto = true;
    struct stat info;

    while (getline(lista, nombreFichero)) {
        if (nombreFichero.empty()) continue;
        
        if (nombreFichero.back() == '\r') {
            nombreFichero.pop_back();
        }

        if (stat(nombreFichero.c_str(), &info) != 0) {
            cerr << "ERROR: No existe el archivo de la lista: " << nombreFichero << endl;
            todoCorrecto = false;
            continue;
        } 
        
        if (S_ISDIR(info.st_mode)) {
            cerr << "ERROR: El archivo de la lista es un directorio: " << nombreFichero << endl;
            todoCorrecto = false;
            continue;
        }

        if (!Tokenizar(nombreFichero)) {
            todoCorrecto = false;
        }
    }

    lista.close();
    return todoCorrecto;
}

bool Tokenizador::TokenizarDirectorio(const string& i) const {
    struct stat info;
    if (stat(i.c_str(), &info) != 0 || !S_ISDIR(info.st_mode)) {
        cerr << "ERROR: No existe el directorio o no es un directorio: " << i << endl;
        return false;
    }

    DIR* dir = opendir(i.c_str());
    if (dir == NULL) {
        cerr << "ERROR: No se pudo abrir el directorio: " << i << endl;
        return false;
    }

    bool todoCorrecto = true;
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        string nombre = entry->d_name;
        // Ignorar "." y ".."
        if (nombre == "." || nombre == "..") continue;

        // Construir ruta. Optimizamos evitando copias innecesarias si es posible,
        // pero la concatenaciï¿½n es necesaria aquï¿½.
        string rutaCompleta = i;
        if (rutaCompleta.back() != '/') {
            rutaCompleta += '/';
        }
        rutaCompleta += nombre;

        // Usamos d_type para evitar llamar a stat
        if (entry->d_type == DT_REG) {
            // Es un archivo regular -> Tokenizar
            if (!Tokenizar(rutaCompleta)) {
                todoCorrecto = false;
            }
        } else if (entry->d_type == DT_DIR) {
            // Es un directorio -> Recursividad
            if (!TokenizarDirectorio(rutaCompleta)) {
                todoCorrecto = false;
            }
        } else if (entry->d_type == DT_UNKNOWN) {
            // Fallback: Si el sistema de ficheros no soporta d_type, usamos stat
            struct stat entryInfo;
            if (stat(rutaCompleta.c_str(), &entryInfo) == 0) {
                if (S_ISREG(entryInfo.st_mode)) {
                    if (!Tokenizar(rutaCompleta)) todoCorrecto = false;
                } else if (S_ISDIR(entryInfo.st_mode)) {
                    if (!TokenizarDirectorio(rutaCompleta)) todoCorrecto = false;
                }
            } else {
                 cerr << "ERROR: Fallo al acceder a: " << rutaCompleta << endl;
                 todoCorrecto = false;
            }
        }
    }

    closedir(dir);
    return todoCorrecto;
}

// --------------------------------------------------------------------------
// GETTERS Y SETTERS (MODIFICADORES)
// --------------------------------------------------------------------------

void Tokenizador::DelimitadoresPalabra(const string& nuevoDelimiters) {
   // Setter: Cambia delimiters filtrando repetidos
    this->delimiters = "";

    for (const char &c : nuevoDelimiters) {
        if (this->delimiters.find(c) == string::npos) {
            this->delimiters += c;
        }
    }
    ActualizarMatrizDelimitadores();
}

void Tokenizador::AnyadirDelimitadoresPalabra(const string& nuevoDelimiters) {
for (const char &c : nuevoDelimiters) {
        // Solo a adimos al final si ese car cter NO est  ya en 'delimiters'
        if (this->delimiters.find(c) == string::npos) {
            this->delimiters += c;
        }
    }
    ActualizarMatrizDelimitadores();
}

string Tokenizador::DelimitadoresPalabra() const {
    return this->delimiters;
}

void Tokenizador::CasosEspeciales(const bool& nuevoCasosEspeciales) {
    this->casosEspeciales = nuevoCasosEspeciales;
    ActualizarMatrizDelimitadores();
}

bool Tokenizador::CasosEspeciales() const{
    return this->casosEspeciales;
}

void Tokenizador::PasarAminuscSinAcentos(const bool& nuevoPasarAminuscSinAcentos) {
    this->pasarAminuscSinAcentos = nuevoPasarAminuscSinAcentos;
}

bool Tokenizador::PasarAminuscSinAcentos() const{
    return this->pasarAminuscSinAcentos;
}

void Tokenizador::ActualizarMatrizDelimitadores() {
    // 1. Limpiar matriz
    for (int i = 0; i < 256; ++i) {
        matrizDelims[i] = false;
    }
    
    // 2. Delimitadores explï¿½citos (los que mete el usuario y se imprimen en el cout)
    for (char c : this->delimiters) {
        matrizDelims[(unsigned char)c] = true;
    }
    
    // 3. Delimitadores implï¿½citos
    matrizDelims['\n'] = true;
    matrizDelims['\r'] = true;
    
    // 4. Solo son delimitadores si casosEspeciales es true:
    if (this->casosEspeciales) {
        matrizDelims[' ']  = true;
        matrizDelims['\t'] = true; // Consideramos el tabulador como espacio en blanco
    }
}

// --------------------------------------------------------------------------
// SOBRECARGA DE OPERADORES 
// --------------------------------------------------------------------------

ostream& operator<<(ostream& os, const Tokenizador& t) {
    os << "DELIMITADORES: " << t.delimiters 
       << " TRATA CASOS ESPECIALES: " << t.casosEspeciales 
       << " PASAR A MINUSCULAS Y SIN ACENTOS: " << t.pasarAminuscSinAcentos;
    
    return os;
}
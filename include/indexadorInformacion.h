//Pablo Mira Amante 50504059S
#ifndef INDEXADORINFORMACION_H
#define INDEXADORINFORMACION_H

#include <iostream>
#include <vector>
// #include <list>
#include <utility>
// #include <unordered_map>
// #include <string>

using namespace std;

// 1. Clase InfTermDoc
class InfTermDoc {
    friend class IndexadorHash;
    friend class Buscador;
    friend ostream& operator<<(ostream& s, const InfTermDoc& p);
    public:
        InfTermDoc(const InfTermDoc&);
        InfTermDoc();
        ~InfTermDoc();
        InfTermDoc& operator=(const InfTermDoc&);
        int getFt() const { return ft; }
        void incrementarFt() { ft++; }
        void addPosicion(int pos) { posTerm.push_back(pos); }
    private:
        int ft;
        vector<int> posTerm;
};

// 2. Clase InformacionTermino
class InformacionTermino {
    friend class IndexadorHash;
    friend class Buscador;
    friend ostream& operator<<(ostream& s, const InformacionTermino& p);
    public:
        InformacionTermino(const InformacionTermino&);
        InformacionTermino();
        ~InformacionTermino();
        InformacionTermino& operator=(const InformacionTermino&);
        void incrementarFtc() { ftc++; }
        int getNumDocs() const { return l_docs.size(); }

        bool esNuevoEnDocumento(int id) const {
            return l_docs.empty() || l_docs.back().first != id;
        }

        bool tieneDocumento(int idDocBuscado, InfTermDoc& infSalida) const {
            // B�squeda lineal. Como un t�rmino aparece en pocos documentos
            // en comparaci�n con la colecci�n total
            for (const auto& par : l_docs) {
                if (par.first == idDocBuscado) {
                    infSalida = par.second;
                    return true;
                }
            }
            return false;
        }
        
        bool eliminarDocumento(int idDocBuscado) {
            for (auto it = l_docs.begin(); it != l_docs.end(); ++it) {
                if (it->first == idDocBuscado) {
                    ftc -= it->second.getFt(); 
                    l_docs.erase(it);          
                    return true;
                }
            }
            return false;
        }

        InfTermDoc& obtenerOcrearInfTermDoc(int id) {
            // El 99% de las veces, el documento que estamos 
            // procesando es exactamente el �ltimo que insertamos en este t�rmino.
            if (!l_docs.empty() && l_docs.back().first == id) {
                return l_docs.back().second;
            }

            // Si no es el �ltimo (ej. reindexaci�n), b�squeda lineal
            for (auto& par : l_docs) {
                if (par.first == id) return par.second;
            }

            // Si no existe de ninguna forma, lo a�adimos al final
            l_docs.push_back({id, InfTermDoc()});
            return l_docs.back().second;
        }

    private:
        int ftc;
        vector<pair<int, InfTermDoc>> l_docs;
};

// Struct auxiliar para gestionar el campo de fecha
struct Fecha {
    long int sys_time; 
};

// 3. Clase InfDoc
class InfDoc {
    friend class IndexadorHash;
    friend class Buscador;
    friend ostream& operator<<(ostream& s, const InfDoc& p);
    public:
        InfDoc(const InfDoc&);
        InfDoc();
        ~InfDoc();
        InfDoc& operator=(const InfDoc&);
        int getIdDoc() const { return idDoc; }
        int getTamBytes() const { return tamBytes; }
        int getNumPal() const { return numPal; }
        int getNumPalSinParada() const { return numPalSinParada; }
        void setIdDoc(int id) { idDoc = id; }
        void setTamBytes(int bytes) { tamBytes = bytes; }
        void setFechaModificacion(Fecha f) { fechaModificacion = f; }
        Fecha getFechaModificacion() const { return fechaModificacion; }
        void setNumPal(int n) { numPal = n; }
        void setNumPalSinParada(int n) { numPalSinParada = n; }
        void setNumPalDiferentes(int n) { numPalDiferentes = n; }
    private:
        int idDoc;
        int numPal;
        int numPalSinParada;
        int numPalDiferentes;
        int tamBytes;
        Fecha fechaModificacion;
};

// 4. Clase InfColeccionDocs
class InfColeccionDocs {
    friend class IndexadorHash;
    friend class Buscador;
    friend ostream& operator<<(ostream& s, const InfColeccionDocs& p);
    public:
        InfColeccionDocs(const InfColeccionDocs&);
        InfColeccionDocs();
        ~InfColeccionDocs();
        InfColeccionDocs& operator=(const InfColeccionDocs&);
        void addNumDocs(int n) { numDocs += n; }
        void addTamBytes(int n) { tamBytes += n; }
        void addTotalPal(int n) { numTotalPal += n; }
        void addTotalPalSinParada(int n) { numTotalPalSinParada += n; }
        void setTotalPalDiferentes(int n) { numTotalPalDiferentes = n; }
    private:
        int numDocs;
        int numTotalPal;
        int numTotalPalSinParada;
        int numTotalPalDiferentes;
        int tamBytes;
};

// 5. Clase InformacionTerminoPregunta
class InformacionTerminoPregunta {
    friend class IndexadorHash;
    friend class Buscador;
    friend ostream& operator<<(ostream& s, const InformacionTerminoPregunta& p);
    public:
        InformacionTerminoPregunta(const InformacionTerminoPregunta&);
        InformacionTerminoPregunta();
        ~InformacionTerminoPregunta();
        InformacionTerminoPregunta& operator=(const InformacionTerminoPregunta&);
        void incrementarFt() { ft++; }
        void addPosicion(int pos) { posTerm.push_back(pos); }
    private:
        int ft;
        vector<int> posTerm;
};

// 6. Clase InformacionPregunta
class InformacionPregunta {
    friend class IndexadorHash;
    friend class Buscador;
    friend ostream& operator<<(ostream& s, const InformacionPregunta& p);
    public:
        InformacionPregunta(const InformacionPregunta&);
        InformacionPregunta();
        ~InformacionPregunta();
        InformacionPregunta& operator=(const InformacionPregunta&);
        int getNumTotalPal() const { return numTotalPal; }
        void setNumTotalPal(int n) { numTotalPal = n; }
        void setNumTotalPalSinParada(int n) { numTotalPalSinParada = n; }
        void setNumTotalPalDiferentes(int n) { numTotalPalDiferentes = n; }
    private:
        int numTotalPal;
        int numTotalPalSinParada;
        int numTotalPalDiferentes;
};

#endif // INDEXADORINFORMACION_H
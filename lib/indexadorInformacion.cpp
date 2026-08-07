//Pablo Mira Amante 50504059S
#include "indexadorInformacion.h"
#include <cstring>

// ==========================================
// Clase: InfTermDoc
// ==========================================
InfTermDoc::InfTermDoc() : ft(0) {}

InfTermDoc::InfTermDoc(const InfTermDoc& other) : ft(other.ft), posTerm(other.posTerm) {}

InfTermDoc::~InfTermDoc() {
    posTerm.clear();
}

InfTermDoc& InfTermDoc::operator=(const InfTermDoc& other) {
    if (this != &other) {
        ft = other.ft;
        posTerm = other.posTerm;
    }
    return *this;
}

ostream& operator<<(ostream& s, const InfTermDoc& p) {
    s << "ft: " << p.ft;
    for (auto const& posicion : p.posTerm) {
        s << "\t" << posicion;
    }
    return s;
}

// ==========================================
// Clase: InformacionTermino
// ==========================================
InformacionTermino::InformacionTermino() : ftc(0) {}

InformacionTermino::InformacionTermino(const InformacionTermino& other) : ftc(other.ftc), l_docs(other.l_docs) {}

InformacionTermino::~InformacionTermino() {
    l_docs.clear();
}

InformacionTermino& InformacionTermino::operator=(const InformacionTermino& other) {
    if (this != &other) {
        ftc = other.ftc;
        l_docs = other.l_docs;
    }
    return *this;
}

ostream& operator<<(ostream& s, const InformacionTermino& p) {
    s << "Frecuencia total: " << p.ftc << "\tfd: " << p.l_docs.size();
    for (auto const& par : p.l_docs) {
        s << "\tId.Doc: " << par.first << "\t" << par.second;
    }
    return s;
}

// ==========================================
// Clase: InfDoc
// ==========================================
InfDoc::InfDoc() {
    memset(this, 0, sizeof(InfDoc));
    idDoc = 0;
    numPal = 0;
    numPalSinParada = 0;
    numPalDiferentes = 0;
    tamBytes = 0;
    fechaModificacion.sys_time = 0;
}

InfDoc::InfDoc(const InfDoc& other) {
    memset(this, 0, sizeof(InfDoc));
    idDoc = other.idDoc;
    numPal = other.numPal;
    numPalSinParada = other.numPalSinParada;
    numPalDiferentes = other.numPalDiferentes;
    tamBytes = other.tamBytes;
    fechaModificacion = other.fechaModificacion;
}

InfDoc::~InfDoc() {}

InfDoc& InfDoc::operator=(const InfDoc& other) {
    if (this != &other) {
        memset(this, 0, sizeof(InfDoc));
        idDoc = other.idDoc;
        numPal = other.numPal;
        numPalSinParada = other.numPalSinParada;
        numPalDiferentes = other.numPalDiferentes;
        tamBytes = other.tamBytes;
        fechaModificacion = other.fechaModificacion;
    }
    return *this;
}

ostream& operator<<(ostream& s, const InfDoc& p) {
    s << "idDoc: " << p.idDoc
      << "\tnumPal: " << p.numPal
      << "\tnumPalSinParada: " << p.numPalSinParada
      << "\tnumPalDiferentes: " << p.numPalDiferentes
      << "\ttamBytes: " << p.tamBytes;
    return s;
}

// ==========================================
// Clase: InfColeccionDocs
// ==========================================
InfColeccionDocs::InfColeccionDocs() {
    memset(this, 0, sizeof(InfColeccionDocs));
    numDocs = 0;
    numTotalPal = 0;
    numTotalPalSinParada = 0;
    numTotalPalDiferentes = 0;
    tamBytes = 0;
}

InfColeccionDocs::InfColeccionDocs(const InfColeccionDocs& other) {
    memset(this, 0, sizeof(InfColeccionDocs));
    numDocs = other.numDocs;
    numTotalPal = other.numTotalPal;
    numTotalPalSinParada = other.numTotalPalSinParada;
    numTotalPalDiferentes = other.numTotalPalDiferentes;
    tamBytes = other.tamBytes;
}

InfColeccionDocs::~InfColeccionDocs() {}

InfColeccionDocs& InfColeccionDocs::operator=(const InfColeccionDocs& other) {
    if (this != &other) {
        memset(this, 0, sizeof(InfColeccionDocs));
        numDocs = other.numDocs;
        numTotalPal = other.numTotalPal;
        numTotalPalSinParada = other.numTotalPalSinParada;
        numTotalPalDiferentes = other.numTotalPalDiferentes;
        tamBytes = other.tamBytes;
    }
    return *this;
}

ostream& operator<<(ostream& s, const InfColeccionDocs& p) {
    s << "numDocs: " << p.numDocs
      << "\tnumTotalPal: " << p.numTotalPal
      << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada
      << "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes
      << "\ttamBytes: " << p.tamBytes;
    return s;
}

// ==========================================
// Clase: InformacionTerminoPregunta
// ==========================================
InformacionTerminoPregunta::InformacionTerminoPregunta() : ft(0) {}

InformacionTerminoPregunta::InformacionTerminoPregunta(const InformacionTerminoPregunta& other) :
    ft(other.ft), posTerm(other.posTerm) {}

InformacionTerminoPregunta::~InformacionTerminoPregunta() {
    posTerm.clear();
}

InformacionTerminoPregunta& InformacionTerminoPregunta::operator=(const InformacionTerminoPregunta& other) {
    if (this != &other) {
        ft = other.ft;
        posTerm = other.posTerm;
    }
    return *this;
}

ostream& operator<<(ostream& s, const InformacionTerminoPregunta& p) {
    s << "ft: " << p.ft;
    for (auto const& posicion : p.posTerm) {
        s << "\t" << posicion;
    }
    return s;
}

// ==========================================
// Clase: InformacionPregunta
// ==========================================
InformacionPregunta::InformacionPregunta() : numTotalPal(0), numTotalPalSinParada(0), numTotalPalDiferentes(0) {}

InformacionPregunta::InformacionPregunta(const InformacionPregunta& other) : numTotalPal(other.numTotalPal),
    numTotalPalSinParada(other.numTotalPalSinParada), numTotalPalDiferentes(other.numTotalPalDiferentes) {}

InformacionPregunta::~InformacionPregunta() {}

InformacionPregunta& InformacionPregunta::operator=(const InformacionPregunta& other) {
    if (this != &other) {
        numTotalPal = other.numTotalPal;
        numTotalPalSinParada = other.numTotalPalSinParada;
        numTotalPalDiferentes = other.numTotalPalDiferentes;
    }
    return *this;
}

ostream& operator<<(ostream& s, const InformacionPregunta& p) {
    s << "numTotalPal: " << p.numTotalPal
      << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada
      << "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes;
    return s;
}
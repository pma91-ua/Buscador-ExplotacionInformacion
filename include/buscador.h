#ifndef BUSCADOR_H
#define BUSCADOR_H

#include <iostream>
#include <string>
#include <queue>
#include "indexadorHash.h"

using namespace std;

// Prototipo de la clase ResultadoRI necesaria para la cola de prioridad
class ResultadoRI {
    friend ostream& operator<<(ostream& os, const ResultadoRI& res);
    friend class Buscador;
public:
    ResultadoRI(const double& kvSimilitud, const long int& kidDoc, const int& np);
    double VSimilitud() const;
    long int IdDoc() const;
    bool operator<(const ResultadoRI& lhs) const;
private:
    double vSimilitud;
    long int idDoc;
    int numPregunta;
};

// Prototipo de la clase Buscador
class Buscador : public IndexadorHash {

    friend ostream& operator<<(ostream& s, const Buscador& p) {
        string preg;
        s << "Buscador: " << "\n";
        if(p.DevuelvePregunta(preg)) 
            s << "\tPregunta indexada: " << preg << "\n";
        else
            s << "\tNo hay ninguna pregunta indexada" << "\n";
        
        s << "\tDatos del indexador: " << "\n" << (IndexadorHash) p;
        return s;
    }

public:
    Buscador(const string& directorioIndexacion, const int& f);
    Buscador(const Buscador&);
    ~Buscador();
    Buscador& operator=(const Buscador&);

    bool Buscar(const int& numDocumentos = 99999);
    bool Buscar(const string& dirPreguntas, const int& numDocumentos, const int& numPregInicio, const int& numPregFin);

    void ImprimirResultadoBusqueda(const int& numDocumentos = 99999) const;
    bool ImprimirResultadoBusqueda(const int& numDocumentos, const string& nombreFichero) const;

    int DevolverFormulaSimilitud() const;
    bool CambiarFormulaSimilitud(const int& f);

    void CambiarParametrosDFR(const double& kc);
    double DevolverParametrosDFR() const;

    void CambiarParametrosBM25(const double& kk1, const double& kb);
    void DevolverParametrosBM25(double& kk1, double& kb) const;

private:	
    Buscador();	
    priority_queue<ResultadoRI> docsOrdenados;	
    int formSimilitud;

    // Constantes del modelo DFR
    double c;

    // Constantes del modelo BM25
    double k1;
    double b;
};

#endif // BUSCADOR_H
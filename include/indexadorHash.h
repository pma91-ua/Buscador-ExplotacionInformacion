//Pablo Mira Amante 50504059S
#ifndef INDEXADORHASH_H
#define INDEXADORHASH_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "indexadorInformacion.h"
#include "tokenizador.h" 
#include "stemmer.h"

using namespace std;

class IndexadorHash {
    friend class Buscador;
    friend ostream& operator<<(ostream& s, const IndexadorHash& p) {
        s << "Fichero con el listado de palabras de parada: " << p.ficheroStopWords << "\n";
        s << "Tokenizador: " << p.tok << "\n";
        s << "Directorio donde se almacenara el indice generado: " << p.directorioIndice << "\n";
        s << "Stemmer utilizado: " << p.tipoStemmer << "\n";
        s << "Informacion de la coleccion indexada: " << p.informacionColeccionDocs << "\n";
        s << "Se almacenaran las posiciones de los terminos: " << p.almacenarPosTerm;
        return s;
    }

public:
    IndexadorHash(const string& fichStopWords, const string& delimitadores,
                  const bool& detectComp, const bool& minuscSinAcentos, const string&
                  dirIndice, const int& tStemmer, const bool& almPosTerm);
    IndexadorHash(const string& directorioIndexacion);
    IndexadorHash(const IndexadorHash&);
    ~IndexadorHash();
    
    IndexadorHash& operator=(const IndexadorHash&);
    
    bool Indexar(const string& ficheroDocumentos);
    bool IndexarDirectorio(const string& dirAIndexar);
    bool GuardarIndexacion() const;
    bool RecuperarIndexacion(const string& directorioIndexacion);
    
    void ImprimirIndexacion() const;
    
    bool IndexarPregunta(const string& preg);
    bool DevuelvePregunta(string& preg) const;
    bool DevuelvePregunta(const string& word, InformacionTerminoPregunta& inf) const;
    bool DevuelvePregunta(InformacionPregunta& inf) const;
    
    void ImprimirIndexacionPregunta();
    void ImprimirPregunta();
    
    bool Devuelve(const string& word, InformacionTermino& inf) const;
    bool Devuelve(const string& word, const string& nomDoc, InfTermDoc& InfDoc) const;
    bool Existe(const string& word) const;
    bool BorraDoc(const string& nomDoc);
    
    void VaciarIndiceDocs();
    void VaciarIndicePreg();
    
    int NumPalIndexadas() const;
    string DevolverFichPalParada() const;
    void ListarPalParada() const;
    int NumPalParada() const;
    string DevolverDelimitadores() const;
    bool DevolverCasosEspeciales() const;
    bool DevolverPasarAminuscSinAcentos() const;
    bool DevolverAlmacenarPosTerm() const;
    string DevolverDirIndice() const;
    int DevolverTipoStemming() const;
    
    void ListarInfColeccDocs() const;
    void ListarTerminos() const;
    bool ListarTerminos(const string& nomDoc) const;
    void ListarDocs() const;
    bool ListarDocs(const string& nomDoc) const;

private:
    IndexadorHash(); 
    
    unordered_map<string, InformacionTermino> indice;
    unordered_map<string, InfDoc> indiceDocs;
    InfColeccionDocs informacionColeccionDocs;
    
    string pregunta;
    unordered_map<string, InformacionTerminoPregunta> indicePregunta;
    InformacionPregunta infPregunta;
    
    unordered_set<string> stopWords;
    string ficheroStopWords;
    
    Tokenizador tok; 
    
    string directorioIndice;
    int tipoStemmer;
    bool almacenarPosTerm;
};

#endif // INDEXADORHASH_H
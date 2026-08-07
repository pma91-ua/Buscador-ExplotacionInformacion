.PHONY= clean

CC=g++
OPTIONS= -g
DEBUG= #-D DEBUG
LIBDIR=lib
INCLUDEDIR=include
_OBJ= buscador.o indexadorHash.o tokenizador.o stemmer.o indexadorInformacion.o
OBJ = $(patsubst %,$(LIBDIR)/%,$(_OBJ))

all: buscador

buscador: src/main.cpp $(OBJ)
	$(CC) $(OPTIONS) $(DEBUG) -I$(INCLUDEDIR) src/main.cpp $(OBJ) -o buscador

$(LIBDIR)/%.o : $(LIBDIR)/%.cpp $(INCLUDEDIR)/%.h
	$(CC) $(OPTIONS) $(DEBUG) -c -I$(INCLUDEDIR) -o $@ $<

clean:
	rm -f $(OBJ) buscador

tar: clean
	@echo "Comprobando ficheros obligatorios..."
	@test -f nombres.txt || (echo "ERROR: falta nombres.txt"; exit 1)
	@test -f src/graficaPrecisionCobertura.pdf || (echo "ERROR: falta src/graficaPrecisionCobertura.pdf"; exit 1)
	@test -f src/main.cpp || (echo "ERROR: falta src/main.cpp"; exit 1)
	tar cvzf PRACTICA.tgz \
		include/buscador.h \
		include/indexadorHash.h \
		include/tokenizador.h \
		include/stemmer.h \
		include/indexadorInformacion.h \
		lib/buscador.cpp \
		lib/indexadorHash.cpp \
		lib/tokenizador.cpp \
		lib/stemmer.cpp \
		lib/indexadorInformacion.cpp \
		src/graficaPrecisionCobertura.pdf \
		src/main_modificado.cpp \
		src/generar_grafica.py \
		nombres.txt
	@echo "================================================="
	@echo "Tamanyo del fichero generado:"
	@ls -lh PRACTICA.tgz
	@echo "AVISO: El fichero NO debe superar 300K segun el enunciado."
	@echo "AVISO 2: Comprueba que el makefile NO este dentro del .tgz."
	@echo "================================================="
#!/bin/bash

# Códigos de colores
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # Sin color
BOLD='\033[1m'

echo -e "${BOLD}Iniciando la corrección de pruebas para el Buscador...${NC}"
echo "--------------------------------------------------------"

# Directorio de los tests
TEST_DIR="src/ficherosPrueba_buscador"
TEST_FILES=$(find "$TEST_DIR" -name "tad*.cpp" | sort)

if [ -z "$TEST_FILES" ]; then
    echo -e "${RED}ERROR: No se encontraron archivos de prueba tad*.cpp en $TEST_DIR${NC}"
    exit 1
fi

passed=0
total=0

for test_path in $TEST_FILES; do
    test_name=$(basename "$test_path" .cpp)
    sal_path="${TEST_DIR}/${test_name}.cpp.sal"
    
    # Comprobar el formato del fichero .sal (.cpp.sal o .sal)
    if [ ! -f "$sal_path" ]; then
        sal_path="${TEST_DIR}/${test_name}.sal"
        if [ ! -f "$sal_path" ]; then
            echo -e "${RED}Falta el archivo de salida esperada (.sal) para: $test_name${NC}"
            continue
        fi
    fi
    
    total=$((total + 1))
    echo -n "Probando $test_name... "
    
    # 1. Copiar a src/main.cpp
    cp "$test_path" src/main.cpp
    
    # 2. Compilar
    make clean > /dev/null 2>&1
    make > /dev/null 2>&1
    
    if [ ! -f ./buscador ]; then
        echo -e "${RED}ERROR DE COMPILACIÓN${NC}"
        continue
    fi
    
    # 3. Ejecutar y comparar
    ./buscador > "${TEST_DIR}/${test_name}.out" 2>/dev/null
    
    # Comparación ignorando espacios en blanco (-w) y líneas vacías (-B)
    diff -w -B "${TEST_DIR}/${test_name}.out" "$sal_path" > "${TEST_DIR}/${test_name}.diff" 2>&1
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}PASADO${NC}"
        passed=$((passed + 1))
        # Limpiar ficheros temporales exitosos
        rm -f "${TEST_DIR}/${test_name}.out" "${TEST_DIR}/${test_name}.diff"
    else
        echo -e "${RED}FALLADO (ver diferencias en ${TEST_DIR}/${test_name}.diff)${NC}"
    fi
done

# Limpieza final
rm -f src/main.cpp
make clean > /dev/null 2>&1

echo "--------------------------------------------------------"
if [ $passed -eq $total ]; then
    echo -e "${GREEN}${BOLD}¡TODOS LOS TESTS PASADOS! ($passed/$total)${NC}"
else
    echo -e "${RED}${BOLD}SE ENCONTRARON FALLOS ($passed/$total tests pasados)${NC}"
fi

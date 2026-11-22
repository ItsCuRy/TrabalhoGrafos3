# Trabalho de Grafos — COS 242 (Partes 2 e 3)

Aplicação C++17 para grafos ponderados em representação CSR. A Parte 2 implementa
Dijkstra em grafos não direcionados e a Parte 3 estende o projeto para grafos
direcionados, Bellman-Ford e Floyd-Warshall.

## Build e testes

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Os testes usam `assert`; o alvo `test_grafo` mantém essas verificações ativas
mesmo em builds Release.

Para criar uma build limpa, use outro diretório:

```bash
cmake -S . -B build-clean -DCMAKE_BUILD_TYPE=Release
cmake --build build-clean --parallel
ctest --test-dir build-clean --output-on-failure
```

## Executar

```bash
# Estudos da Parte 3
./scripts/run_estudos.sh parte3
./build/grafo parte3

# Um grafo direcionado específico
./build/grafo data/grafo_W_1.txt --directed

# Floyd-Warshall completo sem limite automático
./build/grafo data/grafo_W_1.txt --force-floyd

# Estudos anteriores da Parte 2
./scripts/run_estudos.sh parte2
./build/grafo colaboracao
```

Os arquivos disponíveis são `grafo_W_1.txt` a `grafo_W_5.txt`. Sem argumentos,
o programa processa esses cinco arquivos no modo da Parte 3. `--force-floyd`
deve ser usado apenas quando houver RAM suficiente para a matriz `O(V^2)` e tempo
para as operações `O(V^3)`.

## Organização

- `include/` e `src/`: interface `Graph`, implementação CSR e algoritmos P2/P3.
- `tests/test_grafo.cpp`: testes em memória de Dijkstra, direção, Bellman-Ford e Floyd-Warshall.
- `scripts/run_estudos.sh`: compilação, testes e execução dos estudos de caso.
- `docs/`: relatório e apresentação do trabalho.


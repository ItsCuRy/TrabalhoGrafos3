#pragma once
#include "Graph.hpp"
#include <vector>

/**
 * @brief Define a estrutura de dados utilizada pelo algoritmo de Dijkstra
 * para manter e buscar o vértice com menor estimativa de distância.
 */
enum class DijkstraMode {
    VECTOR, ///< Busca linear em vetor: complexidade O(V^2 + E)
    HEAP    ///< Fila de prioridades com Heap Binário Indexado: complexidade O((V + E) log V)
};

/**
 * @brief Armazena o resultado da execução do algoritmo de caminho mínimo.
 * 
 * Vetores dimensionados para (n + 1) para acesso direto 1-based.
 */
struct ShortestPathResult {
    std::vector<double> dist;
    std::vector<int> parent;
};

struct BellmanFordResult {
    std::vector<double> dist;
    std::vector<int> parent;
    bool hasNegativeCycle = false;
};

struct FloydWarshallResult {
    int numVertices = 0;
    std::vector<double> dist;
    std::vector<int> next;
    bool hasNegativeCycle = false;
};

/**
 * @brief Executa o algoritmo de Dijkstra a partir de um vértice origem.
 * 
 * Verifica automaticamente se o grafo possui pesos negativos (lançando exceção
 * caso possua, conforme especificação do Trabalho 2).
 * 
 * @param g Grafo de entrada (interface Graph / CSRGraph).
 * @param source Vértice inicial (1 <= source <= n).
 * @param mode Estrutura de armazenamento das estimativas (VECTOR ou HEAP).
 * @return ShortestPathResult contendo o vetor de distâncias e a árvore geradora (parent).
 */
ShortestPathResult dijkstra(const Graph& g, int source, DijkstraMode mode = DijkstraMode::HEAP);

/**
 * @brief Executa Dijkstra até que todos os destinos pedidos tenham sido finalizados.
 * A busca completa continua disponível em dijkstra(); esta função serve para
 * consultas de caminhos específicos sem percorrer destinos que não serão usados.
 */
ShortestPathResult dijkstraToTargets(const Graph& g, int source, DijkstraMode mode,
                                    const std::vector<int>& targets);

/**
 * @brief Reconstrói o caminho mínimo entre source e target usando o vetor de pais.
 * 
 * @param res Resultado retornado previamente por dijkstra().
 * @param source Vértice de origem.
 * @param target Vértice de destino.
 * @return std::vector<int> Sequência de vértices [source, ..., target], ou vazio se inalcançável.
 */
std::vector<int> reconstructPath(const ShortestPathResult& res, int source, int target);

BellmanFordResult bellmanFord(const Graph& g, int source);
BellmanFordResult bellmanFordQueue(const Graph& g, int source);
FloydWarshallResult floydWarshall(const Graph& g);
std::vector<int> reconstructPath(const BellmanFordResult& res, int source, int target);
std::vector<int> reconstructPath(const FloydWarshallResult& res, int source, int target);
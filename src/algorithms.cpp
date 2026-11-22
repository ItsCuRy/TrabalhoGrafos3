#include "algorithms.hpp"
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <deque>
#include <cmath>

namespace {

constexpr double INF = std::numeric_limits<double>::infinity();

/**
 * @brief Armazenamento de estimativas de distância baseado em Vetor.
 * 
 * Decisão de Projeto:
 * - Inserção / atualização de chave (pushOrDecreaseKey): O(1), apenas atualiza o vetor
 *   e mantém a lista de candidatos ativos.
 * - Extração do mínimo (extractMin): O(V) por varredura linear sobre todos os vértices
 *   ainda não visitados, resultando em complexidade total O(V^2 + E) no Dijkstra.
 */
class VectorDistanceStore {
private:
    int n_;
    const std::vector<double>& dist_;
    std::vector<unsigned char> visited_;
    std::vector<unsigned char> queued_;
    int remaining_active_ = 0;

public:
    VectorDistanceStore(int n, const std::vector<double>& dist)
        : n_(n), dist_(dist), visited_(n + 1, 0), queued_(n + 1, 0), remaining_active_(0) {}

    bool empty() const {
        return remaining_active_ == 0;
    }

    void pushOrDecreaseKey(int u, double /*new_dist*/) {
        // Conta cada candidato uma vez; novas distâncias não criam entradas duplicadas.
        if (!visited_[u] && !queued_[u]) {
            queued_[u] = 1;
            remaining_active_++;
        }
    }

    int extractMin() {
        int best_u = -1;
        double min_d = INF;

        // Busca linear O(V) pelo vértice não visitado de menor distância estimada
        for (int v = 1; v <= n_; ++v) {
            if (!visited_[v] && dist_[v] < min_d) {
                min_d = dist_[v];
                best_u = v;
            }
        }

        if (best_u == -1) return -1;

        visited_[best_u] = 1;
        queued_[best_u] = 0;
        remaining_active_--;
        return best_u;
    }
};

/**
 * @brief Fila de prioridades baseada em Heap Binário Mínimo Indexado (Indexed Min-Heap).
 * 
 * Decisão de Projeto:
 * Diferente de std::priority_queue (que não suporta decrease-key e exige inserções
 * duplicadas no heap), este Heap Indexado mantém um vetor `pos_[v]` que mapeia a posição
 * exata do vértice `v` dentro do array `heap_`.
 * - Garante no máximo V elementos no heap simultaneamente (uso de memória estrito O(V)).
 * - Operação pushOrDecreaseKey executa siftUp diretamente na posição `pos_[v]` em O(log V).
 * - Operação extractMin executa em O(log V).
 * - Complexidade total do Dijkstra: O((V + E) log V).
 */
class HeapDistanceStore {
private:
    std::vector<int> heap_;     ///< Array 0-based representando a árvore binária do heap
    std::vector<int> pos_;      ///< pos_[v] = índice de v em heap_ (ou -1 se fora do heap)
    std::vector<double> keys_;  ///< Chave (distância atual) associada a cada vértice
    int size_ = 0;              ///< Quantidade atual de elementos no heap

    void swapNodes(int i, int j) {
        int u = heap_[i];
        int v = heap_[j];
        std::swap(heap_[i], heap_[j]);
        pos_[u] = j;
        pos_[v] = i;
    }

    void siftUp(int idx) {
        while (idx > 0) {
            int parent = (idx - 1) / 2;
            if (keys_[heap_[idx]] < keys_[heap_[parent]]) {
                swapNodes(idx, parent);
                idx = parent;
            } else {
                break;
            }
        }
    }

    void siftDown(int idx) {
        while (true) {
            int left = 2 * idx + 1;
            int right = 2 * idx + 2;
            int smallest = idx;

            if (left < size_ && keys_[heap_[left]] < keys_[heap_[smallest]]) {
                smallest = left;
            }
            if (right < size_ && keys_[heap_[right]] < keys_[heap_[smallest]]) {
                smallest = right;
            }
            if (smallest != idx) {
                swapNodes(idx, smallest);
                idx = smallest;
            } else {
                break;
            }
        }
    }

public:
    HeapDistanceStore(int n, const std::vector<double>& /*dist*/)
        : heap_(n + 1, 0), pos_(n + 1, -1), keys_(n + 1, INF), size_(0) {}

    bool empty() const {
        return size_ == 0;
    }

    void pushOrDecreaseKey(int u, double new_dist) {
        keys_[u] = new_dist;
        if (pos_[u] == -1) {
            // Vértice ainda não estava no heap: insere na última folha e sobe
            int idx = size_++;
            heap_[idx] = u;
            pos_[u] = idx;
            siftUp(idx);
        } else {
            // Vértice já está no heap: atualiza a chave in-place e restaura a propriedade do heap
            siftUp(pos_[u]);
        }
    }

    int extractMin() {
        if (size_ == 0) return -1;

        int min_v = heap_[0];
        pos_[min_v] = -1;
        size_--;

        if (size_ > 0) {
            heap_[0] = heap_[size_];
            pos_[heap_[0]] = 0;
            siftDown(0);
        }
        return min_v;
    }
};

/**
 * @brief Template interno que unifica a lógica de relaxamento do algoritmo de Dijkstra.
 * 
 * Como o tipo `DistanceStore` é resolvido em tempo de compilação (Zero-Cost Abstraction),
 * o compilador pode aplicar inlining completo nas operações do Heap ou do Vetor sem
 * custo de chamadas virtuais dentro do laço crítico.
 */
template <typename DistanceStore>
ShortestPathResult runDijkstraImpl(const Graph& g, int source,
                                  const std::vector<int>* targets = nullptr) {
    const int n = g.numVertices();

    ShortestPathResult res;
    res.dist.assign(n + 1, INF);
    res.parent.assign(n + 1, 0);

    res.dist[source] = 0.0;

    std::vector<unsigned char> is_target;
    int remaining_targets = 0;
    if (targets != nullptr) {
        is_target.assign(n + 1, 0);
        for (int target : *targets) {
            if (!is_target[target]) {
                is_target[target] = 1;
                remaining_targets++;
            }
        }
        if (remaining_targets == 0) return res;
    }

    DistanceStore store(n, res.dist);
    store.pushOrDecreaseKey(source, 0.0);

    while (!store.empty()) {
        int u = store.extractMin();
        if (u == -1) break;

        // Dijkstra finaliza um vértice ao extraí-lo como o mínimo; destinos restantes
        // não precisam ser explorados quando todos os destinos solicitados já fecharam.
        if (targets != nullptr && is_target[u] && --remaining_targets == 0) break;

        const double d_u = res.dist[u];
        const int* nbr_begin = g.neighborsBegin(u);
        const int deg = g.degree(u);

        // Percorre os vizinhos contíguos na estrutura CSR
        for (int k = 0; k < deg; ++k) {
            int v = nbr_begin[k];
            double w = static_cast<double>(g.edgeWeight(u, k));

            if (d_u + w < res.dist[v]) {
                res.dist[v] = d_u + w;
                res.parent[v] = u;
                store.pushOrDecreaseKey(v, res.dist[v]);
            }
        }
    }

    return res;
}

} // namespace anônimo

namespace {

void validateDijkstraInput(const Graph& g, int source) {
    if (source < 1 || source > g.numVertices()) {
        throw std::out_of_range("Vertice de origem fora do intervalo [1, n].");
    }

    if (g.hasNegativeWeight()) {
        throw std::domain_error(
            "Erro: O grafo possui pesos negativos. A biblioteca ainda nao implementa "
            "caminhos minimos com pesos negativos."
        );
    }
}

BellmanFordResult runBellmanFord(const Graph& g, int source) {

    BellmanFordResult result;
    const int n = g.numVertices();
    result.dist.assign(n + 1, INF);
    result.parent.assign(n + 1, 0);
    result.dist[source] = 0.0;

    // In-place: the maximum distance length is n-1 arcs. An early exit avoids V passes
    // when no relaxation changed in one pass.
    for (int iteration = 1; iteration < n; ++iteration) {
        bool changed = false;
        for (int u = 1; u <= n; ++u) {
            if (result.dist[u] == INF) continue;
            for (int k = 0; k < g.degree(u); ++k) {
                int v = g.neighborsBegin(u)[k];
                double candidate = result.dist[u] + g.edgeWeight(u, k);
                if (candidate < result.dist[v]) {
                    result.dist[v] = candidate;
                    result.parent[v] = u;
                    changed = true;
                }
            }
        }
        if (!changed) break;
    }

    // A distance update after n-1 passes proves that a reachable negative cycle exists.
    for (int u = 1; u <= n; ++u) {
        if (result.dist[u] == INF) continue;
        for (int k = 0; k < g.degree(u); ++k) {
            int v = g.neighborsBegin(u)[k];
            if (result.dist[u] + g.edgeWeight(u, k) < result.dist[v]) {
                result.hasNegativeCycle = true;
                return result;
            }
        }
    }
    return result;
}

BellmanFordResult runBellmanFordQueue(const Graph& g, int source) {

    BellmanFordResult result;
    const int n = g.numVertices();
    result.dist.assign(n + 1, INF);
    result.parent.assign(n + 1, 0);
    result.dist[source] = 0.0;

    std::deque<int> queue;
    std::vector<unsigned char> inQueue(n + 1, 0);
    queue.push_back(source);
    inQueue[source] = 1;

    // Otimizacao por fila: cada vertice entra apenas quando uma distancia melhora.
    while (!queue.empty()) {
        int u = queue.front();
        queue.pop_front();
        inQueue[u] = 0;

        for (int k = 0; k < g.degree(u); ++k) {
            int v = g.neighborsBegin(u)[k];
            double candidate = result.dist[u] + g.edgeWeight(u, k);
            if (candidate < result.dist[v]) {
                result.dist[v] = candidate;
                result.parent[v] = u;
                // A cadeia de pais detecta o ciclo sem esperar V re-enqueues.
                int cur = u;
                bool cycleDetected = false;
                for (int step = 0; step < n; ++step) {
                    if (cur == 0) break;
                    if (cur == v) {
                        cycleDetected = true;
                        break;
                    }
                    cur = result.parent[cur];
                }
                if (cycleDetected) {
                    result.hasNegativeCycle = true;
                    return result;
                }

                if (!inQueue[v]) {
                    queue.push_back(v);
                    inQueue[v] = 1;
                }
            }
        }
    }
    return result;
}

FloydWarshallResult runFloydWarshall(const Graph& g) {
    const int n = g.numVertices();
    FloydWarshallResult result;
    result.numVertices = n;
    result.dist.assign(static_cast<size_t>(n + 1) * (n + 1), std::numeric_limits<float>::infinity());
    result.next.assign(static_cast<size_t>(n + 1) * (n + 1), 0);

    auto index = [n](int u, int v) -> size_t { return static_cast<size_t>(u) * (n + 1) + v; };
    for (int u = 1; u <= n; ++u) {
        result.dist[index(u, u)] = 0.0f;
        result.next[index(u, u)] = u;
        for (int k = 0; k < g.degree(u); ++k) {
            int v = g.neighborsBegin(u)[k];
            double w = g.edgeWeight(u, k);
            size_t pos = index(u, v);
            if (w < result.dist[pos]) {
                result.dist[pos] = w;
                result.next[pos] = v;
            }
        }
    }

    // One-dimensional matrices reduce the auxiliary memory relative to two separate matrices.
    for (int k = 1; k <= n; ++k) {
        for (int i = 1; i <= n; ++i) {
            size_t ik = index(i, k);
            if (std::isinf(result.dist[ik])) continue;
            for (int j = 1; j <= n; ++j) {
                size_t kj = index(k, j);
                if (std::isinf(result.dist[kj])) continue;
                size_t ij = index(i, j);
                double candidate = result.dist[ik] + result.dist[kj];
                if (candidate < result.dist[ij]) {
                    result.dist[ij] = candidate;
                    result.next[ij] = result.next[ik];
                }
            }
        }
    }

    for (int v = 1; v <= n; ++v) {
        if (result.dist[index(v, v)] < 0.0) {
            result.hasNegativeCycle = true;
            break;
        }
    }
    return result;
}

} // namespace

BellmanFordResult bellmanFord(const Graph& g, int source) {
    if (source < 1 || source > g.numVertices()) {
        throw std::out_of_range("Vertice de origem fora do intervalo [1, n].");
    }
    return runBellmanFord(g, source);
}

BellmanFordResult bellmanFordQueue(const Graph& g, int source) {
    if (source < 1 || source > g.numVertices()) {
        throw std::out_of_range("Vertice de origem fora do intervalo [1, n].");
    }
    return runBellmanFordQueue(g, source);
}

FloydWarshallResult floydWarshall(const Graph& g) {
    return runFloydWarshall(g);
}

FloydWarshallResult floydWarshall(const Graph& g, const std::vector<int>& vertices) {
    const int k = static_cast<int>(vertices.size());
    FloydWarshallResult result;
    result.numVertices = k;
    result.dist.assign(static_cast<size_t>(k + 1) * (k + 1), std::numeric_limits<float>::infinity());
    result.next.assign(static_cast<size_t>(k + 1) * (k + 1), 0);

    std::vector<int> original(k + 1, 0);
    std::vector<int> local(g.numVertices() + 1, 0);
    for (int i = 1; i <= k; ++i) {
        original[i] = vertices[i - 1];
        local[vertices[i - 1]] = i;
        result.dist[static_cast<size_t>(i) * (k + 1) + i] = 0.0;
        result.next[static_cast<size_t>(i) * (k + 1) + i] = i;
    }

    auto index = [k](int i, int j) -> size_t {
        return static_cast<size_t>(i) * (k + 1) + j;
    };

    for (int i = 1; i <= k; ++i) {
        int u = original[i];
        for (int edge = 0; edge < g.degree(u); ++edge) {
            int vOriginal = g.neighborsBegin(u)[edge];
            int j = local[vOriginal];
            if (j == 0) continue;
            double w = g.edgeWeight(u, edge);
            size_t pos = index(i, j);
            if (w < result.dist[pos]) {
                result.dist[pos] = w;
                result.next[pos] = j;
            }
        }
    }

    for (int middle = 1; middle <= k; ++middle) {
        for (int i = 1; i <= k; ++i) {
            size_t im = index(i, middle);
            if (std::isinf(result.dist[im])) continue;
            for (int j = 1; j <= k; ++j) {
                size_t mj = index(middle, j);
                if (std::isinf(result.dist[mj])) continue;
                size_t ij = index(i, j);
                double candidate = result.dist[im] + result.dist[mj];
                if (candidate < result.dist[ij]) {
                    result.dist[ij] = candidate;
                    result.next[ij] = result.next[im];
                }
            }
        }
    }

    for (int i = 1; i <= k; ++i) {
        if (result.dist[index(i, i)] < 0.0) {
            result.hasNegativeCycle = true;
            break;
        }
    }
    return result;
}

BreadthSearchResult bfs(const Graph& g, int source) {
    BreadthSearchResult result;
    const int n = g.numVertices();
    result.parent.assign(n + 1, 0);
    result.distance.assign(n + 1, -1);
    if (source < 1 || source > n) return result;

    std::deque<int> queue;
    result.distance[source] = 0;
    queue.push_back(source);
    while (!queue.empty()) {
        int u = queue.front();
        queue.pop_front();
        for (int k = 0; k < g.degree(u); ++k) {
            int v = g.neighborsBegin(u)[k];
            if (result.distance[v] == -1) {
                result.distance[v] = result.distance[u] + 1;
                result.parent[v] = u;
                queue.push_back(v);
            }
        }
    }
    return result;
}

DepthSearchResult dfs(const Graph& g, int source) {
    DepthSearchResult result;
    const int n = g.numVertices();
    result.parent.assign(n + 1, 0);
    result.discovery.assign(n + 1, 0);
    result.finish.assign(n + 1, 0);
    if (source < 1 || source > n) return result;

    std::vector<unsigned char> visited(n + 1, 0);
    int time = 0;
    std::vector<int> stack = {source};
    visited[source] = 1;
    result.discovery[source] = ++time;

    while (!stack.empty()) {
        int u = stack.back();
        bool pushed = false;
        for (int k = 0; k < g.degree(u); ++k) {
            int v = g.neighborsBegin(u)[k];
            if (!visited[v]) {
                visited[v] = 1;
                result.parent[v] = u;
                result.discovery[v] = ++time;
                stack.push_back(v);
                pushed = true;
                break;
            }
        }
        if (!pushed) {
            result.finish[u] = ++time;
            stack.pop_back();
        }
    }
    return result;
}

ShortestPathResult dijkstra(const Graph& g, int source, DijkstraMode mode) {
    validateDijkstraInput(g, source);

    if (mode == DijkstraMode::HEAP) {
        return runDijkstraImpl<HeapDistanceStore>(g, source);
    } else {
        return runDijkstraImpl<VectorDistanceStore>(g, source);
    }
}

ShortestPathResult dijkstraToTargets(const Graph& g, int source, DijkstraMode mode,
                                    const std::vector<int>& targets) {
    validateDijkstraInput(g, source);
    for (int target : targets) {
        if (target < 1 || target > g.numVertices()) {
            throw std::out_of_range("Vertice de destino fora do intervalo [1, n].");
        }
    }

    if (mode == DijkstraMode::HEAP) {
        return runDijkstraImpl<HeapDistanceStore>(g, source, &targets);
    }
    return runDijkstraImpl<VectorDistanceStore>(g, source, &targets);
}

std::vector<int> reconstructPath(const ShortestPathResult& res, int source, int target) {
    if (target < 1 || target >= static_cast<int>(res.dist.size())) {
        return {};
    }
    if (res.dist[target] == INF) {
        return {};
    }

    std::vector<int> path;
    for (int curr = target; curr != 0; curr = res.parent[curr]) {
        path.push_back(curr);
        if (curr == source) break;
    }

    if (path.empty() || path.back() != source) {
        return {};
    }

    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<int> reconstructPath(const BellmanFordResult& res, int source, int target) {
    if (res.hasNegativeCycle || target < 1 || target >= static_cast<int>(res.dist.size()) ||
        res.dist[target] == INF) return {};

    std::vector<int> path;
    for (int cur = target; cur != 0; cur = res.parent[cur]) {
        path.push_back(cur);
        if (cur == source) break;
        if (static_cast<int>(path.size()) > static_cast<int>(res.dist.size())) return {};
    }
    if (path.empty() || path.back() != source) return {};
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<int> reconstructPath(const FloydWarshallResult& res, int source, int target) {
    if (res.hasNegativeCycle || source < 1 || target < 1 ||
        source > res.numVertices || target > res.numVertices) return {};

    size_t pos = static_cast<size_t>(source) * (res.numVertices + 1) + target;
    if (std::isinf(res.dist[pos])) return {};

    std::vector<int> path = {source};
    int cur = source;
    while (cur != target) {
        pos = static_cast<size_t>(cur) * (res.numVertices + 1) + target;
        int next = res.next[pos];
        if (next == 0) return {};
        cur = next;
        path.push_back(cur);
        if (static_cast<int>(path.size()) > res.numVertices) return {};
    }
    return path;
}
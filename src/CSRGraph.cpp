#include "CSRGraph.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <tuple>

CSRGraph::CSRGraph(const std::string& filename, bool weighted, bool directed)
    : n_(0), m_(0), weighted_(weighted), directed_(directed), has_negative_(false) {

    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Erro ao abrir o arquivo de grafo: " + filename);
    }

    // Buffer de 1 MB para acelerar a leitura dos grafos W1-W5.
    std::vector<char> io_buffer(1 << 20);
    file.rdbuf()->pubsetbuf(io_buffer.data(), io_buffer.size());

    if (!(file >> n_) || n_ <= 0) {
        throw std::runtime_error("Cabecalho invalido para numero de vertices em: " + filename);
    }

    std::vector<int> degree_count(n_ + 1, 0);

    auto scan_edges = [&](auto&& process_edge) {
        long long edge_count = 0;
        if (weighted_) {
            int u, v;
            float w;
            while (file >> u >> v >> w) {
                if (u < 1 || u > n_ || v < 1 || v > n_) continue;
                process_edge(u, v, w);
                edge_count++;
            }
        } else {
            int u, v;
            while (file >> u >> v) {
                if (u < 1 || u > n_ || v < 1 || v > n_) continue;
                process_edge(u, v, 1.0f);
                edge_count++;
            }
        }
        return edge_count;
    };

    // Primeira passagem: conta graus e detecta pesos negativos sem guardar as arestas.
    m_ = scan_edges([&](int u, int v, float w) {
        if (w < 0.0f) has_negative_ = true;
        degree_count[u]++;
        if (!directed_) degree_count[v]++;
    });

    // Construção do vetor de offsets (prefix sum dos graus de cada vértice)
    offsets_.assign(n_ + 2, 0);
    for (int i = 1; i <= n_; ++i) {
        offsets_[i + 1] = offsets_[i] + degree_count[i];
    }

    const int total_directed_edges = offsets_[n_ + 1];
    edges_.resize(total_directed_edges);
    if (weighted_) {
        weights_.resize(total_directed_edges);
    }

    // Segunda passagem: preenche o CSR diretamente, sem uma cópia temporária de todas as arestas.
    std::vector<int> current_pos(offsets_.begin(), offsets_.end());

    file.clear();
    file.seekg(0, std::ios::beg);
    int second_vertex_count = 0;
    if (!(file >> second_vertex_count) || second_vertex_count != n_) {
        throw std::runtime_error("Nao foi possivel reler o cabecalho do grafo: " + filename);
    }

    const long long second_edge_count = scan_edges([&](int u, int v, float w) {
        int pos_u = current_pos[u]++;
        edges_[pos_u] = v;
        if (weighted_) weights_[pos_u] = w;

        if (!directed_) {
            int pos_v = current_pos[v]++;
            edges_[pos_v] = u;
            if (weighted_) weights_[pos_v] = w;
        }
    });
    if (second_edge_count != m_) {
        throw std::runtime_error("O arquivo de grafo mudou durante a leitura: " + filename);
    }
}

CSRGraph::CSRGraph(int n, const std::vector<std::pair<int, int>>& edges,
                   bool directed)
    : n_(n), m_(static_cast<long long>(edges.size())), weighted_(false),
      directed_(directed), has_negative_(false) {

    std::vector<int> degree_count(n_ + 1, 0);
    for (const auto& [u, v] : edges) {
        if (u >= 1 && u <= n_ && v >= 1 && v <= n_) {
            degree_count[u]++;
            if (!directed_) degree_count[v]++;
        }
    }

    offsets_.assign(n_ + 2, 0);
    for (int i = 1; i <= n_; ++i) {
        offsets_[i + 1] = offsets_[i] + degree_count[i];
    }

    edges_.resize(offsets_[n_ + 1]);
    std::vector<int> current_pos(offsets_.begin(), offsets_.end());

    for (const auto& [u, v] : edges) {
        if (u >= 1 && u <= n_ && v >= 1 && v <= n_) {
            edges_[current_pos[u]++] = v;
            if (!directed_) edges_[current_pos[v]++] = u;
        }
    }
}

CSRGraph::CSRGraph(int n, const std::vector<std::tuple<int, int, float>>& weighted_edges,
                   bool directed)
    : n_(n), m_(static_cast<long long>(weighted_edges.size())), weighted_(true),
      directed_(directed), has_negative_(false) {

    std::vector<int> degree_count(n_ + 1, 0);
    for (const auto& [u, v, w] : weighted_edges) {
        if (u >= 1 && u <= n_ && v >= 1 && v <= n_) {
            if (w < 0.0f) has_negative_ = true;
            degree_count[u]++;
            if (!directed_) degree_count[v]++;
        }
    }

    offsets_.assign(n_ + 2, 0);
    for (int i = 1; i <= n_; ++i) {
        offsets_[i + 1] = offsets_[i] + degree_count[i];
    }

    const int total_directed = offsets_[n_ + 1];
    edges_.resize(total_directed);
    weights_.resize(total_directed);

    std::vector<int> current_pos(offsets_.begin(), offsets_.end());

    for (const auto& [u, v, w] : weighted_edges) {
        if (u >= 1 && u <= n_ && v >= 1 && v <= n_) {
            int pos_u = current_pos[u]++;
            edges_[pos_u] = v;
            weights_[pos_u] = w;

            if (!directed_) {
                int pos_v = current_pos[v]++;
                edges_[pos_v] = u;
                weights_[pos_v] = w;
            }
        }
    }
}

int CSRGraph::numVertices() const {
    return n_;
}

long long CSRGraph::numEdges() const {
    return m_;
}

bool CSRGraph::isWeighted() const {
    return weighted_;
}

bool CSRGraph::isDirected() const {
    return directed_;
}

bool CSRGraph::hasNegativeWeight() const {
    return has_negative_;
}

int CSRGraph::degree(int u) const {
    return offsets_[u + 1] - offsets_[u];
}

const int* CSRGraph::neighborsBegin(int u) const {
    return edges_.data() + offsets_[u];
}

const int* CSRGraph::neighborsEnd(int u) const {
    return edges_.data() + offsets_[u + 1];
}

float CSRGraph::edgeWeight(int u, int k) const {
    if (!weighted_) return 1.0f;
    return weights_[offsets_[u] + k];
}

std::size_t CSRGraph::memoryUsageBytes() const {
    return sizeof(*this)
         + offsets_.capacity() * sizeof(int)
         + edges_.capacity() * sizeof(int)
         + weights_.capacity() * sizeof(float);
}
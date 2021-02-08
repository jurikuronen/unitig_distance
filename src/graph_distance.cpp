#include <limits>
#include <set>
#include <utility>
#include <vector>

#include "Graph.hpp"
#include "graph_distance.hpp"
#include "types.hpp"

int_t distance_naive(const Graph& graph, int_t source, int_t destination) {
    if (graph[source].component_id() != graph[destination].component_id()) return -1;
    std::vector<int_t> dist(graph.size(), std::numeric_limits<int_t>::max());
    dist[source] = 0;
    std::set<std::pair<int_t, int_t>> queue{{0, source}}; // (distance, node) pairs.
    while (!queue.empty()) {
        auto node = queue.begin()->second;
        if (node == destination) return dist[node];
        queue.erase(queue.begin());
        for (auto neighbor : graph[node].neighbors()) {
            if (dist[node] + 1 < dist[neighbor]) {
                queue.erase({dist[neighbor], neighbor});
                dist[neighbor] = dist[node] + 1;
                queue.insert({dist[neighbor], neighbor});
            }
        }
    }
    return -1;
}

/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef CLIQUE_GUARD_CLIQUE_GRAPH_HH
#define CLIQUE_GUARD_CLIQUE_GRAPH_HH 1

#include <vector>
#include <cstdint>

namespace clique
{
    /**
     * A graph, with an adjaceny matrix representation. We only provide the
     * operations we actually need.
     *
     * Indices start at 0.
     */
    class Graph
    {
        private:
            using AdjacencyMatrix = std::vector<std::uint8_t>;

            int _size = 0;
            AdjacencyMatrix _adjacency;

            /**
             * Return the appropriate offset into _adjacency for the edge (a,
             * b).
             */
            auto _position(int a, int b) const -> AdjacencyMatrix::size_type;

        public:
            /**
             * Number of vertices.
             */
            auto size() const -> int;

            /**
             * Change our size. Must be called before adding an edge, and must
             * not be called afterwards.
             */
            auto resize(int size) -> void;

            /**
             * Add an edge from a to b (and from b to a).
             */
            auto add_edge(int a, int b) -> void;

            /**
             * Are vertices a and b adjacent?
             */
            auto adjacent(int a, int b) const -> bool;

            /**
             * What is the degree of a given vertex?
             */
            auto degree(int a) const -> int;
    };
}

#endif

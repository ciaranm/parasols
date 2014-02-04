/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_GRAPH_HH
#define PARASOLS_GUARD_GRAPH_GRAPH_HH 1

#include <vector>
#include <string>
#include <cstdint>

namespace parasols
{
    /**
     * A graph, with an adjaceny matrix representation. We only provide the
     * operations we actually need.
     *
     * Indices start at 0.
     */
    class Graph
    {
        public:
            /**
             * The adjaceny matrix type. Shouldn't really be public, but we
             * snoop around inside it when doing message passing.
             */
            using AdjacencyMatrix = std::vector<std::uint8_t>;

        private:
            int _size = 0;
            AdjacencyMatrix _adjacency;
            bool _add_one_for_output;

            /**
             * Return the appropriate offset into _adjacency for the edge (a,
             * b).
             */
            auto _position(int a, int b) const -> AdjacencyMatrix::size_type;

        public:
            /**
             * \param initial_size can be 0, if resize() is called afterwards.
             *
             * \param add_one_for_output is true if the graph should be
             * displayed 1-indexed (this affects vertex_name, but vertex
             * numbers are still 0-indexed).
             */
            Graph(int initial_size, bool add_one_for_output);

            Graph(const Graph &) = default;

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

            /**
             * Format a vertex for outputting.
             *
             * Some graphs are 0-indexed, some are 1-indexed. Some might even
             * have named vertices. This turns a vertex number (0, size - 1)
             * into a string for human consumption.
             */
            auto vertex_name(int a) const -> std::string;

            /**
             * The adjaceny matrix. Shouldn't really be public, but we snoop
             * around inside it when doing message passing.
             */
            auto adjaceny_matrix() -> AdjacencyMatrix &
            {
                return _adjacency;
            }

            /**
             * Add one for output?
             */
            auto add_one_for_output() const -> bool
            {
                return _add_one_for_output;
            }
    };
}

#endif

/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <vertex_colouring/naive_vertex_colouring.hh>
#include <threads/output_lock.hh>

#include <iostream>

using std::chrono::steady_clock;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

using namespace parasols;

namespace
{
    using Assignments = std::vector<int>;

    auto print_incumbent(const VertexColouringParams & params, unsigned size) -> void
    {
        if (params.print_incumbents)
            std::cout
                << lock_output()
                << "-- " << duration_cast<milliseconds>(steady_clock::now() - params.start_time).count()
                << " found " << size << std::endl;
    }

    auto expand(const Graph & graph, const VertexColouringParams & params, unsigned long long & nodes,
            Assignments & assignments, std::map<int, int> & colouring, int & smallest_colouring_found,
            int vertex, int last_used_colour) -> void
    {
        if (params.abort->load())
            return;

        ++nodes;

        if (vertex >= graph.size()) {
            print_incumbent(params, last_used_colour);

            colouring.clear();
            for (int i = 0 ; i < graph.size() ; ++i)
                colouring.emplace(i, assignments.at(i));
            smallest_colouring_found = last_used_colour;
            return;
        }

        for (int colour = 1 ; colour <= last_used_colour + 1 ; ++colour) {
            bool ok = true;
            for (int i = 0 ; i < vertex && ok ; ++i)
                if (graph.adjacent(i, vertex) && assignments.at(i) == colour)
                    ok = false;

            if (ok) {
                int new_last_used_colour = std::max(colour, last_used_colour);
                if (new_last_used_colour < smallest_colouring_found) {
                    assignments.push_back(colour);
                    expand(graph, params, nodes, assignments, colouring, smallest_colouring_found, vertex + 1, new_last_used_colour);
                    assignments.pop_back();
                }
            }
        }
    }
}

auto parasols::naive_vertex_colouring(const Graph & graph, const VertexColouringParams & params) -> VertexColouringResult
{
    VertexColouringResult result;
    result.n_colours = graph.size() + 1;

    Assignments assignments;
    assignments.reserve(graph.size());
    expand(graph, params, result.nodes, assignments, result.colouring, result.n_colours, 0, 1);

    return result;
}


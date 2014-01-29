/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/is_vertex_colouring.hh>

using namespace parasols;

auto
parasols::is_vertex_colouring(const Graph & graph, const std::vector<int> & colouring) -> bool
{
    for (int a = 0 ; a < graph.size() ; ++a)
        for (int b = 0 ; b < graph.size() ; ++b)
            if (colouring[a] == colouring[b] && graph.adjacent(a, b))
                return false;

    return true;
}


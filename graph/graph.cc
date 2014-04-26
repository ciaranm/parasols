/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/graph.hh>
#include <algorithm>

using namespace parasols;

Graph::Graph(int size, bool add_one_for_output) :
    _add_one_for_output(add_one_for_output)
{
    if (0 != size)
        resize(size);
}

auto Graph::_position(int a, int b) const -> AdjacencyMatrix::size_type
{
    return (a * _size) + b;
}

auto Graph::resize(int size) -> void
{
    _size = size;
    _adjacency.resize(size * size);
}

auto Graph::add_edge(int a, int b) -> void
{
    _adjacency[_position(a, b)] = 1;
    _adjacency[_position(b, a)] = 1;
}

auto Graph::adjacent(int a, int b) const -> bool
{
    return _adjacency[_position(a, b)];
}

auto Graph::size() const -> int
{
    return _size;
}

auto Graph::degree(int a) const -> int
{
    return std::count_if(
            _adjacency.begin() + a * _size,
            _adjacency.begin() + (a + 1) * _size,
            [] (int x) { return !!x; });
}

auto Graph::vertex_name(int a) const -> std::string
{
    if (_add_one_for_output)
        return std::to_string(a + 1);
    else
        return std::to_string(a);
}

auto Graph::vertex_number(const std::string & t) const -> int
{
    if (_add_one_for_output)
        return std::stoi(t) - 1;
    else
        return std::stoi(t);
}


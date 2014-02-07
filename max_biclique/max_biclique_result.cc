/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_biclique/max_biclique_result.hh>

using namespace parasols;

auto MaxBicliqueResult::merge(MaxBicliqueResult && other) -> void
{
    nodes += other.nodes;
    if (other.size > size) {
        size = other.size;
        members_a = std::move(other.members_a);
        members_b = std::move(other.members_b);
    }
}


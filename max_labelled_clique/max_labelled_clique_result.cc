/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_labelled_clique/max_labelled_clique_result.hh>

using namespace parasols;

auto MaxLabelledCliqueResult::merge(const MaxLabelledCliqueResult & other) -> void
{
    nodes += other.nodes;
    if (other.size > size || (other.size == size && other.cost < cost)) {
        size = other.size;
        cost = other.cost;
        members = std::move(other.members);
    }
}


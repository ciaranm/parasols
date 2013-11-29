/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/max_clique_result.hh>

using namespace parasols;

auto MaxCliqueResult::merge(const MaxCliqueResult & other) -> void
{
    nodes += other.nodes;
    donations += other.donations;
    result_count += other.result_count;
    result_club_count += other.result_club_count;
    if (other.size > size) {
        size = other.size;
        members = std::move(other.members);
    }
}


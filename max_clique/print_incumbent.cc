/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/print_incumbent.hh>
#include <threads/output_lock.hh>
#include <graph/is_club.hh>

#include <iostream>
#include <sstream>

using namespace parasols;

auto parasols::print_incumbent(const MaxCliqueParams & params, unsigned size) -> void
{
    if (params.print_incumbents)
        std::cout
            << lock_output()
            << "-- " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time).count()
            << " found " << size << std::endl;
}

auto parasols::print_incumbent(
        const MaxCliqueParams & params,
        unsigned size,
        const std::set<int> & members,
        const std::vector<int> & positions) -> void
{
    if (params.print_incumbents) {
        std::stringstream w;
        for (auto & p : positions)
            w << " " << p;

        std::stringstream c;
        if (params.check_clubs) {
            if (is_club(*params.original_graph, params.power, members))
                c << " (club)";
            else
                c << " (not club)";
        }

        std::cout
            << lock_output()
            << "-- " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time).count()
            << " found " << size << " at" << w.str() << c.str() << std::endl;
    }
}


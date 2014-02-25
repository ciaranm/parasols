/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_labelled_clique/print_incumbent.hh>
#include <threads/output_lock.hh>
#include <iostream>
#include <sstream>

using namespace parasols;

auto parasols::print_incumbent(
        const MaxLabelledCliqueParams & params,
        unsigned size,
        unsigned cost,
        const std::vector<int> & positions) -> void
{
    if (params.print_incumbents) {
        std::stringstream w;
        for (auto & p : positions)
            w << " " << p;

        std::cout
            << lock_output()
            << "-- " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time).count()
            << " found " << size << " / " << cost << " at" << w.str() << std::endl;
    }
}

auto parasols::print_position(
        const MaxLabelledCliqueParams & params,
        const std::string & message,
        const std::vector<int> & positions) -> void
{
    if (params.print_incumbents) {
        std::stringstream w;
        for (auto & p : positions)
            w << " " << p;

        std::cout
            << lock_output()
            << "-- " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time).count()
            << " " << message << " at" << w.str() << std::endl;
    }
}


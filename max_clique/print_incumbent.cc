/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/print_incumbent.hh>

#include <iostream>

using namespace parasols;

auto parasols::print_incumbent(const MaxCliqueParams & params, unsigned size) -> void
{
    if (params.print_incumbents)
        std::cout << "-- " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time).count()
            << " found " << size << std::endl;
}


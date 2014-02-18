/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_kclub/print_incumbent.hh>
#include <threads/output_lock.hh>
#include <iostream>
#include <sstream>

using namespace parasols;

auto parasols::print_incumbent(
        const MaxKClubParams & params,
        unsigned size,
        unsigned old_best,
        bool feasible,
        const std::vector<int> & positions) -> void
{
    if (params.print_incumbents) {
        std::stringstream w;
        for (auto & p : positions)
            w << " " << p;

        std::cout
            << lock_output()
            << "-- " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time).count()
            << " found " << (feasible ? "" : "(") << size << (feasible ? "" : ")") << " / " << old_best << " at" << w.str() << std::endl;
    }
}


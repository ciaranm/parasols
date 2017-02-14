/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/print_incumbent.hh>
#include <threads/output_lock.hh>

#include <iostream>
#include <sstream>

using namespace parasols;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

auto parasols::print_incumbent(const MaxCliqueParams & params, unsigned size) -> void
{
    if (params.print_incumbents)
        std::cout
            << lock_output()
            << "-- " << duration_cast<milliseconds>(steady_clock::now() - params.start_time).count()
            << " found " << size << std::endl;
}

auto parasols::print_incumbent(
        const MaxCliqueParams & params,
        unsigned size,
        const std::vector<int> & positions) -> void
{
    if (params.print_incumbents) {
        std::stringstream w;
        for (auto & p : positions)
            w << " " << p;

        std::cout
            << lock_output()
            << "-- " << duration_cast<milliseconds>(steady_clock::now() - params.start_time).count()
            << " found " << size << " at" << w.str() << std::endl;
    }
}

auto parasols::print_incumbent(
        const MaxCliqueParams & params,
        unsigned size,
        const std::vector<int> & positions,
        bool is_club) -> void
{
    if (params.print_incumbents) {
        std::stringstream w;
        for (auto & p : positions)
            w << " " << p;

        std::stringstream c;
        if (params.check_clubs) {
            if (is_club)
                c << " (club)";
            else
                c << " (not club)";
        }

        std::cout
            << lock_output()
            << "-- " << duration_cast<milliseconds>(steady_clock::now() - params.start_time).count()
            << " found " << size << " at" << w.str() << c.str() << std::endl;
    }
}

auto parasols::print_position(
        const MaxCliqueParams & params,
        const std::string & message,
        const std::vector<int> & positions) -> void
{
    if (params.print_incumbents) {
        std::stringstream w;
        for (auto & p : positions)
            w << " " << p;

        std::cout
            << lock_output()
            << "-- " << duration_cast<milliseconds>(steady_clock::now() - params.start_time).count()
            << " " << message << " at" << w.str() << std::endl;
    }
}

auto parasols::print_incumbent(const MaxCliqueParams & params,
        unsigned size,
        const std::vector<int> & positions,
        const std::set<int> & vertices) -> void
{
    if (params.print_incumbents) {
        std::stringstream w;
        for (auto & p : positions)
            w << " " << p;

        std::stringstream v;
        for (auto & p : vertices)
            v << " " << p;

        std::cout
            << lock_output()
            << "-- " << duration_cast<milliseconds>(steady_clock::now() - params.start_time).count()
            << " found " << size << " with vertices" << v.str() << " at" << w.str() << std::endl;
    }
}


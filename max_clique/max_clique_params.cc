/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/max_clique_params.hh>

#include <iostream>

using namespace parasols;

auto parasols::print_candidate(const MaxCliqueParams & params, unsigned size) -> void
{
    std::cout << "-- " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time).count()
        << " found " << size << std::endl;
}

auto parasols::print_candidate(
        const MaxCliqueParams & params,
        const MaxCliqueResult & result,
        const std::vector<std::pair<int, int> > & choices_of_n) -> void
{
    std::cout << "-- " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time).count()
        << " found " << result.size << " at";
    
    for (auto & n : choices_of_n) {
        if (0 == n.first)
            break;
        std::cout << " " << n.first << "/" << n.second;
    }

    std::cout << std::endl;
}


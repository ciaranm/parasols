/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <roommates/roommates_file.hh>
#include <roommates/roommates_params.hh>
#include <roommates/roommates_result.hh>
#include <roommates/irving_roommates.hh>

#include <iostream>
#include <iomanip>
#include <exception>
#include <algorithm>
#include <random>
#include <cstdlib>

using namespace parasols;

std::mt19937 rnd;

auto main(int, char * []) -> int
{
    for (int size = 2 ; size <= 25000 ; size += (size < 100 ? 2 : size < 1000 ? 10 : 100)) {
        std::cout << std::setw(8) << size << "  ";

        unsigned yes_count = 0;
        for (int pass = 0 ; pass < 1000 ; ++pass) {
            RoommatesProblem problem;
            problem.size = size;
            problem.preferences.resize(size);
            for (int i = 0 ; i < size ; ++i) {
                problem.preferences.at(i).resize(size);
                unsigned v = 0;
                for (int j = 0 ; j < size ; ++j) {
                    if (i == j)
                        continue;
                    problem.preferences.at(i).at(v++) = j;
                }
                problem.preferences.at(i).at(size - 1) = i;
                std::shuffle(problem.preferences.at(i).begin(), problem.preferences.at(i).end() - 1, rnd);
            }

            problem.rankings.resize(size);
            for (int i = 0 ; i < size ; ++i) {
                problem.rankings.at(i).resize(size);
                for (int j = 0 ; j < size ; ++j) {
                    problem.rankings.at(i).at(problem.preferences.at(i).at(j)) = j;
                }
            }

            RoommatesParams params;
            params.start_time = std::chrono::steady_clock::now();

            auto result = irving_roommates(problem, params);
            if (result.success)
                ++yes_count;
        }

        std::cout << std::setw(8) << yes_count << "  " << std::setw(8)
            << (double(yes_count) / 1000.0) << std::endl;
    }
}


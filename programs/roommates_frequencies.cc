/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <roommates/roommates_file.hh>
#include <roommates/roommates_params.hh>
#include <roommates/roommates_result.hh>
#include <roommates/irving_roommates.hh>

#include <boost/program_options.hpp>

#include <iostream>
#include <iomanip>
#include <exception>
#include <algorithm>
#include <random>
#include <cstdlib>

using namespace parasols;
namespace po = boost::program_options;

std::mt19937 rnd;

void table(int start, int end, int inc, int samples)
{
    for (int size = start ; size <= end ; size += inc) {
        std::cout << std::setw(8) << size << "  ";

        unsigned yes_count = 0;
        for (int pass = 0 ; pass < samples ; ++pass) {
            RoommatesProblem problem;
            problem.size = size;
            problem.preferences.resize(size);
            for (int i = 0 ; i < size ; ++i) {
                problem.preferences[i].resize(size);
                unsigned v = 0;
                for (int j = 0 ; j < size ; ++j) {
                    if (i == j)
                        continue;
                    problem.preferences[i][v++] = j;
                }
                problem.preferences[i][size - 1] = i;
                std::shuffle(problem.preferences[i].begin(), problem.preferences[i].end() - 1, rnd);
            }

            problem.rankings.resize(size);
            for (int i = 0 ; i < size ; ++i) {
                problem.rankings[i].resize(size);
                for (int j = 0 ; j < size ; ++j) {
                    problem.rankings[i][problem.preferences[i][j]] = j;
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

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                  "Display help information")
            ;

        po::options_description all_options{ "All options" };
        all_options.add(display_options);

        all_options.add_options()
            ("start",     po::value<int>(),     "Start size")
            ("end",       po::value<int>(),     "End size")
            ("inc",       po::value<int>(),     "Size increment")
            ("samples",   po::value<int>(),     "Sample size")
            ;

        po::positional_options_description positional_options;
        positional_options
            .add("start",   1)
            .add("end",     1)
            .add("inc",     1)
            .add("samples", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] start end inc samples" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No values specified? Show a message and exit. */
        if (! options_vars.count("start") || ! options_vars.count("end") || ! options_vars.count("inc")
                || ! options_vars.count("samples")) {
            std::cout << "Usage: " << argv[0] << " [options] start end inc samples" << std::endl;
            return EXIT_FAILURE;
        }

        int start = options_vars["start"].as<int>();
        int end = options_vars["end"].as<int>();
        int step = options_vars["inc"].as<int>();
        int samples = options_vars["samples"].as<int>();

        table(start, end, step, samples);

        return EXIT_SUCCESS;
    }
    catch (const po::error & e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Try " << argv[0] << " --help" << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception & e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}


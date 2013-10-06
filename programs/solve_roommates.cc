/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <solver/solver.hh>

#include <roommates/roommates_file.hh>
#include <roommates/roommates_params.hh>
#include <roommates/roommates_result.hh>
#include <roommates/irving_roommates.hh>

#include <boost/program_options.hpp>

#include <iostream>
#include <exception>
#include <cstdlib>
#include <chrono>
#include <thread>

using namespace parasols;
namespace po = boost::program_options;

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                 "Display help information")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("input-file", "Specify the input file")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("input-file", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] file" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No input file specified? Show a message and exit. */
        if (! options_vars.count("input-file")) {
            std::cout << "Usage: " << argv[0] << " [options] file" << std::endl;
            return EXIT_FAILURE;
        }

        /* Read in the graph */
        auto problem = read_roommates(options_vars["input-file"].as<std::string>());

        RoommatesParams params;

        /* Do the actual run. */
        bool aborted = false;
        auto result = run_this(irving_roommates)(
                problem,
                params,
                aborted,
                0);

        /* Stop the clock. */
        auto overall_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - params.start_time);

        /* Display the results. */
        if (result.success) {
            for (auto & m : result.members)
                std::cout << "(" << m.first + 1 << ", " << m.second + 1 << ") ";
            std::cout << std::endl;
        }
        else
            std::cout << "no solution" << std::endl;

        std::cout << (overall_time.count() / 1000.0);
        if (! result.times.empty()) {
            for (auto t : result.times)
                std::cout << " " << t.count();
        }
        if (aborted)
            std::cout << " aborted";
        std::cout << std::endl;

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



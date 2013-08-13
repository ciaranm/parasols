/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <solver/solver.hh>

#include <graph/graph.hh>
#include <graph/dimacs.hh>

#include <max_clique/naive_max_clique.hh>
#include <max_clique/mcsa1_max_clique.hh>
#include <max_clique/tmcsa1_max_clique.hh>
#include <max_clique/bmcsa1_max_clique.hh>
#include <max_clique/tbmcsa1_max_clique.hh>

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
    auto algorithms = {
        std::make_pair( std::string{ "naive" },   run_this(naive_max_clique) ),
        std::make_pair( std::string{ "mcsa1" },   run_this(mcsa1_max_clique) ),
        std::make_pair( std::string{ "tmcsa1" },  run_this(tmcsa1_max_clique) ),
        std::make_pair( std::string{ "bmcsa1" },  run_this(bmcsa1_max_clique) ),
        std::make_pair( std::string{ "tbmcsa1" }, run_this(tbmcsa1_max_clique) )
    };

    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                 "Display help information")
            ("threads",            po::value<int>(), "Number of threads to use (where relevant)")
            ("stop-after-finding", po::value<int>(), "Stop after finding a clique of this size")
            ("initial-bound",      po::value<int>(), "Specify an initial bound")
            ("print-incumbents",                     "Print new incumbents as they are found")
            ("split-depth",        po::value<int>(), "Specify the depth at which to perform splitting (where relevant)")
            ("work-donation",                        "Enable work donation (where relevant)")
            ("timeout",            po::value<int>(), "Abort after this many seconds")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("algorithm", "Specify which algorithm to use")
            ("input-file", "Specify the input file (DIMACS format)")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("algorithm", 1)
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
            std::cout << "Usage: " << argv[0] << " [options] algorithm file" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No algorithm or no input file specified? Show a message and exit. */
        if (! options_vars.count("algorithm") || ! options_vars.count("input-file")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm file" << std::endl;
            return EXIT_FAILURE;
        }

        /* Turn an algorithm string name into a runnable function. */
        auto algorithm = algorithms.begin(), algorithm_end = algorithms.end();
        for ( ; algorithm != algorithm_end ; ++algorithm)
            if (algorithm->first == options_vars["algorithm"].as<std::string>())
                break;

        /* Unknown algorithm? Show a message and exit. */
        if (algorithm == algorithm_end) {
            std::cerr << "Unknown algorithm " << options_vars["algorithm"].as<std::string>() << ", choose from:";
            for (auto a : algorithms)
                std::cerr << " " << a.first;
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        /* Figure out what our options should be. */
        MaxCliqueParams params;

        if (options_vars.count("threads"))
            params.n_threads = options_vars["threads"].as<int>();
        else
            params.n_threads = std::thread::hardware_concurrency();

        if (options_vars.count("stop-after-finding"))
            params.stop_after_finding = options_vars["stop-after-finding"].as<int>();

        if (options_vars.count("initial-bound"))
            params.initial_bound = options_vars["initial-bound"].as<int>();

        if (options_vars.count("print-incumbents"))
            params.print_incumbents = true;

        if (options_vars.count("split-depth"))
            params.split_depth = options_vars["split-depth"].as<int>();

        if (options_vars.count("work-donation"))
            params.work_donation = true;

        /* Read in the graph */
        auto graph = read_dimacs(options_vars["input-file"].as<std::string>());

        /* Do the actual run. */
        bool aborted = false;
        auto result = algorithm->second(
                graph,
                params,
                aborted,
                options_vars.count("timeout") ? options_vars["timeout"].as<int>() : 0);

        /* Stop the clock. */
        auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time);

        /* Display the results. */
        std::cout << result.size << " " << result.nodes;
        if (aborted)
            std::cout << " aborted " << result.top_nodes_done;
        std::cout << std::endl;

        for (auto v : result.members)
            std::cout << v + 1 << " ";
        std::cout << std::endl;

        std::cout << overall_time.count();
        if (! result.times.empty()) {
            for (auto t : result.times)
                std::cout << " " << t.count();
        }
        std::cout << std::endl;

        if (params.work_donation)
            std::cout << result.donations << std::endl;

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


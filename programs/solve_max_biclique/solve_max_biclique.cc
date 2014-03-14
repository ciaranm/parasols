/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <solver/solver.hh>

#include <graph/graph.hh>
#include <graph/file_formats.hh>
#include <graph/orders.hh>

#include <max_biclique/algorithms.hh>

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
            ("help",                                  "Display help information")
            ("threads",            po::value<int>(),  "Number of threads to use (where relevant)")
            ("stop-after-finding", po::value<int>(),  "Stop after finding a biclique of this size")
            ("initial-bound",      po::value<int>(),  "Specify an initial bound")
            ("print-incumbents",                      "Print new incumbents as they are found")
            ("timeout",            po::value<int>(),  "Abort after this many seconds")
            ("break-ab-symmetry",  po::value<bool>(), "Break a/b symmetry (on by default)")
            ("verify",                                "Verify that we have found a valid result (for sanity checking changes)")
            ("format",             po::value<std::string>(), "Specify the format of the input")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("algorithm",  "Specify which algorithm to use")
            ("order",      "Specify the initial vertex order")
            ("input-file", "Specify the input file (DIMACS format, unless --pairs is specified)")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("algorithm", 1)
            .add("order", 1)
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
            std::cout << "Usage: " << argv[0] << " [options] algorithm order file" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No algorithm or no input file specified? Show a message and exit. */
        if (! options_vars.count("algorithm") || ! options_vars.count("input-file")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm order file" << std::endl;
            return EXIT_FAILURE;
        }

        /* Turn an algorithm string name into a runnable function. */
        auto algorithm = max_biclique_algorithms.begin(), algorithm_end = max_biclique_algorithms.end();
        for ( ; algorithm != algorithm_end ; ++algorithm)
            if (algorithm->first == options_vars["algorithm"].as<std::string>())
                break;

        /* Unknown algorithm? Show a message and exit. */
        if (algorithm == algorithm_end) {
            std::cerr << "Unknown algorithm " << options_vars["algorithm"].as<std::string>() << ", choose from:";
            for (auto a : max_biclique_algorithms)
                std::cerr << " " << a.first;
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        /* Turn an order string name into a runnable function. */
        MaxBicliqueOrderFunction order_function;
        for (auto order = orders.begin() ; order != orders.end() ; ++order)
            if (std::get<0>(*order) == options_vars["order"].as<std::string>()) {
                order_function = std::get<1>(*order);
                break;
            }

        /* Unknown algorithm? Show a message and exit. */
        if (! order_function) {
            std::cerr << "Unknown order " << options_vars["order"].as<std::string>() << ", choose from:";
            for (auto a : orders)
                std::cerr << " " << std::get<0>(a);
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        /* Figure out what our options should be. */
        MaxBicliqueParams params;
        params.order_function = order_function;

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

        if (options_vars.count("break-ab-symmetry"))
            params.break_ab_symmetry = options_vars["break-ab-symmetry"].as<bool>();

        /* Turn a format name into a runnable function. */
        auto format = graph_file_formats.begin(), format_end = graph_file_formats.end();
        if (options_vars.count("format"))
            for ( ; format != format_end ; ++format)
                if (format->first == options_vars["format"].as<std::string>())
                    break;

        /* Unknown format? Show a message and exit. */
        if (format == format_end) {
            std::cerr << "Unknown format " << options_vars["format"].as<std::string>() << ", choose from:";
            for (auto a : graph_file_formats)
                std::cerr << " " << a.first;
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        /* Read in the graph */
        auto graph = std::get<1>(*format)(options_vars["input-file"].as<std::string>());

        /* Do the actual run. */
        bool aborted = false;
        auto result = run_this(algorithm->second)(
                graph,
                params,
                aborted,
                options_vars.count("timeout") ? options_vars["timeout"].as<int>() : 0);

        /* Stop the clock. */
        auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time);

        /* Display the results. */
        std::cout << result.size << " " << result.nodes;
        if (aborted)
            std::cout << " aborted";
        std::cout << std::endl;

        for (auto & s : { result.members_a, result.members_b }) {
            for (auto v : s)
                std::cout << graph.vertex_name(v) << " ";
            std::cout << std::endl;
        }

        std::cout << overall_time.count();
        if (! result.times.empty()) {
            for (auto t : result.times)
                std::cout << " " << t.count();
        }
        std::cout << std::endl;

        if (options_vars.count("verify")) {
            for (auto & a : result.members_a)
                for (auto & b : result.members_b)
                    if (! graph.adjacent(a, b)) {
                        std::cerr << "Oops! Not adjacent: " << a << " " << b << std::endl;
                        return EXIT_FAILURE;
                    }
        }

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


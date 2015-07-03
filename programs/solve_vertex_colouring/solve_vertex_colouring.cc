/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <solver/solver.hh>

#include <graph/graph.hh>
#include <graph/file_formats.hh>
#include <graph/orders.hh>

#include <vertex_colouring/algorithms.hh>

#include <boost/program_options.hpp>

#include <iostream>
#include <exception>
#include <cstdlib>
#include <chrono>
#include <thread>

using namespace parasols;
namespace po = boost::program_options;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                  "Display help information")
            ("threads",            po::value<int>(),  "Number of threads to use (where relevant)")
            ("timeout",            po::value<int>(),  "Abort after this many seconds")
            ("format",             po::value<std::string>(), "Specify the format of the input")
            ("verify",                                "Verify that we have found a valid result (for sanity checking changes)")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("algorithm",    "Specify which algorithm to use")
            ("graph",        "Specify the graph file (DIMACS format, unless --format is specified)")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("algorithm", 1)
            .add("graph", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm graph" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No algorithm or no input file specified? Show a message and exit. */
        if (! options_vars.count("algorithm") || ! options_vars.count("graph")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm graph" << std::endl;
            return EXIT_FAILURE;
        }

        /* Turn an algorithm string name into a runnable function. */
        auto algorithm = vertex_colouring_algorithms.begin(), algorithm_end = vertex_colouring_algorithms.end();
        for ( ; algorithm != algorithm_end ; ++algorithm)
            if (algorithm->first == options_vars["algorithm"].as<std::string>())
                break;

        /* Unknown algorithm? Show a message and exit. */
        if (algorithm == algorithm_end) {
            std::cerr << "Unknown algorithm " << options_vars["algorithm"].as<std::string>() << ", choose from:";
            for (auto a : vertex_colouring_algorithms)
                std::cerr << " " << a.first;
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        /* Figure out what our options should be. */
        VertexColouringParams params;

        if (options_vars.count("threads"))
            params.n_threads = options_vars["threads"].as<int>();
        else
            params.n_threads = std::thread::hardware_concurrency();

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

        /* Read in the graphs */
        auto graph = std::get<1>(*format)(options_vars["graph"].as<std::string>(), GraphOptions::None);

        /* Do the actual run. */
        bool aborted = false;
        auto result = run_this(algorithm->second)(
                graph,
                params,
                aborted,
                options_vars.count("timeout") ? options_vars["timeout"].as<int>() : 0);

        /* Stop the clock. */
        auto overall_time = duration_cast<milliseconds>(steady_clock::now() - params.start_time);

        /* Display the results. */
        std::cout << result.n_colours << " " << result.nodes;

        if (aborted)
            std::cout << " aborted";
        std::cout << std::endl;

        for (auto v : result.colouring)
            std::cout << "(" << graph.vertex_name(v.first) << " -> " << v.second << ") ";
        std::cout << std::endl;

        std::cout << overall_time.count();
        if (! result.times.empty()) {
            for (auto t : result.times)
                std::cout << " " << t.count();
        }
        std::cout << std::endl;

        if (options_vars.count("verify"))
            for (int i = 0 ; i < graph.size() ; ++i) {
                for (int j = 0 ; j < graph.size() ; ++j) {
                    if (graph.adjacent(i, j) && *result.colouring.find(i) == *result.colouring.find(j)) {
                        std::cerr << "Oops! not a colouring" << std::endl;
                        return EXIT_FAILURE;
                    }
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


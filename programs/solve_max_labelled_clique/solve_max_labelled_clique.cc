/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <solver/solver.hh>

#include <graph/graph.hh>
#include <graph/file_formats.hh>
#include <graph/is_clique.hh>
#include <graph/orders.hh>

#include <max_labelled_clique/algorithms.hh>
#include <max_labelled_clique/make_random_labels.hh>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

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
            ("help",                                 "Display help information")
            ("threads",            po::value<int>(), "Number of threads to use (where relevant)")
            ("stop-after-finding", po::value<int>(), "Stop after finding a labelled clique of this size")
            ("initial-bound",      po::value<int>(), "Specify an initial bound")
            ("print-incumbents",                     "Print new incumbents as they are found")
            ("timeout",            po::value<int>(), "Abort after this many seconds")
            ("verify",                               "Verify that we have found a valid result (for sanity checking changes)")
            ("format",             po::value<std::string>(), "Specify the format of the input")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("algorithm",  "Specify which algorithm to use")
            ("order",      "Specify the initial vertex order")
            ("labels",     po::value<int>(), "Specify the number of labels to use")
            ("budget",     po::value<int>(), "Specify the budget")
            ("seed",       po::value<int>(), "Specify the seed")
            ("input-file", po::value<std::vector<std::string> >(),
                           "Specify the input file (DIMACS format, unless --format is specified). May be specified multiple times.")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("algorithm", 1)
            .add("order", 1)
            .add("labels", 1)
            .add("budget", 1)
            .add("seed", 1)
            .add("input-file", -1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm order labels budget seed file[...]" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No algorithm or no input file specified? Show a message and exit. */
        if (! options_vars.count("algorithm") || options_vars.count("input-file") < 1
                || ! options_vars.count("labels") || ! options_vars.count("budget") || ! options_vars.count("seed")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm order labels budget seed file[...]" << std::endl;
            return EXIT_FAILURE;
        }

        /* Turn an algorithm string name into a runnable function. */
        auto algorithm = max_labelled_clique_algorithms.begin(), algorithm_end = max_labelled_clique_algorithms.end();
        for ( ; algorithm != algorithm_end ; ++algorithm)
            if (std::get<0>(*algorithm) == options_vars["algorithm"].as<std::string>())
                break;

        /* Unknown algorithm? Show a message and exit. */
        if (algorithm == algorithm_end) {
            std::cerr << "Unknown algorithm " << options_vars["algorithm"].as<std::string>() << ", choose from:";
            for (auto a : max_labelled_clique_algorithms)
                std::cerr << " " << std::get<0>(a);
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        /* Turn an order string name into a runnable function. */
        MaxLabelledCliqueOrderFunction order_function;
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

        /* For each input file... */
        auto input_files = options_vars["input-file"].as<std::vector<std::string> >();
        bool first = true;
        for (auto & input_file : input_files) {
            if (first)
                first = false;
            else
                std::cout << "--" << std::endl;

            /* Figure out what our options should be. */
            MaxLabelledCliqueParams params;

            params.order_function = order_function;
            params.budget = options_vars["budget"].as<int>();

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
            auto graph = std::get<1>(*format)(input_file);

            /* Create labels */
            params.labels = make_random_labels(graph.size(), options_vars["labels"].as<int>(),
                    options_vars["seed"].as<int>());

            /* Do the actual run. */
            bool aborted = false;
            auto result = run_this(std::get<1>(*algorithm))(
                    graph,
                    params,
                    aborted,
                    options_vars.count("timeout") ? options_vars["timeout"].as<int>() : 0);

            /* Stop the clock. */
            auto overall_time = duration_cast<milliseconds>(steady_clock::now() - params.start_time);

            /* Display the results. */
            std::cout << result.size << " " << result.cost << " " << result.nodes;

            if (aborted)
                std::cout << " aborted";

            std::cout << std::endl;

            /* Members. */
            for (auto v : result.members)
                std::cout << graph.vertex_name(v) << " ";
            std::cout << std::endl;

            /* Times */
            std::cout << overall_time.count();
            if (! result.times.empty()) {
                for (auto t : result.times)
                    std::cout << " " << t.count();
            }
            std::cout << std::endl;

            /* verify */
            if (options_vars.count("verify")) {
                if (! is_clique(graph, result.members)) {
                    std::cerr << "Oops! not a clique" << std::endl;
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


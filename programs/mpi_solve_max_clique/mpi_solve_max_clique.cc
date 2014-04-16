/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <solver/solver.hh>

#include <graph/graph.hh>
#include <graph/file_formats.hh>
#include <graph/orders.hh>
#include <graph/is_clique.hh>

#include <max_clique/mpicco_max_clique.hh>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/mpi.hpp>

#include <iostream>
#include <exception>
#include <cstdlib>
#include <chrono>
#include <thread>

using namespace parasols;
namespace po = boost::program_options;
namespace mpi = boost::mpi;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

auto main(int argc, char * argv[]) -> int
{
    mpi::environment env;
    mpi::communicator world;

    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                 "Display help information")
            ("stop-after-finding", po::value<int>(), "Stop after finding a clique of this size")
            ("initial-bound",      po::value<int>(), "Specify an initial bound")
            ("print-incumbents",                     "Print new incumbents as they are found")
            ("timeout",            po::value<int>(), "Abort after this many seconds")
            ("verify",                               "Verify that we have found a valid result (for sanity checking changes)")
            ("format",             po::value<std::string>(), "Specify the format of the input")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("order",      "Specify the initial vertex order")
            ("input-file", po::value<std::vector<std::string> >(),
                           "Specify the input file (DIMACS format, unless --format is specified). May be specified multiple times.")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("order", 1)
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
            if (0 == world.rank()) {
                std::cout << "Usage: " << argv[0] << " [options] order file[...]" << std::endl;
                std::cout << std::endl;
                std::cout << display_options << std::endl;
            }
            return EXIT_SUCCESS;
        }

        /* No order or no input file specified? Show a message and exit. */
        if (! options_vars.count("order") || options_vars.count("input-file") < 1) {
            if (0 == world.rank())
                std::cout << "Usage: " << argv[0] << " [options] order file[...]" << std::endl;
            return EXIT_FAILURE;
        }

        /* Turn an order string name into a runnable function. */
        MaxCliqueOrderFunction order_function;
        for (auto order = orders.begin() ; order != orders.end() ; ++order)
            if (std::get<0>(*order) == options_vars["order"].as<std::string>()) {
                order_function = std::get<1>(*order);
                break;
            }

        /* Unknown order? Show a message and exit. */
        if (! order_function) {
            if (0 == world.rank()) {
                std::cerr << "Unknown order " << options_vars["order"].as<std::string>() << ", choose from:";
                for (auto a : orders)
                    std::cerr << " " << std::get<0>(a);
                std::cerr << std::endl;
            }
            return EXIT_FAILURE;
        }

        /* For each input file... */
        auto input_files = options_vars["input-file"].as<std::vector<std::string> >();
        bool first = true;
        for (auto & input_file : input_files) {
            if (first)
                first = false;
            else if (0 == world.rank())
                std::cout << "--" << std::endl;

            /* Figure out what our options should be. */
            MaxCliqueParams params;

            params.order_function = order_function;

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
                if (0 == world.rank()) {
                    std::cerr << "Unknown format " << options_vars["format"].as<std::string>() << ", choose from:";
                    for (auto a : graph_file_formats)
                        std::cerr << " " << a.first;
                    std::cerr << std::endl;
                }
                return EXIT_FAILURE;
            }

            Graph graph(0, false);

            /* Read in and transmit, or receive the graph */
            if (0 == world.rank()) {
                /* Read in the graph */
                graph = std::get<1>(*format)(input_file);

                int size = graph.size();
                mpi::broadcast(world, size, 0);
                mpi::broadcast(world, graph.adjaceny_matrix(), 0);
            }
            else {
                int size;
                mpi::broadcast(world, size, 0);

                graph = Graph{ size, false };
                mpi::broadcast(world, graph.adjaceny_matrix(), 0);
            }

            params.original_graph = &graph;

            std::atomic<bool> abort;
            abort.store(false);
            params.abort = &abort;

            params.start_time = std::chrono::steady_clock::now();

            /* Do the actual run. */
            auto result = mpicco_max_clique<CCOPermutations::None, CCOInference::None>(env, world, graph, params);

            /* Stop the clock. */
            auto overall_time = duration_cast<milliseconds>(steady_clock::now() - params.start_time);

            /* Display the results. */
            if (0 == world.rank()) {
                std::cout << result.size << " " << result.nodes;

                if (options_vars.count("enumerate")) {
                    std::cout << " " << result.result_count;
                    if (options_vars.count("check-club"))
                        std::cout << " " << result.result_club_count;
                }

                std::cout << std::endl;

                /* Members, and whether it's a club. */
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

                if (options_vars.count("verify")) {
                    if (! is_clique(graph, result.members)) {
                        std::cerr << "Oops! not a clique" << std::endl;
                        return EXIT_FAILURE;
                    }
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


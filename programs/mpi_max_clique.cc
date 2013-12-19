/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <solver/solver.hh>

#include <graph/graph.hh>
#include <graph/dimacs.hh>
#include <graph/pairs.hh>

#include <max_clique/mpi_max_clique.hh>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/environment.hpp>

#include <iostream>
#include <exception>
#include <cstdlib>
#include <chrono>
#include <thread>

using namespace parasols;
namespace po = boost::program_options;
namespace mpi = boost::mpi;

auto main(int argc, char * argv[]) -> int
{
    mpi::environment env;
    mpi::communicator world;

    try {
        if (0 == world.rank()) {
            po::options_description display_options{ "Program options" };
            display_options.add_options()
                ("help",                                 "Display help information")
                ("pairs",                                "Input is in pairs format, not DIMACS")
                ;

            po::options_description all_options{ "All options" };
            all_options.add_options()
                ("input-file", po::value<std::vector<std::string> >(),
                               "Specify the input file (DIMACS format, unless --pairs is specified). May be specified multiple times.")
                ;

            all_options.add(display_options);

            po::positional_options_description positional_options;
            positional_options
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
                std::cout << "Usage: " << argv[0] << " [options] file[...]" << std::endl;
                std::cout << std::endl;
                std::cout << display_options << std::endl;
                return EXIT_SUCCESS;
            }

            /* No algorithm or no input file specified? Show a message and exit. */
            if (options_vars.count("input-file") < 1) {
                std::cout << "Usage: " << argv[0] << " [options] file[...]" << std::endl;
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

                /* Read in the graph */
                auto graph = options_vars.count("pairs") ?  read_pairs(input_file) : read_dimacs(input_file);

                auto start_time = std::chrono::steady_clock::now();

                auto result = mpi_max_clique_master(world, graph);

                /* Stop the clock. */
                auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);

                /* Display the results. */
                std::cout << result.size << " " << result.nodes << std::endl;

                /* Members */
                for (auto v : result.members)
                    std::cout << graph.vertex_name(v) << " ";
                std::cout << std::endl;

                /* Times */
                std::cout << overall_time.count();
                std::cout << std::endl;
            }
        }
        else {
            mpi_max_clique_worker(world);
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


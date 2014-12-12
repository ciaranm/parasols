/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/graph.hh>
#include <graph/file_formats.hh>
#include <graph/power.hh>
#include <graph/complement.hh>
#include <graph/add_dominated_vertices.hh>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <exception>
#include <cstdlib>

using namespace parasols;
namespace po = boost::program_options;

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                 "Display help information")
            ("complement",                           "Take the complement of the graph")
            ("power",              po::value<int>(), "Raise the graph to this power")
            ("add-dominated",      po::value<int>(), "Add this many dominated vertices to the input graph")
            ("dominated-edges",    po::value<double>(), "When adding dominated vertices, keep edges with this probability")
            ("join-dominated",     po::value<double>(), "When adding dominated vertices, join dominated vertices with this probability")
            ("dominated-seed",     po::value<int>(), "Seed for adding dominated vertices")
            ("format",             po::value<std::string>(), "Specify the format of the input")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("input-file", "Specify the input file (DIMACS format, unless --format is specified).")
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

        /* No algorithm or no input file specified? Show a message and exit. */
        if (! options_vars.count("input-file")) {
            std::cout << "Usage: " << argv[0] << " [options] file" << std::endl;
            return EXIT_FAILURE;
        }

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
        auto graph = std::get<1>(*format)(options_vars["input-file"].as<std::string>(), GraphOptions::None);

        /* Options */
        if (options_vars.count("complement"))
            graph = complement(graph);

        if (options_vars.count("power"))
            graph = power(graph, options_vars["power"].as<int>());

        unsigned dominated_vertices = 0;
        double dominated_edge_p = 1.0;
        double dominated_join_p = 0.0;
        unsigned dominated_seed = 0;
        if (options_vars.count("add-dominated"))
            dominated_vertices = options_vars["add-dominated"].as<int>();
        if (options_vars.count("dominated-edges"))
            dominated_edge_p = options_vars["dominated-edges"].as<double>();
        if (options_vars.count("join-dominated"))
            dominated_join_p = options_vars["join-dominated"].as<double>();
        if (options_vars.count("dominated-seed"))
            dominated_seed = options_vars["dominated-seed"].as<int>();

        if (0 != dominated_vertices)
            graph = add_dominated_vertices(graph, dominated_vertices, dominated_edge_p, dominated_join_p, dominated_seed);

        std::cout << "p edge " << graph.size() << " 0" << std::endl;
        for (int e = 1 ; e <= graph.size() ; ++e)
            for (int f = e + 1 ; f <= graph.size() ; ++f)
                if (graph.adjacent(e - 1, f - 1))
                    std::cout << "e " << e << " " << f << std::endl;

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


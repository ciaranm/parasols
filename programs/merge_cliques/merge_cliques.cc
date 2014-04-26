/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include <graph/graph.hh>
#include <graph/file_formats.hh>
#include <graph/is_clique.hh>
#include <graph/merge_cliques.hh>

#include <iostream>
#include <exception>
#include <cstdlib>
#include <set>

using namespace parasols;
namespace po = boost::program_options;

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                         "Display help information")
            ("format",             po::value<std::string>(), "Specify the format of the input")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("graph",   "Main graph (DIMACS format, unless --format is specified).")
            ("clique1", "Vertices in the first clique (comma separated)")
            ("clique2", "Vertices in the second clique (comma separated)")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("graph", 1)
            .add("clique1", 1)
            .add("clique2", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] graph clique1 clique2" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No n specified? Show a message and exit. */
        if (! options_vars.count("graph") || ! options_vars.count("clique1") || ! options_vars.count("clique2")) {
            std::cout << "Usage: " << argv[0] << " [options] graph clique1 clique2" << std::endl;
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
        auto graph = std::get<1>(*format)(options_vars["graph"].as<std::string>());

        boost::char_separator<char> sep(",");

        boost::tokenizer<boost::char_separator<char> > tokenizer1{ options_vars["clique1"].as<std::string>(), sep };
        std::set<int> clique1;
        for (auto & t : tokenizer1)
            clique1.insert(graph.vertex_number(t));

        boost::tokenizer<boost::char_separator<char> > tokenizer2{ options_vars["clique2"].as<std::string>(), sep };
        std::set<int> clique2;
        for (auto & t : tokenizer2)
            clique2.insert(graph.vertex_number(t));

        if (! is_clique(graph, clique1)) {
            std::cerr << "First clique specified is not a clique" << std::endl;
            return EXIT_FAILURE;
        }

        if (! is_clique(graph, clique2)) {
            std::cerr << "Second clique specified is not a clique" << std::endl;
            return EXIT_FAILURE;
        }

        auto merged_clique = merge_cliques(graph, clique1, clique2);

        std::cout << clique1.size() << " " << clique2.size() << " " << merged_clique.size() << std::endl;
        for (auto & m : merged_clique)
            std::cout << graph.vertex_name(m) << " ";
        std::cout << std::endl;

        if (! is_clique(graph, merged_clique)) {
            std::cerr << "Oops! Merged clique is not a clique" << std::endl;
            return EXIT_FAILURE;
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


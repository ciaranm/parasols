/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <boost/program_options.hpp>

#include <iostream>
#include <exception>
#include <cstdlib>
#include <set>

namespace po = boost::program_options;

namespace
{
    auto expand(
            std::set<int> & ca,
            std::set<int> & cb,
            std::set<int> & pa,
            std::set<int> & pb,
            bool break_ab_symmetry) -> void
    {
        while (! pa.empty()) {
            int v = *pa.begin();
            ca.insert(v);
            pa.erase(v);

            std::set<int> new_pa = pa;
            std::set<int> new_pb = pb;
            new_pb.erase(v);

            if (ca.size() == cb.size()) {
                for (auto & a : ca)
                    std::cout << a << " ";
                std::cout << "/ ";
                for (auto & b : cb)
                    std::cout << b << " ";
                std::cout << std::endl;
            }

            if (! new_pb.empty()) {
                expand(cb, ca, new_pb, new_pa, break_ab_symmetry);
            }

            ca.erase(v);

            if (break_ab_symmetry) {
                if (cb.empty())
                    pb.erase(v);
            }
        }
    }
}

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                  "Display help information")
            ("break-ab-symmetry",  po::value<bool>(), "Break a/b symmetry (off by default)")
            ("verify",                                "Verify that we have found a valid result (for sanity checking changes)")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("n",                  po::value<int>(),  "The size of the problem")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("n", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] n" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No n specified? Show a message and exit. */
        if (! options_vars.count("n")) {
            std::cout << "Usage: " << argv[0] << " [options] n" << std::endl;
            return EXIT_FAILURE;
        }

        int n = options_vars["n"].as<int>();

        bool break_ab_symmetry = false;
        if (options_vars.count("break-ab-symmetry"))
            break_ab_symmetry = options_vars["break-ab-symmetry"].as<bool>();

        std::set<int> pa, pb, ca, cb;
        for (int i = 1 ; i <= n ; ++i) {
            pa.insert(i);
            pb.insert(i);
        }

        expand(ca, cb, pa, pb, break_ab_symmetry);

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



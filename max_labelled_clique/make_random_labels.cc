/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_labelled_clique/make_random_labels.hh>

#include <random>

using namespace parasols;

Labels parasols::make_random_labels(unsigned size, unsigned n, unsigned seed)
{
    std::mt19937 rand;
    rand.seed(seed);
    std::uniform_int_distribution<unsigned> dist(0, n - 1);

    Labels result;
    result.resize(size);
    for (auto & r : result)
        r.resize(size);

    for (unsigned i = 0 ; i < size ; ++i)
        for (unsigned j = 0 ; j < i ; ++j)
            result[i][j] = result[j][i] = dist(rand);

    return result;
}


/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_CCO_CCO_MIXIN_HH
#define PARASOLS_GUARD_CCO_CCO_MIXIN_HH 1

#include <cco/cco.hh>
#include <graph/bit_graph.hh>

#include <list>
#include <vector>

namespace parasols
{
    /**
     * Turn a CCOPermutations into a type.
     */
    template <CCOPermutations perm_>
    struct SelectColourClassOrderOverload
    {
    };

    template <unsigned size_, typename VertexType_, typename ActualType_, bool inverse_>
    struct CCOMixin
    {
        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::None> &,
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & p_bounds) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            VertexType_ colour = 0;        // current colour
            VertexType_ i = 0;             // position in p_bounds

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<size_> q = p_left;

                // while we can still give something this colour
                while (! q.empty()) {
                    // first thing we can colour
                    int v = q.first_set_bit();
                    p_left.unset(v);
                    q.unset(v);

                    // can't give anything adjacent to this the same colour
                    if (inverse_)
                        static_cast<ActualType_ *>(this)->graph.intersect_with_row(v, q);
                    else
                        static_cast<ActualType_ *>(this)->graph.intersect_with_row_complement(v, q);

                    // record in result
                    p_bounds[i] = colour;
                    p_order[i] = v;
                    ++i;
                }
            }
        }

        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::Defer1> &,
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & p_bounds) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            VertexType_ colour = 0;        // current colour
            VertexType_ i = 0;             // position in p_bounds

            VertexType_ d = 0;             // number deferred
            std::array<VertexType_, size_ * bits_per_word> defer;

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<size_> q = p_left;

                // while we can still give something this colour
                unsigned number_with_this_colour = 0;
                while (! q.empty()) {
                    // first thing we can colour
                    int v = q.first_set_bit();
                    p_left.unset(v);
                    q.unset(v);

                    // can't give anything adjacent to this the same colour
                    if (inverse_)
                        static_cast<ActualType_ *>(this)->graph.intersect_with_row(v, q);
                    else
                        static_cast<ActualType_ *>(this)->graph.intersect_with_row_complement(v, q);

                    // record in result
                    p_bounds[i] = colour;
                    p_order[i] = v;
                    ++i;
                    ++number_with_this_colour;
                }

                if (1 == number_with_this_colour) {
                    --i;
                    --colour;
                    defer[d++] = p_order[i];
                }
            }

            for (VertexType_ n = 0 ; n < d ; ++n) {
                ++colour;
                p_order[i] = defer[n];
                p_bounds[i] = colour;
                i++;
            }
        }

        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::Sort> &,
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & p_bounds) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            VertexType_ colour = 0;        // current colour
            VertexType_ i = 0;             // position in p_bounds

            // this is sloooooow. is it worth making it fast, or is d nearly as
            // good anyway?
            std::list<std::vector<VertexType_> > colour_classes;

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                colour_classes.emplace_back();

                // things that can still be given this colour
                FixedBitSet<size_> q = p_left;

                // while we can still give something this colour
                while (! q.empty()) {
                    // first thing we can colour
                    int v = q.first_set_bit();
                    p_left.unset(v);
                    q.unset(v);

                    // can't give anything adjacent to this the same colour
                    if (inverse_)
                        static_cast<ActualType_ *>(this)->graph.intersect_with_row(v, q);
                    else
                        static_cast<ActualType_ *>(this)->graph.intersect_with_row_complement(v, q);

                    // record in p_bounds
                    colour_classes.back().push_back(v);
                }
            }

            // this is stable
            colour_classes.sort([] (const std::vector<VertexType_> & a, const std::vector<VertexType_> & b) -> bool {
                    return a.size() > b.size();
                    });

            for (auto & colour_class : colour_classes) {
                ++colour;
                for (auto & v : colour_class) {
                    p_bounds[i] = colour;
                    p_order[i] = v;
                    ++i;
                }
            }
        }
    };

    template <template <CCOPermutations, unsigned, typename VertexType_> class WhichCCO_, CCOPermutations perm_>
    struct ApplyPerm
    {
        template <unsigned size_, typename VertexType_> using Type = WhichCCO_<perm_, size_, VertexType_>;
    };
}

#endif

/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_CCO_CCO_HH
#define PARASOLS_GUARD_CCO_CCO_HH 1

namespace parasols
{
    /**
     * Used by CCO variants to control permutations.
     */
    enum class CCOPermutations
    {
        None,
        Defer1,
        RepairAll,
        RepairAllDefer1,
        RepairSelected,
        RepairSelectedDefer1
    };
}

#endif

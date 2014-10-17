/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_THREADS_ATOMIC_INCUMBENT_HH
#define PARASOLS_GUARD_THREADS_ATOMIC_INCUMBENT_HH 1

#include <atomic>

namespace parasols
{
    struct AtomicIncumbent
    {
        std::atomic<unsigned> value;

        AtomicIncumbent()
        {
            value.store(0, std::memory_order_seq_cst);
        }

        bool update(unsigned v)
        {
            while (true) {
                unsigned cur_v = value.load(std::memory_order_seq_cst);
                if (v > cur_v) {
                    if (value.compare_exchange_strong(cur_v, v, std::memory_order_seq_cst))
                        return true;
                }
                else
                    return false;
            }
        }

        bool beaten_by(unsigned v) const
        {
            return v > value.load(std::memory_order_seq_cst);
        }

        unsigned get() const
        {
            return value.load(std::memory_order_relaxed);
        }
    };
}

#endif

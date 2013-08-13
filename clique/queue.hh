/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef CLIQUE_GUARD_CLIQUE_QUEUE_HH
#define CLIQUE_GUARD_CLIQUE_QUEUE_HH 1

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <list>
#include <utility>

namespace clique
{
    /**
     * A queue that supports an initial producer, followed by work donation.
     *
     * Initially, a producer thread populates the queue using enqueue_blocking.
     * Here the queue is bounded (since the initial producer may end up making
     * millions of items). Once the producer is done, initial_producer_done()
     * is called.
     *
     * There are a fixed number of consumers (ctor parameter). These consumers
     * call dequeue_blocking(), which returns false if the consumer should
     * exit.
     *
     * Because we are dealing with an irregular problem, it may be that the
     * initial producer is done, and the queue is empty, and we have idle
     * threads whilst other threads have a lot of work remaining. Thus, if the
     * initial producer is done and the queue is empty, want_donations() will
     * return true. Consumers may check this, and if it is true, they may
     * donate some of their subproblems to other threads using enqueue(). (It
     * is acceptable for multiple consumers to do this for a single 'queue
     * empty' event, but we don't want to do this too much.)
     *
     * The queue being empty therefore does not imply that consumers should
     * finish (i.e. dequeue_blocking() should return false), unless also the
     * initial producer is done, and no consumer may donate work.
     */
    template <typename Item_>
    class Queue
    {
        private:
            const bool _donations_possible;

            /* These protect and watch all subsequent private members. */
            std::mutex _mutex;
            std::condition_variable _cond;

            std::list<Item_> _list;
            bool _initial_producer_done;
            std::atomic<bool> _want_donations; /* Readable without holding _mutex */

            /* We keep track of how many consumers are busy, to decide whether
             * donations might be produced or whether consumers can exit. */
            unsigned _number_busy;

        public:
            /**
             * We need to know how many consumers we will have. We also allow
             * donations to be disabled. */
            Queue(unsigned number_of_dequeuers, bool donations_possible) :
                _donations_possible(donations_possible),
                _initial_producer_done(false),
                _number_busy(number_of_dequeuers)
            {
                /* Initially we don't want donations */
                _want_donations.store(false, std::memory_order_seq_cst);
            }

            ~Queue() = default;
            Queue(const Queue &) = delete;
            Queue & operator= (const Queue &) = delete;

            /**
             * Called by the initial producer when producing work.
             *
             * To avoid the queue from becoming huge, we block if there are
             * more than size elements pending.
             */
            void enqueue_blocking(Item_ && item, unsigned size)
            {
                std::unique_lock<std::mutex> guard(_mutex);

                /* Don't let the queue get too full. */
                while (_list.size() > size)
                    _cond.wait(guard);

                _list.push_back(std::move(item));

                /* We're not empty, so we don't want donations. */
                _want_donations.store(false, std::memory_order_seq_cst);

                /* Something may be waiting in dequeue_blocking(). */
                _cond.notify_all();
            }

            /**
             * Called by consumers when donating work.
             *
             * May be called even if donations are not being requested.
             */
            void enqueue(Item_ && item)
            {
                std::unique_lock<std::mutex> guard(_mutex);

                _list.push_back(std::move(item));

                /* We are no longer empty, so we don't want donations */
                _want_donations.store(false, std::memory_order_seq_cst);

                /* Something may be waiting in dequeue_blocking(). */
                _cond.notify_all();
            }

            /**
             * Called by consumers waiting for work.
             *
             * Blocks until an item is available, and places that item in the
             * parameter. Returns true if a work item was provided, and false
             * otherwise. A false return value means the consumer should exit.
             *
             * We shouldn't return false if the initial producer is still
             * running, or if any other consumer thread is still going (since
             * it may donate work). But we must return false if the initial
             * producer is done, and if all the consumer threads are waiting
             * for something to do.
             */
            bool dequeue_blocking(Item_ & item)
            {
                std::unique_lock<std::mutex> guard(_mutex);
                while (true) {
                    if (! _list.empty()) {
                        /* We have something to do. */
                        item = std::move(*_list.begin());
                        _list.pop_front();

                        if (_initial_producer_done && _list.empty()) {
                            /* We're now empty, and the initial producer isn't
                             * going to give us anything else, so request
                             * donations. */
                            _want_donations.store(true, std::memory_order_seq_cst);
                        }

                        /* Something might be waiting in enqueue_blocking for
                         * space to become available. */
                        _cond.notify_all();

                        return true;
                    }

                    /* We're either about to block or give up, so we are no
                     * longer busy. */
                    --_number_busy;

                    if (_initial_producer_done && ((! want_donations()) || (0 == _number_busy))) {
                        /* The list is empty, and nothing new can possibly be
                         * produced, so we're done. Other threads may be
                         * waiting for _number_busy to reach 0, so we need to
                         * wake them up. */
                        _cond.notify_all();

                        return false;
                    }

                    _cond.wait(guard);

                    /* We're potentially busy again (if this is a spurious
                     * wakeup, none of the conditions will be met back around
                     * the loop, and we'll decrement it again. */
                    ++_number_busy;
                }

                return false;
            }

            /**
             * Must be called when the initial producer is finished.
             */
            void initial_producer_done()
            {
                std::unique_lock<std::mutex> guard(_mutex);

                _initial_producer_done = true;

                /* The list might be empty, if consumers dequeued quickly. In
                 * that case, we now want donations. */
                if (_list.empty())
                    _want_donations.store(true, std::memory_order_seq_cst);

                /* Maybe every consumer is finished and waiting in
                 * dequeue_blocking(). */
                _cond.notify_all();
            }

            /**
             * If a consumer has an opportunity to donate, should it?
             */
            bool want_donations()
            {
                /* No locking. It's ok if we see an 'older' value of
                 * _want_donations. */
                return _donations_possible && _want_donations.load(std::memory_order_relaxed);
            }
    };
}

#endif

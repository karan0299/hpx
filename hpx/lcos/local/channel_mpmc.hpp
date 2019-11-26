//  Copyright (c) 2019 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This work is inspired by https://github.com/aprell/tasking-2.0

#if !defined(HPX_LCOS_LOCAL_CHANNEL_MPMC_NOV_24_2019_1141AM)
#define HPX_LCOS_LOCAL_CHANNEL_MPMC_NOV_24_2019_1141AM

#include <hpx/config.hpp>
#include <hpx/concurrency.hpp>
#include <hpx/errors.hpp>
#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/thread_support.hpp>

#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>

namespace hpx { namespace lcos { namespace local {

    ////////////////////////////////////////////////////////////////////////////
    // A simple but very high performance implementation of the channel concept.
    // This channel is bounded to a size given at construction time and supports
    // multiple producers and multiple consumers. The data is stored in a
    // ring-buffer.
    template <typename T, typename Mutex = lcos::local::spinlock>
    class base_channel_mpmc
    {
    private:
        using mutex_type = Mutex;

        bool is_full(std::size_t tail) const noexcept
        {
            std::size_t numitems = size_ + tail - head_.data_;
            if (numitems < size_)
            {
                return numitems == size_ - 1;
            }
            return numitems - size_ == size_ - 1;
        }

        bool is_empty(std::size_t head) const noexcept
        {
            return head == tail_.data_;
        }

        bool is_full_unbuffered() const noexcept
        {
            return head_.data_ != 0;
        }

        bool is_empty_unbuffered() const noexcept
        {
            return head_.data_ == 0;
        }

    public:
        explicit base_channel_mpmc(std::size_t size)
          : size_(size + 1)
          , buffer_(new T[size + 1])
          , closed_(false)
        {
            head_.data_ = 0;
            tail_.data_ = 0;
        }

        base_channel_mpmc(base_channel_mpmc&& rhs) noexcept
          : head_(rhs.head_)
          , tail_(rhs.tail_)
          , size_(rhs.size_)
          , buffer_(std::move(rhs.buffer_))
          , closed_(rhs.closed_)
        {
            rhs.size_ = 0;
            rhs.closed_ = true;
        }

        base_channel_mpmc& operator=(base_channel_mpmc&& rhs) noexcept
        {
            head_ = rhs.head_;
            tail_ = rhs.tail_;
            size_ = rhs.size_;
            buffer_ = std::move(rhs.buffer_);
            closed_ = rhs.closed_;
            rhs.closed_ = true;
            return *this;
        }

        ~base_channel_mpmc()
        {
            std::unique_lock<mutex_type> l(mtx_.data_);
            if (!closed_)
            {
                close(l);
            }
        }

        bool get(T* val = nullptr) const noexcept
        {
            std::unique_lock<mutex_type> l(mtx_.data_);
            if (closed_)
            {
                return false;
            }

            if (size_ <= 1)
            {
                // unbuffered operation
                if (is_empty_unbuffered())
                {
                    return false;
                }

                if (val == nullptr)
                {
                    return true;
                }

                *val = std::move(buffer_[0]);
                head_.data_ = 0;
            }
            else
            {
                // buffered operation
                std::size_t head = head_.data_;

                if (is_empty(head))
                {
                    return false;
                }

                if (val == nullptr)
                {
                    return true;
                }

                *val = std::move(buffer_[head]);
                if (++head >= size_)
                {
                    head = 0;
                }
                head_.data_ = head;
            }
            return true;
        }

        bool set(T&& t) noexcept
        {
            std::unique_lock<mutex_type> l(mtx_.data_);
            if (closed_)
            {
                return false;
            }

            if (size_ <= 1)
            {
                // unbuffered operation
                if (is_full_unbuffered())
                {
                    return false;
                }

                buffer_[0] = std::move(t);
                head_.data_ = 1;
            }
            else
            {
                // buffered operation
                std::size_t tail = tail_.data_;

                if (is_full(tail))
                {
                    return false;
                }

                buffer_[tail] = std::move(t);
                if (++tail >= size_)
                {
                    tail = 0;
                }
                tail_.data_ = tail;
            }
            return true;
        }

        std::size_t close()
        {
            std::unique_lock<mutex_type> l(mtx_.data_);
            return close(l);
        }

    protected:
        std::size_t close(std::unique_lock<mutex_type>& l)
        {
            HPX_ASSERT_OWNS_LOCK(l);

            if (closed_)
            {
                l.unlock();
                HPX_THROW_EXCEPTION(hpx::invalid_status,
                    "hpx::lcos::local::base_channel_mpmc::close",
                    "attempting to close an already closed channel");
            }

            closed_ = true;
            return 0;
        }

    private:
        // keep the mutex, the head, and the tail pointer in separate cache
        // lines
        mutable hpx::util::cache_aligned_data<mutex_type> mtx_;
        mutable hpx::util::cache_aligned_data<std::size_t> head_;
        hpx::util::cache_aligned_data<std::size_t> tail_;

        // a channel of size n can buffer n-1 items
        std::size_t size_;

        // channel buffer
        std::unique_ptr<T[]> buffer_;

        // this channel was closed, i.e. no further operations are possible
        bool closed_;
    };

    ////////////////////////////////////////////////////////////////////////////
    // For use with HPX threads, the channel_mpmc defined here is the fastest
    // (even faster than the channel_spsc). Using hpx::util::spinlock as the
    // means of synchronization enables the use of this channel with non-HPX
    // threads, however the performance degrades by a factor of ten compared to
    // using hpx::lcos::local::spinlock.
    template <typename T>
    using channel_mpmc = base_channel_mpmc<T, hpx::lcos::local::spinlock>;

}}}    // namespace hpx::lcos::local

#endif


//          Copyright Oliver Kowalke 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_JOBS_DETAIL_WORK_H
#define BOOST_JOBS_DETAIL_WORK_H

#include <cstddef>
#include <memory>
#include <tuple>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/detail/apply.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/job/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace jobs {
namespace detail {

class work {
private:
    std::size_t     use_count_;

public:
    typedef intrusive_ptr< work >    ptr_t;

    work() :
        use_count_( 0),
        nxt() {
    }

    virtual ~work() {}

    virtual void execute() {
        BOOST_ASSERT_MSG( false, "work::execute()");
    }

    virtual void deallocate() = 0;

    friend void intrusive_ptr_add_ref( work * j) {
        ++j->use_count_;
    }

    friend void intrusive_ptr_release( work * j) {
        BOOST_ASSERT( nullptr != j);

        if ( 0 == --j->use_count_) {
            j->deallocate();
        }
    }

    work::ptr_t   nxt;
};

template< typename Allocator, typename Fn >
class wrapped_work : public work {
public:
    typedef typename Allocator::template rebind< wrapped_work >::other   allocator_t;

    wrapped_work( allocator_t alloc, Fn && fn) :
        alloc_( alloc),
        fn_( std::forward< Fn >( fn) ) {
    }

    void execute() override final {
        fn_();
    }

    void deallocate() override final {
        deallocate_( this);
    }

private:
    allocator_t alloc_; 
    Fn          fn_;

    static void deallocate_( wrapped_work * w) {
        allocator_t alloc( w->alloc_);
        w->~wrapped_work();
        alloc.deallocate( w, 1);
    }
};

template< typename Allocator, typename Fn >
work * create_work_( Allocator a, Fn && fn) {
    typename wrapped_work< Allocator, Fn >::allocator_t alloc( a);
    work * pw = alloc.allocate( 1);
    return new ( pw) wrapped_work< Allocator, Fn >( alloc, std::forward< Fn >( fn) );
}

template< typename Allocator, typename Fn, typename ... Args >
work::ptr_t create_work( Allocator alloc, Fn && fn, Args && ... args) {
    return work::ptr_t(
        create_work_(
            alloc,
            [fn=std::forward< Fn >( fn),tpl=std::make_tuple( std::forward< Args >( args) ...)] () mutable -> decltype( auto) {
                context::detail::apply( std::move( fn), std::move( tpl) );
            }) );
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_JOBS_DETAIL_WORK_H

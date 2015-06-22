
//          Copyright Oliver Kowalke 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_JOBS_DETAIL_JOB_H
#define BOOST_JOBS_DETAIL_JOB_H

#include <cstddef>
#include <tuple>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/job/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace jobs {
namespace detail {

class job {
private:
    std::size_t     use_count_;

public:
    typedef intrusive_ptr< job >    ptr_t;

    job() :
        use_count_( 0) {
    }

    virtual ~job() {}

    virtual void execute() = 0;

    friend void intrusive_ptr_add_ref( job * b) {
        ++b->use_count_;
    }

    friend void intrusive_ptr_release( job * b) {
        BOOST_ASSERT( nullptr != b);

        if ( 0 == --b->use_count_) {
            delete b;
        }
    }
};

template< typename Fn >
class wrapped_job : public job {
private:
    Fn      fn_;

public:
    wrapped_job( Fn && fn) :
        fn_( std::forward< Fn >( fn) ) {
    }

    void execute() {
        fn_();
    }
};

template< typename Fn >
static job * create_wrapped_job_( Fn && fn) {
    return new wrapped_job< Fn >( std::forward< Fn >( fn) );
}

template< typename Fn, typename Tpl, std::size_t ... I >
static job * create_job_( Fn && fn_, Tpl && tpl_, std::index_sequence< I ... >) {
    return create_wrapped_job_(
            [fn=std::forward< Fn >( fn_),tpl=std::forward< Tpl >( tpl_)] () mutable {
                fn(
                    // non-type template parameter pack used to extract the
                    // parameters (arguments) from the tuple and pass them to fn
                    // via parameter pack expansion
                    // std::tuple_element<> does not perfect forwarding
                    std::forward< decltype( std::get< I >( std::declval< Tpl >() ) ) >(
                        std::get< I >( std::forward< Tpl >( tpl) ) ) ... );
            });
}

template< typename Fn, typename ... Args >
job::ptr_t create_job( Fn && fn, Args && ... args) {
    return job::ptr_t(
                create_job_(
                    std::forward< Fn >( fn),
                    std::make_tuple( std::forward< Args >( args) ... ),
                    std::index_sequence_for< Args ... >() ) );
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_JOBS_DETAIL_JOB_H


//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <boost/array.hpp>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>
#include <boost/variant.hpp>

#include <boost/context/execution_context.hpp>
#include <boost/context/detail/config.hpp>

#ifdef BOOST_WINDOWS
#include <windows.h>
#endif

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4723)
#endif

typedef boost::variant<int,std::string> variant_t;

namespace ctx = boost::context;

int value1 = 0;
std::string value2;
double value3 = 0.;

struct X {
    ctx::execution_context< void > foo( int i, ctx::execution_context< void > && ctx) {
        value1 = i;
        return std::move( ctx);
    }
};

struct Y {
    Y() {
        value1 = 3;
    }

    Y( Y const&) = delete;
    Y & operator=( Y const&) = delete;

    ~Y() {
        value1 = 7;
    }
};

class moveable {
public:
    bool    state;
    int     value;

    moveable() :
        state( false),
        value( -1) {
    }

    moveable( int v) :
        state( true),
        value( v) {
    }

    moveable( moveable && other) :
        state( other.state),
        value( other.value) {
        other.state = false;
        other.value = -1;
    }

    moveable & operator=( moveable && other) {
        if ( this == & other) return * this;
        state = other.state;
        value = other.value;
        other.state = false;
        other.value = -1;
        return * this;
    }

    moveable( moveable const& other) = delete;
    moveable & operator=( moveable const& other) = delete;

    void operator()() {
        value1 = value;
    }
};

struct my_exception : public std::runtime_error {
    my_exception( char const* what) :
        std::runtime_error( what) {
    }
};

#ifdef BOOST_MSVC
// Optimizations can remove the integer-divide-by-zero here.
#pragma optimize("", off)
void seh( bool & catched) {
    __try {
        int i = 1;
        i /= 0;
    } __except( EXCEPTION_EXECUTE_HANDLER) {
        catched = true;
    }
}
#pragma optimize("", on)
#endif

ctx::execution_context< void > fn1( int i, ctx::execution_context< void > && ctx) {
    value1 = i;
    return std::move( ctx);
}

ctx::execution_context< void > fn2( const char * what, ctx::execution_context< void > && ctx) {
    try {
        throw std::runtime_error( what);
    } catch ( std::runtime_error const& e) {
        value2 = e.what();
    }
    return std::move( ctx);
}

ctx::execution_context< void > fn3( double d, ctx::execution_context< void > && ctx) {
    d += 3.45;
    value3 = d;
    return std::move( ctx);
}

ctx::execution_context< void > fn5( ctx::execution_context< void > && ctx) {
    value1 = 3;
    return std::move( ctx);
}

ctx::execution_context< void > fn4( ctx::execution_context< void > && ctx) {
    ctx::execution_context< void > ctx1( fn5);
    ctx1();
    value3 = 3.14;
    return std::move( ctx);
}

ctx::execution_context< void > fn6( ctx::execution_context< void > && ctx) {
    try {
        value1 = 3;
        ctx = ctx();
        value1 = 7;
        ctx = ctx();
    } catch ( my_exception & e) {
        value2 = e.what();
    }
    return std::move( ctx);
}

ctx::execution_context< void > fn7( ctx::execution_context< void > && ctx) {
    Y y;
    return ctx();
}

ctx::execution_context< int > fn8( ctx::execution_context< int > && ctx, int i) {
    value1 = i;
    return std::move( ctx);
}

ctx::execution_context< int > fn9( ctx::execution_context< int > && ctx, int i) {
    value1 = i;
    std::tie( ctx, i) = ctx( i);
    value1 = i;
    return std::move( ctx);
}

ctx::execution_context< int & > fn10( ctx::execution_context< int & > && ctx, int & i) {
    std::tie( ctx, i) = ctx( i);
    return std::move( ctx);
}

ctx::execution_context< moveable > fn11( ctx::execution_context< moveable > && ctx, moveable m) {
    std::tie( ctx, m) = ctx( std::move( m) );
    return std::move( ctx);
}

ctx::execution_context< int, std::string > fn12( ctx::execution_context< int, std::string > && ctx, int i, std::string str) {
    std::tie( ctx, i, str) = ctx( i, str);
    return std::move( ctx);
}

ctx::execution_context< int, moveable > fn13( ctx::execution_context< int, moveable > && ctx, int i, moveable m) {
    std::tie( ctx, i, m) = ctx( i, std::move( m) );
    return std::move( ctx);
}

ctx::execution_context< variant_t > fn14( ctx::execution_context< variant_t > && ctx, variant_t data) {
    int i = boost::get< int >( data);
    data = boost::lexical_cast< std::string >( i);
    std::tie( ctx, data) = ctx( data);
    return std::move( ctx);
}

ctx::execution_context< Y * > fn15( ctx::execution_context< Y * > && ctx, Y * py) {
    ctx( py);
    return std::move( ctx);
}

ctx::execution_context< int > fn16( ctx::execution_context< int > && ctx, int i) {
    value1 = i;
    std::tie( ctx, i) = ctx( i);
    value1 = i;
    return std::move( ctx);
}

ctx::execution_context< int, int > fn17( ctx::execution_context< int, int > && ctx, int i, int j) {
    for (;;) {
        std::tie( ctx, i, j) = ctx( i, j);
    }
    return std::move( ctx);
}


void test_move() {
    value1 = 0;
    ctx::execution_context< void > ctx;
    BOOST_CHECK( ! ctx);
    ctx::execution_context< void > ctx1( fn1, 1);
    ctx::execution_context< void > ctx2( fn1, 3);
    BOOST_CHECK( ctx1);
    BOOST_CHECK( ctx2);
    ctx1 = std::move( ctx2);
    BOOST_CHECK( ctx1);
    BOOST_CHECK( ! ctx2);
    BOOST_CHECK_EQUAL( 0, value1);
    ctx1();
    BOOST_CHECK_EQUAL( 3, value1);
    BOOST_CHECK( ! ctx1);
    BOOST_CHECK( ! ctx2);
}

void test_memfn() {
    value1 = 0;
    X x;
    ctx::execution_context< void > ctx( & X::foo, x, 7);
    ctx();
    BOOST_CHECK_EQUAL( 7, value1);
}

void test_exception() {
    {
        const char * what = "hello world";
        ctx::execution_context< void > ctx( fn2, what);
        BOOST_CHECK( ctx);
        ctx();
        BOOST_CHECK_EQUAL( std::string( what), value2);
        BOOST_CHECK( ! ctx);
    }
#ifdef BOOST_MSVC
    {
        bool catched = false;
        std::thread([&catched](){
                ctx::execution_context< void > ctx([&catched](ctx::execution_context< void > && ctx){
                        seh( catched);
                        return std::move( ctx);
                        });
            BOOST_CHECK( ctx);
            ctx();
        }).join();
        BOOST_CHECK( catched);
    }
#endif
}

void test_fp() {
    double d = 7.13;
    ctx::execution_context< void > ctx( fn3, d);
    BOOST_CHECK( ctx);
    ctx();
    BOOST_CHECK_EQUAL( 10.58, value3);
    BOOST_CHECK( ! ctx);
}

void test_stacked() {
    value1 = 0;
    value3 = 0.;
    ctx::execution_context< void > ctx( fn4);
    BOOST_CHECK( ctx);
    ctx();
    BOOST_CHECK_EQUAL( 3, value1);
    BOOST_CHECK_EQUAL( 3.14, value3);
    BOOST_CHECK( ! ctx);
}

void test_prealloc() {
    value1 = 0;
    ctx::default_stack alloc;
    ctx::stack_context sctx( alloc.allocate() );
    void * sp = static_cast< char * >( sctx.sp) - 10;
    std::size_t size = sctx.size - 10;
    ctx::execution_context< void > ctx( std::allocator_arg, ctx::preallocated( sp, size, sctx), alloc, fn1, 7);
    BOOST_CHECK( ctx);
    ctx();
    BOOST_CHECK_EQUAL( 7, value1);
    BOOST_CHECK( ! ctx);
}

void test_ontop() {
    {
        int i = 3, j = 0;
        ctx::execution_context< int > ctx([]( ctx::execution_context< int > && ctx, int x) {
                    for (;;) {
                        std::tie( ctx, x) = ctx( x*10);
                    }
                    return std::move( ctx);
                });
        std::tie( ctx, j) = ctx( ctx::exec_ontop_arg,
                []( int x){
                    return x-10;
                },
                i);
        BOOST_CHECK( ctx);
        BOOST_CHECK_EQUAL( j, -70);
    }
    {
        int i = 3, j = 1;
        ctx::execution_context< int, int > ctx( fn17);
        std::tie( ctx, i, j) = ctx( i, j);
        std::tie( ctx, i, j) = ctx( ctx::exec_ontop_arg,
                []( int x, int y) {
                    return std::make_tuple( x - y, x + y);
                },
                i, j);
        BOOST_CHECK_EQUAL( i, 2);
        BOOST_CHECK_EQUAL( j, 4);
    }
    {
        moveable m1( 7), m2;
        BOOST_CHECK( 7 == m1.value);
        BOOST_CHECK( m1.state);
        BOOST_CHECK( -1 == m2.value);
        BOOST_CHECK( ! m2.state);
        ctx::execution_context< moveable > ctx( fn11);
        std::tie( ctx, m2) = ctx( ctx::exec_ontop_arg,
                []( moveable m){
                    return std::move( m);
                },
                std::move( m1) );
        BOOST_CHECK( -1 == m1.value);
        BOOST_CHECK( ! m1.state);
        BOOST_CHECK( 7 == m2.value);
        BOOST_CHECK( m2.state);
    }
}

void test_ontop_exception() {
    {
        value1 = 0;
        value2 = "";
        ctx::execution_context< void > ctx([](ctx::execution_context< void > && ctx){
                for (;;) {
                    value1 = 3;
                    try {
                        ctx = ctx();
                    } catch ( ctx::ontop_error const& e) {
                        try {
                            std::rethrow_if_nested( e);
                        } catch ( my_exception const& ex) {
                            value2 = ex.what();
                        }
                        return e.get_context< void >();
                    }
                }
                return std::move( ctx);
        });
        ctx = ctx();
        BOOST_CHECK_EQUAL( 3, value1);
        const char * what = "hello world";
        ctx( ctx::exec_ontop_arg,
             [what](){
                throw my_exception( what);
             });
        BOOST_CHECK_EQUAL( 3, value1);
        BOOST_CHECK_EQUAL( std::string( what), value2);
    }
    {
        value2 = "";
        int i = 3, j = 1;
        ctx::execution_context< int, int > ctx([]( ctx::execution_context< int, int > && ctx, int x, int y) {
                for (;;) {
                    try {
                            std::tie( ctx, x, y) = ctx( x+y,x-y);
                    } catch ( ctx::ontop_error const& e) {
                        try {
                            std::rethrow_if_nested( e);
                        } catch ( my_exception const& ex) {
                            value2 = ex.what();
                        }
                        return e.get_context< int, int >();
                    }
                }
                return std::move( ctx);
        });
        std::tie( ctx, i, j) = ctx( i, j);
        BOOST_CHECK( ctx);
        BOOST_CHECK_EQUAL( i, 4);
        BOOST_CHECK_EQUAL( j, 2);
        const char * what = "hello world";
        std::tie( ctx, i, j) = ctx( ctx::exec_ontop_arg,
                [what](int x, int y) {
                    throw my_exception(what);
                    return std::make_tuple( x*y, x/y);
                },
                i, j);
        BOOST_CHECK_EQUAL( i, 4);
        BOOST_CHECK_EQUAL( j, 2);
        BOOST_CHECK_EQUAL( std::string( what), value2);
    }
}

void test_termination() {
    {
        value1 = 0;
        ctx::execution_context< void > ctx( fn7);
        BOOST_CHECK_EQUAL( 0, value1);
        ctx = ctx();
        BOOST_CHECK_EQUAL( 3, value1);
    }
    BOOST_CHECK_EQUAL( 7, value1);
    {
        value1 = 0;
        BOOST_CHECK_EQUAL( 0, value1);
        ctx::execution_context< void > ctx( fn5);
        BOOST_CHECK( ctx);
        ctx();
        BOOST_CHECK_EQUAL( 3, value1);
        BOOST_CHECK( ! ctx);
    }
    {
        value1 = 0;
        BOOST_CHECK_EQUAL( 0, value1);
        int i = 3, j = 0;
        ctx::execution_context< int > ctx( fn9);
        BOOST_CHECK( ctx);
        std::tie( ctx, j) = ctx( i);
        BOOST_CHECK_EQUAL( i, value1);
        BOOST_CHECK( ctx);
        BOOST_CHECK_EQUAL( i, j);
        i = 7;
        std::tie( ctx, j) = ctx( i);
        BOOST_CHECK_EQUAL( i, value1);
        BOOST_CHECK( ! ctx);
        BOOST_CHECK_EQUAL( i, j);
    }
}

void test_one_arg() {
    {
        value1 = 0;
        ctx::execution_context< int > ctx( fn8);
        ctx( 7);
        BOOST_CHECK_EQUAL( 7, value1);
    }
    {
        int i = 3, j = 0;
        ctx::execution_context< int > ctx( fn9);
        std::tie( ctx, j) = ctx( i);
        BOOST_CHECK_EQUAL( i, j);
    }
    {
        int i = 3, j = 0;
        int & k = j;
        BOOST_CHECK( & i != & k);
        BOOST_CHECK( & j == & k);
        ctx::execution_context< int & > ctx( fn10);
        std::tie( ctx, k) = ctx( i);
        BOOST_CHECK( & i != & k);
    }
    {
        Y y;
        Y * py = nullptr;
        ctx::execution_context< Y * > ctx( fn15);
        std::tie( ctx, py) = ctx( & y);
        BOOST_CHECK( py == & y);
    }
    {
        moveable m1( 7), m2;
        BOOST_CHECK( 7 == m1.value);
        BOOST_CHECK( m1.state);
        BOOST_CHECK( -1 == m2.value);
        BOOST_CHECK( ! m2.state);
        ctx::execution_context< moveable > ctx( fn11);
        std::tie( ctx, m2) = ctx( std::move( m1) );
        BOOST_CHECK( -1 == m1.value);
        BOOST_CHECK( ! m1.state);
        BOOST_CHECK( 7 == m2.value);
        BOOST_CHECK( m2.state);
    }
}

void test_two_args() {
    {
        int i1 = 3, i2 = 0;
        std::string str1("abc"), str2;
        ctx::execution_context< int, std::string > ctx( fn12);
        std::tie( ctx, i2, str2) = ctx( i1, str1);
        BOOST_CHECK_EQUAL( i1, i2);
        BOOST_CHECK_EQUAL( str1, str2);
    }
    {
        int i1 = 3, i2 = 0;
        moveable m1( 7), m2;
        BOOST_CHECK( 7 == m1.value);
        BOOST_CHECK( m1.state);
        BOOST_CHECK( -1 == m2.value);
        BOOST_CHECK( ! m2.state);
        ctx::execution_context< int, moveable > ctx( fn13);
        std::tie( ctx, i2, m2) = ctx( i1, std::move( m1) );
        BOOST_CHECK_EQUAL( i1, i2);
        BOOST_CHECK( -1 == m1.value);
        BOOST_CHECK( ! m1.state);
        BOOST_CHECK( 7 == m2.value);
        BOOST_CHECK( m2.state);
    }
}

void test_variant() {
    {
        int i = 7;
        variant_t data1 = i, data2;
        ctx::execution_context< variant_t > ctx( fn14);
        std::tie( ctx, data2) = ctx( data1);
        std::string str = boost::get< std::string >( data2);
        BOOST_CHECK_EQUAL( std::string("7"), str);
    }
}

#ifdef BOOST_WINDOWS
void test_bug12215() {
        ctx::execution_context< void > ctx(
            [](ctx::execution_context< void > && ctx) {
                char buffer[MAX_PATH];
                GetModuleFileName( nullptr, buffer, MAX_PATH);
                return std::move( ctx);
            });
        ctx();

}
#endif

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Context: execution_context v2 test suite");

    test->add( BOOST_TEST_CASE( & test_move) );
    test->add( BOOST_TEST_CASE( & test_memfn) );
    test->add( BOOST_TEST_CASE( & test_exception) );
    test->add( BOOST_TEST_CASE( & test_fp) );
    test->add( BOOST_TEST_CASE( & test_stacked) );
    test->add( BOOST_TEST_CASE( & test_prealloc) );
    test->add( BOOST_TEST_CASE( & test_ontop) );
    test->add( BOOST_TEST_CASE( & test_ontop_exception) );
    test->add( BOOST_TEST_CASE( & test_termination) );
    test->add( BOOST_TEST_CASE( & test_one_arg) );
    test->add( BOOST_TEST_CASE( & test_two_args) );
    test->add( BOOST_TEST_CASE( & test_variant) );
#ifdef BOOST_WINDOWS
    test->add( BOOST_TEST_CASE( & test_bug12215) );
#endif

    return test;
}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

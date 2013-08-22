#define BOOST_TEST_DYN_LINK

#include "escalator.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

using namespace boost::unit_test;

using namespace navetas::escalator;


template<typename T1, typename T2>
void CHECK_SAME_ELEMENTS( const T1& t1, const T2& t2 )
{
    BOOST_CHECK_EQUAL( t1.size(), t2.size() );
    auto s1 = lift(t1);
    auto s2 = lift(t2);
    
    auto z = s1.zip(s2);
    
    z.foreach( []( std::pair<typename T1::value_type, typename T2::value_type> v )
    {
        BOOST_CHECK_EQUAL( v.first, v.second );
    } );
}

// A place for tests for the underlying structure of escalator - including the intrinsic move
// semantics and use of reference_wrapper after lift or retain

void testStructuralRequirements()
{
    // Simple tests
    {
        std::vector<int> a = { 5, 4, 3, 2, 1 };
        
        std::vector<std::reference_wrapper<const int>> res1 = lift(a).lower<std::vector>();
        std::vector<int> res2 = lift(a).lower_values<std::vector>();
        std::vector<std::reference_wrapper<const int>> res3 = lift(a).retain<std::vector>();
        std::vector<int> res4 = lift(a).retain_values<std::vector>();
        
        std::vector<int> res5 = lift(a).map( []( const int& v ) -> int { return v; } ).retain_values<std::vector>();
        
        std::vector<int> res6 = lift(a)
            .filter( []( const int& v ) { return v > 2; } )
            .copyElements()
            .sortWith( []( const int& l, const int& r ) { return l < r; } )
            .retain_values<std::vector>();
            
        {
            std::vector<std::reference_wrapper<const int>> foo = lift(a).lower<std::vector>();
            std::sort( foo.begin(), foo.end() );
            
            ContainerWrapper<std::vector<std::reference_wrapper<const int>>, std::reference_wrapper<const int>> vw( std::move(foo) );
            
            vw.getIterator().hasNext();
            
            // TODO: the types are inconsistent in ContainerWrapper::Iterator - need to sort out the
            // WrappedContainerVRef - it should probably be replaced by ContainerType::iterator::deref_type
            //vw.getIterator().next();
        }
            
    }
    
    
    typedef std::unique_ptr<int> upInt_t;
    
    std::vector<upInt_t> a;
    a.emplace_back( new int(3) );
    a.emplace_back( new int(1) );
    a.emplace_back( new int(4) );
    a.emplace_back( new int(4) );
    a.emplace_back( new int(2) );
    
    std::vector<upInt_t> res1 = mlift(a)
        .map( []( upInt_t& v ) { return std::move(v); } )
        .lower<std::vector>();
        
    mlift(res1).foreach( []( upInt_t& v )
    {
        bool valid = static_cast<bool>(v);
        BOOST_REQUIRE( valid );
    } );
        
    // Retain into a vector of unique pointers
    auto res2 = mlift(res1)
        .checkIteratorElementType<std::reference_wrapper<upInt_t>>()
        .map( []( upInt_t& v ) { return std::move(v); } )
        .retain<std::vector>();
        
    // Try two iterations, the second must not have been invalidated
    // by moves from the first (because the unique_ptr is wrapped in
    // a reference_wrapper)
    res2
        .foreach( []( upInt_t& v )
        {
            bool valid = static_cast<bool>(v);
            BOOST_REQUIRE( valid );
        } );
        
    res2
        .foreach( []( upInt_t& v )
        {
            bool valid = static_cast<bool>(v);
            BOOST_REQUIRE( valid );
        } );
    
    // Currently zipping two lifted things gives a pair of ref-wrappers which is
    // perhaps a bit opaque to the user
    {
        std::vector<int> b = { 3, 1, 4, 4, 2 };
        
        std::vector<int> res3 = lift(b).zip(lift(b))
            .map( []( std::pair<std::reference_wrapper<const int>, std::reference_wrapper<const int>> p )
            {
                return p.first * p.second;
            } )
            .checkIteratorElementType<int>()
            // Check that multiple consecutive retains are stably typed
            .retain<std::vector>()
            .checkRawElementType<int>()
            .retain<std::vector>()
            .lower<std::vector>();

        CHECK_SAME_ELEMENTS( res3, std::vector<int> { 9, 1, 16, 16, 4 } );
    }
    
    {
        std::vector<std::vector<int>> p = { { 1, 2, 3 }, { 4, 5 }, {}, { 6 } };
        
        auto cpP = lift(p)
            .map( []( const std::vector<int>& row )
            {
                return lift(row)
                    .map( []( int v ) { return v + 1; } )
                    .retain_values<std::vector>();
            } )
            .checkIteratorElementType<ContainerWrapper<std::vector<int>, int>>()
            .retain_values<std::vector>();
            
        // Run over the container twice
        CHECK_SAME_ELEMENTS( cpP
            .copyElements()
            .flatten()
            .lower_values<std::vector>(), std::vector<int> { 2, 3, 4, 5, 6, 7 } );
            
        CHECK_SAME_ELEMENTS( cpP
            .copyElements()
            .flatten()
            .lower_values<std::vector>(), std::vector<int> { 2, 3, 4, 5, 6, 7 } );
    }
    
    {
        std::vector<int> b = { 1, 2, 3, 4, 5, 4, 3, 2, 1, 6, 7, 8, 9 };
        auto res = lift(b)
            .countBy( []( int v ) { return v % 2; } )
            // This map is nasty - but without removing the const from the first half of the pair,
            // we are unable to retain this pair by value into a container as the STL containers
            // will not take immutable elements.
            
            // TODO: Get the collection wrapper iterator, in this instance (and sortBy etc), to strip the const whilst retaining the const in the inner map?
            
            .map( []( const std::pair<const int, size_t> p ) { return std::pair<int, size_t>( p.first, p.second ); } )
            .sortBy( []( const std::pair<int, size_t>& p ) { return p.second; } );
    }
    
    {
        std::vector<upInt_t> a2;
        a2.emplace_back( new int(3) );
        a2.emplace_back( new int(1) );
        a2.emplace_back( new int(4) );
        a2.emplace_back( new int(4) );
        a2.emplace_back( new int(2) );
        
        auto res = lift(a2)
            .filter( []( const upInt_t& v ) { return *v > 2; } )
            .sortWith( []( const upInt_t& lhs, const upInt_t& rhs ) { return *lhs > *rhs; } )
            .map( []( const upInt_t& v ) -> int { return *v; } )
            .checkIteratorElementType<int>()
            .lower_values<std::vector>();
            
        CHECK_SAME_ELEMENTS( res, std::vector<int> { 4, 4, 3 } );
    }
}

void test1()
{
    // vec, set, list etc as things convertible to the std versions but that are still lifted.
    // scan.
    // concat (a ++ b)
    // :+, +:
    
    std::vector<int> a = { 3, 1, 4, 4, 2 };
    
    std::vector<int> res1 = lift(a).retain_values<std::vector>();
    std::set<int> res2 = lift(a).retain_values<std::set>();
    std::list<int> res3 = lift(a).retain_values<std::list>();
    std::multiset<int> res2a = lift(a).retain_values<std::multiset>();
    std::deque<int> res3a = lift(a).retain_values<std::deque>();
    
    BOOST_CHECK_EQUAL( res1.size(), 5 );
    BOOST_CHECK_EQUAL( res2.size(), 4 );
    BOOST_CHECK_EQUAL( res3.size(), 5 );
    BOOST_CHECK_EQUAL( res2a.size(), 5 );
    BOOST_CHECK_EQUAL( res3a.size(), 5 );
    
    std::set<int> q = { 1, 2, 3, 4 };
    auto z = lift(res2).zip(lift(q)).lower<std::vector>();
    BOOST_CHECK_EQUAL( z.size(), 4 );
    
    {
        auto tmp = lift(res2).zip( lift(q) ).lower<std::vector>();
        BOOST_CHECK_EQUAL( tmp.size(), 4 );
    }
    
    CHECK_SAME_ELEMENTS( res2, std::set<int> { 1, 2, 3, 4 } );
    
    std::vector<std::pair<int, int>> foo = lift(a).copyElements().zip( lift(a).copyElements() ).retain<std::vector>();
    
    auto foo2 = lift(a)
        .map( []( const int& v ) -> const int& { return v; } )
        .castElements<int>()
        .retain<std::vector>();

    BOOST_CHECK_EQUAL( foo.size(), 5 );    
    CHECK_SAME_ELEMENTS( lift(foo).map( []( std::pair<int, int> a ) { return a.first; } ).lower<std::vector>(), a );
    CHECK_SAME_ELEMENTS( lift(foo).map( []( std::pair<int, int> a ) { return a.second; } ).lower<std::vector>(), a );
    
    std::vector<std::string> res4 = lift(a)
        .map( [](int x) { return static_cast<double>( x * x ); } )
        .map( [](double x) { return boost::lexical_cast<std::string>(x); } ).retain<std::vector>();    
    BOOST_CHECK_EQUAL( res4.size(), 5 );
    CHECK_SAME_ELEMENTS( res4, std::vector<std::string> { "9", "1", "16", "16", "4" } );
        
    std::vector<int> res5 = lift(a).filter( []( int a ) { return a < 4; } ).retain_values<std::vector>();
    BOOST_CHECK_EQUAL( res5.size(), 3 );
    CHECK_SAME_ELEMENTS( res5, std::vector<int> { 3, 1, 2 } );
    
    std::vector<int> res6 = lift(a)
        .zipWithIndex()
        .filter( []( const std::pair<int, size_t>& v ) { return v.second > 0; } )
        .map( []( const std::pair<int, size_t>& v ) { return v.first; } )
        .retain<std::vector>();
        
    BOOST_CHECK_EQUAL( res6.size(), 4 );
    CHECK_SAME_ELEMENTS( res6, std::vector<int> { 1, 4, 4, 2 } );
    
    
    std::vector<int> res6a;
    lift(a)
        .zipWithIndex()
        .filter( []( const std::pair<int, size_t>& v ) { return v.second > 0; } )
        .foreach( [&res6a]( const std::pair<int, size_t>& v ) { res6a.push_back( v.first ); } );
        
    BOOST_CHECK_EQUAL( res6a.size(), 4 );
    CHECK_SAME_ELEMENTS( res6a, std::vector<int> { 1, 4, 4, 2 } );


    {
        std::vector<int> foo = {};
        auto res = lift(foo)
            .sliding2()
            .lower<std::vector>();
            
        BOOST_REQUIRE_EQUAL( res.size(), 0 );
    }
    
    {
        std::vector<int> foo = { 1 };
        auto res = lift(foo)
            .sliding2()
            .lower<std::vector>();
            
        BOOST_REQUIRE_EQUAL( res.size(), 0 );
    }
    
    std::vector<int> res7 = lift(a)
        .sliding2()
        .map( []( const std::pair<int, int>& v ) { return v.second - v.first; } )
        .retain<std::vector>();

    BOOST_CHECK_EQUAL( res7.size(), 4 );
    CHECK_SAME_ELEMENTS( res7, std::vector<int> { -2, 3, 0, -2 } );
        
        
    std::vector<int> res8 = lift(a)
        .map( []( int v ) { return v * v; } )
        .sortWith( []( int a, int b ) { return a < b; } );
    
    BOOST_CHECK_EQUAL( res8.size(), 5 );
    CHECK_SAME_ELEMENTS( res8, std::vector<int> { 1, 4, 9, 16, 16 } );
    
    std::vector<int> res8Default = lift(a)
        .map( []( int v ) { return v * v; } )
        .sort();
    
    BOOST_CHECK_EQUAL( res8Default.size(), 5 );
    CHECK_SAME_ELEMENTS( res8Default, std::vector<int> { 1, 4, 9, 16, 16 } );
    
    int sum = lift(a)
        .fold( 10, []( int acc, int v ) { return acc + v; } );
        
    BOOST_CHECK_EQUAL( sum, 24 );
    
    // Should preserve order
    
    // FIXME: Currently fails compilation because retain (via the ContainerWrapper) re-wraps the copied elements with a reference_wrapper
    //std::vector<int> res9 = lift(a).distinct().copyElements().retain<std::vector>();
    std::vector<int> res9 = lift(a).copyElements().distinct().retain_values<std::vector>();
    CHECK_SAME_ELEMENTS( res9, std::vector<int> { 3, 1, 4, 2 } );
    
    auto res9a = lift(a).copyElements().distinct().lower_values<std::vector>();
    CHECK_SAME_ELEMENTS( res9, res9a );
    
    {
        std::vector<std::shared_ptr<int>> foo = {
            std::make_shared<int>( 1 ),
            std::make_shared<int>( 1 ),
            std::make_shared<int>( 3 ),
            std::make_shared<int>( 4 ),
            std::make_shared<int>( 1 ) };
    
        std::vector<int> unique = lift(foo)
            .copyElements()
            .distinctWith( []( const std::shared_ptr<int>& a, const std::shared_ptr<int>& b )
            {
                BOOST_REQUIRE( a );
                BOOST_REQUIRE( b );
                return *a < *b;
            } )
            .map( []( const std::shared_ptr<int>& a ) -> int { return *a; } )
            .retain<std::vector>();
            
        CHECK_SAME_ELEMENTS( unique, std::vector<int> { 1, 3, 4 } );
    }
    
    // Shouldn't preserve order
    std::set<int> res10 = lift(a).retain_values<std::set>();
    CHECK_SAME_ELEMENTS( res10, std::vector<int> { 1, 2, 3, 4 } );
    
    std::multiset<int> ms = lift(a).retain_values<std::multiset>();
    CHECK_SAME_ELEMENTS( ms, std::vector<int> { 1, 2, 3, 4, 4 } );
    
    CHECK_SAME_ELEMENTS( lift(a).drop(0).lower_values<std::vector>(), std::vector<int> { 3, 1, 4, 4, 2 } );
    CHECK_SAME_ELEMENTS( lift(a).drop(3).lower_values<std::vector>(), std::vector<int> { 4, 2 } );
    CHECK_SAME_ELEMENTS( lift(a).slice(1, 3).lower_values<std::vector>(), std::vector<int> { 1, 4 } );
    CHECK_SAME_ELEMENTS( lift(a).take(3).lower_values<std::vector>(), std::vector<int> { 3, 1, 4 } );
    
    std::vector<std::pair<int, std::string>> b =
    {
        std::make_pair( 1, "B" ),
        std::make_pair( 2, "D" ),
        std::make_pair( 3, "X" ),
        std::make_pair( 1, "C" ),
        std::make_pair( 2, "A" ),
        std::make_pair( 2, "B" ),
        std::make_pair( 4, "Z" ),
        std::make_pair( 2, "C" ),
        std::make_pair( 1, "A" ),
        std::make_pair( 3, "Y" )
    };
    
    
            
    std::map<int, std::string> mv = lift(b).copyElements().lower<std::map>();
    BOOST_REQUIRE_EQUAL( mv.size(), 4 );
    BOOST_CHECK_EQUAL( mv[1], "B" );
    BOOST_CHECK_EQUAL( mv[2], "D" );
    BOOST_CHECK_EQUAL( mv[3], "X" );
    BOOST_CHECK_EQUAL( mv[4], "Z" );
    
    std::multimap<int, std::string> mmv = lift(b).copyElements().lower<std::multimap>();
    BOOST_REQUIRE_EQUAL( mmv.size(), 10 );
    
    auto grouped = lift(b)
        .groupBy(
            []( const std::pair<int, std::string>& v ) { return v.first; },
            []( const std::pair<int, std::string>& v ) -> std::string { return v.second; } )
        .lower_values<std::map>();
        
        
    BOOST_REQUIRE_EQUAL( grouped.size(), 4 );
    CHECK_SAME_ELEMENTS( grouped[1], std::vector<std::string> { "B", "C", "A" } );
    CHECK_SAME_ELEMENTS( grouped[2], std::vector<std::string> { "D", "A", "B", "C" } );
    CHECK_SAME_ELEMENTS( grouped[3], std::vector<std::string> { "X", "Y" } );
    CHECK_SAME_ELEMENTS( grouped[4], std::vector<std::string> { "Z" } );
    
    auto counts = lift(b)
        .countBy( []( const std::pair<int, std::string>& v ) { return v.first; } )
        .lower_values<std::map>();
        
    BOOST_REQUIRE_EQUAL( counts.size(), 4 );
    BOOST_CHECK_EQUAL( counts[1], 3 );
    BOOST_CHECK_EQUAL( counts[2], 4 );
    BOOST_CHECK_EQUAL( counts[3], 2 );
    BOOST_CHECK_EQUAL( counts[4], 1 );
    
    std::vector<double> c = { 1.0, 2.0, 3.0, 4.0, 6.0, 7.0, 8.0, 4.9, 4.9, 5.2, 4.9, 4.9, 5.2, 9.0, 5.0 };
    
    BOOST_CHECK_EQUAL( lift(c).retain_values<std::vector>().count(), 15 );
    BOOST_CHECK_EQUAL( lift(c).retain_values<std::list>().count(), 15 );
    BOOST_CHECK_EQUAL( lift(c).retain_values<std::deque>().count(), 15 );
    //BOOST_CHECK_EQUAL( lift(c).retain<std::set>().count(), 11 );
    
    BOOST_CHECK_CLOSE( lift(c).mean(), 5.0, 1e-6 );
    BOOST_CHECK_CLOSE( lift(c).median(), 4.9, 1e-6 );
    
    BOOST_CHECK_CLOSE( lift(c).min(), 1.0, 1e-6 );
    BOOST_CHECK_CLOSE( lift(c).max(), 9.0, 1e-6 );
    
    BOOST_CHECK_EQUAL( lift(c).argMin().first, 0 );
    BOOST_CHECK_EQUAL( lift(c).argMax().first, 13 );
    
    BOOST_CHECK_CLOSE( lift(c).take(1).median(), 1.0, 1e-6 );
    BOOST_CHECK_CLOSE( lift(c).take(2).median(), 1.5, 1e-6 );
    BOOST_CHECK_CLOSE( lift(c).take(3).median(), 2.0, 1e-6 );
    BOOST_CHECK_CLOSE( lift(c).take(4).median(), 2.5, 1e-6 );
    
    struct Smook
    {
        Smook( int a ) : m_a(a) {}
        int m_a;    
    };
    
    // Simple check that a class that does not have +-/ defined
    // will still work despite the presence of mean, median, min, max
    // operations that do not apply to it.
    std::vector<Smook> smook = { Smook(1), Smook(2), Smook(3) };
    BOOST_CHECK_EQUAL( lift(smook).take(2).copyElements().retain<std::vector>().count(), 2 );
}

// Tests for nested containers, and functionality like flatMap and flatten
int identity( int a ) { return a; }

template<typename T>
struct LambdaWrapper
{
    LambdaWrapper( T fn ) : m_fn(fn)
    {
    }
    
    T m_fn;
};

template<typename T>
LambdaWrapper<T> makeLambdaWrapper( T fn ) { return LambdaWrapper<T>(fn); }

void testFlatMap()
{
    std::vector<std::vector<int>> d = { {1, 2, 3, 4, 5, 6, 7}, {4, 5}, {6}, {}, { 7, 8, 9, 10 } };
    
    {
        auto addOneBase = mlift(d)
            .map( []( const std::vector<int>& v )
            {
                return lift(v).map( identity ).lower<std::vector>();
            } )
            .retain<std::vector>();
        
    
        addOneBase.zip(lift(d)).foreach( []( std::pair<const std::vector<int>&, const std::vector<int>&> r )
        {
            CHECK_SAME_ELEMENTS( r.first, r.second );
        } );
    }

    // flatMap
    {
        auto addOne = mlift(d).map( []( const std::vector<int>& v )
        {
            return lift(v).map( []( int a ) { return a+1; } );
        } );
        
        std::vector<int> res = addOne.flatMap( []( int a ) { return a*2; } ).retain<std::vector>();

        CHECK_SAME_ELEMENTS( res, std::vector<int> { 4, 6, 8, 10, 12, 14, 16, 10, 12, 14, 16, 18, 20, 22 } );
    }
    
    // flatten
    {
        auto addOne = mlift(d).map( []( const std::vector<int>& v )
        {
            return lift(v).map( []( int a ) { return 2*(a+1); } );
        } );
        
        std::vector<int> res = addOne.flatten().retain<std::vector>();

        CHECK_SAME_ELEMENTS( res, std::vector<int> { 4, 6, 8, 10, 12, 14, 16, 10, 12, 14, 16, 18, 20, 22 } );
    }

    // Returning new vectors from map
    {
        std::vector<int> res = lift(d)
            .map([](const std::vector<int>& v)
            {
                std::vector<int> r;
                for( int x : v ) { r.push_back( x ); }
                return clift(std::move(r));
            })
            .checkIteratorElementType<ContainerWrapper<std::vector<int>, int>>()
            .flatten()
            .retain_values<std::vector>();

        CHECK_SAME_ELEMENTS( res, std::vector<int> { 1, 2, 3, 4, 5, 6, 7, 4, 5, 6, 7, 8, 9, 10 } );
    }
}

void test3()
{
    {
        std::vector<int> foo = { 1, 2, 3, 4, 5 };
        
        mlift(foo)
            .foreach( []( int& a )
            {
                a += 1;
            } );
        
        CHECK_SAME_ELEMENTS( foo, std::vector<int> { 2, 3, 4, 5, 6 } );
    }
    
    {
        std::vector<int> foo = { 1, 2, 3, 4, 5 };
        
        mlift(foo)
            .filter( []( int & a ) { return a > 3; } )
            .foreach( []( int& a )
            {
                a += 1;
            } );
        
        CHECK_SAME_ELEMENTS( foo, std::vector<int> { 1, 2, 3, 5, 6 } );
    }
    
    {
        std::vector<int> foo = {};
        auto res = lift(foo)
            .sliding2()
            .lower<std::vector>();
            
        BOOST_REQUIRE_EQUAL( res.size(), 0 );
    }
    
    {
        std::vector<int> foo = { 1 };
        auto res = lift(foo)
            .sliding2()
            .lower<std::vector>();
            
        BOOST_REQUIRE_EQUAL( res.size(), 0 );
    }
    
    {
        std::vector<int> foo = { 1, 2, 3, 4, 5 };
        mlift(foo)
            .map( []( int& a ) { return &a; } )
            .sliding2()
            .foreach( []( std::pair<int*, int*> t )
            {
                *t.first += *t.second;
            } );
            
        CHECK_SAME_ELEMENTS( foo, std::vector<int> { 3, 5, 7, 9, 5 } );
    }
    
    {
        std::vector<int> foo = { 1, 2, 3, 4, 5 };
        mlift(foo)
            .sliding2()
            .foreach( []( std::pair<int&, int&> t )
            {
                t.first += t.second;
            } );
        CHECK_SAME_ELEMENTS( foo, std::vector<int> { 3, 5, 7, 9, 5 } );
    }
}

void testStringManip()
{
    std::vector<std::string> els1 = { "Foo", "Bar", "Baz", "Qux", "Bippy" };
    BOOST_CHECK_EQUAL( lift(els1).mkString(", "), "Foo, Bar, Baz, Qux, Bippy" );
    
    std::vector<int> els2 = { 1, 2, 3, 4, 5 };
    BOOST_CHECK_EQUAL( lift(els2).mkString("-"), "1-2-3-4-5" );
    
    BOOST_CHECK_EQUAL( lift( " dasd adsd   " ).trim().toString(), "dasd adsd" );
    
    std::string elements = " 1,  2, 3,  4,    5 ";
    std::vector<int> tidied = lift(elements)
        .split(",")
        .map( []( const std::string& el ) { return boost::lexical_cast<int>(lift(el).trim().toString()); } )
        .retain<std::vector>();

    CHECK_SAME_ELEMENTS( els2, tidied );
    
    std::string lines( "1, 2 ,3  \n 4 ,5, 6\n7,8,9\n10,   11,12  " );
    
    std::istringstream iss(lines);
    std::vector<int> res = lift(iss)
        .map( []( const std::string& line )
        {
            auto els = lift(line).split(",").retain_values<std::vector>();
            
            std::vector<int> numEls = els.map( []( const std::string& el )
            {
                auto trimmed = lift(el).trim().toString();
                return boost::lexical_cast<int>( trimmed );
            } ).retain_values<std::vector>();
            return numEls[1];
        } )
        .retain<std::vector>();
        
    CHECK_SAME_ELEMENTS( res, std::vector<int> { 2, 5, 8, 11 } );
}

void testPartitions()
{
    std::vector<int> a = { 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1 };
    
    auto fn = []( int a ) { return a < 5; };
    
    auto p = lift(a).partition( fn );
    CHECK_SAME_ELEMENTS( p.first, std::vector<int> { 1, 2, 3, 4, 4, 3, 2, 1 } );
    CHECK_SAME_ELEMENTS( p.second, std::vector<int> { 5, 6, 7, 6, 5 } );
    
    auto pw = lift(a).partitionWhile( fn );
    CHECK_SAME_ELEMENTS( pw.first, std::vector<int> { 1, 2, 3, 4 } );
    CHECK_SAME_ELEMENTS( pw.second, std::vector<int> { 5, 6, 7, 6, 5, 4, 3, 2, 1 } );
}

void testCounter()
{
    CHECK_SAME_ELEMENTS( counter().take(5).lower<std::vector>(), std::vector<int> { 0, 1, 2, 3, 4 } );
}

void testStream()
{
    class Source
    {
    public:
        Source() : m_eof(false), m_val(0) {}
        typedef int value_type;
        bool eof() { return m_eof; }
        int pop() { return m_val; }

        bool m_eof;
        int m_val;
    };

    Source s;
    s.m_val = 1;
    std::vector<int> res = slift(s).take(2).retain<std::vector>();
    CHECK_SAME_ELEMENTS( res, std::vector<int> { 1, 1 } );
}

void testShortInputs()
{
    auto res = std::vector<int>( { 1, 2, 3, 4 } );
    CHECK_SAME_ELEMENTS( lift(res).take(4, ASSERT_WHEN_INSUFFICIENT).lower<std::vector>(), res );

    BOOST_CHECK_THROW( lift(res).take(5, ASSERT_WHEN_INSUFFICIENT).retain<std::vector>(), SliceError );
    BOOST_CHECK_THROW( lift(res).drop(5, ASSERT_WHEN_INSUFFICIENT).retain<std::vector>(), SliceError );
    BOOST_CHECK_THROW( lift(res).slice(6, 8, ASSERT_WHEN_INSUFFICIENT).retain<std::vector>(), SliceError );
    BOOST_CHECK_THROW( lift(res).slice(3, 8, ASSERT_WHEN_INSUFFICIENT).retain<std::vector>(), SliceError );

    BOOST_CHECK_THROW( lift(std::vector<int>()).mean(), std::runtime_error );
    
}

void testNonCopyable()
{
    class NoCopy : public boost::noncopyable
    {
    };
    
    std::vector<NoCopy> v(10);

    int count = 0;
    lift(v)
    .foreach([&count](const NoCopy& n)
    {
        count++;
    });
    BOOST_CHECK_EQUAL( count, 10 );
}

void testNestedReferences()
{
    int a=1;
    int b=2;
    std::vector<std::reference_wrapper<int>> v = {a, b};

    auto s = lift(v)
    .map([](const int& i)
    {
        return i+1;
    })
    .sum();
    BOOST_CHECK_EQUAL( s, 5 );
}

void testOptional()
{
    struct Thing
    {
        Thing(int& res) : m_res(res) {}
        ~Thing() { m_res++; }

        int& m_res;
        int a;
        long b;
    };

    {
        Optional<Thing> o;
        BOOST_CHECK_EQUAL( static_cast<bool>(o), false );
    }

    int res = 0;
    Thing t(res); t.a=1; t.b=2;
    {
        Optional<Thing>o(t);
        BOOST_CHECK_EQUAL( res, 0 );
        BOOST_CHECK_EQUAL( static_cast<bool>(o), true );
        BOOST_CHECK_EQUAL( o->a, 1 );
        BOOST_CHECK_EQUAL( o->b, 2 );
        BOOST_CHECK_EQUAL( (*o).a, 1 );
        BOOST_CHECK_EQUAL( (*o).b, 2 );

        Thing u(res); u.a=0; u.b=-1;
        o = u;
        BOOST_CHECK_EQUAL( res, 1 );
        BOOST_CHECK_EQUAL( o->a, 0 );

        Optional<Thing> o2(o);
        BOOST_CHECK_EQUAL( o->a, 0 );
    }
    // res incremented by all of u, o & o2 dying
    BOOST_CHECK_EQUAL( res, 4 );

    {
        Optional<std::unique_ptr<Thing>> o;
    }

    int unique_res=0;
    std::unique_ptr<Thing> p(new Thing(unique_res));
    p->a = 3; p->b = 4;
    {
        Optional<std::unique_ptr<Thing>> o(std::move(p));
        BOOST_CHECK_EQUAL( unique_res, 0 );
        BOOST_CHECK_EQUAL( (*o)->a, 3 );
        
        std::unique_ptr<Thing> q(new Thing(unique_res));
        q->a = 5; q->b = 6;
        o = std::move(q);
        // unique_res now increments as the object originally pointed at by p
        // has now been destroyed
        BOOST_CHECK_EQUAL( unique_res, 1 );
        BOOST_CHECK_EQUAL( (*o)->a, 5 );

        Optional<std::unique_ptr<Thing>> o2(std::move(o));
        BOOST_CHECK_EQUAL( unique_res, 1 );
        BOOST_CHECK_EQUAL( (*o2)->a, 5 );
    }
    BOOST_CHECK_EQUAL( unique_res, 2 );
}



void testIteratorAndIterable()
{
    // Check that we can iterate repeatedly over lifted container wrappers (they are iterable,
    // rather than just iterators)
    {
        std::vector<int> a = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        
        auto minusOne = lift(a)
            .map( []( int v ) { return v - 1; } )
            .retain<std::vector>();
            
        std::vector<int> squared = minusOne
            .map( []( int v ) { return v * v; } )
            .retain<std::vector>(); 
           
        CHECK_SAME_ELEMENTS( squared, std::vector<int> { 0, 1, 4, 9, 16, 25, 36, 49, 64 } );
           
        std::vector<std::string> asString = minusOne
            .map( []( int v ) { return boost::lexical_cast<std::string>(v); } )
            .retain<std::vector>(); 
            
        
        CHECK_SAME_ELEMENTS( asString, std::vector<std::string> { "0", "1", "2", "3", "4", "5", "6", "7", "8" } );
    }
    
    // Quick check that a lifted empty container behaves itself
    {
        std::vector<int> a;
        
        std::vector<int> res = lift(a).map( []( int v ) { return v + 1; } ).retain<std::vector>();
        BOOST_CHECK( res.empty() );
    }
    
    // And that the same doesn't work without collapsing to a vec
    {
        std::vector<int> a = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        
        auto minusOne = lift(a)
            .map( []( int v ) { return v - 1; } );
            
        std::vector<int> squared = minusOne
            .map( []( int v ) { return v * v; } )
            .retain<std::vector>(); 

        CHECK_SAME_ELEMENTS( squared, std::vector<int> { 0, 1, 4, 9, 16, 25, 36, 49, 64 } );            
         
        // Re-enable when moving to g++ 4.8.1
        /*   
        std::vector<std::string> asString = minusOne
            .map( []( int v ) { return boost::lexical_cast<std::string>(v); } )
            .retain<std::vector>(); 

        for ( auto s : asString ) std::cout << "FOOT: " << s << std::endl;
        BOOST_CHECK( asString.empty() );
        */
    }
}

void addTests( test_suite *t )
{
    t->add( BOOST_TEST_CASE( testStructuralRequirements ) );
    t->add( BOOST_TEST_CASE( test1 ) );
    t->add( BOOST_TEST_CASE( testFlatMap ) );
    t->add( BOOST_TEST_CASE( test3 ) );
    t->add( BOOST_TEST_CASE( testStream ) );
    t->add( BOOST_TEST_CASE( testStringManip ) );
    t->add( BOOST_TEST_CASE( testPartitions ) );
    t->add( BOOST_TEST_CASE( testCounter ) );
    t->add( BOOST_TEST_CASE( testShortInputs) );
    t->add( BOOST_TEST_CASE( testNonCopyable) );
    t->add( BOOST_TEST_CASE( testNestedReferences) );
    t->add( BOOST_TEST_CASE( testOptional ) );
    t->add( BOOST_TEST_CASE( testIteratorAndIterable ) );
}

bool init()
{
    addTests( &framework::master_test_suite() );
    return true;
}

int main(int argc, char* argv[])
{
    return unit_test_main(&init, argc, argv);
}


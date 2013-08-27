#pragma once

#include <set>
#include <map>
#include <list>
#include <deque>
#include <tuple>
#include <vector>
#include <sstream>
#include <type_traits>

#include <boost/optional.hpp>
#include <boost/function.hpp>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>


#define ESCALATOR_ASSERT( predicate, message ) \
if ( !(predicate) ) \
{ \
    std::stringstream ss; \
    ss << "Escalator assertion failure: " << #predicate << ": " << message; \
    throw std::runtime_error( ss.str() ); \
}


namespace navetas { namespace escalator {

    template<typename T>
    class move_on_copy_wrapper
    {
    public:
        move_on_copy_wrapper(T&& t):
            m_value(std::move(t))
        {
        }

        move_on_copy_wrapper(move_on_copy_wrapper& other):
            m_value(std::move(other.m_value))
        {
        }

        move_on_copy_wrapper& operator=(move_on_copy_wrapper& other)
        {
            m_value = std::move(other.m_value);
            return *this;
        }
        
        move_on_copy_wrapper(const move_on_copy_wrapper& other):
            m_value(std::move(other.m_value))
        {
        }

        move_on_copy_wrapper& operator=(const move_on_copy_wrapper& other)
        {
            m_value = std::move(other.m_value);
            return *this;
        }
        
        const T& get() const { return m_value; }
        T& get() { return m_value; }
        
    private:
        mutable T   m_value;
    };
    
    template<typename T>
    move_on_copy_wrapper<T> moc_wrap( T&& v )
    {
        return move_on_copy_wrapper<T>( std::move(v) );
    }

    template<typename FunctorT, typename InputT>
    struct FunctorHelper
    {
        typedef decltype(std::declval<FunctorT>()( std::declval<InputT>() )) out_t;
    };

    template<typename Source, typename InputT>
    class CopyWrapper;

    template<typename Source, typename FunctorT, typename InputT, typename ElT>
    class MapWrapper;
    
    template<typename Source, typename FunctorT, typename InputT, typename InnerT, typename ElT>
    class FlatMapWrapper;
    
    template<typename Source, typename FunctorT, typename InputT, typename ElT, typename StateT>
    class MapWithStateWrapper;
    
    template<typename Source, typename FunctorT, typename ElT>
    class FilterWrapper;
    
    template<typename ContainerT, typename IterT, template<typename> class FunctorT>
    class IteratorWrapper;
    
    template<typename Container, typename ElT>
    class ContainerWrapper;

    enum SliceBehavior
    {
        RETURN_UPTO,
        ASSERT_WHEN_INSUFFICIENT
    };
    class SliceError : public std::range_error
    {
    public:
        SliceError( const std::string& what_arg ) : std::range_error( what_arg ) {}
        SliceError( const char* what_arg ) : std::range_error( what_arg ) {}
    };
    template<typename SourceT, typename ElT>
    class SliceWrapper;
    
    template<typename Source1T, typename El1T, typename Source2T, typename El2T>
    class ZipWrapper;

    
    template<typename R>
    struct remove_all_reference
    {
        typedef typename std::remove_reference<R>::type type;
    };

    template<typename R>
    struct remove_all_reference<std::reference_wrapper<R>>
    {
        typedef R type;
    };
    
    template<typename T>
    struct remove_all_reference_then_remove_const
    {
        typedef typename std::remove_const<typename remove_all_reference<T>::type>::type type;
    };
    
    template<typename T>
    class WrapWithReferenceWrapper
    {
    public:
        typedef std::reference_wrapper<typename std::remove_reference<T>::type> type;
        type operator()( T& v ) { return v; }
    };
    
    template<typename T>
    class IdentityFunctor
    {
    public:
        typedef T type;
        T operator()( T t ) const { return t; }
    };
    
    template<typename T>
    class CopyFunctor
    {
    public:
        typedef typename std::remove_reference<T>::type type;
        type operator()( T t ) const { return t; }
    };
    
    template<typename T>
    class CopyStripConstFunctor
    {
    public:
        typedef typename std::remove_const<typename std::remove_reference<T>::type>::type type;
        type operator()( T t ) const { return t; }
    };

    template<typename ContainerT>
    ContainerWrapper<ContainerT, typename ContainerT::value_type>
    clift( ContainerT&& cont );

    template<typename ContainerT>
    IteratorWrapper<
        ContainerT,
        typename ContainerT::const_iterator,
        CopyStripConstFunctor>
    lift( const ContainerT& cont );

    class EmptyError : public std::range_error
    {
    public:
        EmptyError( const std::string& what_arg ) : std::range_error( what_arg ) {}
        EmptyError( const char* what_arg ) : std::range_error( what_arg ) {}
    };

    // Non-templatised marker trait for type trait functionality
    class Lifted {};
    
    template<typename ElT, template<typename, typename ...> class Container>
    struct MakeContainerType
    {
        typedef Container<ElT> type;
    };
    
    template<typename El1T, typename El2T>
    struct MakeContainerType<std::pair<El1T, El2T>, std::map>
    {
        typedef std::map<El1T, El2T> type;
    };
    
    template<typename El1T, typename El2T>
    struct MakeContainerType<std::pair<El1T, El2T>, std::multimap>
    {
        typedef std::multimap<El1T, El2T> type;
    };
    
    
    template<typename ElT, template<typename, typename ...> class Container>
    class ConversionHelper
    {
    public:
        typedef typename MakeContainerType<ElT, Container>::type ContainerType;
    
        template<typename InputIterator>
        static ContainerType lower( InputIterator it )
        {
            ContainerType t;
            while ( it.hasNext() ) t.insert( t.end(), it.next() );
            return t;
        }
        
        template<typename InputIterator>
        static ContainerWrapper<ContainerType, ElT> retain( InputIterator it )
        {
            ContainerType t;
            while ( it.hasNext() ) t.insert( t.end(), it.next() );
            return ContainerWrapper<ContainerType, ElT>( std::move(t) );
        }
    };
    
    
    template<typename BaseT, typename ElT, typename RetainElT>
    class ConversionsBase : public Lifted
    {
    protected:
        // BaseT always implements:
        // getIterator(), which returns a type Iterator which implements:
        //     ElT next();
        //     bool hasNext();
        BaseT& get() { return static_cast<BaseT&>(*this); }
        
    public:
        typedef ElT el_t;
        //typedef typename remove_all_reference_then_remove_const<ElT>::type mutable_value_type;
        typedef typename std::remove_const<typename std::remove_reference<ElT>::type>::type mutable_value_type;
        
        template< class OutputIterator >
        void toContainer( OutputIterator v ) 
        {
            auto it = get().getIterator();
            while ( it.hasNext() ) *v++ = it.next();
        }
        
        template<typename ElementCheckType>
        BaseT& checkIteratorElementType()
        {
            static_assert( std::is_same<ElementCheckType, ElT>::value, "Element type does not match requirements" );
            return get();
        }
        
        template<typename ElementCheckType>
        BaseT& checkRawElementType()
        {
            static_assert( std::is_same<ElementCheckType, RetainElT>::value, "Element type does not match requirements" );
            return get();
        }
        
        template<template<typename, typename ...> class Container>
        typename ConversionHelper<mutable_value_type, Container>::ContainerType lower()
        {
            return ConversionHelper<mutable_value_type, Container>::lower( get().getIterator() );
        }
        
        template<template<typename, typename ...> class Container>
        ContainerWrapper<typename ConversionHelper<mutable_value_type, Container>::ContainerType, mutable_value_type> retain()
        {
            return ConversionHelper<mutable_value_type, Container>::retain( get().getIterator() );
        }
        
        template<typename FunctorT>
        bool forall( FunctorT fn )
        {
            bool pred = true;
            auto it = get().getIterator();
            while ( pred && it.hasNext() ) pred &= fn( it.next() );
            return pred;
        }
        
        template<typename FunctorT>
        bool exists( FunctorT fn )
        {
            bool pred = false;
            auto it = get().getIterator();
            while ( it.hasNext() ) pred |= fn( it.next() );
            return pred;
        }
        
        // TODO: Can this remain lifted?
        template<typename FunctorT>
        std::pair<std::vector<ElT>, std::vector<ElT>> partition( FunctorT fn )
        {
            std::pair<std::vector<ElT>, std::vector<ElT>> res;
            
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                ElT val = it.next();
                if ( fn(val) ) res.first.push_back( val );
                else res.second.push_back( val );
            }
            
            return res;
        }
        
        // TODO: Can this remain lifted?
        template<typename FunctorT>
        std::pair<std::vector<ElT>, std::vector<ElT>> partitionWhile( FunctorT fn )
        {
            std::pair<std::vector<ElT>, std::vector<ElT>> res;
            
            bool inFirst = true;
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                ElT val = it.next();
                if ( !fn(val) ) inFirst = false;
                
                if ( inFirst ) res.first.push_back( val );
                else res.second.push_back( val );
            }
            
            return res;
        }
        
        // TODO: This should probably be lazy
        template<typename FunctorT>
        ContainerWrapper<std::vector<ElT>, ElT> takeWhile( FunctorT fn ) { return ContainerWrapper<std::vector<ElT>, ElT>( partitionWhile(fn).first ); }
        
        template<typename FunctorT>
        ContainerWrapper<std::vector<ElT>, ElT> dropWhile( FunctorT fn ) { return ContainerWrapper<std::vector<ElT>, ElT>( partitionWhile(fn).second ); }
        
        template<typename FunctorT>
        MapWrapper<BaseT, FunctorT, ElT, typename FunctorHelper<FunctorT, ElT>::out_t> map( FunctorT fn )
        {
            return MapWrapper<BaseT, FunctorT, ElT, typename FunctorHelper<FunctorT, ElT>::out_t>( get().getIterator(), fn );
        }

        CopyWrapper<BaseT, ElT> copyElements()
        {
            return CopyWrapper<BaseT, ElT>( std::move(get().getIterator()) );
        }
        
        template<typename FromT, typename ToT>
        class CastFunctor
        {
        public:
            ToT operator()( const FromT v ) const { return static_cast<ToT>( v ); }
        };
        
        template<typename ToT>
        MapWrapper<BaseT, CastFunctor<ElT, ToT>, ElT, ToT> castElements()
        {
            CastFunctor<ElT, ToT> cf;
            return map( cf );
        }
        
        template<typename FunctorT>
        FilterWrapper<BaseT, FunctorT, ElT> filter( FunctorT fn )
        {
            return FilterWrapper<BaseT, FunctorT, ElT>( std::move(get().getIterator()), fn );
        }
        
        typedef MapWithStateWrapper<BaseT, std::function<std::pair<ElT, size_t>(const ElT&, size_t&)>, ElT, std::pair<ElT, size_t>, size_t> zipWithIndexWrapper_t;
        zipWithIndexWrapper_t zipWithIndex()
        {
            auto it = get().getIterator();
            return zipWithIndexWrapper_t(
                std::move(it),
                []( const ElT& el, size_t& index ) { return std::make_pair( el, index++ ); },
                0 );
        }
        
        typedef MapWithStateWrapper<BaseT, std::function<std::pair<ElT, ElT>( ElT, boost::optional<ElT>& state )>, ElT, std::pair<ElT, ElT>, boost::optional<ElT>> sliding2_t;
        sliding2_t sliding2()
        {
            auto it = get().getIterator();
            boost::optional<ElT> startState;
            if ( it.hasNext() ) startState = it.next();
            return sliding2_t(
                std::move(it),
                []( ElT el, boost::optional<ElT>& state )
                {
                    std::pair<ElT, ElT> tp = std::pair<ElT, ElT>( state.get(), el );
                    state = el;
                    return tp;
                },
                startState );
        }
        
        template<typename OrderingF>
        ContainerWrapper<std::vector<ElT>, ElT> sortWith( OrderingF orderingFn )
        {
            std::vector<ElT> v = lower<std::vector>();
            std::sort( v.begin(), v.end(), orderingFn );
            ContainerWrapper<std::vector<ElT>, ElT> vw( std::move(v) );
            
            return vw;
        }
        
        template<typename KeyF>
        ContainerWrapper<std::vector<ElT>, ElT> sortBy( KeyF keyFn )
        {
            std::vector<ElT> v = lower<std::vector>();
            std::sort( v.begin(), v.end(), [keyFn]( const ElT& lhs, const ElT& rhs ) { return keyFn(lhs) < keyFn(rhs); } );
            
            ContainerWrapper<std::vector<ElT>, ElT> vw( std::move(v) );
            
            return vw;
        }

        ContainerWrapper<std::vector<ElT>, ElT> sort()
        {
            std::vector<ElT> v = lower<std::vector>();
            std::sort( v.begin(), v.end(), [](const ElT& a, const ElT& b)
            {
                //May be asked to compare std::reference_wrappers around types
                //This doesn't seem to find the operator< by default,
                //probably as it's defined on the mutable_value_type as a class method
                //and C++ won't try the default operator& on the reference wrapper
                //and dig around.
                const mutable_value_type& v_a = a;
                const mutable_value_type& v_b = b;
                return v_a < v_b;
            });
            
            ContainerWrapper<std::vector<ElT>, ElT> vw( std::move(v) );
            
            return vw;
        }
        
        template<typename FunctorT>
        void foreach( FunctorT fn )
        {
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                fn( it.next() );
            }
        }
        
        template<typename KeyFunctorT, typename ValueFunctorT>
        auto groupBy( KeyFunctorT keyFn, ValueFunctorT valueFn ) ->
            ContainerWrapper<
                std::map<typename FunctorHelper<KeyFunctorT, ElT>::out_t, std::vector<typename FunctorHelper<ValueFunctorT, ElT>::out_t>>,
                std::pair<const typename FunctorHelper<KeyFunctorT, ElT>::out_t, std::vector<typename FunctorHelper<ValueFunctorT, ElT>::out_t>>>
        {
            typedef typename FunctorHelper<KeyFunctorT, ElT>::out_t key_t;
            typedef typename FunctorHelper<ValueFunctorT, ElT>::out_t mutable_value_type;
            
            std::map<key_t, std::vector<mutable_value_type>> grouped;
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                auto v = it.next();
                auto key = keyFn(v);
                grouped[key].push_back( valueFn(v) );
            }
            
            return ContainerWrapper<
                std::map<typename FunctorHelper<KeyFunctorT, ElT>::out_t, std::vector<typename FunctorHelper<ValueFunctorT, ElT>::out_t>>,
                std::pair<const typename FunctorHelper<KeyFunctorT, ElT>::out_t, std::vector<typename FunctorHelper<ValueFunctorT, ElT>::out_t>>>
                (grouped);
        }
        
        template<typename KeyFunctorT>
        auto countBy( KeyFunctorT keyFn ) ->
            ContainerWrapper<
                std::map<typename FunctorHelper<KeyFunctorT, ElT>::out_t, size_t>,
                std::pair<const typename FunctorHelper<KeyFunctorT, ElT>::out_t, size_t>>
        {
            typedef typename FunctorHelper<KeyFunctorT, ElT>::out_t key_t;
            
            std::map<key_t, size_t> counts;
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                auto v = it.next();
                auto key = keyFn(v);
                auto findIt = counts.find( key );
                if ( findIt == counts.end() )
                {
                    counts.insert( std::make_pair(key, 1) );
                }
                else
                {
                    findIt->second += 1;
                }
            }
            
            return ContainerWrapper<
                std::map<typename FunctorHelper<KeyFunctorT, ElT>::out_t, size_t>,
                std::pair<const typename FunctorHelper<KeyFunctorT, ElT>::out_t, size_t>>( counts );
        }
        
        // TODO : Note that this forces evaluation of the input stream
        // TODO: distinct should be wrappable into distinctWith using
        // std::less
        ContainerWrapper<std::vector<ElT>, ElT> distinct()
        {
            std::set<ElT> seen;
            std::vector<typename std::set<ElT>::iterator> ordering;
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                auto res = seen.insert( it.next() );
                if ( res.second ) ordering.push_back( res.first );
            }
            
            std::vector<ElT> res;
            for ( auto& it : ordering )
            {
                res.push_back( *it );
            }
            
            ContainerWrapper<std::vector<ElT>,  ElT> vw( res );
            return vw;
        }
        
        template<typename SetOrdering>
        ContainerWrapper<std::vector<ElT>, ElT> distinctWith( SetOrdering cmp )
        {
            //Same pattern as distinct above
            std::set<ElT, SetOrdering> seen( cmp );
            std::vector<typename std::set<ElT>::iterator> ordering;
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                auto res = seen.insert( it.next() );
                if ( res.second ) ordering.push_back( res.first );
            }
            
            std::vector<ElT> res;
            for ( auto& it : ordering )
            {
                res.push_back( *it );
            }
            
            ContainerWrapper<std::vector<ElT>,  ElT> vw( std::move(res) );
            return vw;
        }
        
        template<typename FunctorT, typename AccT>
        AccT fold( AccT init, FunctorT fn )
        {
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                init = fn( init, it.next() );
            }
            return init;
        }
        
        template<typename Source2T>
        ZipWrapper<BaseT, ElT, typename std::remove_reference<Source2T>::type, typename std::remove_reference<Source2T>::type::el_t>
        zip( Source2T&& source2 )
        {
            typedef typename std::remove_reference<Source2T>::type Source2TNoRef;
            auto it1 = get().getIterator();
            auto it2 = source2.getIterator();
            return ZipWrapper<BaseT
                             , ElT
                             , Source2TNoRef
                             , typename Source2TNoRef::el_t>( std::move(it1), std::move(it2) );
        }
        
        SliceWrapper<BaseT, ElT> slice( size_t from, size_t to, SliceBehavior behavior=RETURN_UPTO )
        {
            auto it = get().getIterator();
            return SliceWrapper<BaseT, ElT>( std::move(it), from, to, behavior );
        }
        
        SliceWrapper<BaseT, ElT> drop( size_t num, SliceBehavior behavior=RETURN_UPTO )
        {
            auto it = get().getIterator();
            return SliceWrapper<BaseT, ElT>( std::move(it), num, std::numeric_limits<size_t>::max(), behavior );
        }
        
        SliceWrapper<BaseT, ElT> take( size_t num, SliceBehavior behavior=RETURN_UPTO )
        {
            auto it = get().getIterator();
            return SliceWrapper<BaseT, ElT>( std::move(it), 0, num, behavior );
        }
        
        size_t count()
        {
            size_t count = 0;
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                it.next();
                count++;
            }
            return count;
        }
        
        mutable_value_type sum()
        {
            auto it = get().getIterator();
            mutable_value_type acc = mutable_value_type();

            while ( it.hasNext() )
            {
                acc += it.next();
            }
            return acc;
        }
        
        mutable_value_type mean()
        {
            auto it = get().getIterator();
            ESCALATOR_ASSERT( it.hasNext(), "Mean over insufficient items" );
            
            size_t count = 1;
            mutable_value_type acc = it.next();
            while ( it.hasNext() )
            {
                acc += it.next();
                count++;
            }
            
            return acc / static_cast<double>(count);
        }
        
        mutable_value_type median()
        {
            auto it = get().getIterator();
            ESCALATOR_ASSERT( it.hasNext(), "Median over insufficient items" );
            
            std::vector<ElT> values;
            size_t count = 0;
            while ( it.hasNext() )
            {
                values.push_back( it.next() );
                count++;
            }
            std::sort( values.begin(), values.end() );
            
            if ( count & 1 )
            {
                return values[count/2];
            }
            else
            {
                return (values[count/2] + values[(count/2)-1]) / 2.0;
            }
        }
        
        // TODO: Should these use boost::optional to work round init requirements?
        std::pair<size_t, mutable_value_type> argMin()
        {
            bool init = true;
            mutable_value_type ext = mutable_value_type();
            
            size_t minIndex = 0;
            auto it = get().getIterator();
            for ( int i = 0; it.hasNext(); ++i )
            {
                if ( init ) ext = it.next();
                else
                {
                    auto n = it.next();
                    if ( n < ext )
                    {
                        ext = n;
                        minIndex = i;
                    }
                }
                init = false;
            }
            return std::make_pair( minIndex, ext );
        }
        
        std::pair<size_t, mutable_value_type> argMax()
        {
            bool init = true;
            mutable_value_type ext = mutable_value_type();
            
            size_t maxIndex = 0;
            auto it = get().getIterator();
            for ( int i = 0; it.hasNext(); ++i )
            {
                if ( init ) ext = it.next();
                else
                {
                    auto n = it.next();
                    if ( n > ext )
                    {
                        ext = n;
                        maxIndex = i;
                    }
                }
                init = false;
            }
            return std::make_pair( maxIndex, ext );
        }
        
        mutable_value_type min() { return std::template get<1>(argMin()); }
        mutable_value_type max() { return std::template get<1>(argMax()); }
        
        std::string mkString( const std::string& sep )
        {
            std::stringstream ss;
            bool init = true;
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                if ( !init ) ss << sep;
                auto val = it.next();
                ss << static_cast<const mutable_value_type&>(val);
                init = false;
            }
            return ss.str();
        }
        
        double increasing();
    };

    template<typename BaseT, typename ElT, typename RetainElT, typename ElIsLifted>
    class ConversionsImpl : public ConversionsBase<BaseT, ElT, RetainElT>
    {
    };
    
    
    
    template<typename BaseT, typename ElT, typename RetainElT>
    class ConversionsImpl<BaseT, ElT, RetainElT, std::true_type> : public ConversionsBase<BaseT, ElT, RetainElT>
    {
    public:
        template<typename FunctorT>
        FlatMapWrapper<BaseT, FunctorT, ElT, typename ElT::el_t, typename FunctorHelper<FunctorT, typename ElT::el_t>::out_t> flatMap( FunctorT fn )
        {
            return FlatMapWrapper<BaseT, FunctorT, ElT, typename ElT::el_t, typename FunctorHelper<FunctorT, typename ElT::el_t>::out_t>( std::move(this->get().getIterator()), fn );
        }
        
        FlatMapWrapper<BaseT, IdentityFunctor<typename ElT::el_t>, ElT, typename ElT::el_t, typename ElT::el_t> flatten()
        {
            return FlatMapWrapper<BaseT, IdentityFunctor<typename ElT::el_t>, ElT, typename ElT::el_t, typename ElT::el_t>(
                std::move(this->get().getIterator()),
                IdentityFunctor<typename ElT::el_t>() );
        }
    };
    
    template<typename BaseT, typename ElT, typename RetainElT>
    class Conversions : public ConversionsImpl<BaseT, ElT, RetainElT, typename std::is_base_of<Lifted, ElT>::type>
    {
    };
    
    
    template<typename Source, typename FunctorT, typename ElT>
    class FilterWrapper : public Conversions<FilterWrapper<Source, FunctorT, ElT>, ElT, ElT>
    {
    public:
        FilterWrapper( const typename Source::Iterator& source, FunctorT fn ) : m_source(source), m_fn(fn)
        {
            populateNext();
        }

        FilterWrapper( typename Source::Iterator && source, FunctorT fn ) : m_source(std::move(source)), m_fn(fn)
        {
            populateNext();
        }
        
        typedef FilterWrapper<Source, FunctorT, ElT> Iterator;
        Iterator& getIterator() { return *this; }

        ElT next()
        {
            ElT v = m_next.get();
            populateNext();
            return v;
        }
        
        bool hasNext() { return m_next; }
        
    private:
        void populateNext()
        {
            m_next.reset();
            while ( m_source.hasNext() )
            {
                ElT next = m_source.next();
                if ( m_fn( next ) )
                {
                    m_next = next;
                    break;
                }
            }
        }
    
    private:
        typename Source::Iterator   m_source;
        FunctorT                    m_fn;
        boost::optional<ElT>        m_next;
    };

    template<typename Source, typename FunctorT, typename InnerT, typename InputT, typename ElT>
    class FlatMapWrapper : public Conversions<FlatMapWrapper<Source, FunctorT, InnerT, InputT, ElT>, ElT, ElT>
    {
    public:
        FlatMapWrapper( const typename Source::Iterator& source, FunctorT fn ) : m_source(source), m_fn(fn), m_requirePopulateNext(true)
        {
        }
        
        typedef FlatMapWrapper<Source, FunctorT, InnerT, InputT, ElT> Iterator;
        Iterator& getIterator() { return *this; }

        ElT next()
        {
            if ( m_requirePopulateNext ) populateNext();
            ElT res = m_fn( m_next.get() );
            m_requirePopulateNext = true;
            return res;
        }
        
        bool hasNext()
        {
            if ( m_requirePopulateNext ) populateNext();
            return m_next;
        }
        
    private:
        void populateNext()
        {
            m_next.reset();
            while ( (!m_innerIt || !m_innerIt->hasNext()) && m_source.hasNext() )
            {
                // We need to take a copy of the inner object as well as its
                // iterator as otherwise the iterator may be a dangling pointer
                m_inner = m_source.next();
                m_innerIt = m_inner->getIterator();
            }
            
            if ( m_innerIt && m_innerIt->hasNext() )
            {
                m_next = m_innerIt->next();
            }
            
            m_requirePopulateNext = false;
        }
    
    private:
        typedef std::function<ElT(InputT)> FunctorHolder_t;
        
        typename Source::Iterator                   m_source;
        FunctorHolder_t                             m_fn;
        boost::optional<InnerT>                     m_inner;
        boost::optional<typename InnerT::Iterator>  m_innerIt;
        boost::optional<InputT>                     m_next;
        
        bool                                        m_requirePopulateNext;
    };

    template<typename Source, typename InputT>
    class CopyWrapper : public Conversions<CopyWrapper<Source, InputT>,
                                           typename std::remove_const<typename InputT::type>::type, typename std::remove_const<typename InputT::type>::type>
    {
    private:
        typedef CopyWrapper<Source, InputT> self_t;
    public:
        CopyWrapper( const typename Source::Iterator& source ) : m_source(source) {}
        CopyWrapper( typename Source::Iterator&& source ) : m_source(std::move(source)) {}
        typedef CopyWrapper<Source, InputT> Iterator;
        Iterator& getIterator() { return *this; }
        bool hasNext() { return m_source.hasNext(); }
        typename std::remove_const<typename InputT::type>::type next() { return m_source.next(); }
    private:
        typename Source::Iterator m_source;
    };

    template<typename Source, typename FunctorT, typename InputT, typename ElT>
    class MapWrapper : public Conversions<MapWrapper<Source, FunctorT, InputT, ElT>, ElT, ElT>
    {
    private:
        typedef MapWrapper<Source, FunctorT, InputT, ElT> self_t;
    public:
        MapWrapper( const typename Source::Iterator& source, FunctorT fn ) : m_source(source), m_fn(fn)
        {
        }

        MapWrapper( typename Source::Iterator&& source, FunctorT fn ) : m_source(std::move(source)), m_fn(fn)
        {
        }

        typedef MapWrapper<Source, FunctorT, InputT, ElT> Iterator;
        Iterator& getIterator() { return *this; }
        bool hasNext() { return m_source.hasNext(); }
        ElT next() { return m_fn( m_source.next() ); }
    
    private:
        typedef std::function<ElT(InputT)> FunctorHolder_t;
    
        typename Source::Iterator   m_source;
        FunctorHolder_t             m_fn;
    };
    
    template<typename Source1T, typename El1T, typename Source2T, typename El2T>
    class ZipWrapper : public Conversions<ZipWrapper<Source1T, El1T, Source2T, El2T>, std::pair<El1T, El2T>, std::pair<El1T, El2T>>
    {
    public:
        ZipWrapper( const typename Source1T::Iterator& source1, const typename Source2T::Iterator& source2 ) :
            m_source1(source1),
            m_source2(source2)
        {
        }
        
        ZipWrapper( typename Source1T::Iterator&& source1, typename Source2T::Iterator&& source2 ) :
            m_source1(std::move(source1)),
            m_source2(std::move(source2))
        {
        }

        typedef ZipWrapper<Source1T, El1T, Source2T, El2T> Iterator;
        Iterator& getIterator() { return *this; }
        bool hasNext() { return m_source1.hasNext() && m_source2.hasNext(); }
        std::pair<El1T, El2T> next() { return std::make_pair( std::move(m_source1.next()), std::move(m_source2.next()) ); }
        
    private:
        typename Source1T::Iterator m_source1;
        typename Source2T::Iterator m_source2;
    };
    
    template<typename Source, typename FunctorT, typename InputT, typename ElT, typename StateT>
    class MapWithStateWrapper : public Conversions<MapWithStateWrapper<Source, FunctorT, InputT, ElT, StateT>, ElT, ElT>
    {
    public:
        typedef MapWithStateWrapper<Source, FunctorT, InputT, ElT, StateT> self_t;
        
        MapWithStateWrapper( const typename Source::Iterator& source, FunctorT fn, StateT state ) : m_source(source), m_fn(fn), m_state(state)
        {
        }

        MapWithStateWrapper( typename Source::Iterator&& source, FunctorT fn, StateT state ) : m_source(std::move(source)), m_fn(fn), m_state(state)
        {
        }
        
        typedef MapWithStateWrapper<Source, FunctorT, InputT, ElT, StateT> Iterator;
        Iterator& getIterator() { return *this; }
        bool hasNext() { return m_source.hasNext(); }
        ElT next() { return m_fn( m_source.next(), m_state ); }
    
    private:
        typename Source::Iterator   m_source;
        FunctorT                    m_fn;
        StateT                      m_state;
    };
    
    template<typename Container, typename ValueT>
    class ContainerIteratorTransformer
    {
    public:
        typedef ValueT type;
        ValueT& operator()( ValueT& v ) const { return v; }
    };
    
    // Strip the const from the first element of a map/multimap pair before
    // exposing it to Escalator
    /*template<typename KeyT, typename ValueT, typename ElT>
    class ContainerIteratorTransformer<std::map<KeyT, ValueT>, ElT>
    {
    public:
        typedef std::pair<KeyT, ValueT> type;
        
        type operator()( ValueT v ) const
        {
            return type( v.first, v.second );
        }
    };*/
    
    /*template<typename IterT>
    class IteratorDereferenceType
    {
    public:
        typedef typename std::remove_reference<typename IterT::reference>::type type;
    };
    
 
    
    template<typename ContainerT, typename IterT>
    using TransformedIteratorType =
            ContainerIteratorTransformer<
                ContainerT,
                typename IteratorDereferenceType<IterT>::type>;*/
                

    template<typename IterT, template<typename> class FunctorT>
    class TransformedValue
    {
    public:
        typedef typename IterT::reference IteratorDereferenceType;
        typedef decltype( std::declval<FunctorT<IteratorDereferenceType>>()( std::declval<IteratorDereferenceType>() ) ) type;
    };
   
    template<typename ContainerT, typename IterT, template<typename> class FunctorT>
    class IteratorWrapper : public Conversions<IteratorWrapper<ContainerT, IterT, FunctorT>,
        typename TransformedValue<IterT, FunctorT>::type,
        typename TransformedValue<IterT, FunctorT>::type>
    {
    public:
        typedef IteratorWrapper<ContainerT, IterT, FunctorT> self_t;
        
        typedef FunctorT<typename IterT::reference> transformer_t;
        typedef typename transformer_t::type el_t;
        
        IteratorWrapper( IterT start, IterT end ) : m_iter(start), m_end(end)
        {
        }
        
        typedef IteratorWrapper<ContainerT, IterT, FunctorT> Iterator;
        Iterator& getIterator() { return *this; }
        
        bool hasNext()
        {
            return m_iter != m_end;
        }
        
        el_t next()
        {
            IterT curr = m_iter++;
            
            transformer_t transformer;

            return transformer( *curr );
        }

    private:
        IterT m_iter;
        IterT m_end;
    };
    
    template<typename SourceT, typename ElT>
    class SliceWrapper : public Conversions<SliceWrapper<SourceT, ElT>, ElT, ElT>
    {
    public:
        SliceWrapper( const typename SourceT::Iterator& source, size_t from, size_t to, SliceBehavior behavior )
            : m_source(source), m_from(from), m_to(to), m_count(0), m_behavior( behavior )
        {
            initiate(from, to, behavior);
        }

        SliceWrapper( typename SourceT::Iterator&& source, size_t from, size_t to, SliceBehavior behavior )
            : m_source(std::move(source)), m_from(from), m_to(to), m_count(0), m_behavior( behavior )
        {
            initiate(from, to, behavior);
        }

        typedef SliceWrapper<SourceT, ElT> Iterator;
        Iterator& getIterator() { return *this; }
        bool hasNext()
        {
            if(m_count==m_to) return false;

            if(m_behavior == ASSERT_WHEN_INSUFFICIENT)
            {
                if(!m_source.hasNext())
                {
                    throw SliceError( "Iterator unexpectedly exhausted" );
                }
                else
                {
                    return true;
                }
            }
            else
            {
                return m_source.hasNext();
            }
        }
        
        ElT next()
        {
            m_count++;
            ESCALATOR_ASSERT( m_source.hasNext(), "Iterator exhausted" );
            return m_source.next();
        }
        
    private:
        void initiate( size_t from, size_t to, SliceBehavior behavior )
        {
            while ( m_source.hasNext() && m_count < m_from )
            {
                m_source.next();
                m_count++;
            }

            if(m_behavior == ASSERT_WHEN_INSUFFICIENT && m_count < m_from)
            {
                throw SliceError( "Iterator unexpectedly exhausted" );
            }
        }
        
        typename SourceT::Iterator  m_source;
        size_t                      m_from;
        size_t                      m_to;
        size_t                      m_count;
        SliceBehavior               m_behavior;
    };
    
    template<typename Container, typename ElT>
    class ContainerWrapper : public Conversions<ContainerWrapper<Container, ElT>, ElT, ElT>
    {
    public:
        typedef typename Container::iterator iterator;
        
        ContainerWrapper( Container&& data ) : m_data(std::move(data))
        {
        }
        
        ContainerWrapper( const Container& data ) : m_data(data)
        {
        }
        
        operator const Container&() const { return m_data; }
        //operator Container&&() { return std::move(m_data); }

        // When copying, copy data that is left to be consumed only
        // Then new object delivers all data in its container
        ContainerWrapper( const ContainerWrapper& other ) : m_data(other.m_data)
        {
        }

        ContainerWrapper( ContainerWrapper&& other ) : m_data(std::move(other.m_data))
        {
        }

        ContainerWrapper& operator=( const ContainerWrapper& other )
        {
            m_data = other.m_data;
            return *this;
        }

        ContainerWrapper& operator=( ContainerWrapper&& other )
        {
            // Can't move into the already existing m_data
            // Move elements individually
            m_data.clear();
            m_data = std::move(other.m_data);
            return *this;
        }
        
        typedef IteratorWrapper<
            Container,
            typename Container::iterator,
            CopyFunctor> Iterator;
        
        Iterator getIterator() { return Iterator( m_data.begin(), m_data.end() ); }
        
        operator const Container&() { return m_data; }
        const Container& get() const { return m_data; }
        Container& get() { return m_data; }
        
    protected:
        Container       m_data;
    };

    class Counter : public Conversions<Counter, int, int>
    {
    public:
        Counter() : m_count(0) {}
        
        typedef Counter Iterator;
        Iterator& getIterator() { return *this; }
        bool hasNext() { return true; }
        
        int next()
        {
            return m_count++;
        }
        
    private:
        int m_count;
    };

    template<typename StreamT>
    class StreamWrapper : public Conversions<StreamWrapper<StreamT>, typename StreamT::value_type, typename StreamT::value_type>
    {
    public:
        StreamWrapper( StreamT& stream ) : m_stream(stream) {}
        
        typedef StreamWrapper<StreamT> Iterator;
        Iterator& getIterator() { return *this; }
        bool hasNext() { return !m_stream.eof(); }
        
        typename StreamT::value_type next()
        {
            return m_stream.pop();
        }
        
    private:
        StreamT&       m_stream;
    };

    /*template<typename ValueT>
    class OptionalWrapper : public Conversions<OptionalWrapper<ValueT>, typename ValueT::value_type, typename ValueT::value_type>
    {
    public:
        OptionalWrapper( ValueT&& op ) : m_op( std::move(op) ) {}
        typedef OptionalWrapper<ValueT> Iterator;
        Iterator& getIterator() { return *this; }
        bool hasNext() { return m_op; }
        typename ValueT::value_type next()
        {
            typename ValueT::value_type val = std::move(m_op.get());
            m_op.reset();
            return val;
        }
    private:
        ValueT m_op;
    };*/

    class IStreamWrapper : public Conversions<IStreamWrapper, std::string, std::string>
    {
    public:
        IStreamWrapper( std::istream& stream ) : m_stream(stream)
        {
            populateNext();
        }
        
        typedef IStreamWrapper Iterator;
        Iterator& getIterator() { return *this; }
        bool hasNext() { return m_hasNext; }
        std::string next()
        {
            std::string curr = m_currLine;
            populateNext();
            return curr;
        }
        
    private:
        void populateNext()
        {
            m_hasNext = std::getline( m_stream, m_currLine );
        }
        
    private:
        std::istream&       m_stream;
        bool                m_hasNext;
        std::string         m_currLine;
    };
    
    class StringWrapper : public ContainerWrapper<std::string, char>
    {
    public:
        StringWrapper( const std::string& data ) : ContainerWrapper(data)
        {
        }
        
        StringWrapper trim()
        {
            return StringWrapper( boost::algorithm::trim_copy(m_data) );
        }
        
        ContainerWrapper<std::vector<std::string>, std::string> split( const std::string& splitChars )
        {
            std::vector<std::string> splitVec;
            
            boost::algorithm::split( splitVec, m_data, boost::algorithm::is_any_of(splitChars) );
            
            return ContainerWrapper<std::vector<std::string>, std::string>( std::move(splitVec) );
        }
        
        const std::string& toString() { return m_data; }
    };
    
    template<typename BaseT, typename ElT, typename RetainElT>
    double ConversionsBase<BaseT, ElT, RetainElT>::increasing()
    {
        std::vector<double> res = zipWithIndex()
            .sortWith( []( const std::pair<ElT, size_t>& a, const std::pair<ElT, size_t>& b ) { return std::get<0>(a) < std::get<0>(b); } )
            .map( []( const std::pair<ElT, size_t>& v ) { return std::get<1>(v); } )
            .zipWithIndex()
            .map( []( const std::pair<size_t, size_t>& v ) { return std::abs( static_cast<double>( std::get<0>(v) ) - static_cast<double>( std::get<1>(v) ) ); } )
            .template lower<std::vector>();
            
        double scale = static_cast<double>(res.size() * res.size()) / 2.0;
        
        return 1.0 - (lift(res).sum() / scale);
    }
    
    inline Counter counter() { return Counter(); }
    inline IStreamWrapper lift( std::istream& data ) { return IStreamWrapper(data); }
    inline StringWrapper lift( const std::string& data ) { return StringWrapper(data); }
    
    template<typename StreamT>
    StreamWrapper<StreamT> slift( StreamT& stream ) { return StreamWrapper<StreamT>( stream ); }

    template<typename ContainerT>
    ContainerWrapper<ContainerT, typename ContainerT::value_type>
    lift_copy_container( ContainerT&& cont )
    {
        return ContainerWrapper<ContainerT, typename ContainerT::value_type>( std::forward<ContainerT>(cont) );
    }


    template<typename ContainerT>
    IteratorWrapper<
        ContainerT,
        typename ContainerT::const_iterator,
        CopyStripConstFunctor>
    lift( const ContainerT& cont )
    {
        return IteratorWrapper<
            ContainerT,
            typename ContainerT::const_iterator,
            CopyStripConstFunctor>( cont.begin(), cont.end() );
    }
    
    template<typename ContainerT>
    IteratorWrapper<
        ContainerT,
        typename ContainerT::iterator,
        IdentityFunctor>
    lift_ref( ContainerT& cont )
    {
        return IteratorWrapper<
            ContainerT,
            typename ContainerT::iterator,
            IdentityFunctor>( cont.begin(), cont.end() );
    }
    
    template<typename ContainerT>
    IteratorWrapper<
        ContainerT,
        typename ContainerT::const_iterator,
        IdentityFunctor>
    lift_cref( ContainerT& cont )
    {
        return IteratorWrapper<
            ContainerT,
            typename ContainerT::const_iterator,
            IdentityFunctor>( cont.begin(), cont.end() );
    }
    
    template<typename ContainerT>
    IteratorWrapper<
        ContainerT,
        typename ContainerT::iterator,
        WrapWithReferenceWrapper>
    lift_ref_wrapped( ContainerT& cont )
    {
        return IteratorWrapper<
            ContainerT,
            typename ContainerT::iterator,
            WrapWithReferenceWrapper>( cont.begin(), cont.end() );
    }
    
}}


#pragma once

#include <set>
#include <map>
#include <list>
#include <deque>
#include <tuple>
#include <vector>
#include <sstream>
#include <type_traits>

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
    class Optional
    {
    private:
        void assign(const T& val)
        {
            m_set = true;
            new (&m_val.data) T(val);
        }
        void assign(T&& val)
        {
            m_set = true;
            new (&m_val.data) T(std::move(val));
        }

    public:
        typedef T value_type;
        Optional() : m_set(false) {}
        Optional(const T& val)
        {
            assign(val);
        }
        Optional(T&& val) : m_set(true)
        {
            assign(std::move(val));
        }

        Optional(const Optional& other)
        {
            if(other.m_set) assign(other.get());
            else m_set = false;
        }
        Optional& operator=(const Optional& other)
        {
            reset();
            if(other.m_set) assign(other.get());
            else m_set = false;
        }
        Optional(Optional&& other)
        {
            if(other.m_set)
            {
                assign(std::move(other.get()));
                other.m_set = false;
            }
            else m_set = false;
        }
        Optional& operator=(Optional&& other)
        {
            reset();
            if(other.m_set)
            {
                assign(std::move(other.get()));
                other.m_set = false;
            }
            else m_set = false;
        }

        Optional& operator=(const T& val)
        {
            // Support assignment to this->get()
            if(getPtr() == &val) return *this;

            reset();
            assign(val);
            return *this;
        }
        Optional& operator=(T&& val)
        {
            // Support assignment to this->get()
            if(getPtr() == &val) return *this;

            reset();
            assign(std::move(val));
            return *this;
        }

        operator bool() { return m_set; }

        T& get()
        {
            if(!m_set) throw std::runtime_error( "Optional not set" );
            return *getPtr();
        }
        const T& get() const
        {
            if(!m_set) throw std::runtime_error( "Optional not set" );
            return *getPtr();
        }
        T* getPtr()
        {
            return reinterpret_cast<T*>(&m_val.data);
        }
        const T* getPtr() const
        {
            return reinterpret_cast<const T*>(&m_val.data);
        }

        T* operator->() { return &get(); }
        T& operator*() { return get(); }

        void reset()
        {
            if(m_set)
            {
                get().~T();
                m_set = false;
            }
        }

        ~Optional()
        {
            reset();
        }
    private:
// TODO: When we move to gcc 4.8, both will support alignas
#if defined(__clang__)
        struct alignas(std::alignment_of<T>::value) space_t
#else
        struct space_t
#endif
        {
            char data[sizeof(T)];
        }
#if !defined(__clang__)
        __attribute__ ((aligned))
#endif
        ;

        bool m_set;
        space_t m_val;
    };

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
    
    template<typename ElT, typename IterT>
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
        typedef typename std::remove_const<typename std::remove_reference<R>::type>::type type;
    };

    template<typename R>
    struct remove_all_reference<std::reference_wrapper<R>>
    {
        typedef typename std::remove_const<R>::type type;
    };

    template<typename ContainerT>
    using WrappedContainerConstVRef = std::reference_wrapper<
                                          const typename remove_all_reference<
                                              typename ContainerT::value_type
                                          >::type>;
    template<typename ContainerT>
    using WrappedContainerVRef = std::reference_wrapper<
                                     typename remove_all_reference<
                                         typename ContainerT::value_type
                                     >::type>;


    template<typename ContainerT>
    ContainerWrapper<ContainerT, typename ContainerT::value_type>
    clift( ContainerT&& cont );

    template<typename ContainerT>
    IteratorWrapper<WrappedContainerConstVRef<ContainerT>,
                    typename ContainerT::const_iterator> 
    lift( const ContainerT& cont );
    
    template<typename ContainerT>
    IteratorWrapper<WrappedContainerVRef<ContainerT>,
                    typename ContainerT::iterator>
    mlift( ContainerT& cont );
    
    template<typename Iterator>
    IteratorWrapper<WrappedContainerConstVRef<Iterator>,
                    Iterator>
    lift( Iterator begin, Iterator end );

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
            return std::move(t);
        }
        
        template<typename InputIterator>
        static ContainerWrapper<ContainerType, ElT> retain( InputIterator it )
        {
            ContainerType t;
            while ( it.hasNext() ) t.insert( t.end(), it.next() );
            return ContainerWrapper<ContainerType, ElT>( std::move(t) );
        }
    };
    
    
    template<typename BaseT, typename ElT>
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
        typedef typename remove_all_reference<ElT>::type value_type;
        
        template< class OutputIterator >
        void toContainer( OutputIterator v ) 
        {
            auto it = get().getIterator();
            while ( it.hasNext() ) *v++ = it.next();
        }
        
        template<typename ElementCheckType>
        BaseT& checkElementType()
        {
            static_assert( std::is_same<ElementCheckType, ElT>::value, "Element type does not match requirements" );
            return get();
        }
        
        template<template<typename, typename ...> class Container>
        typename ConversionHelper<ElT, Container>::ContainerType lower()
        {
            return std::move( ConversionHelper<ElT, Container>::lower( get().getIterator() ) );
        }
        
        template<template<typename, typename ...> class Container>
        ContainerWrapper<typename ConversionHelper<ElT, Container>::ContainerType, ElT> retain()
        {
            return std::move( ConversionHelper<ElT, Container>::retain( get().getIterator() ) );
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
                ElT val = std::move(it.next());
                if ( fn(val) ) res.first.push_back( std::move(val) );
                else res.second.push_back( std::move(val) );
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
                
                if ( inFirst ) res.first.push_back( std::move(val) );
                else res.second.push_back( std::move(val) );
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
        
        typedef MapWithStateWrapper<BaseT, std::function<std::pair<ElT, ElT>( ElT, Optional<ElT>& state )>, ElT, std::pair<ElT, ElT>, Optional<ElT>> sliding2_t;
        sliding2_t sliding2()
        {
            auto it = get().getIterator();
            Optional<ElT> startState;
            if ( it.hasNext() ) startState = it.next();
            return sliding2_t(
                std::move(it),
                []( ElT el, Optional<ElT>& state )
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
                //probably as it's defined on the value_type as a class method
                //and C++ won't try the default operator& on the reference wrapper
                //and dig around.
                const value_type& v_a = a;
                const value_type& v_b = b;
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
                std::pair<typename FunctorHelper<KeyFunctorT, ElT>::out_t, std::vector<typename FunctorHelper<ValueFunctorT, ElT>::out_t>>>
        {
            typedef typename FunctorHelper<KeyFunctorT, ElT>::out_t key_t;
            typedef typename FunctorHelper<ValueFunctorT, ElT>::out_t value_type;
            
            std::map<key_t, std::vector<value_type>> grouped;
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                auto v = it.next();
                auto key = keyFn(v);
                grouped[std::move(key)].push_back( valueFn(std::move(v)) );
            }
            
            return ContainerWrapper<
                std::map<typename FunctorHelper<KeyFunctorT, ElT>::out_t, std::vector<typename FunctorHelper<ValueFunctorT, ElT>::out_t>>,
                std::pair<typename FunctorHelper<KeyFunctorT, ElT>::out_t, std::vector<typename FunctorHelper<ValueFunctorT, ElT>::out_t>>>
                (grouped);
        }
        
        template<typename KeyFunctorT>
        auto countBy( KeyFunctorT keyFn ) ->
            ContainerWrapper<
                std::map<typename FunctorHelper<KeyFunctorT, ElT>::out_t, size_t>,
                std::pair<typename FunctorHelper<KeyFunctorT, ElT>::out_t, size_t>>
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
                std::pair<typename FunctorHelper<KeyFunctorT, ElT>::out_t, size_t>>( counts );
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
                auto res = seen.insert( std::move( it.next() ) );
                if ( res.second ) ordering.push_back( res.first );
            }
            
            std::vector<ElT> res;
            for ( auto& it : ordering )
            {
                res.push_back( std::move(*it) );
            }
            
            ContainerWrapper<std::vector<ElT>,  ElT> vw( std::move(res) );
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
                auto res = seen.insert( std::move( it.next() ) );
                if ( res.second ) ordering.push_back( res.first );
            }
            
            std::vector<ElT> res;
            for ( auto& it : ordering )
            {
                res.push_back( std::move(*it) );
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
                init = fn( std::move(init), std::move(it.next()) );
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
        
        value_type sum()
        {
            auto it = get().getIterator();
            ESCALATOR_ASSERT( it.hasNext(), "Mean over insufficient items" );
            value_type acc = it.next();
            while ( it.hasNext() )
            {
                acc += it.next();
            }
            return acc;
        }
        
        value_type mean()
        {
            auto it = get().getIterator();
            ESCALATOR_ASSERT( it.hasNext(), "Mean over insufficient items" );
            
            size_t count = 1;
            value_type acc = it.next();
            while ( it.hasNext() )
            {
                acc += it.next();
                count++;
            }
            
            return acc / static_cast<double>(count);
        }
        
        value_type median()
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
        
        //TODO: Should these use boost::optional to work round init requirements?
        std::pair<size_t, value_type> argMin()
        {
            bool init = true;
            value_type ext = value_type();
            
            size_t minIndex = 0;
            auto it = get().getIterator();
            for ( int i = 0; it.hasNext(); ++i )
            {
                if ( init ) ext = std::move(it.next());
                else
                {
                    auto n = std::move(it.next());
                    if ( n < ext )
                    {
                        ext = std::move(n);
                        minIndex = i;
                    }
                }
                init = false;
            }
            return std::make_pair( minIndex, std::move(ext) );
        }
        
        std::pair<size_t, value_type> argMax()
        {
            bool init = true;
            value_type ext = value_type();
            
            size_t maxIndex = 0;
            auto it = get().getIterator();
            for ( int i = 0; it.hasNext(); ++i )
            {
                if ( init ) ext = std::move(it.next());
                else
                {
                    auto n = std::move(it.next());
                    if ( n > ext )
                    {
                        ext = std::move(n);
                        maxIndex = i;
                    }
                }
                init = false;
            }
            return std::make_pair( maxIndex, std::move(ext) );
        }
        
        value_type min() { return std::template get<1>(argMin()); }
        value_type max() { return std::template get<1>(argMax()); }
        
        std::string mkString( const std::string& sep )
        {
            std::stringstream ss;
            bool init = true;
            auto it = get().getIterator();
            while ( it.hasNext() )
            {
                if ( !init ) ss << sep;
                auto val = std::move(it.next());
                ss << static_cast<const value_type&>(val);
                init = false;
            }
            return ss.str();
        }
        
        double increasing();
    };

    template<typename BaseT, typename ElT, typename ElIsLifted>
    class ConversionsImpl : public ConversionsBase<BaseT, ElT>
    {
    };
    
    
    
    template<typename BaseT, typename ElT>
    class ConversionsImpl<BaseT, ElT, std::true_type> : public ConversionsBase<BaseT, ElT>
    {
    public:
        template<typename FunctorT>
        FlatMapWrapper<BaseT, FunctorT, ElT, typename ElT::el_t, typename FunctorHelper<FunctorT, typename ElT::el_t>::out_t> flatMap( FunctorT fn )
        {
            return FlatMapWrapper<BaseT, FunctorT, ElT, typename ElT::el_t, typename FunctorHelper<FunctorT, typename ElT::el_t>::out_t>( std::move(this->get().getIterator()), fn );
        }

        template<typename T>
        class Identity
        {
        public:
            T operator()( T t ) const { return t; }
        };
        
        FlatMapWrapper<BaseT, Identity<typename ElT::el_t>, ElT, typename ElT::el_t, typename ElT::el_t> flatten()
        {
            return FlatMapWrapper<BaseT, Identity<typename ElT::el_t>, ElT, typename ElT::el_t, typename ElT::el_t>( std::move(this->get().getIterator()), Identity<typename ElT::el_t>() );
        }
    };
    
    template<typename BaseT, typename ElT>
    class Conversions : public ConversionsImpl<BaseT, ElT, typename std::is_base_of<Lifted, ElT>::type>
    {
    };
    
    
    template<typename Source, typename FunctorT, typename ElT>
    class FilterWrapper : public Conversions<FilterWrapper<Source, FunctorT, ElT>, ElT>
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
            ElT v = std::move(m_next.get());
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
                ElT next = std::move(m_source.next());
                if ( m_fn( next ) )
                {
                    m_next = std::move(next);
                    break;
                }
            }
        }
    
    private:
        typename Source::Iterator   m_source;
        FunctorT                    m_fn;
        Optional<ElT>               m_next;
    };

    template<typename Source, typename FunctorT, typename InnerT, typename InputT, typename ElT>
    class FlatMapWrapper : public Conversions<FlatMapWrapper<Source, FunctorT, InnerT, InputT, ElT>, ElT>
    {
    public:
        FlatMapWrapper( const typename Source::Iterator& source, FunctorT fn ) : m_source(source), m_fn(fn)
        {
            populateNext();
        }
        
        typedef FlatMapWrapper<Source, FunctorT, InnerT, InputT, ElT> Iterator;
        Iterator& getIterator() { return *this; }

        ElT next()
        {
            ElT res = m_fn( std::move(m_next.get()) ); //Moves through here OK
            populateNext();
            return res;
        }
        
        bool hasNext() { return m_next; }
        
    private:
        void populateNext()
        {
            m_next.reset();
            while ( (!m_innerIt || !m_innerIt->hasNext()) && m_source.hasNext() )
            {
                // We need to take ownership of the inner object as well as its
                // iterator as otherwise the iterator may be a dangling pointer
                m_inner = std::move(m_source.next());
                m_innerIt = m_inner->getIterator();
            }
            
            if ( m_innerIt && m_innerIt->hasNext() )
            {
                m_next = std::move(m_innerIt->next());
            }
        }
    
    private:
        typedef std::function<ElT(InputT)> FunctorHolder_t;
        typename Source::Iterator           m_source;
        FunctorHolder_t                     m_fn;
        Optional<InnerT>                    m_inner;
        Optional<typename InnerT::Iterator> m_innerIt;
        Optional<InputT>                    m_next;
    };

    template<typename Source, typename InputT>
    class CopyWrapper : public Conversions<CopyWrapper<Source, InputT>,
                                           typename std::remove_const<typename InputT::type>::type>
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
    class MapWrapper : public Conversions<MapWrapper<Source, FunctorT, InputT, ElT>, ElT>
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
        ElT next() { return m_fn( std::move(m_source.next()) ); }
    
    private:
        typedef std::function<ElT(InputT)> FunctorHolder_t;
    
        typename Source::Iterator   m_source;
        FunctorHolder_t             m_fn;
    };
    
    template<typename Source1T, typename El1T, typename Source2T, typename El2T>
    class ZipWrapper : public Conversions<ZipWrapper<Source1T, El1T, Source2T, El2T>, std::pair<El1T, El2T>>
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
    class MapWithStateWrapper : public Conversions<MapWithStateWrapper<Source, FunctorT, InputT, ElT, StateT>, ElT>
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
        ElT next() { return m_fn( std::move(m_source.next()), m_state ); }
    
    private:
        typename Source::Iterator   m_source;
        FunctorT                    m_fn;
        StateT                      m_state;
    };

    /**
     * std::reference_wrapper<T> won't auto-convert to a
     * std::reference_wrapper<const T>. Use this function to get
     * the reference inside whether or not there is a reference wrapper.
     */
    template<typename R>
    static R& StripReferenceWrapper(R& v) { return v; }
    template<typename R>
    static R& StripReferenceWrapper(std::reference_wrapper<R>& v) { return v.get(); }
    template<typename R>
    static const R& StripReferenceWrapper(const R& v) { return v; }
    template<typename R>
    static const R& StripReferenceWrapper(const std::reference_wrapper<R>& v) { return v.get(); }

    template<typename ElT, typename IterT>
    class IteratorWrapper : public Conversions<IteratorWrapper<ElT, IterT>, ElT>
    {
    public:
        typedef IteratorWrapper<ElT, IterT> self_t;
        typedef ElT el_t;
        
        IteratorWrapper( IterT start, IterT end ) : m_iter(start), m_end(end)
        {
        }
        
        typedef IteratorWrapper<ElT, IterT> Iterator;
        Iterator& getIterator() { return *this; }
        bool hasNext()
        {
            return m_iter != m_end;
        }
        
        ElT next()
        {
            IterT curr = m_iter++;
            return StripReferenceWrapper(*curr);
        }

    private:
        IterT m_iter;
        IterT m_end;
    };
    
    template<typename SourceT, typename ElT>
    class SliceWrapper : public Conversions<SliceWrapper<SourceT, ElT>, ElT>
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
    class ContainerWrapper : public Conversions<ContainerWrapper<Container, ElT>, WrappedContainerVRef<Container>>
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
        }

        ContainerWrapper& operator=( ContainerWrapper&& other )
        {
            // Can't move into the already existing m_data
            // Move elements individually
            m_data.clear();
            m_data = std::move(other.m_data);
            return *this;
        }
        
        /*class Iterator
        {
        public:
            Iterator( const iterator& begin, const iterator& end ) : m_iter(begin), m_end(end)
            {
            }
            
            bool hasNext()
            {
                return m_iter != m_end;
            }
            
            typename iterator::reference next()
            {
                ESCALATOR_ASSERT( m_iter != m_end, "Iterator exhausted" );
                iterator curr = m_iter++;
                return *curr;
            }
            
        private:
            iterator        m_iter;
            const iterator  m_end;
        };
        
        Iterator getIterator() { return Iterator(m_data.begin(), m_data.end()); }*/
        
        typedef IteratorWrapper<WrappedContainerVRef<Container>, typename Container::iterator> Iterator;
        
        Iterator getIterator() { return Iterator( m_data.begin(), m_data.end() ); }
        
        operator const Container&() { return m_data; }
        const Container& get() const { return m_data; }
        Container& get() { return m_data; }
        
    protected:
        Container       m_data;
    };

    class Counter : public Conversions<Counter, int>
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
    class StreamWrapper : public Conversions<StreamWrapper<StreamT>, typename StreamT::value_type>
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

    template<typename ValueT>
    class OptionalWrapper : public Conversions<OptionalWrapper<ValueT>, typename ValueT::value_type>
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
    };

    class IStreamWrapper : public Conversions<IStreamWrapper, std::string>
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
            std::string curr = std::move(m_currLine);
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
    
    template<typename BaseT, typename ElT>
    double ConversionsBase<BaseT, ElT>::increasing()
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
    clift( ContainerT&& cont )
    {
        return ContainerWrapper<ContainerT, typename ContainerT::value_type>( std::forward<ContainerT>(cont) );
    }

    template<typename ElT>
    inline OptionalWrapper<Optional<ElT>>
    clift( Optional<ElT>&& op )
    {
        return OptionalWrapper<Optional<ElT>>( std::forward<Optional<ElT>>(op) );
    }

    template<typename ContainerT>
    IteratorWrapper<WrappedContainerConstVRef<ContainerT>,
                    typename ContainerT::const_iterator> 
    lift( const ContainerT& cont )
    {
        return IteratorWrapper<WrappedContainerConstVRef<ContainerT>,
                               typename ContainerT::const_iterator>( cont.begin(), cont.end() );
    } 

    template<typename ContainerT>
    IteratorWrapper<WrappedContainerVRef<ContainerT>,
                    typename ContainerT::iterator>
    mlift( ContainerT& cont )
    {
        return IteratorWrapper<WrappedContainerVRef<ContainerT>,
                               typename ContainerT::iterator>( cont.begin(), cont.end() );
    }
    
    template<typename Iterator>
    IteratorWrapper<WrappedContainerConstVRef<Iterator>,
                    Iterator>
    lift( Iterator begin, Iterator end )
    {
        return IteratorWrapper<WrappedContainerConstVRef<Iterator>,
                               Iterator>( begin, end );
    }
}}


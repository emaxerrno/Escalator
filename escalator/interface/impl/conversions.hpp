#if !defined(ESCALATOR_INTERNAL)
#   error "This file is an escalator implementation file. Please do not include directly."
#else

namespace navetas { namespace escalator {
    // Non-templatised marker trait for type trait functionality
    class Lifted {};
    
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
                std::pair<typename FunctorHelper<KeyFunctorT, ElT>::out_t, std::vector<typename FunctorHelper<ValueFunctorT, ElT>::out_t>>,
                DeconstMapKeyFunctor>
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
                std::pair<typename FunctorHelper<KeyFunctorT, ElT>::out_t, std::vector<typename FunctorHelper<ValueFunctorT, ElT>::out_t>>,
                DeconstMapKeyFunctor>
                (grouped);
        }
        
        template<typename KeyFunctorT>
        auto countBy( KeyFunctorT keyFn ) ->
            ContainerWrapper<
                std::map<typename FunctorHelper<KeyFunctorT, ElT>::out_t, size_t>,
                std::pair<typename FunctorHelper<KeyFunctorT, ElT>::out_t, size_t>,
                DeconstMapKeyFunctor>
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
                std::pair<typename FunctorHelper<KeyFunctorT, ElT>::out_t, size_t>,
                DeconstMapKeyFunctor>( counts );
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

}}

#endif


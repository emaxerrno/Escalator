#if !defined(ESCALATOR_INTERNAL)
#   error "This file is an escalator implementation file. Please do not include directly."
#else


namespace navetas { namespace escalator {
 
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

    template<typename IterT, template<typename> class FunctorT>
    class TransformedValue
    {
    public:
        typedef typename IterT::reference IteratorDereferenceType;
        typedef decltype( std::declval<FunctorT<IteratorDereferenceType>>()( std::declval<IteratorDereferenceType>() ) ) type;
    };
   
    template<typename IterT, template<typename> class FunctorT>
    class IteratorWrapper : public Conversions<IteratorWrapper<IterT, FunctorT>,
        typename TransformedValue<IterT, FunctorT>::type,
        typename TransformedValue<IterT, FunctorT>::type>
    {
    public:
        typedef IteratorWrapper<IterT, FunctorT> self_t;
        
        typedef FunctorT<typename IterT::reference> transformer_t;
        typedef typename transformer_t::type el_t;
        
        IteratorWrapper( IterT start, IterT end ) : m_iter(start), m_end(end)
        {
        }
        
        typedef IteratorWrapper<IterT, FunctorT> Iterator;
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
    
    template<typename Container, typename ElT, template<typename> class IteratorTransformFunctorT>
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
            typename Container::iterator,
            IteratorTransformFunctorT> Iterator;
        
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
    
    template<typename HasNextFnT, typename GetNextFnT>
    class GenericWrapper : public Conversions<GenericWrapper<HasNextFnT, GetNextFnT>,
        decltype( std::declval<GetNextFnT>()() ),
        decltype( std::declval<GetNextFnT>()() )>
    {
    public:
        GenericWrapper( HasNextFnT hasNextFn, GetNextFnT getNextFn ) :
            m_hasNextFn(hasNextFn), m_getNextFn(getNextFn)
        {
        }
        
        typedef GenericWrapper<HasNextFnT, GetNextFnT> Iterator;
        Iterator& getIterator() { return *this; }
        
        bool hasNext() { return m_hasNextFn(); }
        decltype( std::declval<GetNextFnT>()() ) next() { return m_getNextFn(); }
        
    private:
        HasNextFnT      m_hasNextFn;
        GetNextFnT      m_getNextFn;
    };

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
    
    template<typename ContainerT>
    ContainerWrapper<ContainerT, typename ContainerT::value_type>
    lift_copy_container( ContainerT&& cont )
    {
        return ContainerWrapper<ContainerT, typename ContainerT::value_type>( std::forward<ContainerT>(cont) );
    }


    template<typename ContainerT>
    IteratorWrapper<
        typename ContainerT::const_iterator,
        CopyStripConstFunctor>
    lift( const ContainerT& cont )
    {
        return IteratorWrapper<
            typename ContainerT::const_iterator,
            CopyStripConstFunctor>( cont.begin(), cont.end() );
    }
    
    template<typename ContainerT>
    IteratorWrapper<
        typename ContainerT::iterator,
        IdentityFunctor>
    lift_ref( ContainerT& cont )
    {
        return IteratorWrapper<
            typename ContainerT::iterator,
            IdentityFunctor>( cont.begin(), cont.end() );
    }
    
    template<typename ContainerT>
    IteratorWrapper<
        typename ContainerT::const_iterator,
        IdentityFunctor>
    lift_cref( ContainerT& cont )
    {
        return IteratorWrapper<
            typename ContainerT::const_iterator,
            IdentityFunctor>( cont.begin(), cont.end() );
    }
    
    template<typename ContainerT>
    IteratorWrapper<
        typename ContainerT::iterator,
        WrapWithReferenceWrapper>
    lift_ref_wrapped( ContainerT& cont )
    {
        return IteratorWrapper<
            typename ContainerT::iterator,
            WrapWithReferenceWrapper>( cont.begin(), cont.end() );
    }
    
    template<typename IterT>
    IteratorWrapper<IterT, IdentityFunctor>
    lift( IterT begin, IterT end )
    {
        return IteratorWrapper<IterT, IdentityFunctor>( begin, end );
    }
    
    template<typename HasNextFnT, typename GetNextFnT>
    GenericWrapper<HasNextFnT, GetNextFnT>
    lift_generic( HasNextFnT hasNextFn, GetNextFnT getNextFn )
    {
        return GenericWrapper<HasNextFnT, GetNextFnT>( hasNextFn, getNextFn );
    }
    
}}

#endif

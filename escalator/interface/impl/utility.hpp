#if !defined(ESCALATOR_INTERNAL)
#   error "This file is an escalator implementation file. Please do not include directly."
#else

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
    
    
    template<typename Container>
    struct SanitiseMapPairWrapper
    {
        template<typename T>
        using TransformFunctor=CopyStripConstFunctor<T>;
    };
    
    template<typename T>
    class DeconstMapKeyFunctor
    {
    public:
        typedef std::pair<typename std::remove_const<typename T::first_type>::type, typename T::second_type> type;
        type operator()( const T& kvp ) { return type( kvp.first, kvp.second ); }    
    };
    
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
    
    class EmptyError : public std::range_error
    {
    public:
        EmptyError( const std::string& what_arg ) : std::range_error( what_arg ) {}
        EmptyError( const char* what_arg ) : std::range_error( what_arg ) {}
    };
    
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
    
}}

#endif

#if !defined(ESCALATOR_INTERNAL)
#   error "This file is an escalator implementation file. Please do not include directly."
#else


// All required forward declarations

namespace navetas { namespace escalator {

    
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
    
    template<typename IterT, template<typename> class FunctorT>
    class IteratorWrapper;
    
    template<typename Container, typename ElT, template<typename> class IteratorTransformFunctorT=IdentityFunctor>
    class ContainerWrapper;
   
    template<typename SourceT, typename ElT>
    class SliceWrapper;
    
    template<typename Source1T, typename El1T, typename Source2T, typename El2T>
    class ZipWrapper;

    template<typename ContainerT>
    IteratorWrapper<
        typename ContainerT::const_iterator,
        CopyStripConstFunctor>
    lift( const ContainerT& cont );
    
}}

#endif

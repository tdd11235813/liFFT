#pragma once

namespace foobar {
namespace policies {

    /**
     * Accessor that transforms all elements on access using the specified functor
     */
    template< class T_BaseAccessor, class T_Func >
    struct TransformAccessor
    {
    private:
        using BaseAccessor = T_BaseAccessor;
        using Func = T_Func;

        BaseAccessor acc_;
        Func func_;
    public:

        TransformAccessor(){}
        template< class T, class U >
        explicit TransformAccessor(T&& baseAccessor, U&& func = T_Func()): acc_(std::forward<T>(baseAccessor)), func_(std::forward<U>(func)){}

        template< class T_Index, class T_Data >
        auto
        operator()(const T_Index& idx, const T_Data& data) const
        -> decltype(func_(acc_(idx, data)))
        {
            return func_(acc_(idx, data));
        }
    };

    /**
     * Creates a transform accessor for the given function and accessor
     * This will transform all values returned by the base accessor with the function
     *
     * @param acc Base accessor
     * @param func Transform functor
     * @return TransformAccessor
     */
    template< class T_BaseAccessor, class T_Func >
    TransformAccessor< T_BaseAccessor, T_Func >
    makeTransformAccessor(T_BaseAccessor&& acc, T_Func&& func)
    {
        return TransformAccessor< T_BaseAccessor, T_Func >(std::forward<T_BaseAccessor>(acc), std::forward<T_Func>(func));
    }

    /**
     * Creates a transform accessor for the given function when applied to the given container using its default accessor
     * @param func Transform functor
     * @param      Container instance
     * @return TransformAccessor
     */
    template< class T, class T_Func >
    TransformAccessor< traits::DefaultAccessor_t<T>, T_Func >
    makeTransformAccessorFor(T_Func&& func, const T& = T())
    {
        return TransformAccessor< traits::DefaultAccessor_t<T>, T_Func >(traits::DefaultAccessor_t<T>(), std::forward<T_Func>(func));
    }

}  // namespace policies
}  // namespace foobar

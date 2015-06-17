#pragma once

#include "foobar/traits/NumDims.hpp"
#include "foobar/policies/GetExtents.hpp"

namespace foobar {
namespace policies {

    /**
     * Returns the total number of elements for a given data structure
     * regarding only the last n dimensions
     */
    template< typename T_Data >
    struct GetNumElements
    {
        static constexpr unsigned numDims = traits::NumDims<T_Data>::value;
        using Extents = GetExtents< T_Data >;

        unsigned operator()(const T_Data& data){
            Extents extents(data);
            static_assert(numDims>0, "No dimensions?");
            unsigned result = 1;
            for(unsigned i=0; i<numDims; ++i)
                result *= extents[i];
            return result;
        }
    };

    template< typename T_Data >
    unsigned getNumElements(const T_Data& data){
        return GetNumElements<T_Data>()(data);
    }

}  // namespace policies
}  // namespace foobar
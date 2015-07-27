#pragma once

#include <cassert>
#include <type_traits>
#include <cufft.h>
#include <string>
#include "foobar/types/TypePair.hpp"
#include "foobar/libraries/cuFFT/Plan.hpp"
#include "foobar/libraries/cuFFT/traits/FFTType.hpp"
#include "foobar/libraries/cuFFT/traits/Sign.hpp"
#include "foobar/libraries/cuFFT/traits/LibTypes.hpp"
#include "foobar/policies/SafePtrCast.hpp"

namespace foobar {
namespace libraries {
namespace cuFFT {

namespace policies {

    namespace detail {

        template<
                typename T_Precision,
                bool T_isComplexIn,
                bool T_isComplexOut,
                bool T_isFwd
        >
        struct ExecutePlan;

        template< bool T_isFwd >
        struct ExecutePlan< float, true, true, T_isFwd >: traits::LibTypes< float, true, true >
        {
          cufftResult operator()(cufftHandle plan, InType* in, OutType* out)
          {
              return cufftExecC2C(plan, in, out, traits::Sign< T_isFwd >::value);
          }
        };

        template< bool T_isFwd >
        struct ExecutePlan< float, false, true, T_isFwd >: traits::LibTypes< float, false, true >
        {
          cufftResult operator()(cufftHandle plan, InType* in, OutType* out)
          {
              static_assert(T_isFwd, "R2C is always a forward transform!");
              return cufftExecR2C(plan, in, out);
          }
        };

        template< bool T_isFwd >
        struct ExecutePlan< float, true, false, T_isFwd >: traits::LibTypes< float, true, false >
        {
          cufftResult operator()(cufftHandle plan, InType* in, OutType* out)
          {
              static_assert(T_isFwd, "C2R is always a inverse transform!");
              return cufftExecC2R(plan, in, out);
          }
        };

        template< bool T_isFwd >
        struct ExecutePlan< double, true, true, T_isFwd >: traits::LibTypes< double, true, true >
        {
          cufftResult operator()(cufftHandle plan, InType* in, OutType* out)
          {
              return cufftExecZ2Z(plan, in, out, traits::Sign< T_isFwd >::value);
          }
        };

        template< bool T_isFwd >
        struct ExecutePlan< double, false, true, T_isFwd >: traits::LibTypes< double, false, true >
        {
          cufftResult operator()(cufftHandle plan, InType* in, OutType* out)
          {
              static_assert(T_isFwd, "R2C is always a forward transform!");
              return cufftExecD2Z(plan, in, out);
          }
        };

        template< bool T_isFwd >
        struct ExecutePlan< double, true, false, T_isFwd >: traits::LibTypes< double, true, false >
        {
          cufftResult operator()(cufftHandle plan, InType* in, OutType* out)
          {
              static_assert(T_isFwd, "C2R is always a inverse transform!");
              return cufftExecZ2D(plan, in, out);
          }
        };

    }  // namespace detail

    template<
            typename T_Precision,
            class T_InOutTypes,
            bool T_isFwd,
            bool T_isInplace,
            unsigned T_numDims,
            bool T_isComplexIn,
            bool T_isComplexOut
    >
    struct ExecutePlan
    {
    private:
        using Precision = T_Precision;
        using Input = typename T_InOutTypes::First;
        using Output = typename T_InOutTypes::Second;
        static constexpr bool isFwd = T_isFwd;
        static constexpr unsigned numDims = T_numDims;
        static constexpr bool isInplace = T_isInplace;
        static constexpr bool isComplexIn = T_isComplexIn;
        static constexpr bool isComplexOut = T_isComplexOut;

        using LibTypes = traits::LibTypes< Precision, isComplexIn, isComplexOut >;
        using LibInType = typename LibTypes::InType;
        using LibOutType = typename LibTypes::OutType;

        using Executer = detail::ExecutePlan< Precision, isComplexIn, isComplexOut, isFwd >;

        template< class T_Extents, typename T_Ptr, class T_Copier >
        void
        copyIn(const T_Extents& inExtents, const T_Extents& outExtents, T_Ptr dest, T_Ptr src, bool inPlace, const T_Copier& copy)
        {
            size_t numElements = foobar::policies::getNumElementsFromExtents(inExtents);
            size_t w = inExtents[numDims-1] * sizeof(LibInType);
            size_t h = numElements * sizeof(LibInType) / w;
            size_t pitch = (inPlace && !isComplexIn) ? outExtents[numDims-1] * sizeof(LibOutType) : w;
            copy.H2D(dest, src, w, h, pitch, w);
        }

        template< class T_Extents, typename T_Ptr, class T_Copier >
        void
        copyOut(const T_Extents& inExtents, const T_Extents& outExtents, T_Ptr dest, T_Ptr src, bool inPlace, const T_Copier& copy)
        {
            size_t numElements = foobar::policies::getNumElementsFromExtents(outExtents);
            size_t w = outExtents[numDims-1] * sizeof(LibOutType);
            size_t h = numElements * sizeof(LibOutType) / w;
            size_t pitch = (inPlace && !isComplexOut) ? inExtents[numDims-1] * sizeof(LibInType) : w;
            copy.D2H(dest, src, w, h, pitch, w);
        }

    public:
        template< class T_Plan, class T_Copier >
        void
        operator()(T_Plan& plan, Input& input, Output& output, bool useInplaceForHost, const T_Copier& copy)
        {
            using foobar::policies::safe_ptr_cast;
            static_assert(!isInplace, "Cannot be used for inplace transforms!");

            auto pIn = safe_ptr_cast<LibInType*>(input.getDataPtr());
            if( plan.InDevicePtr )
            {
                copyIn(input.getExtents(), output.getExtents(), plan.InDevicePtr.get(), pIn, useInplaceForHost, copy);
                pIn = plan.InDevicePtr.get();
            }
            LibOutType* pOut;
            if(plan.OutDevicePtr)
                pOut = plan.OutDevicePtr.get();
            else if(Output::IsDeviceMemory::value)
                pOut = safe_ptr_cast<LibOutType*>(output.getDataPtr());
            else if(useInplaceForHost)
                pOut = reinterpret_cast<LibOutType*>(pIn);
            else
                throw std::runtime_error("No out device pointer");
            cufftResult result = Executer()(plan.handle, pIn, pOut);
            if(result != CUFFT_SUCCESS)
                throw std::runtime_error("Error executing plan: " + std::to_string(result));
            if( plan.OutDevicePtr || !Output::IsDeviceMemory::value)
            {
                copyOut(input.getExtents(), output.getExtents(), safe_ptr_cast<LibOutType*>(output.getDataPtr()), pOut, useInplaceForHost, copy);
            }
        }

        template< class T_Plan, class T_Copier >
        void
        operator()(T_Plan& plan, Input& inOut, const T_Copier& copy)
        {
            using foobar::policies::safe_ptr_cast;
            static_assert(isInplace, "Must be used for inplace transforms!");

            const auto& inExtents = inOut.getExtents();
            auto outExtents = inOut.getFullExtents();
            // For R2C we have to adjust the out extents
            if(isComplexOut && !isComplexIn)
                outExtents[numDims - 1] = outExtents[numDims - 1] / 2 + 1;

            auto pIn = safe_ptr_cast<LibInType*>(inOut.getDataPtr());
            if( plan.InDevicePtr )
            {
                copyIn(inExtents, outExtents, plan.InDevicePtr.get(), pIn, true, copy);
                pIn = plan.InDevicePtr.get();
            }
            LibOutType* pOut = reinterpret_cast<LibOutType*>(pIn);
            cufftResult result = Executer()(plan.handle, pIn, pOut);
            if(result != CUFFT_SUCCESS)
                throw std::runtime_error("Error executing plan: " + std::to_string(result));
            if( plan.InDevicePtr )
            {
                LibInType* pOutHost = safe_ptr_cast<LibInType*>(inOut.getDataPtr());
                copyOut(inExtents, outExtents, reinterpret_cast<LibOutType*>(pOutHost), pOut, true, copy);
            }
        }
    };

}  // namespace policies
}  // namespace cuFFT
}  // namespace libraries
}  // namespace foobar

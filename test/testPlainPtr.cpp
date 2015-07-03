#include "testPlainPtr.hpp"
#include "testUtils.hpp"
#include "foobar/types/Complex.hpp"
#include "foobar/types/Real.hpp"
#include "foobar/mem/PlainPtrWrapper.hpp"
#include "foobar/FFT.hpp"
#include "foobar/policies/Copy.hpp"
#include <iostream>

namespace foobarTest {

    void testPlainPtr()
    {
        static_assert(testNumDims == 2, "Currently only 2D is tested");
        using Real = foobar::types::Real<TestPrecision>;
        using Complex = foobar::types::Complex<TestPrecision>;
        std::unique_ptr<Real[]> input(new Real[testSize*testSize]);
        std::unique_ptr<Complex[]> output(new Complex[testSize*(testSize/2+1)]);
        for(unsigned i=0; i<testSize*testSize; ++i)
            input[i] = std::rand() / RAND_MAX;
        using FFT_TYPE = foobar::FFT_2D_R2C<TestPrecision>;
        auto inWrapped = FFT_TYPE::wrapFFT_Input(foobar::mem::wrapPtr<false>(input.get(), testSize, testSize));
        auto outWrapped = FFT_TYPE::wrapFFT_Output(foobar::mem::wrapPtr<true>(output.get(), testSize, testSize/2+1));
        auto fft = foobar::makeFFT<TestLibrary>(inWrapped, outWrapped);
        fft(inWrapped, outWrapped);
        foobar::policies::copy(inWrapped, baseR2CInput);
        execBaseR2C();
        auto res = compare(baseR2COutput, outWrapped);
        if(!res.first)
            std::cerr << "Error for R2C with PlainPtrWrapper: " << res.second << std::endl;
        else
            std::cout << "R2C with PlainPtrWrapper passed" << std::endl;
    }

}  // namespace foobarTest

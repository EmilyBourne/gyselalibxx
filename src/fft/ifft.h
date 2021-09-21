#pragma once

#include <complex>

#include <ddc/BlockSpan>
#include <ddc/MDomain>
#include <ddc/NonUniformMesh>

#include <geometry.h>

template <class... Tags>
class IInverseFourierTransform
{
public:
    IInverseFourierTransform() = default;

    IInverseFourierTransform(const IInverseFourierTransform& x) = default;

    IInverseFourierTransform(IInverseFourierTransform&& x) = default;

    virtual ~IInverseFourierTransform() = default;

    IInverseFourierTransform& operator=(const IInverseFourierTransform& x) = default;

    IInverseFourierTransform& operator=(IInverseFourierTransform&& x) = default;

    virtual void operator()(
            BlockSpan<
                    std::complex<double>,
                    ProductMDomain<UniformMesh<Tags>...>,
                    std::experimental::layout_right> const& out_values,
            BlockSpan<
                    std::complex<double>,
                    ProductMDomain<NonUniformMesh<Fourier<Tags>>...>,
                    std::experimental::layout_right> const& in_values) const noexcept = 0;
};

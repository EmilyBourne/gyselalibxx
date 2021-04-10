#include <cassert>
#include <cmath>

#include "advection1d.h"
#include "spline_1d.h"

using namespace std;
using namespace std::experimental;

Advection1D::Advection1D(const BSplines& bspl, const Spline_interpolator_1D& spl_interp)
    : m_bspl(bspl)
    , m_spline_interpolator(spl_interp)
    , m_bc_left(NullBoundaryValue::value)
    , m_bc_right(NullBoundaryValue::value)
{
    assert(bspl.periodic);
}

Advection1D::Advection1D(
        const BSplines& bspl,
        const Spline_interpolator_1D& spl_interp,
        const BoundaryValue& bc_left,
        const BoundaryValue& bc_right)
    : m_bspl(bspl)
    , m_spline_interpolator(spl_interp)
    , m_bc_left(bc_left)
    , m_bc_right(bc_right)
{
}

void Advection1D::operator()(DBlockView2D& fdistribu, double mass_ratio, double dt) const
{
    View1D<double> x = m_spline_interpolator.get_interp_points();
    unique_ptr<double[]> new_points_ptr = make_unique<double[]>(x.extent(0));
    View1D<double> new_points(new_points_ptr.get(), x.extent(0));

    unique_ptr<double[]> current_values_ptr = make_unique<double[]>(x.extent(0));
    View1D<double> current_values(current_values_ptr.get(), x.extent(0));

    Spline_1D spline(m_bspl, m_bc_left, m_bc_right);
    for (size_t vii = 0; vii < fdistribu.extent(1); ++vii) {
        const double dx = mass_ratio * dt * fdistribu.domain(1).mesh()({vii})[0];

        // copy the line in a contiguous array
        const auto current_values_nc = subspan(fdistribu.raw_view(), all, vii);
        for (size_t ii = 0; ii < current_values_nc.extent(0); ++ii) {
            current_values[ii] = current_values_nc[ii];
        }

        m_spline_interpolator.compute_interpolant(spline, current_values);
        // splitting in x direction
        for (size_t xii = 0; xii < current_values.extent(0); ++xii) {
            new_points[xii] = x[xii] - dx;
        }
        spline.eval_array(new_points, current_values);
    }
}

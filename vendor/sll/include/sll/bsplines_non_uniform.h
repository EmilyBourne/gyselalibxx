#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <vector>

#include <ddc/MDomain>
#include <ddc/NonUniformMesh>
#include <ddc/ProductMDomain>

#include "sll/bsplines.h"

/// NonUniformMesh specialization of BSplines
template <class Tag, std::size_t D>
class BSplines<NonUniformMesh<Tag>, D>
{
    static_assert(D > 0, "Parameter `D` must be positive");

private:
    using mesh_type = NonUniformMesh<Tag>;

    using domain_type = ProductMDomain<mesh_type>;

public:
    using tag_type = Tag;

    using rcoord_type = RCoord<Tag>;

    using rlength_type = RLength<Tag>;

    using mcoord_type = MCoord<BSplines>;

public:
    static constexpr std::size_t degree() noexcept
    {
        return D;
    }

    static constexpr bool is_periodic() noexcept
    {
        return Tag::PERIODIC;
    }

    static constexpr bool is_radial() noexcept
    {
        return false;
    }

    static constexpr bool is_uniform() noexcept
    {
        return false;
    }

private:
    mesh_type m_mesh;

    MDomain<mesh_type> m_domain;

    std::vector<double> m_knots;

public:
    BSplines() = default;

    explicit BSplines(domain_type const& domain);

    BSplines(BSplines const& x) = default;

    BSplines(BSplines&& x) = default;

    ~BSplines() = default;

    BSplines& operator=(BSplines const& x) = default;

    BSplines& operator=(BSplines&& x) = default;

    void eval_basis(
            double x,
            std::experimental::mdspan<double, std::experimental::dextents<1>>& values,
            int& jmin) const;

    void eval_deriv(
            double x,
            std::experimental::mdspan<double, std::experimental::dextents<1>>& derivs,
            int& jmin) const;

    void eval_basis_and_n_derivs(
            double x,
            int n,
            std::experimental::mdspan<double, std::experimental::dextents<2>>& derivs,
            int& jmin) const;

    void integrals(
            std::experimental::mdspan<double, std::experimental::dextents<1>>& int_vals) const;

    double get_knot(int break_idx) const noexcept
    {
        // TODO: assert break_idx >= 1 - degree
        // TODO: assert break_idx <= npoints + degree
        return m_knots[break_idx + degree()];
    }

    double rmin() const noexcept
    {
        return m_domain.rmin();
    }

    double rmax() const noexcept
    {
        return m_domain.rmax();
    }

    double length() const noexcept
    {
        return rmax() - rmin();
    }

    std::size_t size() const noexcept
    {
        return degree() + ncells();
    }

    std::size_t npoints() const noexcept
    {
        return m_domain.size();
    }

    std::size_t nbasis() const noexcept
    {
        return ncells() + !is_periodic() * degree();
    }

    std::size_t ncells() const noexcept
    {
        return m_domain.size() - 1;
    }

private:
    int find_cell(double x) const;

    double& get_knot(int break_idx)
    {
        // TODO: assert break_idx >= 1 - degree
        // TODO: assert break_idx <= npoints + degree
        return m_knots[break_idx + degree()];
    }
};

template <class Tag, std::size_t D>
BSplines<NonUniformMesh<Tag>, D>::BSplines(domain_type const& domain)
    : m_mesh(get<mesh_type>(domain).mesh())
    , m_domain(m_mesh, domain.front(), domain.back())
    , m_knots(m_domain.size() + 2 * degree())
{
    assert(ncells() > 0);
    assert(rmin() < rmax());
    assert(npoints() == ncells() + 1);

    for (int i(0); i < npoints(); ++i) {
        get_knot(i) = m_mesh.to_real(m_domain[i]);
    }

    // Fill out the extra nodes
    if constexpr (is_periodic()) {
        double period = m_mesh.to_real(m_domain[npoints() - 1]) - m_mesh.to_real(m_domain[0]);
        for (int i(1); i < degree() + 1; ++i) {
            get_knot(-i) = m_mesh.to_real(m_domain[npoints() - 1 - i]) - period;
            get_knot(npoints() - 1 + i) = m_mesh.to_real(m_domain[i]) + period;
        }
    } else // open
    {
        for (int i(1); i < degree() + 1; ++i) {
            get_knot(-i) = m_mesh.to_real(m_domain[0]);
            get_knot(npoints() - 1 + i) = m_mesh.to_real(m_domain[npoints() - 1]);
        }
    }
}

template <class Tag, std::size_t D>
void BSplines<NonUniformMesh<Tag>, D>::eval_basis(
        double x,
        std::experimental::mdspan<double, std::experimental::dextents<1>>& values,
        int& jmin) const
{
    std::array<double, degree()> left;
    std::array<double, degree()> right;

    assert(x >= rmin());
    assert(x <= rmax());
    assert(values.extent(0) == degree() + 1);

    // 1. Compute cell index 'icell'
    int icell = find_cell(x);

    assert(icell >= 0);
    assert(icell <= ncells() - 1);
    assert(get_knot(icell) <= x);
    assert(get_knot(icell + 1) >= x);

    // 2. Compute index range of B-splines with support over cell 'icell'
    jmin = icell;

    // 3. Compute values of aforementioned B-splines
    double temp;
    values(0) = 1.0;
    for (int j(0); j < degree(); ++j) {
        left[j] = x - get_knot(icell - j);
        right[j] = get_knot(icell + j + 1) - x;
        double saved(0.0);
        for (int r(0); r < j + 1; ++r) {
            temp = values(r) / (right[r] + left[j - r]);
            values(r) = saved + right[r] * temp;
            saved = left[j - r] * temp;
        }
        values(j + 1) = saved;
    }
}

template <class Tag, std::size_t D>
void BSplines<NonUniformMesh<Tag>, D>::eval_deriv(
        double x,
        std::experimental::mdspan<double, std::experimental::dextents<1>>& derivs,
        int& jmin) const
{
    std::array<double, degree()> left;
    std::array<double, degree()> right;

    assert(x >= rmin());
    assert(x <= rmax());
    assert(derivs.extent(0) == degree() + 1);

    // 1. Compute cell index 'icell'
    int icell = find_cell(x);

    assert(icell >= 0);
    assert(icell <= ncells() - 1);
    assert(get_knot(icell) <= x);
    assert(get_knot(icell + 1) >= x);

    // 2. Compute index range of B-splines with support over cell 'icell'
    jmin = icell;

    // 3. Compute values of aforementioned B-splines

    /*
     * Compute nonzero basis functions and knot differences
     * for splines up to degree degree-1 which are needed to compute derivative
     * First part of Algorithm  A3.2 of NURBS book
     */
    double saved, temp;
    derivs(0) = 1.0;
    for (int j(0); j < degree() - 1; ++j) {
        left[j] = x - get_knot(icell - j);
        right[j] = get_knot(icell + j + 1) - x;
        saved = 0.0;
        for (int r(0); r < j + 1; ++r) {
            temp = derivs(r) / (right[r] + left[j - r]);
            derivs(r) = saved + right[r] * temp;
            saved = left[j - r] * temp;
        }
        derivs(j + 1) = saved;
    }

    /*
     * Compute derivatives at x using values stored in bsdx and formula
     * for spline derivative based on difference of splines of degree degree-1
     */
    saved = degree() * derivs(0) / (get_knot(icell + 1) - get_knot(icell + 1 - degree()));
    derivs(0) = -saved;
    for (int j(1); j < degree(); ++j) {
        temp = saved;
        saved = degree() * derivs(j)
                / (get_knot(icell + j + 1) - get_knot(icell + j + 1 - degree()));
        derivs(j) = temp - saved;
    }
    derivs(degree()) = saved;
}

template <class Tag, std::size_t D>
void BSplines<NonUniformMesh<Tag>, D>::eval_basis_and_n_derivs(
        double x,
        int n,
        std::experimental::mdspan<double, std::experimental::dextents<2>>& derivs,
        int& jmin) const
{
    std::array<double, degree()> left;
    std::array<double, degree()> right;

    std::array<double, 2 * (degree() + 1)> a_ptr;
    std::experimental::mdspan<double, std::experimental::extents<degree() + 1, 2>> a(a_ptr.data());

    std::array<double, (degree() + 1) * (degree() + 1)> ndu_ptr;
    std::experimental::mdspan<double, std::experimental::dextents<2>>
            ndu(ndu_ptr.data(), degree() + 1, degree() + 1);

    assert(x >= rmin());
    assert(x <= rmax());
    assert(n >= 0);
    assert(n <= degree());
    assert(derivs.extent(0) == 1 + degree());
    assert(derivs.extent(1) == 1 + n);

    // 1. Compute cell index 'icell' and x_offset
    int icell(find_cell(x));

    assert(icell >= 0);
    assert(icell <= ncells() - 1);
    assert(get_knot(icell) <= x);
    assert(get_knot(icell + 1) >= x);

    // 2. Compute index range of B-splines with support over cell 'icell'
    jmin = icell;

    // 3. Compute nonzero basis functions and knot differences for splines
    //    up to degree (degree-1) which are needed to compute derivative
    //    Algorithm  A2.3 of NURBS book
    //
    //    21.08.2017: save inverse of knot differences to avoid unnecessary
    //    divisions
    //                [Yaman Güçlü, Edoardo Zoni]

    double saved, temp;
    ndu(0, 0) = 1.0;
    for (int j(0); j < degree(); ++j) {
        left[j] = x - get_knot(icell - j);
        right[j] = get_knot(icell + j + 1) - x;
        saved = 0.0;
        for (int r(0); r < j + 1; ++r) {
            // compute inverse of knot differences and save them into lower
            // triangular part of ndu
            ndu(r, j + 1) = 1.0 / (right[r] + left[j - r]);
            // compute basis functions and save them into upper triangular part
            // of ndu
            temp = ndu(j, r) * ndu(r, j + 1);
            ndu(j + 1, r) = saved + right[r] * temp;
            saved = left[j - r] * temp;
        }
        ndu(j + 1, j + 1) = saved;
    }
    // Save 0-th derivative
    for (int j(0); j < degree() + 1; ++j) {
        derivs(j, 0) = ndu(degree(), j);
    }

    for (int r(0); r < degree() + 1; ++r) {
        int s1 = 0;
        int s2 = 1;
        a(0, 0) = 1.0;
        for (int k(1); k < n + 1; ++k) {
            double d(0.0);
            int rk = r - k;
            int pk = degree() - k;
            if (r >= k) {
                a(0, s2) = a(0, s1) * ndu(rk, pk + 1);
                d = a(0, s2) * ndu(pk, rk);
            }
            int j1 = rk > -1 ? 1 : (-rk);
            int j2 = (r - 1) <= pk ? k : (degree() - r + 1);
            for (int j(j1); j < j2; ++j) {
                a(j, s2) = (a(j, s1) - a(j - 1, s1)) * ndu(rk + j, pk + 1);
                d += a(j, s2) * ndu(pk, rk + j);
            }
            if (r <= pk) {
                a(k, s2) = -a(k - 1, s1) * ndu(r, pk + 1);
                d += a(k, s2) * ndu(pk, r);
            }
            derivs(r, k) = d;
            int tmp(s1);
            s1 = s2;
            s2 = tmp;
        }
    }

    int r(degree());
    for (int k(1); k < n + 1; ++k) {
        for (int i(0); i < derivs.extent(0); i++) {
            derivs(i, k) *= r;
        }
        r *= (degree() - k);
    }
}

template <class Tag, std::size_t D>
int BSplines<NonUniformMesh<Tag>, D>::find_cell(double x) const
{
    if (x > rmax())
        return -1;
    if (x < rmin())
        return -1;

    if (x == rmin())
        return 0;
    if (x == rmax())
        return ncells() - 1;

    // Binary search
    int low(0), high(ncells());
    int icell((low + high) / 2);
    while (x < get_knot(icell) or x >= get_knot(icell + 1)) {
        if (x < get_knot(icell)) {
            high = icell;
        } else {
            low = icell;
        }
        icell = (low + high) / 2;
    }
    return icell;
}

template <class Tag, std::size_t D>
void BSplines<NonUniformMesh<Tag>, D>::integrals(
        std::experimental::mdspan<double, std::experimental::dextents<1>>& int_vals) const
{
    assert(int_vals.extent(0) == nbasis() + degree() * is_periodic());

    double inv_deg(1.0 / (degree() + 1));

    for (int i(0); i < nbasis(); ++i) {
        int_vals(i) = (get_knot(i + 1) - get_knot(i - degree())) * inv_deg;
    }

    if constexpr (is_periodic()) {
        for (int i(0); i < degree(); ++i) {
            int_vals(nbasis() + i) = 0;
        }
    }
}

template <class Tag, std::size_t D>
using NonUniformBSplines = BSplines<NonUniformMesh<Tag>, D>;

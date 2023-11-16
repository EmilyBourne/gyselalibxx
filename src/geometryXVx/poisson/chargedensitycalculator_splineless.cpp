// SPDX-License-Identifier: MIT

#include <ddc/ddc.hpp>

#include "chargedensitycalculator_splineless.hpp"
#include "quadrature.hpp"
#include "simpson_quadrature.hpp"



ChargeDensityCalculatorSplineless::ChargeDensityCalculatorSplineless(Quadrature<IDimVx> const& quad)
    : m_quad(quad)
{
}

void ChargeDensityCalculatorSplineless::operator()(DSpanX const rho, DViewSpXVx const allfdistribu)
        const
{
    IndexSp const last_kin_species = allfdistribu.domain<IDimSp>().back();
    IndexSp const last_species = ddc::discrete_space<IDimSp>().charges().domain().back();
    double chargedens_adiabspecies = 0.;
    if (last_kin_species != last_species) {
        chargedens_adiabspecies = double(charge(last_species));
    }

    ddc::for_each(rho.domain(), [&](IndexX const ix) {
        rho(ix) = chargedens_adiabspecies;
        ddc::for_each(ddc::get_domain<IDimSp>(allfdistribu), [&](IndexSp const isp) {
            rho(ix) += charge(isp) * m_quad(allfdistribu[isp][ix]);
        });
    });
}
// SPDX-License-Identifier: MIT

#pragma once

#include <ddc/ddc.hpp>

#include <ddc_helper.hpp>

/// @brief Species discrete dimension to access constant attributes related to species.
class SpeciesInformation
{
public:
    /// alias of the discrete dimension
    using discrete_dimension_type = SpeciesInformation;

public:
    /// @brief Impl object storing attributes in `MemorySpace`.
    template <class DDim, class MemorySpace>
    class Impl
    {
        template <class ODDim, class OMemorySpace>
        friend class Impl;

        /// alias of the discrete element of this discrete dimension
        using discrete_element_type = ddc::DiscreteElement<DDim>;

        /// alias of the discrete domain of this discrete dimension
        using discrete_domain_type = ddc::DiscreteDomain<DDim>;

        /// alias of the discrete vector of this discrete dimension
        using discrete_vector_type = ddc::DiscreteVector<DDim>;

    private:
        // charge of the particles (kinetic + adiabatic)
        ddc::Chunk<int, discrete_domain_type, ddc::KokkosAllocator<int, MemorySpace>> m_charge;

        // mass of the particles of all kinetic species
        ddc::Chunk<double, discrete_domain_type, ddc::KokkosAllocator<double, MemorySpace>> m_mass;

        // workaround to access charges on the device
        ddc::ChunkView<int, discrete_domain_type, std::experimental::layout_right, MemorySpace>
                m_charge_view;

        // workaround to access masses on the device
        ddc::ChunkView<double, discrete_domain_type, std::experimental::layout_right, MemorySpace>
                m_mass_view;

        discrete_element_type m_ielec;

    public:
        /// alias of the discrete dimension
        using discrete_dimension_type = SpeciesInformation;

        /**
         * @brief Conversion constructor between different memory spaces.
         * @param[in] impl object from `OMemorySpace` that will be used to initialize this object on `MemorySpace`
         */
        template <class OMemorySpace>
        explicit Impl(Impl<DDim, OMemorySpace> const& impl)
            : m_charge(impl.m_charge.domain())
            , m_mass(impl.m_mass.domain())
            , m_ielec(impl.m_ielec)
        {
            m_charge_view = m_charge.span_cview();
            m_mass_view = m_mass.span_cview();
            ddc::parallel_deepcopy(m_charge, impl.m_charge);
            ddc::parallel_deepcopy(m_mass, impl.m_mass);
        }

        /**
         * @brief Main constructor taking all attributes
         * @param[in] charge array storing both kinetic and adiabatic charges
         * @param[in] mass array storing both kinetic and adiabatic masses
         */
        Impl(ddc::Chunk<int, discrete_domain_type, ddc::KokkosAllocator<int, MemorySpace>> charge,
             ddc::Chunk<double, discrete_domain_type, ddc::KokkosAllocator<double, MemorySpace>>
                     mass)
            : m_charge(std::move(charge))
            , m_mass(std::move(mass))
        {
            m_charge_view = m_charge.span_cview();
            m_mass_view = m_mass.span_cview();
            assert(charge.size() >= 2);
            bool electron_found = false;
            for (discrete_element_type const isp : m_charge.domain()) {
                if (m_charge(isp) == -1) {
                    electron_found = true;
                    m_ielec = isp;
                }
            }
            if (!electron_found) {
                throw std::runtime_error("electron not found");
            }
        }

        /// @return the discrete element representing the electron species
        KOKKOS_FUNCTION discrete_element_type ielec() const
        {
            return m_ielec;
        }

        /**
         * @param[in] isp a discrete element of either a kinetic or adiabatic species
         * @return the charge associated to the discrete element
         */
        KOKKOS_FUNCTION int charge(discrete_element_type const isp) const
        {
            return m_charge_view(isp);
        }

        /**
         * @param[in] isp a discrete element of either a kinetic or adiabatic species
         * @return the mass associated to the discrete element
         */
        KOKKOS_FUNCTION double mass(discrete_element_type const isp) const
        {
            return m_mass_view(isp);
        }

        /// @return kinetic and adiabatic charges array
        auto charges() const
        {
            return m_charge.span_view();
        }

        /// @return kinetic and adiabatic masses array
        auto masses() const
        {
            return m_mass.span_view();
        }
    };
};

template <class DDim>
inline constexpr bool is_species_information_v
        = std::is_same_v<typename DDim::discrete_dimension_type, SpeciesInformation>;

// Species dimension
struct IDimSp : SpeciesInformation
{
};

/// @return the discrete element representing the electron species
KOKKOS_INLINE_FUNCTION ddc::DiscreteElement<IDimSp> ielec()
{
    return ddc::discrete_space<IDimSp>().ielec();
}

/**
 * @param[in] isp a discrete element of either a kinetic or adiabatic species
 * @return the charge associated to the discrete element
 */
KOKKOS_INLINE_FUNCTION int charge(ddc::DiscreteElement<IDimSp> const isp)
{
    return ddc::discrete_space<IDimSp>().charge(isp);
}

/**
 * @param[in] isp a discrete element of either a kinetic or adiabatic species
 * @return the mass associated to the discrete element
 */
KOKKOS_INLINE_FUNCTION double mass(ddc::DiscreteElement<IDimSp> const isp)
{
    return ddc::discrete_space<IDimSp>().mass(isp);
}

using IndexSp = ddc::DiscreteElement<IDimSp>;
using IDomainSp = ddc::DiscreteDomain<IDimSp>; // --> Should be DomSp
using IVectSp = ddc::DiscreteVector<IDimSp>; // --> Should be VectSp or VecSp

template <class ElementType>
using FieldSp = ddc::Chunk<ElementType, IDomainSp>;
using DFieldSp = FieldSp<double>;
using IFieldSp = device_t<ddc::Chunk<int, IDomainSp>>;

template <class ElementType>
using ViewSp = ddc::ChunkView<ElementType const, IDomainSp>;
using DViewSp = ViewSp<double>;

template <class ElementType>
using SpanSp = device_t<ddc::ChunkSpan<ElementType, IDomainSp>>;
using DSpanSp = SpanSp<double>;

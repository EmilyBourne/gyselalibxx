// SPDX-License-Identifier: MIT
#pragma once
#include <ddc/ddc.hpp>

#include "ddc_aliases.hpp"
#include "ddc_helper.hpp"
#include "vector_field.hpp"

template <class IdxRangeLaplacian, class IdxRangeFull, class MemorySpace, class LayoutSpace>
class IPoissonSolver;

/**
 * An abstract class from which a Poisson solver can inherit.
 * Classes inheriting from this must implement a way to solve the following equation:
 * @f$ -\Delta \phi = \rho @f$
 *
 * @tparam IdxRangeLaplacian The index range on which the equation is defined.
 * @tparam IdxRangeFull The index range on which the operator() acts. This is equal to the
 *                      IdxRangeLaplacian plus any batched dimensions.
 * @tparam LayoutSpace The layout space of the Fields passed to operator().
 * @tparam MemorySpace The space (CPU/GPU) where the Fields passed to operator()
 *                      are saved.
 */
template <class... ODims, class IdxRangeFull, class MemorySpace, class LayoutSpace>
class IPoissonSolver<IdxRange<ODims...>, IdxRangeFull, MemorySpace, LayoutSpace>
{
protected:
    /// @brief The tags describing the real dimensions in the equation.
    using real_laplacian_tags = ddc::detail::TypeSeq<typename ODims::continuous_dimension_type...>;
    /// @brief The tags describing the discrete dimensions in the equation.
    using laplacian_tags = ddc::detail::TypeSeq<ODims...>;
    /// @brief The tags describing the dimensions of the index range on which the operator acts.
    using space_tags = ddc::to_type_seq_t<IdxRangeFull>;
    /// @brief The tags describing the batched dimensions.
    using batch_tags = ddc::type_seq_remove_t<space_tags, laplacian_tags>;

protected:
    /// @brief Indicates whether the gradient is represented by a VectorField or a Field.
    static constexpr bool using_vector_field = ddc::type_seq_size_v<laplacian_tags> == 1;

public:
    /// @brief The Field type of the arguments to operator().
    using field_type = DField<IdxRangeFull, MemorySpace, LayoutSpace>;
    /// @brief The const Field type of the arguments to operator().
    using const_field_type = DConstField<IdxRangeFull, MemorySpace, LayoutSpace>;

    /// @brief The type of the derivative of @f$ \phi @f$.
    using vector_field_type = std::conditional_t<
            ddc::type_seq_size_v<laplacian_tags> == 1,
            field_type,
            VectorField<double, IdxRangeFull, real_laplacian_tags, MemorySpace, LayoutSpace>>;

    /// @brief The index range type describing the batch dimensions.
    using batch_idx_range_type =
            typename ddc::detail::convert_type_seq_to_discrete_domain_t<batch_tags>;
    /// @brief The index for indexing a batch dimension.
    using batch_index_type = typename batch_idx_range_type::discrete_element_type;

    /// @brief The type of the index range on which the equation is defined.
    using laplacian_idx_range_type = IdxRange<ODims...>;

    /// @brief The layout space of the Fields passed to operator().
    using layout_space = LayoutSpace;
    /// @brief The space (CPU/GPU) where the Fields passed to operator() are saved.
    using memory_space = MemorySpace;

public:
    /**
     * @brief An operator which calculates the solution @f$\phi@f$ to Poisson's equation:
     * @f$ -\Delta \phi = \rho @f$
     *
     * @param[out] phi The solution to Poisson's equation.
     * @param[in] rho The right-hand side of Poisson's equation.
     *
     * @return A reference to the solution to Poisson's equation.
     */
    virtual field_type operator()(field_type phi, field_type rho) const = 0;

    /**
     * @brief An operator which calculates the solution @f$\phi@f$ to Poisson's equation and
     * its derivative:
     * @f$ - \Delta \phi = \rho @f$
     * @f$ E = - \nabla \phi @f$
     *
     * @param[out] phi The solution to Poisson's equation.
     * @param[out] E The derivative of the solution to Poisson's equation.
     * @param[in] rho The right-hand side of Poisson's equation.
     *
     * @return A reference to the solution to Poisson's equation.
     */
    virtual field_type operator()(field_type phi, vector_field_type E, field_type rho) const = 0;
};

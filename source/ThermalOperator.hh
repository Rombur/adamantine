/* Copyright (c) 2016 - 2021, the adamantine authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#ifndef THERMAL_OPERATOR_HH
#define THERMAL_OPERATOR_HH

#include <MaterialProperty.hh>
#include <ThermalOperatorBase.hh>

#include <deal.II/matrix_free/matrix_free.h>

namespace adamantine
{
/**
 * This class is the operator associated with the heat equation, i.e., vmult
 * performs \f$ dst = -\nabla k \nabla src \f$.
 */
template <int dim, int fe_degree, typename MemorySpaceType>
class ThermalOperator final : public ThermalOperatorBase<dim, MemorySpaceType>
{
public:
  ThermalOperator(MPI_Comm const &communicator,
                  std::shared_ptr<MaterialProperty<dim>> material_properties,
                  BoundaryType boundary_type);

  /**
   * Associate the AffineConstraints<double> and the MatrixFree objects to the
   * underlying Triangulation.
   */
  void reinit(dealii::DoFHandler<dim> const &dof_handler,
              dealii::AffineConstraints<double> const &affine_constraints,
              dealii::hp::QCollection<1> const &quad) override;

  /**
   * Compute the inverse of the mass matrix and update the material properties.
   */
  void compute_inverse_mass_matrix(
      dealii::DoFHandler<dim> const &dof_handler,
      dealii::AffineConstraints<double> const &affine_constraints,
      dealii::hp::FECollection<dim> const &fe_collection) override;

  /**
   * Clear the MatrixFree object and resize the inverse of the mass matrix to
   * zero.
   */
  void clear();

  dealii::types::global_dof_index m() const override;

  dealii::types::global_dof_index n() const override;

  /**
   * Return a shared pointer to the inverse of the mass matrix.
   */
  std::shared_ptr<dealii::LA::distributed::Vector<double, MemorySpaceType>>
  get_inverse_mass_matrix() const override;

  /**
   * Return a shared pointer to the underlying MatrixFree object.
   */
  dealii::MatrixFree<dim, double> const &get_matrix_free() const;

  void vmult(dealii::LA::distributed::Vector<double, MemorySpaceType> &dst,
             dealii::LA::distributed::Vector<double, MemorySpaceType> const
                 &src) const override;

  void Tvmult(dealii::LA::distributed::Vector<double, MemorySpaceType> &dst,
              dealii::LA::distributed::Vector<double, MemorySpaceType> const
                  &src) const override;

  void vmult_add(dealii::LA::distributed::Vector<double, MemorySpaceType> &dst,
                 dealii::LA::distributed::Vector<double, MemorySpaceType> const
                     &src) const override;

  void Tvmult_add(dealii::LA::distributed::Vector<double, MemorySpaceType> &dst,
                  dealii::LA::distributed::Vector<double, MemorySpaceType> const
                      &src) const override;

  void
  jacobian_vmult(dealii::LA::distributed::Vector<double, MemorySpaceType> &dst,
                 dealii::LA::distributed::Vector<double, MemorySpaceType> const
                     &src) const override;

  void initialize_dof_vector(
      dealii::LA::distributed::Vector<double, MemorySpaceType> &vector)
      const override;

  /**
   * Evaluate the material properties for a given state field.
   */
  void evaluate_material_properties(
      dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host> const
          &state) override;

  /**
   * Return the value of \f$ \frac{1}{\rho C_p} \f$ for a given cell.
   */
  double get_inv_rho_cp(
      typename dealii::DoFHandler<dim>::cell_iterator const &) const override;

private:
  /**
   * Apply the operator on a given set of quadrature points.
   */
  void cell_local_apply(
      dealii::MatrixFree<dim, double> const &data,
      dealii::LA::distributed::Vector<double, MemorySpaceType> &dst,
      dealii::LA::distributed::Vector<double, MemorySpaceType> const &src,
      std::pair<unsigned int, unsigned int> const &cell_range) const;

  /**
   * MPI communicator.
   */
  MPI_Comm const &_communicator;
  /**
   * Data to configure the MatrixFree object.
   */
  typename dealii::MatrixFree<dim, double>::AdditionalData _matrix_free_data;
  /**
   * Store the \f$ \frac{1}{\rho C_p}\f$ coefficient.
   */
  dealii::Table<2, dealii::VectorizedArray<double>> _inv_rho_cp;
  /**
   * Table of thermal conductivity coefficient.
   */
  dealii::Table<2, dealii::VectorizedArray<double>> _thermal_conductivity;
  /**
   * Material properties associated with the domain.
   */
  std::shared_ptr<MaterialProperty<dim>> _material_properties;
  /**
   * Type of boundary.
   */
  BoundaryType _boundary_type;
  /**
   * Underlying MatrixFree object.
   */
  dealii::MatrixFree<dim, double> _matrix_free;
  /**
   * Non-owning pointer to the AffineConstraints from ThermalPhysics.
   */
  dealii::AffineConstraints<double> const *_affine_constraints;
  /**
   * The inverse of the mass matrix is computed using an inexact
   * Gauss-Lobatto quadrature. This inexact quadrature makes the mass matrix
   * and therefore also its inverse, a diagonal matrix.
   */
  std::shared_ptr<dealii::LA::distributed::Vector<double, MemorySpaceType>>
      _inverse_mass_matrix;
  /**
   * Map between the cell iterator and the position in _inv_rho_cp table.
   */
  std::map<typename dealii::DoFHandler<dim>::cell_iterator,
           std::pair<unsigned int, unsigned int>>
      _cell_it_to_mf_cell_map;
};

template <int dim, int fe_degree, typename MemorySpaceType>
inline dealii::types::global_dof_index
ThermalOperator<dim, fe_degree, MemorySpaceType>::m() const
{
  return _matrix_free.get_vector_partitioner()->size();
}

template <int dim, int fe_degree, typename MemorySpaceType>
inline dealii::types::global_dof_index
ThermalOperator<dim, fe_degree, MemorySpaceType>::n() const
{
  return _matrix_free.get_vector_partitioner()->size();
}

template <int dim, int fe_degree, typename MemorySpaceType>
inline std::shared_ptr<dealii::LA::distributed::Vector<double, MemorySpaceType>>
ThermalOperator<dim, fe_degree, MemorySpaceType>::get_inverse_mass_matrix()
    const
{
  return _inverse_mass_matrix;
}

template <int dim, int fe_degree, typename MemorySpaceType>
inline dealii::MatrixFree<dim, double> const &
ThermalOperator<dim, fe_degree, MemorySpaceType>::get_matrix_free() const
{
  return _matrix_free;
}

template <int dim, int fe_degree, typename MemorySpaceType>
inline void ThermalOperator<dim, fe_degree, MemorySpaceType>::jacobian_vmult(
    dealii::LA::distributed::Vector<double, MemorySpaceType> &dst,
    dealii::LA::distributed::Vector<double, MemorySpaceType> const &src) const
{
  vmult(dst, src);
}

template <int dim, int fe_degree, typename MemorySpaceType>
inline void
ThermalOperator<dim, fe_degree, MemorySpaceType>::initialize_dof_vector(
    dealii::LA::distributed::Vector<double, MemorySpaceType> &vector) const
{
  _matrix_free.initialize_dof_vector(vector);
}

template <int dim, int fe_degree, typename MemorySpaceType>
inline double ThermalOperator<dim, fe_degree, MemorySpaceType>::get_inv_rho_cp(
    typename dealii::DoFHandler<dim>::cell_iterator const &cell) const
{
  auto cell_comp_pair = _cell_it_to_mf_cell_map.find(cell);
  ASSERT(cell_comp_pair != _cell_it_to_mf_cell_map.end(), "Internal error");

  return _inv_rho_cp(cell_comp_pair->second.first,
                     0)[cell_comp_pair->second.second];
}

} // namespace adamantine

#endif

/* SPDX-FileCopyrightText: Copyright (c) 2022 - 2025, the adamantine authors.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <MechanicalPhysics.hh>
#include <instantiation.hh>

#include <deal.II/base/symmetric_tensor.h>
#include <deal.II/base/tensor.h>
#include <deal.II/dofs/dof_tools.h>
#include <deal.II/fe/fe_nothing.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/hp/fe_values.h>
#include <deal.II/lac/la_parallel_vector.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/trilinos_precondition.h>
#include <deal.II/numerics/vector_tools.h>

#ifdef ADAMANTINE_WITH_CALIPER
#include <caliper/cali.h>
#endif

namespace adamantine
{
template <int dim, int p_order, typename MaterialStates,
          typename MemorySpaceType>
MechanicalPhysics<dim, p_order, MaterialStates, MemorySpaceType>::
    MechanicalPhysics(MPI_Comm const &communicator,
                      unsigned int const fe_degree, Geometry<dim> &geometry,
                      Boundary const &boundary,
                      MaterialProperty<dim, p_order, MaterialStates,
                                       MemorySpaceType> &material_properties,
                      std::vector<double> const &reference_temperatures)
    : _geometry(geometry), _boundary(boundary),
      _material_properties(material_properties),
      _dof_handler(_geometry.get_triangulation()),
      _solution_transfer(_dof_handler),
      _cell_data_transfer(
          dynamic_cast<const dealii::parallel::distributed::Triangulation<dim>
                           &>(_dof_handler.get_triangulation()))
{
  // Create the FECollection
  _fe_collection.push_back(
      dealii::FESystem<dim>(dealii::FE_Q<dim>(fe_degree) ^ dim));
  _fe_collection.push_back(
      dealii::FESystem<dim>(dealii::FE_Nothing<dim>() ^ dim));

  // Create the QCollection
  _q_collection.push_back(dealii::QGauss<dim>(fe_degree + 1));
  _q_collection.push_back(dealii::QGauss<dim>(1));

  // Solve the mechanical problem only on the part of the domain that has solid
  // material.
  unsigned int n_active_cells =
      _dof_handler.get_triangulation().n_active_cells();
  for (auto const &cell :
       dealii::filter_iterators(_dof_handler.active_cell_iterators(),
                                dealii::IteratorFilters::LocallyOwnedCell()))
  {
    if (_material_properties.get_state_ratio(
            cell, MaterialStates::State::solid) > 0.99)
    {
      cell->set_active_fe_index(0);
    }
    else
    {
      cell->set_active_fe_index(1);
    }
  }

  // Create the mechanical operator
  _mechanical_operator = std::make_unique<
      MechanicalOperator<dim, p_order, MaterialStates, MemorySpaceType>>(
      communicator, _material_properties, reference_temperatures);

  // Create the data used to compute the stress tensor
  unsigned int const n_quad_pts = _q_collection.max_n_quadrature_points();
  _plastic_internal_variable.reserve(n_active_cells);

  for (auto const &cell : _dof_handler.active_cell_iterators())
  {
    if (cell->is_locally_owned())
    {
      auto elastic_limit = _material_properties.get_mechanical_property(
          cell, StateProperty::elastic_limit);
      _plastic_internal_variable.emplace_back(
          std::vector<double>(n_quad_pts, elastic_limit));
    }
    else
    {
      _plastic_internal_variable.emplace_back(std::vector<double>(
          n_quad_pts, std::numeric_limits<double>::signaling_NaN()));
    }
  }
  _stress.resize(n_active_cells,
                 std::vector<dealii::SymmetricTensor<2, dim>>(n_quad_pts));
  _back_stress.resize(n_active_cells,
                      std::vector<dealii::SymmetricTensor<2, dim>>(n_quad_pts));
}

template <int dim, int p_order, typename MaterialStates,
          typename MemorySpaceType>
void MechanicalPhysics<dim, p_order, MaterialStates, MemorySpaceType>::
    setup_dofs(std::vector<std::shared_ptr<BodyForce<dim>>> const &body_forces)
{
  _dof_handler.distribute_dofs(_fe_collection);
  dealii::IndexSet locally_relevant_dofs;
  dealii::DoFTools::extract_locally_relevant_dofs(_dof_handler,
                                                  locally_relevant_dofs);
  _affine_constraints.clear();
  _affine_constraints.reinit(locally_relevant_dofs);
  dealii::DoFTools::make_hanging_node_constraints(_dof_handler,
                                                  _affine_constraints);

  std::map<dealii::types::boundary_id, const dealii::Function<dim> *>
      boundary_function_map;
  dealii::Functions::ZeroFunction<dim> zero_function(dim);
  auto boundary_ids = _boundary.get_boundary_ids(BoundaryType::clamped);
  for (auto id : boundary_ids)
  {
    boundary_function_map[id] = &zero_function;
  }
  dealii::VectorTools::interpolate_boundary_values(
      _dof_handler, boundary_function_map, _affine_constraints);
  _affine_constraints.close();

  _mechanical_operator->reinit(_dof_handler, _affine_constraints, _q_collection,
                               body_forces);
}

template <int dim, int p_order, typename MaterialStates,
          typename MemorySpaceType>
void MechanicalPhysics<dim, p_order, MaterialStates,
                       MemorySpaceType>::prepare_transfer_mpi()
{
  _old_displacement.update_ghost_values();
  _solution_transfer.prepare_for_coarsening_and_refinement(_old_displacement);

  _data_to_transfer.clear();
  unsigned int const n_quad_pts = _q_collection.max_n_quadrature_points();
  unsigned int const n_doubles_per_quad_plastic = 1;
  unsigned int const n_doubles_per_quad_stress =
      dealii::SymmetricTensor<2, dim>::n_independent_components;

  unsigned int const n_doubles_per_quad =
      n_doubles_per_quad_plastic + n_doubles_per_quad_stress * 2;
  std::vector<double> dummy_cell_data(n_quad_pts * n_doubles_per_quad,
                                      std::numeric_limits<double>::infinity());

  unsigned int cell_id = 0;
  for (auto const &cell : _dof_handler.active_cell_iterators())
  {
    if (cell->is_locally_owned())
    {
      std::vector<double> cell_data(n_quad_pts * n_doubles_per_quad);
      unsigned int const stress_offset =
          n_quad_pts * n_doubles_per_quad_plastic;
      unsigned int const back_stress_offset =
          n_quad_pts * (n_doubles_per_quad_plastic + n_doubles_per_quad_stress);

      std::copy(_plastic_internal_variable[cell_id].begin(),
                _plastic_internal_variable[cell_id].end(), cell_data.begin());

      for (unsigned int quad = 0; quad < n_quad_pts; ++quad)
      {
        for (unsigned int i = 0; i < n_doubles_per_quad_stress; ++i)
        {
          cell_data[stress_offset + quad * n_doubles_per_quad_stress + i] =
              _stress[cell_id][quad].access_raw_entry(i);
          cell_data[back_stress_offset + quad * n_doubles_per_quad_stress + i] =
              _back_stress[cell_id][quad].access_raw_entry(i);
        }
      }
      _data_to_transfer.push_back(std::move(cell_data));
    }
    else
    {
      _data_to_transfer.push_back(dummy_cell_data);
    }
    ++cell_id;
  }
  _cell_data_transfer.prepare_for_coarsening_and_refinement(_data_to_transfer);
}

template <int dim, int p_order, typename MaterialStates,
          typename MemorySpaceType>
void MechanicalPhysics<dim, p_order, MaterialStates,
                       MemorySpaceType>::complete_transfer_mpi()
{
  _dof_handler.distribute_dofs(_fe_collection);

  const dealii::IndexSet locally_relevant_dofs =
      dealii::DoFTools::extract_locally_relevant_dofs(_dof_handler);
  _old_displacement.reinit(_dof_handler.locally_owned_dofs(),
                           locally_relevant_dofs,
                           _dof_handler.get_communicator());
  _solution_transfer.interpolate(_old_displacement);

  auto n_active_cells = _dof_handler.get_triangulation().n_active_cells();
  unsigned int const n_quad_pts = _q_collection.max_n_quadrature_points();

  _plastic_internal_variable.resize(n_active_cells,
                                    std::vector<double>(n_quad_pts));
  _stress.resize(n_active_cells,
                 std::vector<dealii::SymmetricTensor<2, dim>>(n_quad_pts));
  _back_stress.resize(n_active_cells,
                      std::vector<dealii::SymmetricTensor<2, dim>>(n_quad_pts));

  unsigned int const n_doubles_per_quad_plastic = 1;
  unsigned int const n_doubles_per_quad_stress =
      dealii::SymmetricTensor<2, dim>::n_independent_components;

  unsigned int const n_doubles_per_quad =
      n_doubles_per_quad_plastic + n_doubles_per_quad_stress * 2;
  std::vector<std::vector<double>> data_to_unpack(
      n_active_cells, std::vector<double>(n_doubles_per_quad * n_quad_pts));
  _cell_data_transfer.unpack(data_to_unpack);

  unsigned int cell_id = 0;
  for (auto const &cell : _dof_handler.active_cell_iterators())
  {
    if (cell->is_locally_owned())
    {
      unsigned int const stress_offset =
          n_quad_pts * n_doubles_per_quad_plastic;
      unsigned int const back_stress_offset =
          n_quad_pts * (n_doubles_per_quad_plastic + n_doubles_per_quad_stress);

      std::copy(data_to_unpack[cell_id].begin(),
                data_to_unpack[cell_id].begin() + stress_offset,
                _plastic_internal_variable[cell_id].begin());

      for (unsigned int quad = 0; quad < n_quad_pts; ++quad)
      {
        for (unsigned int i = 0; i < n_doubles_per_quad_stress; ++i)
        {
          _stress[cell_id][quad].access_raw_entry(i) =
              data_to_unpack[cell_id][stress_offset +
                                      quad * n_doubles_per_quad_stress + i];
          _back_stress[cell_id][quad].access_raw_entry(i) =
              data_to_unpack[cell_id][back_stress_offset +
                                      quad * n_doubles_per_quad_stress + i];
        }
      }
    }
    ++cell_id;
  }
}

template <int dim, int p_order, typename MaterialStates,
          typename MemorySpaceType>
void MechanicalPhysics<dim, p_order, MaterialStates, MemorySpaceType>::
    setup_dofs(
        dealii::DoFHandler<dim> const &thermal_dof_handler,
        dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host> const
            &temperature,
        std::vector<bool> const &has_melted,
        std::vector<std::shared_ptr<BodyForce<dim>>> const &body_forces)
{
  _mechanical_operator->update_temperature(thermal_dof_handler, temperature,
                                           has_melted);
  // Update the active fe indices, the plastic variables, and the displacement.
  unsigned int const n_quad_pts = _q_collection.max_n_quadrature_points();
  unsigned int cell_id = 0;
  std::vector<std::vector<double>> saved_old_displacement;
  std::vector<std::vector<double>> tmp_plastic_internal_variable;
  std::vector<std::vector<dealii::SymmetricTensor<2, dim>>> tmp_stress;
  std::vector<std::vector<dealii::SymmetricTensor<2, dim>>> tmp_back_stress;
  // The number of cells to activate/deactive should be small, so we can
  // already reserve the memory.
  unsigned int const n_dofs_per_cell = _fe_collection.max_dofs_per_cell();
  unsigned int const n_old_active_cells = _plastic_internal_variable.size();
  std::vector<dealii::types::global_dof_index> global_dof_indices(
      n_dofs_per_cell);
  tmp_plastic_internal_variable.reserve(n_old_active_cells);
  tmp_stress.reserve(n_old_active_cells);
  tmp_back_stress.reserve(_back_stress.size());
  // First we save _old_displacement if it exists
  if (_old_displacement.size())
  {
    _old_displacement.update_ghost_values();

    std::vector<double> cell_values(n_dofs_per_cell);
    saved_old_displacement.reserve(n_old_active_cells);
    for (auto const &cell : _dof_handler.active_cell_iterators())
    {
      if (cell->is_locally_owned())
      {
        auto fe_index = cell->active_fe_index();
        if (fe_index == 0)
        {
          // The cell contains solid material, we need to save the displacement
          cell->get_dof_indices(global_dof_indices);
          for (unsigned int i = 0; i < n_dofs_per_cell; ++i)
          {
            cell_values[i] = _old_displacement[global_dof_indices[i]];
          }
        }
        else
        {
          // The cell does not contain material or it is liquid. The
          // displacement is ignored.
          cell_values.assign(n_dofs_per_cell, 0.);
        }
        saved_old_displacement.push_back(cell_values);
      }
      else
      {
        saved_old_displacement.push_back(std::vector<double>(n_dofs_per_cell));
      }
    }
  }

  // Now we can update the fe indices and the plastic variables.
  for (auto const &cell : _dof_handler.active_cell_iterators())
  {
    if (cell->is_locally_owned())
    {
      auto current_fe_index = cell->active_fe_index();
      if (_material_properties.get_state_ratio(
              cell, MaterialStates::State::solid) > 0.99)
      {
        // Only enable the cell if it is also enabled for the thermal simulation
        // Get the thermal DoFHandler cell iterator
        dealii::DoFCellAccessor<dim, dim, false> thermal_cell(
            &(_dof_handler.get_triangulation()), cell->level(), cell->index(),
            &thermal_dof_handler);
        auto updated_fe_index = thermal_cell.active_fe_index();
        if (current_fe_index == updated_fe_index)
        {
          // The cells is unchanged, we just copy the plastic variables as-is.
          tmp_plastic_internal_variable.push_back(
              _plastic_internal_variable[cell_id]);
          tmp_stress.push_back(_stress[cell_id]);
          tmp_back_stress.push_back(_back_stress[cell_id]);
        }
        else
        {
          // The cell has solidified or material has been added. The new cells
          // are initialized with default values.
          auto elastic_limit = _material_properties.get_mechanical_property(
              cell, StateProperty::elastic_limit);
          tmp_plastic_internal_variable.push_back(
              std::vector<double>(n_quad_pts, elastic_limit));
          tmp_stress.push_back(
              std::vector<dealii::SymmetricTensor<2, dim>>(n_quad_pts));
          tmp_back_stress.push_back(
              std::vector<dealii::SymmetricTensor<2, dim>>(n_quad_pts));

          cell->set_active_fe_index(updated_fe_index);
        }
      }
      else
      {
        // The cell is liquid. We don't need to save the plastic variables.
        cell->set_active_fe_index(1);
        tmp_plastic_internal_variable.push_back(std::vector<double>(
            n_quad_pts, std::numeric_limits<double>::signaling_NaN()));
        tmp_stress.push_back(
            std::vector<dealii::SymmetricTensor<2, dim>>(n_quad_pts));
        tmp_back_stress.push_back(
            std::vector<dealii::SymmetricTensor<2, dim>>(n_quad_pts));
      }
    }
    else
    {
      tmp_plastic_internal_variable.push_back(std::vector<double>(
          n_quad_pts, std::numeric_limits<double>::signaling_NaN()));
      tmp_stress.push_back(
          std::vector<dealii::SymmetricTensor<2, dim>>(n_quad_pts));
      tmp_back_stress.push_back(
          std::vector<dealii::SymmetricTensor<2, dim>>(n_quad_pts));
    }
    ++cell_id;
  }
  _plastic_internal_variable.swap(tmp_plastic_internal_variable);
  _stress.swap(tmp_stress);
  _back_stress.swap(tmp_back_stress);

  setup_dofs(body_forces);

  // Update _old_displacement if necessary
  const dealii::IndexSet locally_relevant_dofs =
      dealii::DoFTools::extract_locally_relevant_dofs(_dof_handler);
  const dealii::IndexSet locally_owned_dofs = _dof_handler.locally_owned_dofs();
  _old_displacement.reinit(locally_owned_dofs, locally_relevant_dofs,
                           _dof_handler.get_communicator());

  if (saved_old_displacement.size())
  {
    cell_id = 0;
    for (auto const &cell : _dof_handler.active_cell_iterators())
    {
      if (cell->is_locally_owned())
      {
        auto fe_index = cell->active_fe_index();
        if (fe_index == 0)
        {
          cell->get_dof_indices(global_dof_indices);
          for (unsigned int i = 0; i < n_dofs_per_cell; ++i)
          {
            if (locally_owned_dofs.is_element(global_dof_indices[i]))
              _old_displacement[global_dof_indices[i]] =
                  saved_old_displacement[cell_id][i];
          }
        }
      }
      ++cell_id;
    }
    _old_displacement.compress(dealii::VectorOperation::insert);
  }
}

template <int dim, int p_order, typename MaterialStates,
          typename MemorySpaceType>
dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host>
MechanicalPhysics<dim, p_order, MaterialStates, MemorySpaceType>::solve()
{
#ifdef ADAMANTINE_WITH_CALIPER
  CALI_MARK_BEGIN("solve mechanical system");
#endif

  dealii::IndexSet locally_relevant_dofs =
      dealii::DoFTools::extract_locally_relevant_dofs(_dof_handler);
  dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host>
      displacement(_dof_handler.locally_owned_dofs(), locally_relevant_dofs,
                   _mechanical_operator->rhs().get_mpi_communicator());

  // Solve the mechanical problem assuming that the deformation is elastic
  // TODO check that we are computing only difference of the displacement
  // compared to the previous time step!!
  unsigned int const max_iter = _dof_handler.n_dofs() / 10;
  double const tol = 1e-12 * _mechanical_operator->rhs().l2_norm();
  dealii::SolverControl solver_control(max_iter, tol);
  dealii::SolverCG<
      dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host>>
      cg(solver_control);
  // FIXME Use better preconditioner
  dealii::TrilinosWrappers::PreconditionSSOR preconditioner;
  preconditioner.initialize(_mechanical_operator->system_matrix());
  cg.solve(_mechanical_operator->system_matrix(), displacement,
           _mechanical_operator->rhs(), preconditioner);
  _affine_constraints.distribute(displacement);

  // Compute the new stress assuming the deformation is elastic.
  // If the stress is under the yield criterion, the deformation is elastic and
  // we are done. Otherwise we need to use the radial return algorithm to
  // compute the plastic deformation.
  dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host>
      incremental_displacement(
          _dof_handler.locally_owned_dofs(), locally_relevant_dofs,
          _mechanical_operator->rhs().get_mpi_communicator());
  incremental_displacement = displacement;
  if (_old_displacement.size() > 0)
  {
    incremental_displacement -= _old_displacement;
  }
  incremental_displacement.update_ghost_values();
  compute_stress(incremental_displacement);

  _old_displacement.swap(displacement);

#ifdef ADAMANTINE_WITH_CALIPER
  CALI_MARK_END("solve mechanical system");
#endif

  return _old_displacement;
}

template <int dim, int p_order, typename MaterialStates,
          typename MemorySpaceType>
void MechanicalPhysics<dim, p_order, MaterialStates, MemorySpaceType>::
    compute_stress(
        dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host> const
            &displacement)
{
  dealii::hp::FEValues<dim> displacement_hp_fe_values(
      _fe_collection, _q_collection, dealii::update_gradients);
  unsigned int const dofs_per_cell = _fe_collection.max_dofs_per_cell();
  std::vector<dealii::types::global_dof_index> local_dof_indices(dofs_per_cell);
  unsigned int const n_q_points = _q_collection.max_n_quadrature_points();
  std::vector<dealii::SymmetricTensor<2, dim>> strain_tensor(n_q_points);
  const dealii::FEValuesExtractors::Vector displacement_extr(0);
  unsigned int cell_id = 0;
  for (auto const &cell : _dof_handler.active_cell_iterators())
  {
    if (cell->is_locally_owned() && cell->active_fe_index() == 0)
    {
      // Formulation based on the combined isotropic-kinematic hardening model
      // for J2 plasticity in Chapter 3 of R. Borja, Plasticity: Modeling and
      // Computation, Springer-Verlag, 2013. DOI: 10.1007/978-3-642-38547-6
      //
      // Compute the strain. We get the strain for all the quadrature points at
      // once.
      displacement_hp_fe_values.reinit(cell);
      auto const &fe_values = displacement_hp_fe_values.get_present_fe_values();
      cell->get_dof_indices(local_dof_indices);

      fe_values[displacement_extr].get_function_symmetric_gradients(
          displacement, strain_tensor);

      double const lambda = _material_properties.get_mechanical_property(
          cell, StateProperty::lame_first_parameter);
      double const mu = _material_properties.get_mechanical_property(
          cell, StateProperty::lame_second_parameter);
      double const plastic_modulus =
          _material_properties.get_mechanical_property(
              cell, StateProperty::plastic_modulus);
      double const iso_hardening_coef =
          _material_properties.get_mechanical_property(
              cell, StateProperty::isotropic_hardening);
      dealii::SymmetricTensor<4, dim> stiffness_tensor =
          lambda * dealii::outer_product(dealii::unit_symmetric_tensor<dim>(),
                                         dealii::unit_symmetric_tensor<dim>()) +
          2 * mu * dealii::identity_tensor<dim>();
      // Loop over the quadrature points.
      for (auto const q : fe_values.quadrature_point_indices())
      {
        // Compute the trial elastic stress.
        dealii::SymmetricTensor<2, dim> elastic_stress = _stress[cell_id][q];
        elastic_stress += stiffness_tensor * strain_tensor[q];

        auto stress_deviator = dealii::deviator(elastic_stress);
        auto effective_stress = stress_deviator - _back_stress[cell_id][q];
        double const effective_stress_norm = effective_stress.norm();
        if (effective_stress_norm < _plastic_internal_variable[cell_id][q])
        {
          // The deformation is elastic. We just update the stress with the
          // elastic stress.
          _stress[cell_id][q] = elastic_stress;
        }
        else
        {
          // The deformation is plastic. We need to compute a new stress and
          // update the plastic internal variable and the back stress.
          double plastic_strain_increment =
              (effective_stress_norm - _plastic_internal_variable[cell_id][q]) /
              (2. * mu + plastic_modulus);
          auto plastic_flow_direction =
              effective_stress / effective_stress_norm;
          // Update stress
          _stress[cell_id][q] = elastic_stress - 2. * mu *
                                                     plastic_strain_increment *
                                                     plastic_flow_direction;
          // Update plastic internal variable
          _plastic_internal_variable[cell_id][q] +=
              iso_hardening_coef * plastic_modulus * plastic_strain_increment;
          // Update back stress
          _back_stress[cell_id][q] += (1. - iso_hardening_coef) *
                                      plastic_modulus * plastic_modulus *
                                      plastic_flow_direction;
        }
      }
    }
    ++cell_id;
  }
}

} // namespace adamantine

INSTANTIATE_DIM_PORDER_MATERIALSTATES_HOST(TUPLE(MechanicalPhysics))
INSTANTIATE_DIM_PORDER_MATERIALSTATES_DEVICE(TUPLE(MechanicalPhysics))

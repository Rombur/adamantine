/* Copyright (c) 2023, the adamantine authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#ifndef REFINE_TRANSFER_HELPER_HH
#define REFINE_TRANSFER_HELPER_HH

#include <MaterialProperty.hh>
#include <MechanicalPhysics.hh>
#include <ThermalPhysicsInterface.hh>

#include <deal.II/arborx/distributed_tree.h>
#include <deal.II/distributed/cell_data_transfer.templates.h>
#include <deal.II/distributed/solution_transfer.h>
#include <deal.II/grid/grid_refinement.h>

#include <memory>

namespace adamantine
{
// TODO
template <int dim, typename MemorySpaceType>
class RefineTransferHelper
{
public:
  RefineTransferHelper(
      std::unique_ptr<ThermalPhysicsInterface<dim, MemorySpaceType>>
          &thermal_physics,
      std::unique_ptr<MechanicalPhysics<dim, MemorySpaceType>>
          &mechanical_physics,
      MaterialProperty<dim, MemorySpaceType> &material_properties,
      dealii::parallel::distributed::Triangulation<dim> &triangulation,
      dealii::LA::distributed::Vector<double, MemorySpaceType> &temperature,
      dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host>
          &displacement);

  void execute();

private:
  void prepare_material_data();

  void prepare_thermal_data();

  void prepare_mechanical_data();

  void update_material_data(dealii::parallel::distributed::CellDataTransfer<
                            dim, dim, std::vector<std::vector<double>>>
                                &material_cell_data_trans);

  void update_thermal_data(
      dealii::parallel::distributed::CellDataTransfer<
          dim, dim, std::vector<std::vector<double>>> &thermal_cell_data_trans);

  void update_mechanical_data();

  bool const _use_thermal_physics;
  bool const _use_mechanical_physics;
  std::unique_ptr<ThermalPhysicsInterface<dim, MemorySpaceType>>
      &_thermal_physics;
  std::unique_ptr<MechanicalPhysics<dim, MemorySpaceType>> &_mechanical_physics;
  MaterialProperty<dim, MemorySpaceType> &_material_properties;
  dealii::parallel::distributed::Triangulation<dim> &_triangulation;
  dealii::LA::distributed::Vector<double, MemorySpaceType> &_temperature;
  dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host>
      &_displacement;
  std::unique_ptr<dealii::parallel::distributed::SolutionTransfer<
      dim, dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host>>>
      _temperature_transfer;
  std::unique_ptr<dealii::parallel::distributed::SolutionTransfer<
      dim, dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host>>>
      _displacement_transfer;
  std::vector<std::vector<double>> _thermal_data_to_transfer;
  std::vector<std::vector<double>> _material_data_to_transfer;
  std::unique_ptr<dealii::ArborXWrappers::DistributedTree> _distributed_tree;
};
} // namespace adamantine
#endif

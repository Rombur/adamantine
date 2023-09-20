/* Copyright (c) 2023, the adamantine authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#include <RefineTransferHelper.hh>
#include <instantiation.hh>

#include <deal.II/dofs/dof_handler.h>
#include <deal.II/fe/fe_nothing.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/grid/filtered_iterator.h>
#include <deal.II/grid/tria_iterator.h>
#include <deal.II/hp/fe_collection.h>
#include <deal.II/hp/fe_values.h>

#include <unordered_map>
#include <unordered_set>

#ifdef ADAMANTINE_WITH_CALIPER
#include <caliper/cali.h>
#endif

#include <boost/container_hash/hash.hpp>

#include <memory>

namespace adamantine
{
template <int dim, typename MemorySpaceType>
RefineTransferHelper<dim, MemorySpaceType>::RefineTransferHelper(
    std::unique_ptr<ThermalPhysicsInterface<dim, MemorySpaceType>>
        &thermal_physics,
    std::unique_ptr<MechanicalPhysics<dim, MemorySpaceType>>
        &mechanical_physics,
    MaterialProperty<dim, MemorySpaceType> &material_properties,
    dealii::parallel::distributed::Triangulation<dim> &triangulation,
    dealii::LA::distributed::Vector<double, MemorySpaceType> &temperature,
    dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host>
        &displacement)
    : _use_thermal_physics(thermal_physics == nullptr ? false : true),
      _use_mechanical_physics(mechanical_physics == nullptr ? false : true),
      _thermal_physics(thermal_physics),
      _mechanical_physics(mechanical_physics),
      _material_properties(material_properties), _triangulation(triangulation),
      _temperature(temperature), _displacement(displacement)
{
  prepare_material_data();

  if (_use_thermal_physics)
  {
    // Update the material state from the ThermalOperator to MaterialProperty
    // because, for now, we need to use state from MaterialProperty to perform
    // the transfer to the refined mesh.
    _thermal_physics->set_state_to_material_properties();

    _temperature_transfer =
        std::make_unique<dealii::parallel::distributed::SolutionTransfer<
            dim, dealii::LA::distributed::Vector<double,
                                                 dealii::MemorySpace::Host>>>(
            _thermal_physics->get_dof_handler());

    prepare_thermal_data();
  }

  if (_use_mechanical_physics)
  {
    _displacement_transfer =
        std::make_unique<dealii::parallel::distributed::SolutionTransfer<
            dim, dealii::LA::distributed::Vector<double,
                                                 dealii::MemorySpace::Host>>>(
            mechanical_physics->get_dof_handler());

    prepare_mechanical_data();
  }
}

template <int dim, typename MemorySpaceType>
void RefineTransferHelper<dim, MemorySpaceType>::execute()
{
  // Prepare the Triangulation and the diffent data transfer objects for
  // refinement
  _triangulation.prepare_coarsening_and_refinement();

  // Prepare for refinement of the material properties
  dealii::parallel::distributed::CellDataTransfer<
      dim, dim, std::vector<std::vector<double>>>
      material_cell_data_trans(_triangulation);
  material_cell_data_trans.prepare_for_coarsening_and_refinement(
      _material_data_to_transfer);

  // Prepare for refinement of the temperature
  std::unique_ptr<dealii::parallel::distributed::CellDataTransfer<
      dim, dim, std::vector<std::vector<double>>>>
      thermal_cell_data_trans;
  if (_use_thermal_physics)
  {
    if constexpr (std::is_same_v<MemorySpaceType, dealii::MemorySpace::Host>)
    {
      // We need to apply the constraints before the mesh transfer
      _thermal_physics->get_affine_constraints().distribute(_temperature);
      // We need to update the ghost values before we can do the interpolation
      // on the new mesh.
      _temperature.update_ghost_values();
      _temperature_transfer->prepare_for_coarsening_and_refinement(
          _temperature);
    }
    else
    {
      dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host>
          temperature_host(_temperature.get_partitioner());
      temperature_host.import(_temperature, dealii::VectorOperation::insert);
      // We need to apply the constraints before the mesh transfer
      _thermal_physics->get_affine_constraints().distribute(temperature_host);
      // We need to update the ghost values before we can do the interpolation
      // on the new mesh.
      temperature_host.update_ghost_values();
      _temperature_transfer->prepare_for_coarsening_and_refinement(
          temperature_host);
    }

    thermal_cell_data_trans =
        std::make_unique<dealii::parallel::distributed::CellDataTransfer<
            dim, dim, std::vector<std::vector<double>>>>(_triangulation);
    thermal_cell_data_trans->prepare_for_coarsening_and_refinement(
        _thermal_data_to_transfer);
  }

  std::unique_ptr<dealii::parallel::distributed::CellDataTransfer<
      dim, dim, std::vector<std::vector<double>>>>
      mechanical_cell_data_trans;
  if (_use_mechanical_physics)
  {
    // TODO
  }

#ifdef ADAMANTINE_WITH_CALIPER
  CALI_MARK_BEGIN("refine triangulation");
#endif
  // Execute the refinement
  _triangulation.execute_coarsening_and_refinement();
#ifdef ADAMANTINE_WITH_CALIPER
  CALI_MARK_END("refine triangulation");
#endif

  update_material_data(material_cell_data_trans);

  if (_use_thermal_physics)
  {
    update_thermal_data(*thermal_cell_data_trans);
  }

  if (_use_mechanical_physics)
  {
    update_mechanical_data();
  }
}

template <int dim, typename MemorySpaceType>
void RefineTransferHelper<dim, MemorySpaceType>::prepare_thermal_data()
{
  unsigned int constexpr direction_data_size = 2;
  unsigned int constexpr phase_history_data_size = 1;
  unsigned int constexpr data_size =
      direction_data_size + phase_history_data_size;
  unsigned int activated_cell_id = 0;
  _thermal_data_to_transfer.reserve(_triangulation.n_active_cells());

  std::vector<double> cell_data(direction_data_size + phase_history_data_size);
  auto const &dof_handler = _thermal_physics->get_dof_handler();
  for (auto const &cell : dof_handler.active_cell_iterators())
  {
    cell_data.assign(data_size, std::numeric_limits<double>::infinity());
    if (cell->is_locally_owned())
    {
      // If the cell has material, we save the thermal data
      if (cell->active_fe_index() == 0)
      {
        cell_data[0] = _thermal_physics->get_deposition_cos(activated_cell_id);
        cell_data[1] = _thermal_physics->get_deposition_sin(activated_cell_id);

        if (_thermal_physics->get_has_melted(activated_cell_id))
          cell_data[direction_data_size] = 1.0;
        else
          cell_data[direction_data_size] = 0.0;

        ++activated_cell_id;
      }
    }
    _thermal_data_to_transfer.push_back(cell_data);
  }
}

template <int dim, typename MemorySpaceType>
void RefineTransferHelper<dim, MemorySpaceType>::prepare_mechanical_data()
{
  // The mechanical data is stored at the quadrature point and it is potentially
  // discontinuous. To transfer the data we could use
  // dealii::ContinuousQuadratureDataTransfer. This creates a FiniteElement to
  // the transfer of the quadrature point data between cells in case of adaptive
  // refinement using L2 projection. However there are two issues:
  //   1. coarsening is currently not supported
  //   2. the documentation explictly says that the class should not be used
  //   when transfering discontinuous data such as the ones used in
  //   elasto-plastic problem.
  // We could average the data on the cells and then  use CellDataTransfer but
  // this is just a worse version of the previous class. The only advantage is
  // that there is no issue with h-refinement. The last possibility is to use
  // ArborX. As of version 1.4.1, we can use the nearest neighbor capabilities
  // of ArborX to assign data associated to the old quadrature points to the new
  // ones. In a future release, ArborX will provide the Moving Least Square
  // algorithm (MLS). It is debatable if MLS is better than the L2 projection
  // provided by deal.II. However it does work even if the mesh is coarsen.
  // There is also the added difficulty that we are working with tensors and not
  // scalars. Ultimately, the transfer of the mechanical data is a difficult
  // problem and we may need to be revisited later.

  // The mechanical data is saved by [cell][quadrature point]. This means that
  // some points may be duplicated for example when the quadrature point is on a
  // vertex. To avoid any issue with ArborX, we need to make sure that we remove
  // the duplicated points. To assign a unique ID to each quadrature points, we
  // create a DoFHandler which has its dof associated to the quadrature points.
  // We can then create a mapping between the ArborX IDs and the mechanical IDs
  // (cell id, quad id) and ensure that ArborX does not get duplicated points.
  unsigned int const fe_degree = _mechanical_physics->get_dof_handler()
                                     .get_fe_collection()
                                     .max_dofs_per_cell();
  dealii::FE_Q<dim> fe(dealii::QGauss<1>(fe_degree + 1));
  dealii::hp::FECollection<dim> fe_collection(fe, dealii::FE_Nothing<dim>());
  dealii::DoFHandler<dim> quad_dof_handler(_triangulation);
  for (auto const &tria_cell : _triangulation.active_cell_iterators() |
                                   dealii::IteratorFilters::LocallyOwnedCell())
  {
    dealii::TriaIterator<dealii::DoFCellAccessor<dim, dim, false>> dof_cell(
        &_triangulation, tria_cell->level(), tria_cell->index(),
        &quad_dof_handler);
    if (_material_properties.get_state_ratio(tria_cell, MaterialState::solid) >
        0.99)
    {
      dof_cell->set_active_fe_index(0);
    }
    else
    {
      dof_cell->set_active_fe_index(1);
    }
  }
  quad_dof_handler.distribute_dofs(fe_collection);

  dealii::FEValues<dim, dim> fe_values(fe, fe.get_unit_support_points(),
                                       dealii::update_quadrature_points);
  std::vector<dealii::types::global_dof_index> local_dof_indices(
      fe.n_dofs_per_cell());
  auto locally_owned_dofs = quad_dof_handler.locally_owned_dofs();
  std::vector<std::pair<unsigned int, unsigned int>> arborx_2_mecha_indices;
  std::unordered_map<std::pair<unsigned int, unsigned int>, unsigned int,
                     boost::hash<std::pair<unsigned int, unsigned int>>>
      mecha_2_arborx_indices;
  std::vector<dealii::Point<dim>> quadrature_points;
  std::unordered_set<dealii::types::global_dof_index> visited_dof_indices;
  unsigned int cell_id = 0;
  for (auto const &cell :
       quad_dof_handler.active_cell_iterators() |
           dealii::IteratorFilters::ActiveFEIndexEqualTo(0, true))
  {
    fe_values.reinit(cell);
    cell->get_dof_indices(local_dof_indices);
    std::vector<dealii::Point<dim>> const &points =
        fe_values.get_quadrature_points();
    for (unsigned int i = 0; i < fe.n_dofs_per_cell(); ++i)
    {
      // Skip duplicate points like vertices and indices that correspond to
      // ghosted elements
      if ((visited_dof_indices.count(local_dof_indices[i]) == 0) &&
          (locally_owned_dofs.is_element(local_dof_indices[i])))
      {
        arborx_2_mecha_indices.emplace_back(cell_id, i);
        quadrature_points.push_back(points[i]);
        visited_dof_indices.insert(local_dof_indices[i]);
      }
      mecha_2_arborx_indices[std::make_pair(cell_id, i)] =
          arborx_2_mecha_indices.size() - 1;
    }
    ++cell_id;
  }

  // FIXME use MLS once it's implemented in a newer version of ArborX
  // Now that we have the points can create the distributed tree.
  _distributed_tree = std::make_unique<dealii::ArborXWrappers::DistributedTree>(
      _triangulation.get_communicator(), quadrature_points);
}

template <int dim, typename MemorySpaceType>
void RefineTransferHelper<dim, MemorySpaceType>::prepare_material_data()
{
  MemoryBlockView<double, MemorySpaceType> material_state_view =
      _material_properties.get_state();
  MemoryBlock<double, dealii::MemorySpace::Host> material_state_host(
      material_state_view.extent(0), material_state_view.extent(1));
  typename decltype(material_state_view)::memory_space memspace;
  deep_copy(material_state_host.data(), dealii::MemorySpace::Host{},
            material_state_view.data(), memspace, material_state_view.size());

  adamantine::MemoryBlockView<double, dealii::MemorySpace::Host>
      state_host_view(material_state_host);
  unsigned int cell_id = 0;
  std::vector<double> cell_data(g_n_material_states);
  for (auto const &cell : _triangulation.active_cell_iterators())
  {
    cell_data.assign(g_n_material_states,
                     std::numeric_limits<double>::infinity());
    if (cell->is_locally_owned())
    {
      for (unsigned int i = 0; i < g_n_material_states; ++i)
      {
        cell_data[i] = state_host_view(i, cell_id);
      }
      ++cell_id;
    }
    _material_data_to_transfer.push_back(cell_data);
  }
}

template <int dim, typename MemorySpaceType>
void RefineTransferHelper<dim, MemorySpaceType>::update_material_data(
    dealii::parallel::distributed::CellDataTransfer<
        dim, dim, std::vector<std::vector<double>>> &material_cell_data_trans)
{
  // Update MaterialProperty DoFHandler and resize the state vectors
  _material_properties.reinit_dofs();

  // Unpack the material state and repopulate the material state
  std::vector<std::vector<double>> transferred_material_data(
      _triangulation.n_active_cells(),
      std::vector<double>(g_n_material_states));
  material_cell_data_trans.unpack(transferred_material_data);
  MemoryBlockView<double, MemorySpaceType> material_state_view =
      _material_properties.get_state();
  MemoryBlock<double, dealii::MemorySpace::Host> material_state_host(
      material_state_view.extent(0), material_state_view.extent(1));
  MemoryBlockView<double, dealii::MemorySpace::Host> state_host_view(
      material_state_host);
  unsigned int total_cell_id = 0;
  unsigned int cell_id = 0;
  for (auto const &cell : _triangulation.active_cell_iterators())
  {
    if (cell->is_locally_owned())
    {
      for (unsigned int i = 0; i < g_n_material_states; ++i)
      {
        state_host_view(i, cell_id) =
            transferred_material_data[total_cell_id][i];
      }
      ++cell_id;
    }
    ++total_cell_id;
  }

  // Copy the data back to material_property
  typename decltype(material_state_view)::memory_space memspace;
  adamantine::deep_copy(material_state_view.data(), memspace,
                        material_state_host.data(), dealii::MemorySpace::Host{},
                        material_state_view.size());

#if ADAMANTINE_DEBUG
  // Check that we are not losing material
  int cell_id_debug = 0;
  for (auto const &cell : _triangulation.active_cell_iterators())
  {
    if (cell->is_locally_owned())
    {
      double material_ratio = 0.;
      for (unsigned int i = 0; i < g_n_material_states; ++i)
      {
        material_ratio += state_host_view(i, cell_id_debug);
      }
      ASSERT(std::abs(material_ratio - 1.) < 1e-14, "Material is lost.");
      ++cell_id_debug;
    }
  }
#endif
}

template <int dim, typename MemorySpaceType>
void RefineTransferHelper<dim, MemorySpaceType>::update_thermal_data(
    dealii::parallel::distributed::CellDataTransfer<
        dim, dim, std::vector<std::vector<double>>> &thermal_cell_data_trans)
{
  // Update the AffineConstraints and resize the temperature
  _thermal_physics->setup_dofs();
  _thermal_physics->initialize_dof_vector(_temperature);

  // Interpolate the temperature
  if constexpr (std::is_same_v<MemorySpaceType, dealii::MemorySpace::Host>)
  {
    _temperature_transfer->interpolate(_temperature);
  }
  else
  {
    dealii::LA::distributed::Vector<double, dealii::MemorySpace::Host>
        temperature_host(_temperature.get_partitioner());
    temperature_host.reinit(_temperature.get_partitioner());
    _temperature_transfer->interpolate(temperature_host);
    _temperature.import(temperature_host, dealii::VectorOperation::insert);
  }

  unsigned int constexpr direction_data_size = 2;
  unsigned int constexpr phase_history_data_size = 1;
  unsigned int constexpr data_size =
      direction_data_size + phase_history_data_size;
  std::vector<std::vector<double>> transferred_thermal_data(
      _triangulation.n_active_cells(), std::vector<double>(data_size));
  thermal_cell_data_trans.unpack(transferred_thermal_data);

  std::vector<double> transferred_cos;
  std::vector<double> transferred_sin;
  std::vector<bool> has_melted;
  unsigned int const n_locally_owned_active_cells =
      _triangulation.n_locally_owned_active_cells();
  transferred_cos.reserve(n_locally_owned_active_cells);
  transferred_sin.reserve(n_locally_owned_active_cells);
  has_melted.reserve(n_locally_owned_active_cells);

  unsigned int total_cell_id = 0;
  auto const &dof_handler = _thermal_physics->get_dof_handler();
  for (auto const &cell : dof_handler.active_cell_iterators())
  {
    if (cell->is_locally_owned())
    {
      if (cell->active_fe_index() == 0)
      {
        transferred_cos.push_back(transferred_thermal_data[total_cell_id][0]);
        transferred_sin.push_back(transferred_thermal_data[total_cell_id][1]);

#if ADAMANTINE_DEBUG
        double const squared_cos = transferred_thermal_data[total_cell_id][0] *
                                   transferred_thermal_data[total_cell_id][0];
        double const squared_sin = transferred_thermal_data[total_cell_id][1] *
                                   transferred_thermal_data[total_cell_id][1];
        ASSERT(std::abs(squared_cos + squared_sin - 1.) < 1e-14,
               "Error when transfering sine/cosine on the refined mesh.");
#endif

        // Convert from double back to bool
        if (transferred_thermal_data[total_cell_id][direction_data_size] > 0.5)
          has_melted.push_back(true);
        else
          has_melted.push_back(false);
      }
    }
    ++total_cell_id;
  }

  // Update the deposition cos and sin
  _thermal_physics->set_material_deposition_orientation(transferred_cos,
                                                        transferred_sin);

  // Update the melted indicator
  _thermal_physics->set_has_melted_vector(has_melted);

  // Update the material states in the ThermalOperator
  _thermal_physics->get_state_from_material_properties();
}

template <int dim, typename MemorySpaceType>
void RefineTransferHelper<dim, MemorySpaceType>::update_mechanical_data()
{
  // Create the list of new quadrature points. Here we don't need to worry about
  // points being duplicated. First we create a new DoFHandler to extract the
  // quadrature points. We could reuse the DoFHandler from
  // prepare_mechanical_data but since we need to update the active fe indices
  // and redistribute the dofs, I don't think that we will gain much.
  unsigned int const fe_degree = _mechanical_physics->get_dof_handler()
                                     .get_fe_collection()
                                     .max_dofs_per_cell();
  dealii::FE_Q<dim> fe(dealii::QGauss<1>(fe_degree + 1));
  dealii::hp::FECollection<dim> fe_collection(fe, dealii::FE_Nothing<dim>());
  dealii::DoFHandler<dim> quad_dof_handler(_triangulation);
  for (auto const &tria_cell : _triangulation.active_cell_iterators() |
                                   dealii::IteratorFilters::LocallyOwnedCell())
  {
    dealii::TriaIterator<dealii::DoFCellAccessor<dim, dim, false>> dof_cell(
        &_triangulation, tria_cell->level(), tria_cell->index(),
        &quad_dof_handler);
    // This requires that _material_properties was updated before.
    if (_material_properties.get_state_ratio(tria_cell, MaterialState::solid) >
        0.99)
    {
      dof_cell->set_active_fe_index(0);
    }
    else
    {
      dof_cell->set_active_fe_index(1);
    }
  }
  quad_dof_handler.distribute_dofs(fe_collection);

  // Assemble the quadrature points on the new DoFHandler
  dealii::FEValues<dim, dim> fe_values(fe, fe.get_unit_support_points(),
                                       dealii::update_quadrature_points);
  std::vector<dealii::Point<dim>> quadrature_points;
  for (auto const &cell :
       quad_dof_handler.active_cell_iterators() |
           dealii::IteratorFilters::ActiveFEIndexEqualTo(0, true))
  {
    fe_values.reinit(cell);
    std::vector<dealii::Point<dim>> const &points =
        fe_values.get_quadrature_points();
    quadrature_points.insert(quadrature_points.end(), points.begin(),
                             points.end());
  }

  // Use ArborX to find the nearest neighbor on the old mesh
  dealii::ArborXWrappers::PointNearestPredicate nearest_point(quadrature_points,
                                                              1);
  auto [indices_ranks, offsets] = _distributed_tree->query(nearest_point);

  ASSERT(offsets.size() == quadrature_points.size(),
         "There is an error with ArborX's result.");
#ifdef ADAMANTINE_DEBUG
  std::vector<int> iota(offsets.size());
  std::iota(iota.begin(), iota.end(), 0);
  for (unsigned int i = 0; offsets.size(); ++i)
  {
    ASSERT(offsets[i] == iota[i], "There is an error with ArborX's result.");
  }
#endif

  // TODO: we need to take care of the MPI communication

  // Perform the interpolation
  std::vector<std::vector<double>> new_plastic_internal_variable;
  std::vector<std::vector<dealii::SymmetricTensor<2, dim>>> new_stress;
  std::vector<std::vector<dealii::SymmetricTensor<2, dim>>> new_back_stress;
  for (auto const &cell :
       quad_dof_handler.active_cell_iterators() |
           dealii::IteratorFilters::ActiveFEIndexEqualTo(0, true))
  {
    fe_values.reinit(cell);
    for (auto const q : fe_values.quadrature_point_indices())
    {
      // TODO
    }
  }
}
} // namespace adamantine

INSTANTIATE_DIM_HOST(RefineTransferHelper)
#ifdef ADAMANTINE_HAVE_CUDA
INSTANTIATE_DIM_DEVICE(RefineTransferHelper)
#endif

import PyDealII.Debug as dealii
from operator import add

mm = 1e-3
merge_tol = 0.5*mm

# When creating the mesh, we cannot refine it. If we do, we won't be able to
# reload it.
# Substrate parameters
substrate_corner_bot_left = dealii.Point([0, 0, 0])
substrate_corner_top_right = dealii.Point([154.*mm, 12.7*mm, 76.5*mm])
substrate_cell_size_x = [16*mm, 10*mm] + [102.*mm/50]*50 + [10*mm, 16*mm]
substrate_cell_size_y = [1.35*mm] + [10*mm/5.]*5 + [1.35*mm]
substrate_cell_size_z = [25*mm, 20*mm, 15*mm, 10*mm, 3.5*mm, 3.*mm]

# Wall parameters
wall_offset = [26*mm, 1.35*mm, 76.5*mm]
wall_corner_bot_left = dealii.Point(list(map(add, [0, 0, 0], wall_offset)))
wall_cornert_top_right = dealii.Point(list(map(add, [102*mm, 10*mm, 75*mm], wall_offset)))
wall_cell_size_x = [102*mm/50.]*50
wall_cell_size_y = [10*mm/5.]*5 
wall_cell_size_z = [75*mm/35.]*35

# Build the substrate mesh
substrate_triangulation = dealii.Triangulation('3D')
substrate_triangulation.generate_subdivided_steps_hyper_rectangle([substrate_cell_size_x,
    substrate_cell_size_y, substrate_cell_size_z], substrate_corner_bot_left,
    substrate_corner_top_right)
substrate_triangulation.write('substrate_mesh.vtu', 'vtu')

# Build the wall mesh
wall_triangulation = dealii.Triangulation('3D')
wall_triangulation.generate_subdivided_steps_hyper_rectangle([wall_cell_size_x,
    wall_cell_size_y, wall_cell_size_z], wall_corner_bot_left,
    wall_cornert_top_right)
wall_triangulation.write('wall_mesh.vtu', 'vtu')

# Merge the two meshes
experiment_triangulation = dealii.Triangulation('3D')
experiment_triangulation.merge_triangulations([substrate_triangulation,
        wall_triangulation], merge_tol)

# Set the material ID
for cell in experiment_triangulation.active_cells():
    cell.material_id = 0

# Write the mesh
experiment_triangulation.write('experiment_mesh.vtu', 'vtu')

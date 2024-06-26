/* Copyright (c) 2020 - 2022, the adamantine authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#include <CubeHeatSource.hh>
#include <instantiation.hh>
#include <types.hh>

namespace adamantine
{
template <int dim>
CubeHeatSource<dim>::CubeHeatSource(boost::property_tree::ptree const &database)
    : HeatSource<dim>()
{
  _start_time = database.get<double>("start_time");
  _end_time = database.get<double>("end_time");
  _value = database.get<double>("value");
  _min_point[0] = database.get<double>("min_x");
  _max_point[0] = database.get<double>("max_x");
  _min_point[1] = database.get<double>("min_y");
  _max_point[1] = database.get<double>("max_y");
  if (dim == 3)
  {
    _min_point[2] = database.get<double>("min_z");
    _max_point[2] = database.get<double>("max_z");
  }
}

template <int dim>
void CubeHeatSource<dim>::update_time(double time)
{
  _source_on = ((time > _start_time) && (time < _end_time));
}

template <int dim>
double CubeHeatSource<dim>::value(dealii::Point<dim> const &point,
                                  double const /*height*/) const
{
  if (_source_on)
  {
    bool in_source = true;
    for (int i = 0; i < dim; ++i)
    {
      if ((point[i] < _min_point[i]) || (point[i] > _max_point[i]))
      {
        in_source = false;
        break;
      }
    }

    if (in_source)
      return _value;
  }

  return 0.;
}

template <int dim>
double CubeHeatSource<dim>::get_current_height(double const /*time*/) const
{
  return _max_point[axis<dim>::z];
}

} // namespace adamantine

INSTANTIATE_DIM(CubeHeatSource)

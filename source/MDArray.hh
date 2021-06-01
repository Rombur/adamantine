/* Copyright (c) 2021, the adamantine authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#ifndef MDARRAY_HH
#define MDARRAY_HH

#include <utils.hh>

namespace adamantine
{
template <int dim_0, int dim_1, int dim_2, int dim_3, typename MemorySpaceType>
class MDArray
{
public:
  MDArray();

  ~MDArray();

  ADAMANTINE_HOST_DEV double &operator()(unsigned int i, unsigned int j,
                                         unsigned int k, unsigned int l);

  ADAMANTINE_HOST_DEV double const &operator()(unsigned int i, unsigned int j,
                                               unsigned int k,
                                               unsigned int l) const;

private:
  std::unique_ptr<double[], void (*)(double *)> _values;
};

template <int dim_0, int dim_1, int dim_2, int dim_3, typename MemorySpaceType>
MDArray<dim_0, dim_1, dim_2, dim_3, MemorySpaceType>::MDArray()
    : _values(Memory<double, MemorySpaceType>::allocate_data(dim_0 * dim_1 *
                                                             dim_2 * dim_3))
{
  static_assert(dim_0 > 0, "dim_0 should be greater than 0.");
  static_assert(dim_1 > 0, "dim_1 should be greater than 0.");
  static_assert(dim_2 > 0, "dim_2 should be greater than 0.");
  static_assert(dim_3 > 0, "dim_3 should be greater than 0.");
}

template <int dim_0, int dim_1, int dim_2, int dim_3, typename MemorySpaceType>
MDArray<dim_0, dim_1, dim_2, dim_3, MemorySpaceType>::~MDArray()
{
  Memory<double, MemorySpaceType>::delete_data(_values);
}

template <int dim_0, int dim_1, int dim_2, int dim_3, typename MemorySpaceType>
ADAMANTINE_HOST_DEV double &
MDArray<dim_0, dim_1, dim_2, dim_3, MemorySpaceType>::operator()(unsigned int i,
                                                                 unsigned int j,
                                                                 unsigned int k,
                                                                 unsigned int l)
{
  return _values[i * (dim_1 * dim_2 * dim_3) + j * (dim_2 * dim_3) + k * dim_3 +
                 l];
}

template <int dim_0, int dim_1, int dim_2, int dim_3, typename MemorySpaceType>
ADAMANTINE_HOST_DEV double const &
MDArray<dim_0, dim_1, dim_2, dim_3, MemorySpaceType>::operator()(
    unsigned int i, unsigned int j, unsigned int k, unsigned int l) const
{
  return _values[i * (dim_1 * dim_2 * dim_3) + j * (dim_2 * dim_3) + k * dim_3 +
                 l];
}

template <int dim_0, int dim_1, int dim_2, typename MemorySpaceType>
class MDArray<dim_0, dim_1, dim_2, -1, typename MemorySpaceType>
{
public:
  MDArray(unsigned int size_3);

  ~MDArray();

  ADAMANTINE_HOST_DEV double &operator()(unsigned int i, unsigned int j,
                                         unsigned int k, unsigned int l);

  ADAMANTINE_HOST_DEV double const &operator()(unsigned int i, unsigned int j,
                                               unsigned int k,
                                               unsigned int l) const;

private:
  unsigned int _dim_3;

  std::unique_ptr<double[], void (*)(double *)> _values;
};

template <int dim_0, int dim_1, int dim_2, typename MemorySpaceType>
MDArray<dim_0, dim_1, dim_2, -1, MemorySpaceType>::MDArray(unsigned int size_3)
    : _dim_3(size_3), _values(Memory<double, MemorySpaceType>::allocate_data(
                          dim_0 * dim_1 * dim_2 * _dim_3))
{
  static_assert(dim_0 > 0, "dim_0 should be greater than 0.");
  static_assert(dim_1 > 0, "dim_1 should be greater than 0.");
  static_assert(dim_2 > 0, "dim_2 should be greater than 0.");
  ASSERT(_dim_3 > 0, "dim_3 should be greater than 0.");
}

template <int dim_0, int dim_1, int dim_2, typename MemorySpaceType>
MDArray<dim_0, dim_1, dim_2, -1, MemorySpaceType>::~MDArray()
{
  Memory<double, MemorySpaceType>::delete_data(_values);
}

template <int dim_0, int dim_1, int dim_2, typename MemorySpaceType>
ADAMANTINE_HOST_DEV double &
MDArray<dim_0, dim_1, dim_2, -1, MemorySpaceType>::operator()(unsigned int i,
                                                              unsigned int j,
                                                              unsigned int k,
                                                              unsigned int l)
{
  return _values[i * (dim_1 * dim_2 * _dim_3) + j * (dim_2 * _dim_3) +
                 k * _dim_3 + l];
}

template <int dim_0, int dim_1, int dim_2, typename MemorySpaceType>
ADAMANTINE_HOST_DEV double const &
MDArray<dim_0, dim_1, dim_2, -1, MemorySpaceType>::operator()(
    unsigned int i, unsigned int j, unsigned int k, unsigned int l) const
{
  return _values[i * (dim_1 * dim_2 * _dim_3) + j * (dim_2 * _dim_3) +
                 k * _dim_3 + l];
}

template <int dim_1, int dim_2, typename MemorySpaceType>
class MDArray<-1, dim_1, dim_2, -1, typename MemorySpaceType>
{
public:
  MDArray(unsigned int size_0, unsigned int size_3);

  ~MDArray();

  ADAMANTINE_HOST_DEV double &operator()(unsigned int i, unsigned int j,
                                         unsigned int k, unsigned int l);

  ADAMANTINE_HOST_DEV double const &operator()(unsigned int i, unsigned int j,
                                               unsigned int k,
                                               unsigned int l) const;

private:
  unsigned int _dim_0;
  unsigned int _dim_3;

  double *_values;
};

template <int dim_1, int dim_2, typename MemorySpaceType>
MDArray<-1, dim_1, dim_2, -1, MemorySpaceType>::MDArray(unsigned int size_0,
                                                        unsigned int size_3)
    : _dim_0(size_0), _dim_3(size_3),
      _values(Memory<double, MemorySpaceType>::allocate_data(_dim_0 * dim_1 *
                                                             dim_2 * _dim_3))
{
  ASSERT(_dim_0 > 0, "dim_0 should be greater than 0.");
  static_assert(dim_1 > 0, "dim_1 should be greater than 0.");
  static_assert(dim_2 > 0, "dim_2 should be greater than 0.");
  ASSERT(_dim_3 > 0, "dim_3 should be greater than 0.");
}

template <int dim_1, int dim_2, typename MemorySpaceType>
MDArray<-1, dim_1, dim_2, -1, MemorySpaceType>::~MDArray()
{
  Memory<double, MemorySpaceType>::delete_data(_values);
}

template <int dim_1, int dim_2, typename MemorySpaceType>
ADAMANTINE_HOST_DEV double &
MDArray<-1, dim_1, dim_2, -1, MemorySpaceType>::operator()(unsigned int i,
                                                           unsigned int j,
                                                           unsigned int k,
                                                           unsigned int l)
{
  return _values[i * (dim_1 * dim_2 * _dim_3) + j * (dim_2 * _dim_3) +
                 k * _dim_3 + l];
}

template <int dim_1, int dim_2, typename MemorySpaceType>
ADAMANTINE_HOST_DEV double const &
MDArray<-1, dim_1, dim_2, -1, MemorySpaceType>::operator()(unsigned int i,
                                                           unsigned int j,
                                                           unsigned int k,
                                                           unsigned int l) const
{
  return _values[i * (dim_1 * dim_2 * _dim_3) + j * (dim_2 * _dim_3) +
                 k * _dim_3 + l];
}

} // namespace adamantine

#endif

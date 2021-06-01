/* Copyright (c) 2021, the adamantine authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#define BOOST_TEST_MODULE MDArrayDevice

#include <MDArray.hh>

#include "main.cc"

template <int dim_0, int dim_1, int dim_2, int dim_3>
void fill_mdarrays(
    adamantine::MDArray<dim_0, dim_1, dim_2, dim_3, dealii::MemorySpace::CUDA>
        mdarray_template,
    adamantine::MDArray<dim_0, dim_1, dim_2, -1, dealii::MemorySpace::CUDA>
        mdarray_partial_template_1,
    adamantine::MDArray<-1, dim_1, dim_2, -1, dealii::MemorySpace::CUDA>
        mdarray_partial_template_2)
{
  int id = threadIdx.x + gridDim.x * blockIdx.x;
  if (id < dim_0 * dim_1 * dim_2 * dim_3)
  {
    int i = id / (dim_1 * dim_2 * dim_3);
    int j = (id - i * (dim_1 * dim_2 * dim_3)) / (dim_2 * dim_3);
    int k = (id - i * (dim_1 * dim_2 * dim_3) - j * (dim_2 * dim_3)) / dim_3;
    int l = id - i * (dim_1 * dim_2 * dim_3) - j * (dim_2 * dim_3) - k * dim_3;
    mdarray_template(i, j, k, l) = id;
    mdarray_partial_template_1(i, j, k, l) = id;
    mdarray_partial_template_2(i, j, k, l) = id;
  }
}

template <int dim_0, int dim_1, int dim_2, int dim_3>
void check_mdarrays(
    adamantine::MDArray<dim_0, dim_1, dim_2, dim_3, dealii::MemorySpace::CUDA>
        mdarray_template,
    adamantine::MDArray<dim_0, dim_1, dim_2, -1, dealii::MemorySpace::CUDA>
        mdarray_partial_template_1,
    adamantine::MDArray<-1, dim_1, dim_2, -1, dealii::MemorySpace::CUDA>
        mdarray_partial_template_2,
    int *n_errors)
{
  int id = threadIdx.x + gridDim.x * blockIdx.x;
  if (id < dim_0 * dim_1 * dim_2 * dim_3)
  {
    int i = id / (dim_1 * dim_2 * dim_3);
    int j = (id - i * (dim_1 * dim_2 * dim_3)) / (dim_2 * dim_3);
    int k = (id - i * (dim_1 * dim_2 * dim_3) - j * (dim_2 * dim_3)) / dim_3;
    int l = id - i * (dim_1 * dim_2 * dim_3) - j * (dim_2 * dim_3) - k * dim_3;
    if (mdarray_template(i, j, k, l) != id)
      atomicAdd(&n_errors[0], 1);
    if (mdarray_partial_template_1(i, j, k, l) != id)
      atomicAdd(&n_errors[1], 1);
    if (mdarray_partial_template_2(i, j, k, l) != id)
      atomicAdd(&n_errors[2], 1);
  }
}

BOOST_AUTO_TEST_CASE(mdarray_all_specialization)
{

  int constexpr dim_0 = 2;
  int constexpr dim_1 = 3;
  int constexpr dim_2 = 4;
  int constexpr dim_3 = 5;

  adamantine::MDArray<dim_0, dim_1, dim_2, dim_3, dealii::MemorySpace::CUDA>
      mdarray_template;
  adamantine::MDArray<dim_0, dim_1, dim_2, -1, dealii::MemorySpace::CUDA>
      mdarray_partial_template_1(dim_3);
  adamantine::MDArray<-1, dim_1, dim_2, -1, dealii::MemorySpace::CUDA>
      mdarray_partial_template_2(dim_0, dim_3);

  for (int i = 0; i < dim_0; ++i)
    for (int j = 0; j < dim_1; ++j)
      for (int k = 0; k < dim_2; ++k)
        for (int l = 0; l < dim_3; ++l)
        {
          mdarray_template(i, j, k, l) = i + j + k + l;
          mdarray_partial_template_1(i, j, k, l) = i + j + k + l;
          mdarray_partial_template_2(i, j, k, l) = i + j + k + l;
        }

  for (int i = 0; i < dim_0; ++i)
    for (int j = 0; j < dim_1; ++j)
      for (int k = 0; k < dim_2; ++k)
        for (int l = 0; l < dim_3; ++l)
        {
          BOOST_CHECK(mdarray_template(i, j, k, l) == i + j + k + l);
          BOOST_CHECK(mdarray_partial_template_1(i, j, k, l) == i + j + k + l);
          BOOST_CHECK(mdarray_partial_template_2(i, j, k, l) == i + j + k + l);
        }
}


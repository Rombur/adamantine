/* Copyright (c) 2021, the adamantine authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#define BOOST_TEST_MODULE MDArray

#include <MDArray.hh>

#include "main.cc"

BOOST_AUTO_TEST_CASE(mdarray_all_specialization)
{

  int constexpr dim_0 = 2;
  int constexpr dim_1 = 3;
  int constexpr dim_2 = 4;
  int constexpr dim_3 = 5;

  adamantine::MDArray<dim_0, dim_1, dim_2, dim_3, dealii::MemorySpace::Host>
      mdarray_template;
  adamantine::MDArray<dim_0, dim_1, dim_2, -1, dealii::MemorySpace::Host>
      mdarray_partial_template_1(dim_3);
  adamantine::MDArray<-1, dim_1, dim_2, -1, dealii::MemorySpace::Host>
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

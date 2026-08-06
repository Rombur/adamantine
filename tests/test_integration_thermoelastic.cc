/* SPDX-FileCopyrightText: Copyright (c) 2016 - 2026, the adamantine authors.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "MaterialStates.hh"
#define BOOST_TEST_MODULE Integration_Thermoelastic

#include "../application/adamantine.hh"

#include <boost/property_tree/info_parser.hpp>

#include <filesystem>
#include <fstream>

#include "main.cc"

namespace utf = boost::unit_test;

BOOST_AUTO_TEST_CASE(integration_thermoelastic, *utf::tolerance(1.0e-5))
{
  MPI_Comm communicator = MPI_COMM_WORLD;

  std::vector<adamantine::Timer> timers;
  initialize_timers(communicator, timers);

  // Read the input.
  std::string const filename = "thermoelastic_bare_plate.info";
  adamantine::ASSERT_THROW(std::filesystem::exists(filename) == true,
                           "The file " + filename + " does not exist.");
  boost::property_tree::ptree database;
  boost::property_tree::info_parser::read_info(filename, database);
  database.put("materials.material_0.solid.thermal_expansion_coef", 17.2e-3);

  auto [temperature, displacement] =
      run<3, -1, 3, adamantine::SolidLiquidPowder, dealii::MemorySpace::Host>(
          communicator, database, timers);

  // For now doing a simple regression test. Without a dof handler, it's hard to
  // do something more meaningful with the vector.

  // To generate a new gold solution
  // std::cout << "dis l2:" << displacement.l2_norm() << std::endl;

  BOOST_TEST(displacement.l2_norm() == 0.21537566016824577);
}

BOOST_AUTO_TEST_CASE(integration_thermoelastic_add_material,
                     *utf::tolerance(1.0e-5))
{
  MPI_Comm communicator = MPI_COMM_WORLD;

  std::vector<adamantine::Timer> timers;
  initialize_timers(communicator, timers);

  // Read the input.
  std::string const filename = "thermoelastic_bare_plate.info";
  adamantine::ASSERT_THROW(std::filesystem::exists(filename) == true,
                           "The file " + filename + " does not exist.");
  boost::property_tree::ptree database;
  boost::property_tree::info_parser::read_info(filename, database);
  database.put("geometry.height", 8.0e-3);
  database.put("geometry.height_division", 4);
  database.put("sources.beam_0.scan_path_file",
               "thermoelastic_bare_plate_add_material_scan_path.txt");
  database.put("materials.material_0.solid.thermal_expansion_coef", 17.2e-3);

  auto [temperature, displacement] =
      run<3, 1, 2, adamantine::SolidLiquidPowder, dealii::MemorySpace::Host>(
          communicator, database, timers);

  // For now doing a simple regression test. Without a dof handler, it's hard to
  // do something more meaningful with the vector.

  BOOST_TEST(displacement.l2_norm() == 0.24117708846731);
}

BOOST_AUTO_TEST_CASE(integration_thermomechanical_linearity)
{
  MPI_Comm communicator = MPI_COMM_WORLD;
  std::string const filename = "thermoelastic_bare_plate.info";
  adamantine::ASSERT_THROW(std::filesystem::exists(filename) == true,
                           "The file " + filename + " does not exist.");

  boost::property_tree::ptree base_database;
  boost::property_tree::info_parser::read_info(filename, base_database);
  base_database.put("geometry.material_deposition", false);
  base_database.put("geometry.length_divisions", 4);
  base_database.put("geometry.height_divisions", 2);
  base_database.put("geometry.width_divisions", 2);
  base_database.put("time_stepping.duration", 4.0e-3);
  base_database.put("post_processor.time_steps_between_output", 1000);

  auto run_case = [&](double thermal_expansion, std::string const &suffix)
  {
    auto database = base_database;
    database.put("materials.material_0.solid.thermal_expansion_coef",
                 thermal_expansion);
    database.put("post_processor.filename_prefix",
                 "thermomechanical_linearity_" + suffix);
    std::vector<adamantine::Timer> timers;
    initialize_timers(communicator, timers);
    return run<3, -1, 3, adamantine::SolidLiquidPowder,
               dealii::MemorySpace::Host>(communicator, database, timers);
  };

  // Linear thermoelastic superposition requires identical temperatures and
  // u(2 alpha) = 2 u(alpha). See Y. C. Fung and Pin Tong, Classical and
  // Computational Solid Mechanics, World Scientific, 2001, Chapter 14,
  // DOI: 10.1142/4362.
  // The deliberately large test-only coefficients amplify displacement above
  // roundoff without changing this linear relation.
  auto [temperature_1, displacement_1] = run_case(1.0, "alpha");
  auto [temperature_2, displacement_2] = run_case(2.0, "two_alpha");

  auto temperature_difference = temperature_2;
  temperature_difference -= temperature_1;
  BOOST_CHECK_SMALL(temperature_difference.l2_norm() / temperature_1.l2_norm(),
                    1.e-13);

  double const displacement_norm = displacement_1.l2_norm();
  BOOST_TEST(displacement_norm > 1.e-16);
  auto displacement_difference = displacement_2;
  displacement_difference -= displacement_1;
  displacement_difference -= displacement_1;
  BOOST_CHECK_SMALL(displacement_difference.l2_norm() / displacement_norm,
                    1.e-9);
}

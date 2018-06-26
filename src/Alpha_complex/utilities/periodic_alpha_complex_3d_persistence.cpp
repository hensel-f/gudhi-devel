/*    This file is part of the Gudhi Library. The Gudhi library
 *    (Geometric Understanding in Higher Dimensions) is a generic C++
 *    library for computational topology.
 *
 *    Author(s):       Vincent Rouvreau
 *                     Pawel Dlotko - 2017 - Swansea University, UK
 *
 *    Copyright (C) 2014 Inria
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/program_options.hpp>
#include <boost/variant.hpp>

#include <gudhi/Alpha_complex_3d.h>
#include <gudhi/Alpha_complex_3d_options.h>
#include <gudhi/Simplex_tree.h>
#include <gudhi/Persistent_cohomology.h>
#include <gudhi/Points_3D_off_io.h>

#include <fstream>
#include <cmath>
#include <string>
#include <tuple>
#include <map>
#include <utility>
#include <vector>
#include <cstdlib>

// gudhi type definition
using Alpha_complex_3d = Gudhi::alpha_complex::Alpha_complex_3d<Gudhi::alpha_complex::Periodic_alpha_shapes_3d>;
using Simplex_tree = Gudhi::Simplex_tree<Gudhi::Simplex_tree_options_fast_persistence>;
using Filtration_value = Simplex_tree::Filtration_value;
using Persistent_cohomology =
    Gudhi::persistent_cohomology::Persistent_cohomology<Simplex_tree, Gudhi::persistent_cohomology::Field_Zp>;

void program_options(int argc, char *argv[], std::string &off_file_points, std::string &cuboid_file,
                     std::string &output_file_diag, int &coeff_field_characteristic, Filtration_value &min_persistence);

int main(int argc, char **argv) {
  std::string off_file_points;
  std::string cuboid_file;
  std::string output_file_diag;
  int coeff_field_characteristic;
  Filtration_value min_persistence;

  program_options(argc, argv, off_file_points, cuboid_file, output_file_diag, coeff_field_characteristic,
                  min_persistence);

  // Read the OFF file (input file name given as parameter) and triangulate points
  Gudhi::Points_3D_off_reader<Alpha_complex_3d::Point_3> off_reader(off_file_points);
  // Check the read operation was correct
  if (!off_reader.is_valid()) {
    std::cerr << "Unable to read file " << off_file_points << std::endl;
    exit(-1);
  }

  // Read iso_cuboid_3 information from file
  std::ifstream iso_cuboid_str(cuboid_file);
  double x_min, y_min, z_min, x_max, y_max, z_max;
  if (iso_cuboid_str.good()) {
    iso_cuboid_str >> x_min >> y_min >> z_min >> x_max >> y_max >> z_max;
  } else {
    std::cerr << "Unable to read file " << cuboid_file << std::endl;
    exit(-1);
  }
  // Checking if the cuboid is the same in x,y and z direction. If not, CGAL will not process it.
  if ((x_max - x_min != y_max - y_min) || (x_max - x_min != z_max - z_min) || (z_max - z_min != y_max - y_min)) {
    std::cerr << "The size of the cuboid in every directions is not the same." << std::endl;
    exit(-1);
  }

  Alpha_complex_3d alpha_complex(off_reader.get_point_cloud(), x_min, y_min, z_min, x_max, y_max, z_max);

  Simplex_tree simplex_tree;

  alpha_complex.create_complex(simplex_tree);

  // Sort the simplices in the order of the filtration
  simplex_tree.initialize_filtration();

  std::cout << "Simplex_tree dim: " << simplex_tree.dimension() << std::endl;
  // Compute the persistence diagram of the complex
  Persistent_cohomology pcoh(simplex_tree, true);
  // initializes the coefficient field for homology
  pcoh.init_coefficients(coeff_field_characteristic);

  pcoh.compute_persistent_cohomology(min_persistence);

  // Output the diagram in filediag
  if (output_file_diag.empty()) {
    pcoh.output_diagram();
  } else {
    std::cout << "Result in file: " << output_file_diag << std::endl;
    std::ofstream out(output_file_diag);
    pcoh.output_diagram(out);
    out.close();
  }

  return 0;
}

void program_options(int argc, char *argv[], std::string &off_file_points, std::string &cuboid_file,
                     std::string &output_file_diag, int &coeff_field_characteristic,
                     Filtration_value &min_persistence) {
  namespace po = boost::program_options;
  po::options_description hidden("Hidden options");
  hidden.add_options()("input-file", po::value<std::string>(&off_file_points),
                       "Name of file containing a point set. Format is one point per line:   X1 ... Xd ")(
      "cuboid-file", po::value<std::string>(&cuboid_file),
      "Name of file describing the periodic domain. Format is: min_hx min_hy min_hz\nmax_hx max_hy max_hz");

  po::options_description visible("Allowed options", 100);
  visible.add_options()("help,h", "produce help message")(
      "output-file,o", po::value<std::string>(&output_file_diag)->default_value(std::string()),
      "Name of file in which the persistence diagram is written. Default print in std::cout")(
      "field-charac,p", po::value<int>(&coeff_field_characteristic)->default_value(11),
      "Characteristic p of the coefficient field Z/pZ for computing homology.")(
      "min-persistence,m", po::value<Filtration_value>(&min_persistence),
      "Minimal lifetime of homology feature to be recorded. Default is 0. Enter a negative value to see zero length "
      "intervals");

  po::positional_options_description pos;
  pos.add("input-file", 1);
  pos.add("cuboid-file", 2);

  po::options_description all;
  all.add(visible).add(hidden);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(all).positional(pos).run(), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("input-file") || !vm.count("cuboid-file")) {
    std::cout << std::endl;
    std::cout << "Compute the persistent homology with coefficient field Z/pZ \n";
    std::cout << "of a periodic 3D Alpha complex defined on a set of input points.\n \n";
    std::cout << "The output diagram contains one bar per line, written with the convention: \n";
    std::cout << "   p   dim b d \n";
    std::cout << "where dim is the dimension of the homological feature,\n";
    std::cout << "b and d are respectively the birth and death of the feature and \n";
    std::cout << "p is the characteristic of the field Z/pZ used for homology coefficients." << std::endl << std::endl;

    std::cout << "Usage: " << argv[0] << " [options] input-file cuboid-file" << std::endl << std::endl;
    std::cout << visible << std::endl;
    std::abort();
  }
}

/*    This file is part of the Gudhi Library. The Gudhi library
 *    (Geometric Understanding in Higher Dimensions) is a generic C++
 *    library for computational topology.
 *
 *    Author(s):       Vincent Rouvreau
 *
 *    Copyright (C) 2018 Inria
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

#ifndef ALPHA_COMPLEX_3D_H_
#define ALPHA_COMPLEX_3D_H_

#include <boost/version.hpp>
#include <boost/variant.hpp>

#if BOOST_VERSION >= 105400
#include <boost/container/static_vector.hpp>
#endif

#include <gudhi/Debug_utils.h>
#include <gudhi/Alpha_complex_3d_options.h>

#include <CGAL/Object.h>
#include <CGAL/tuple.h>
#include <CGAL/iterator.h>
#include <CGAL/version.h>

#include <iostream>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstddef>
#include <memory>  // for std::unique_ptr

#if CGAL_VERSION_NR < 1041101000
  // Make compilation fail - required for external projects - https://gitlab.inria.fr/GUDHI/gudhi-devel/issues/10
  static_assert(false,
                "Alpha_complex_3d is only available for CGAL >= 4.11");
#endif

namespace Gudhi {

namespace alpha_complex {

template<typename AlphaComplex3dOptions>
class Alpha_complex_3d {
  using Alpha_shape_3 = typename AlphaComplex3dOptions::Alpha_shape_3;
  using Alpha_value_type = typename Alpha_shape_3::FT;
  using Dispatch =
  CGAL::Dispatch_output_iterator<CGAL::cpp11::tuple<CGAL::Object, Alpha_value_type>,
  CGAL::cpp11::tuple<std::back_insert_iterator<std::vector<CGAL::Object> >,
      std::back_insert_iterator<std::vector<Alpha_value_type> > > >;

  using Cell_handle = typename Alpha_shape_3::Cell_handle;
  using Facet = typename Alpha_shape_3::Facet;
  using Edge = typename Alpha_shape_3::Edge;
  using Alpha_vertex_handle = typename Alpha_shape_3::Vertex_handle;
#if BOOST_VERSION >= 105400
  using Vertex_list = boost::container::static_vector<Alpha_vertex_handle, 4>;
#else
  using Vertex_list = std::vector<Alpha_vertex_handle>;
#endif

public:
  using Point_3 = typename AlphaComplex3dOptions::Point_3;

public:
  /** \brief Alpha_complex constructor from a list of points.
  *
  * Duplicate points are inserted once in the Alpha_complex. This is the reason why the vertices may be not contiguous.
  *
  * @param[in] points Range of points to triangulate. Points must be in AlphaComplex3dOptions::Point_3
  *
  * The type InputPointRange must be a range for which std::begin and
  * std::end return input iterators on a AlphaComplex3dOptions::Point_3.
  */
  template<typename InputPointRange >
  Alpha_complex_3d(const InputPointRange& points) {
    static_assert(!AlphaComplex3dOptions::weighted,
                  "This constructor is not available for weighted versions of Alpha_complex_3d");
    static_assert(!AlphaComplex3dOptions::periodic,
                  "This constructor is not available for periodic versions of Alpha_complex_3d");

    alpha_shape_3_ptr_ = std::unique_ptr<Alpha_shape_3>(new Alpha_shape_3(std::begin(points), std::end(points), 0,
                                                        Alpha_shape_3::GENERAL));
    Dispatch dispatcher = CGAL::dispatch_output<CGAL::Object, Alpha_value_type>(std::back_inserter(objects_),
                                                                                std::back_inserter(alpha_values_));

    alpha_shape_3_ptr_->filtration_with_alpha_values(dispatcher);
#ifdef DEBUG_TRACES
    std::cout << "filtration_with_alpha_values returns : " << objects_.size() << " objects" << std::endl;
#endif  // DEBUG_TRACES

  }

  /** \brief Alpha_complex constructor from a list of points.
*
* Duplicate points are inserted once in the Alpha_complex. This is the reason why the vertices may be not contiguous.
*
* @param[in] points Range of points to triangulate. Points must be in Kernel::Point_d
*
* The type InputPointRange must be a range for which std::begin and
* std::end return input iterators on a Kernel::Point_d.
*/
  template<typename InputPointRange , typename WeightRange>
  Alpha_complex_3d(const InputPointRange& points, WeightRange weights) {
    static_assert(AlphaComplex3dOptions::weighted,
                  "This constructor is not available for non-weighted versions of Alpha_complex_3d");
    static_assert(!AlphaComplex3dOptions::periodic,
                  "This constructor is not available for periodic versions of Alpha_complex_3d");
    GUDHI_CHECK((weights.size() == points.size()),
                std::invalid_argument("Points number in range different from weights range number"));

    using Weighted_point_3 = typename AlphaComplex3dOptions::Weighted_point_3;
    std::vector<Weighted_point_3> weighted_points_3;

    std::size_t index = 0;
    weighted_points_3.reserve(points.size());
    while ((index < weights.size()) && (index < points.size())) {
      weighted_points_3.push_back(Weighted_point_3(points[index], weights[index]));
      index++;
    }

    alpha_shape_3_ptr_ = std::unique_ptr<Alpha_shape_3>(new Alpha_shape_3(std::begin(weighted_points_3),
                                                                          std::end(weighted_points_3),
                                                                          0,
                                                                          Alpha_shape_3::GENERAL));

    Dispatch dispatcher = CGAL::dispatch_output<CGAL::Object, Alpha_value_type>(std::back_inserter(objects_),
                                                                    std::back_inserter(alpha_values_));

    alpha_shape_3_ptr_->filtration_with_alpha_values(dispatcher);
#ifdef DEBUG_TRACES
    std::cout << "filtration_with_alpha_values returns : " << objects_.size() << " objects" << std::endl;
#endif  // DEBUG_TRACES
  }

  /** \brief Alpha_complex constructor from a list of points.
*
* Duplicate points are inserted once in the Alpha_complex. This is the reason why the vertices may be not contiguous.
*
* @param[in] points Range of points to triangulate. Points must be in Kernel::Point_d
*
* The type InputPointRange must be a range for which std::begin and
* std::end return input iterators on a Kernel::Point_d.
*
* @exception std::invalid_argument In case the number of simplices is more than Simplex_key type numeric limit.
*/
  template<typename InputPointRange>
  Alpha_complex_3d(const InputPointRange& points,
                   Alpha_value_type x_min, Alpha_value_type y_min, Alpha_value_type z_min,
                   Alpha_value_type x_max, Alpha_value_type y_max, Alpha_value_type z_max) {
    static_assert(!AlphaComplex3dOptions::weighted,
                  "This constructor is not available for weighted versions of Alpha_complex_3d");
    static_assert(AlphaComplex3dOptions::periodic,
                  "This constructor is not available for non-periodic versions of Alpha_complex_3d");
    // Checking if the cuboid is the same in x,y and z direction. If not, CGAL will not process it.
    GUDHI_CHECK((x_max - x_min == y_max - y_min) ||
                (x_max - x_min == z_max - z_min) ||
                (z_max - z_min == y_max - y_min),
                std::invalid_argument("The size of the cuboid in every directions is not the same."));

    using Periodic_delaunay_triangulation_3 = typename AlphaComplex3dOptions::Periodic_delaunay_triangulation_3;
    using Iso_cuboid_3 = typename AlphaComplex3dOptions::Iso_cuboid_3;
    // Define the periodic cube
    Periodic_delaunay_triangulation_3 pdt(Iso_cuboid_3(x_min, y_min, z_min, x_max, y_max, z_max));
    // Heuristic for inserting large point sets (if pts is reasonably large)
    pdt.insert(std::begin(points), std::end(points), true);
    // As pdt won't be modified anymore switch to 1-sheeted cover if possible
    if (!pdt.is_triangulation_in_1_sheet()) {
      throw std::invalid_argument("Unable to construct a triangulation within a single periodic domain.");
    }
    pdt.convert_to_1_sheeted_covering();

    // alpha shape construction from points. CGAL has a strange behavior in REGULARIZED mode. This is the default mode
    // Maybe need to set it to GENERAL mode
    alpha_shape_3_ptr_ = std::unique_ptr<Alpha_shape_3>(new Alpha_shape_3(pdt, 0,
                                                                          Alpha_shape_3::GENERAL));

    Dispatch dispatcher = CGAL::dispatch_output<CGAL::Object, Alpha_value_type>(std::back_inserter(objects_),
                                                                    std::back_inserter(alpha_values_));

    alpha_shape_3_ptr_->filtration_with_alpha_values(dispatcher);
#ifdef DEBUG_TRACES
    std::cout << "filtration_with_alpha_values returns : " << objects_.size() << " objects" << std::endl;
#endif  // DEBUG_TRACES

  }

  /** \brief Alpha_complex constructor from a list of points.
*
* Duplicate points are inserted once in the Alpha_complex. This is the reason why the vertices may be not contiguous.
*
* @param[in] points Range of points to triangulate. Points must be in Kernel::Point_d
*
* The type InputPointRange must be a range for which std::begin and
* std::end return input iterators on a Kernel::Point_d.
*/
  template<typename InputPointRange , typename WeightRange>
  Alpha_complex_3d(const InputPointRange& points, WeightRange weights,
                   Alpha_value_type x_min, Alpha_value_type y_min, Alpha_value_type z_min,
                   Alpha_value_type x_max, Alpha_value_type y_max, Alpha_value_type z_max) {
    static_assert(AlphaComplex3dOptions::weighted,
                  "This constructor is not available for non-weighted versions of Alpha_complex_3d");
    static_assert(AlphaComplex3dOptions::periodic,
                  "This constructor is not available for non-periodic versions of Alpha_complex_3d");
    GUDHI_CHECK((weights.size() == points.size()),
                std::invalid_argument("Points number in range different from weights range number"));
    // Checking if the cuboid is the same in x,y and z direction. If not, CGAL will not process it.
    GUDHI_CHECK((x_max - x_min == y_max - y_min) ||
                (x_max - x_min == z_max - z_min) ||
                (z_max - z_min == y_max - y_min),
                std::invalid_argument("The size of the cuboid in every directions is not the same."));

    using Weighted_point_3 = typename AlphaComplex3dOptions::Weighted_point_3;
    std::vector<Weighted_point_3> weighted_points_3;

    std::size_t index = 0;
    weighted_points_3.reserve(points.size());

#ifdef GUDHI_DEBUG
    // Defined in GUDHI_DEBUG to avoid unused variable warning for GUDHI_CHECK
    double maximal_possible_weight = 0.015625 * (x_max - x_min) * (x_max - x_min);
#endif

    while ((index < weights.size()) && (index < points.size())) {
      GUDHI_CHECK((weights[index] < maximal_possible_weight) || (weights[index] >= 0),
                  std::invalid_argument("Invalid weight at line" + std::to_string(index + 1) +
                                        ". Must be positive and less than maximal possible weight = 1/64*cuboid length "
                                        "squared, which is not an acceptable input."));
      weighted_points_3.push_back(Weighted_point_3(points[index], weights[index]));
      index++;
    }

    using Periodic_delaunay_triangulation_3 = typename AlphaComplex3dOptions::Periodic_delaunay_triangulation_3;
    using Iso_cuboid_3 = typename AlphaComplex3dOptions::Iso_cuboid_3;
    // Define the periodic cube
    Periodic_delaunay_triangulation_3 pdt(Iso_cuboid_3(x_min, y_min, z_min, x_max, y_max, z_max));
    // Heuristic for inserting large point sets (if pts is reasonably large)
    pdt.insert(std::begin(weighted_points_3), std::end(weighted_points_3), true);
    // As pdt won't be modified anymore switch to 1-sheeted cover if possible
    if (!pdt.is_triangulation_in_1_sheet()) {
      throw std::invalid_argument("Unable to construct a triangulation within a single periodic domain.");
    }
    pdt.convert_to_1_sheeted_covering();

    // alpha shape construction from points. CGAL has a strange behavior in REGULARIZED mode. This is the default mode
    // Maybe need to set it to GENERAL mode
    alpha_shape_3_ptr_ = std::unique_ptr<Alpha_shape_3>(new Alpha_shape_3(pdt, 0,
                                                                          Alpha_shape_3::GENERAL));

    Dispatch dispatcher = CGAL::dispatch_output<CGAL::Object, Alpha_value_type>(std::back_inserter(objects_),
                                                                    std::back_inserter(alpha_values_));

    alpha_shape_3_ptr_->filtration_with_alpha_values(dispatcher);
#ifdef DEBUG_TRACES
    std::cout << "filtration_with_alpha_values returns : " << objects_.size() << " objects" << std::endl;
#endif  // DEBUG_TRACES
  }


  template <typename SimplicialComplexForAlpha3d>
  void create_complex(SimplicialComplexForAlpha3d& complex) {
    using Filtration_value = typename SimplicialComplexForAlpha3d::Filtration_value;
    create_complex(complex, std::numeric_limits<Filtration_value>::infinity());
  }

  /** \brief Inserts all Delaunay triangulation into the simplicial complex.
 * It also computes the filtration values accordingly to the \ref createcomplexalgorithm
 *
 * \tparam SimplicialComplexForAlpha3d must meet `SimplicialComplexForAlpha3d` concept.
 *
 * @param[in] complex SimplicialComplexForAlpha3d to be created.
 * @param[in] max_alpha_square maximum for alpha square value. Default value is +\f$\infty\f$.
 *
 * @return true if creation succeeds, false otherwise.
 *
 * @pre The simplicial complex must be empty (no vertices)
 *
 * Initialization can be launched once.
 */
  template <typename SimplicialComplexForAlpha3d>
  bool create_complex(SimplicialComplexForAlpha3d& complex,
                      typename SimplicialComplexForAlpha3d::Filtration_value max_alpha_square) {
    if (complex.num_vertices() > 0) {
      std::cerr << "Alpha_complex_3d create_complex - complex is not empty\n";
      return false;  // ----- >>
    }

    using Filtration_value = typename SimplicialComplexForAlpha3d::Filtration_value;
    using Complex_vertex_handle = typename SimplicialComplexForAlpha3d::Vertex_handle;
    using Alpha_shape_simplex_tree_map = std::map<Alpha_vertex_handle,
                                                  Complex_vertex_handle>;
    using Simplex_tree_vector_vertex = std::vector<Complex_vertex_handle>;

#ifdef DEBUG_TRACES
    std::size_t count_vertices = 0;
    std::size_t count_edges = 0;
    std::size_t count_facets = 0;
    std::size_t count_cells = 0;
#endif  // DEBUG_TRACES

    Alpha_shape_simplex_tree_map map_cgal_simplex_tree;
    auto the_alpha_value_iterator = alpha_values_.begin();
    for (auto object_iterator : objects_) {
      Vertex_list vertex_list;

      // Retrieve Alpha shape vertex list from object
      if (const Cell_handle *cell = CGAL::object_cast<Cell_handle>(&object_iterator)) {
        for (auto i = 0; i < 4; i++) {
#ifdef DEBUG_TRACES
          std::cout << "from cell[" << i << "]=" << (*cell)->vertex(i)->point() << std::endl;
#endif  // DEBUG_TRACES
          vertex_list.push_back((*cell)->vertex(i));
        }
#ifdef DEBUG_TRACES
        count_cells++;
#endif  // DEBUG_TRACES
      } else if (const Facet *facet = CGAL::object_cast<Facet>(&object_iterator)) {
          for (auto i = 0; i < 4; i++) {
            if ((*facet).second != i) {
#ifdef DEBUG_TRACES
              std::cout << "from facet=[" << i << "]" << (*facet).first->vertex(i)->point() << std::endl;
#endif  // DEBUG_TRACES
              vertex_list.push_back((*facet).first->vertex(i));
            }
          }
#ifdef DEBUG_TRACES
        count_facets++;
#endif  // DEBUG_TRACES
      } else if (const Edge *edge = CGAL::object_cast<Edge>(&object_iterator)) {
            for (auto i : {(*edge).second, (*edge).third}) {
#ifdef DEBUG_TRACES
              std::cout << "from edge[" << i << "]=" << (*edge).first->vertex(i)->point() << std::endl;
#endif  // DEBUG_TRACES
              vertex_list.push_back((*edge).first->vertex(i));
            }
#ifdef DEBUG_TRACES
        count_edges++;
#endif  // DEBUG_TRACES
      } else if (const Alpha_vertex_handle *vertex = CGAL::object_cast<Alpha_vertex_handle>(&object_iterator)) {
#ifdef DEBUG_TRACES
        count_vertices++;
        std::cout << "from vertex=" << (*vertex)->point() << std::endl;
#endif  // DEBUG_TRACES
        vertex_list.push_back((*vertex));
      }
      // Construction of the vector of simplex_tree vertex from list of alpha_shapes vertex
      Simplex_tree_vector_vertex the_simplex;
      for (auto the_alpha_shape_vertex : vertex_list) {
        auto the_map_iterator = map_cgal_simplex_tree.find(the_alpha_shape_vertex);
        if (the_map_iterator == map_cgal_simplex_tree.end()) {
          // alpha shape not found
          Complex_vertex_handle vertex = map_cgal_simplex_tree.size();
#ifdef DEBUG_TRACES
          std::cout << "vertex [" << the_alpha_shape_vertex->point() << "] not found - insert " << vertex << std::endl;
#endif  // DEBUG_TRACES
          the_simplex.push_back(vertex);
          map_cgal_simplex_tree.emplace(the_alpha_shape_vertex, vertex);
        } else {
          // alpha shape found
          Complex_vertex_handle vertex = the_map_iterator->second;
#ifdef DEBUG_TRACES
          std::cout << "vertex [" << the_alpha_shape_vertex->point() << "] found in " << vertex << std::endl;
#endif  // DEBUG_TRACES
          the_simplex.push_back(vertex);
        }
      }
      // Construction of the simplex_tree
      //Alpha_value_type filtr;
      Filtration_value filtr =
        AlphaComplex3dOptions::template value_from_iterator<Filtration_value,
                                                            typename std::vector<Alpha_value_type>::iterator>
                                                              (the_alpha_value_iterator);
#ifdef DEBUG_TRACES
      std::cout << "filtration = " << filtr << std::endl;
#endif  // DEBUG_TRACES
      complex.insert_simplex(the_simplex, static_cast<Filtration_value>(filtr));
      GUDHI_CHECK(the_alpha_value_iterator != alpha_values_.end(), "CGAL provided more simplices than values");
      ++the_alpha_value_iterator;
    }

#ifdef DEBUG_TRACES
    std::cout << "vertices \t" << count_vertices << std::endl;
    std::cout << "edges \t\t" << count_edges << std::endl;
    std::cout << "facets \t\t" << count_facets << std::endl;
    std::cout << "cells \t\t" << count_cells << std::endl;
#endif  // DEBUG_TRACES
    return true;
  }

private:
  // Needs to store alpha_shape_3_ptr_ as objects_ and alpha_shape_3_ptr_ are freed with alpha_shape_3_ptr_
  std::unique_ptr<Alpha_shape_3> alpha_shape_3_ptr_;
  std::vector<CGAL::Object> objects_;
  std::vector<Alpha_value_type> alpha_values_;

};

}  // namespace alpha_complex

}  // namespace Gudhi

#endif  // ALPHA_COMPLEX_3D_H_

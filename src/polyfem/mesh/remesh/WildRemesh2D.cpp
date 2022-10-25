#include "WildRemesh2D.hpp"

#include <polyfem/mesh/mesh2D/CMesh2D.hpp>
#include <polyfem/mesh/remesh/wild_remesh/AMIPSForm.hpp>
#include <polyfem/mesh/remesh/L2Projection.hpp>
#include <polyfem/basis/FEBasis2d.hpp>
#include <polyfem/utils/GeometryUtils.hpp>
#include <polyfem/utils/MatrixUtils.hpp>
#include <polyfem/io/OBJWriter.hpp>

#include <wmtk/utils/TupleUtils.hpp>

#include <igl/boundary_facets.h>
#include <igl/predicates/predicates.h>

#define VERTEX_ATTRIBUTE_GETTER(name, attribute)                                           \
	Eigen::MatrixXd WildRemeshing2D::name() const                                          \
	{                                                                                      \
		Eigen::MatrixXd attributes = Eigen::MatrixXd::Constant(vert_capacity(), DIM, NaN); \
		for (const Tuple &t : get_vertices())                                              \
			attributes.row(t.vid(*this)) = vertex_attrs[t.vid(*this)].attribute;           \
		return attributes;                                                                 \
	}

#define VERTEX_ATTRIBUTE_SETTER(name, attribute)                                 \
	void WildRemeshing2D::name(const Eigen::MatrixXd &attributes)                \
	{                                                                            \
		for (const Tuple &t : get_vertices())                                    \
			vertex_attrs[t.vid(*this)].attribute = attributes.row(t.vid(*this)); \
	}

namespace polyfem::mesh
{
	namespace
	{
		constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

		template <int N>
		double harmonic_mean(const std::array<double, N> &x)
		{
			double inv_sum = 0;
			for (double xi : x)
				inv_sum += 1.0 / xi;
			return N / inv_sum;
		}

		template <int N>
		double root_mean_squared(const std::array<double, N> &x)
		{
			double RMS = 0;
			for (double xi : x)
				RMS += xi * xi;
			return std::sqrt(RMS / N);
		}
	} // namespace

	void WildRemeshing2D::init(
		const Eigen::MatrixXd &rest_positions,
		const Eigen::MatrixXd &positions,
		const Eigen::MatrixXi &triangles,
		const Eigen::MatrixXd &projection_quantities,
		const EdgeMap &edge_to_boundary_id,
		const std::vector<int> &body_ids)
	{
		assert(triangles.size() > 0);

		// --------------------------------------------------------------------
		// Determine which vertices are on the boundary

		Eigen::MatrixXi boundary_edges;
		igl::boundary_facets(triangles, boundary_edges);

		std::vector<bool> is_boundary_vertex(positions.rows(), false);
		for (int i = 0; i < boundary_edges.rows(); ++i)
		{
			is_boundary_vertex[boundary_edges(i, 0)] = true;
			is_boundary_vertex[boundary_edges(i, 1)] = true;
		}

		// --------------------------------------------------------------------

		// Register attributes
		p_vertex_attrs = &vertex_attrs;
		p_edge_attrs = &edge_attrs;
		p_face_attrs = &face_attrs;

		// Convert from eigen to internal representation
		std::vector<std::array<size_t, 3>> tri(triangles.rows());
		for (int i = 0; i < triangles.rows(); i++)
			for (int j = 0; j < 3; j++)
				tri[i][j] = (size_t)triangles(i, j);

		// Initialize the trimesh class which handles connectivity
		wmtk::TriMesh::create_mesh(positions.rows(), tri);

		// Save the vertex position in the vertex attributes
		set_rest_positions(rest_positions);
		set_positions(positions);
		set_projected_quantities(projection_quantities);
		set_fixed(is_boundary_vertex);

		set_boundary_ids(edge_to_boundary_id);
		set_body_ids(body_ids);
	}

	// ========================================================================
	// Getters

	VERTEX_ATTRIBUTE_GETTER(rest_positions, rest_position)
	VERTEX_ATTRIBUTE_GETTER(positions, position)
	VERTEX_ATTRIBUTE_GETTER(displacements, displacement())

	Eigen::MatrixXi WildRemeshing2D::edges() const
	{
		const std::vector<Tuple> edges = get_edges();
		Eigen::MatrixXi E = Eigen::MatrixXi::Constant(edges.size(), 2, -1);
		for (int i = 0; i < edges.size(); ++i)
		{
			const Tuple &e = edges[i];
			E(i, 0) = e.vid(*this);
			E(i, 1) = e.switch_vertex(*this).vid(*this);
		}
		return E;
	}

	Eigen::MatrixXi WildRemeshing2D::triangles() const
	{
		const std::vector<Tuple> faces = get_faces();
		Eigen::MatrixXi triangles = Eigen::MatrixXi::Constant(faces.size(), 3, -1);
		for (size_t i = 0; i < faces.size(); i++)
		{
			const Tuple &t = faces[i];
			const std::array<Tuple, 3> vs = oriented_tri_vertices(t);
			for (int j = 0; j < 3; j++)
			{
				triangles(i, j) = vs[j].vid(*this);
			}
		}
		return triangles;
	}

	Eigen::MatrixXd WildRemeshing2D::projected_quantities() const
	{
		Eigen::MatrixXd projected_quantities =
			Eigen::MatrixXd::Constant(DIM * vert_capacity(), n_quantities, NaN);

		for (const Tuple &t : get_vertices())
		{
			const int vi = t.vid(*this);
			projected_quantities.middleRows<DIM>(DIM * vi) = vertex_attrs[vi].projection_quantities;
		}

		return projected_quantities;
	}

	WildRemeshing2D::EdgeMap WildRemeshing2D::boundary_ids() const
	{
		const std::vector<Tuple> edges = get_edges();
		EdgeMap boundary_ids;
		for (int i = 0; i < edges.size(); ++i)
		{
			size_t e0 = edges[i].vid(*this);
			size_t e1 = edges[i].switch_vertex(*this).vid(*this);
			if (e1 < e0)
				std::swap(e0, e1);
			boundary_ids[std::make_pair(e0, e1)] = edge_attrs[edges[i].eid(*this)].boundary_id;
		}
		return boundary_ids;
	}

	std::vector<int> WildRemeshing2D::body_ids() const
	{
		const std::vector<Tuple> faces = get_faces();
		std::vector<int> body_ids(faces.size(), -1);
		for (size_t i = 0; i < faces.size(); i++)
		{
			const Tuple &t = faces[i];
			body_ids[i] = face_attrs[t.fid(*this)].body_id;
		}
		return body_ids;
	}

	// ========================================================================
	// Setters

	VERTEX_ATTRIBUTE_SETTER(set_rest_positions, rest_position)
	VERTEX_ATTRIBUTE_SETTER(set_positions, position)

	void WildRemeshing2D::set_fixed(const std::vector<bool> &fixed)
	{
		assert(fixed.size() == vert_capacity());
		for (const Tuple &t : get_vertices())
			vertex_attrs[t.vid(*this)].fixed = fixed[t.vid(*this)];
	}

	void WildRemeshing2D::set_projected_quantities(const Eigen::MatrixXd &projected_quantities)
	{
		assert(projected_quantities.rows() == DIM * vert_capacity());
		n_quantities = projected_quantities.cols();

		for (const Tuple &t : get_vertices())
		{
			const int vi = t.vid(*this);
			vertex_attrs[vi].projection_quantities =
				projected_quantities.middleRows<DIM>(DIM * vi);
		}
	}

	void WildRemeshing2D::set_boundary_ids(const EdgeMap &edge_to_boundary_id)
	{
		for (const Tuple &e : get_edges())
		{
			size_t e0 = e.vid(*this);
			size_t e1 = e.switch_vertex(*this).vid(*this);
			if (e1 < e0)
				std::swap(e0, e1);
			edge_attrs[e.eid(*this)].boundary_id = edge_to_boundary_id.at(std::make_pair(e0, e1));
		}
	}

	void WildRemeshing2D::set_body_ids(const std::vector<int> &body_ids)
	{
		for (const Tuple &f : get_faces())
		{
			face_attrs[f.fid(*this)].body_id = body_ids.at(f.fid(*this));
		}
	}

	// ========================================================================

	void WildRemeshing2D::write_obj(const std::string &path, bool deformed) const
	{
		io::OBJWriter::write(path, deformed ? positions() : rest_positions(), triangles());
	}

	double WildRemeshing2D::compute_global_energy() const
	{
		double energy = 0;
		for (const Tuple &t : get_faces())
		{
			// Global ids of the vertices of the triangle
			const std::array<size_t, 3> its = super::oriented_tri_vids(t);

			const double area = utils::triangle_area_2D(
				vertex_attrs[its[0]].rest_position,
				vertex_attrs[its[1]].rest_position,
				vertex_attrs[its[2]].rest_position);
			assert(area > 0);

			const double AMIPS_energy = solver::AMIPSForm::energy(
				vertex_attrs[its[0]].rest_position,
				vertex_attrs[its[1]].rest_position,
				vertex_attrs[its[2]].rest_position,
				vertex_attrs[its[0]].position,
				vertex_attrs[its[1]].position,
				vertex_attrs[its[2]].position);
			assert(energy >= 0);

			energy += area * AMIPS_energy;
		}
		assert(energy >= 0);
		return energy;
	}

	double WildRemeshing2D::compute_global_wicke_measure() const
	{
		double measure = 0;
		for (const Tuple &t : get_faces())
		{
			// Global ids of the vertices of the triangle
			const std::array<size_t, 3> its = super::oriented_tri_vids(t);

			const std::array<double, 3> edge_lengths{{
				(vertex_attrs[its[1]].position - vertex_attrs[its[0]].position).norm(),
				(vertex_attrs[its[2]].position - vertex_attrs[its[1]].position).norm(),
				(vertex_attrs[its[0]].position - vertex_attrs[its[2]].position).norm(),
			}};

			const double rest_area = utils::triangle_area_2D(
				vertex_attrs[its[0]].rest_position,
				vertex_attrs[its[1]].rest_position,
				vertex_attrs[its[2]].rest_position);

			const double area = utils::triangle_area_2D(
				vertex_attrs[its[0]].position,
				vertex_attrs[its[1]].position,
				vertex_attrs[its[2]].position);

			measure += rest_area * (4 / std::sqrt(3) * area * harmonic_mean<3>(edge_lengths) / std::pow(root_mean_squared<3>(edge_lengths), 3));
		}
		return measure;
	}

	bool WildRemeshing2D::is_inverted(const Tuple &loc) const
	{
		// Get the vertices ids
		const std::array<Tuple, 3> vs = oriented_tri_vertices(loc);

		igl::predicates::exactinit();

		// Use igl for checking orientation
		igl::predicates::Orientation res = igl::predicates::orient2d(
			vertex_attrs[vs[0].vid(*this)].rest_position,
			vertex_attrs[vs[1].vid(*this)].rest_position,
			vertex_attrs[vs[2].vid(*this)].rest_position);

		// The element is inverted if it not positive (i.e. it is negative or it is degenerate)
		return (res != igl::predicates::Orientation::POSITIVE);
	}

	bool WildRemeshing2D::invariants(const std::vector<Tuple> &new_tris)
	{
		for (auto &t : new_tris)
		{
			if (is_inverted(t))
			{
				return false;
			}
		}
		return true;
	}

	std::vector<int> WildRemeshing2D::boundary_nodes() const
	{
		std::vector<int> boundary_nodes;
		for (const Tuple &t : get_vertices())
			if (vertex_attrs[t.vid(*this)].fixed)
				boundary_nodes.push_back(t.vid(*this));
		return boundary_nodes;
	}

	void WildRemeshing2D::cache_before()
	{
		rest_positions_before = rest_positions();
		positions_before = positions();
		projected_quantities_before = projected_quantities();
		triangles_before = triangles();
		energy_before = compute_global_energy();
		write_rest_obj("rest_mesh_before.obj");
		write_deformed_obj("deformed_mesh_before.obj");
	}

	WildRemeshing2D::EdgeCache::EdgeCache(const WildRemeshing2D &m, const Tuple &t)
	{
		v0 = m.vertex_attrs[t.vid(m)];
		v1 = m.vertex_attrs[t.switch_vertex(m).vid(m)];

		edges = {{
			m.edge_attrs[t.eid(m)],
			m.edge_attrs[t.switch_vertex(m).switch_edge(m).eid(m)],
			m.edge_attrs[t.switch_edge(m).eid(m)],
		}};

		faces = {{
			m.face_attrs[t.fid(m)],
		}};

		if (t.switch_face(m))
		{
			const Tuple t1 = t.switch_face(m).value();
			edges.push_back(m.edge_attrs[t1.switch_edge(m).eid(m)]);
			edges.push_back(m.edge_attrs[t1.switch_vertex(m).switch_edge(m).eid(m)]);
			faces.push_back(m.face_attrs[t1.fid(m)]);
		}
	}

	int WildRemeshing2D::build_bases(
		const Eigen::MatrixXd &V,
		const Eigen::MatrixXi &F,
		const std::string &assembler_formulation,
		std::vector<polyfem::basis::ElementBases> &bases,
		Eigen::VectorXi &vertex_to_basis)
	{
		using namespace polyfem::basis;

		CMesh2D mesh;
		mesh.build_from_matrices(V, F);
		std::vector<LocalBoundary> local_boundary;
		std::map<int, basis::InterfaceData> poly_edge_to_data;
		std::shared_ptr<mesh::MeshNodes> mesh_nodes;
		const int n_bases = FEBasis2d::build_bases(
			mesh,
			assembler_formulation,
			/*quadrature_order=*/1,
			/*mass_quadrature_order=*/2,
			/*discr_order=*/1,
			/*serendipity=*/false,
			/*has_polys=*/false,
			/*is_geom_bases=*/false,
			bases,
			local_boundary,
			poly_edge_to_data,
			mesh_nodes);

		vertex_to_basis.setConstant(V.rows(), -1);
		for (const ElementBases &elm : bases)
		{
			for (const Basis &basis : elm.bases)
			{
				assert(basis.global().size() == 1);
				const int basis_id = basis.global()[0].index;
				const RowVectorNd v = basis.global()[0].node;

				for (int i = 0; i < V.rows(); i++)
				{
					// if ((V.row(i) - v).norm() < 1e-14)
					if ((V.row(i).array() == v.array()).all())
					{
						if (vertex_to_basis[i] == -1)
							vertex_to_basis[i] = basis_id;
						assert(vertex_to_basis[i] == basis_id);
						break;
					}
				}
			}
		}

		return n_bases;
	}

	void WildRemeshing2D::update_positions()
	{
		// Assume the rest positions and triangles have been updated
		const Eigen::MatrixXd proposed_rest_positions = rest_positions();
		const Eigen::MatrixXi proposed_triangles = triangles();

		// --------------------------------------------------------------------

		// Assume isoparametric
		std::vector<polyfem::basis::ElementBases> bases_before;
		Eigen::VectorXi vertex_to_basis_before;
		int n_bases_before = build_bases(
			rest_positions_before, triangles_before, assembler_formulation,
			bases_before, vertex_to_basis_before);
		const std::vector<polyfem::basis::ElementBases> &geom_bases_before = bases_before;
		n_bases_before += obstacle.n_vertices();

		// Old values of independent variables
		Eigen::MatrixXd y(n_bases_before * DIM, 1 + n_quantities);
		y.col(0) = utils::flatten(utils::reorder_matrix(
			positions_before - rest_positions_before, vertex_to_basis_before, n_bases_before));
		y.rightCols(n_quantities) = utils::reorder_matrix(
			projected_quantities_before, vertex_to_basis_before, n_bases_before, DIM);

		// --------------------------------------------------------------------

		const int num_vertices = proposed_rest_positions.rows();

		std::vector<polyfem::basis::ElementBases> bases;
		Eigen::VectorXi vertex_to_basis;
		int n_bases = build_bases(
			proposed_rest_positions, proposed_triangles, assembler_formulation,
			bases, vertex_to_basis);
		const std::vector<polyfem::basis::ElementBases> &geom_bases = bases;
		n_bases += obstacle.n_vertices();

		Eigen::MatrixXd target_x = Eigen::MatrixXd::Zero(n_bases, DIM);
		for (int i = 0; i < num_vertices; i++)
		{
			const int j = vertex_to_basis[i];
			if (j < 0)
				continue;
			target_x.row(j) = vertex_attrs[i].displacement().transpose();
		}
		target_x = utils::flatten(target_x);

		std::vector<int> boundary_nodes = this->boundary_nodes();
		for (int &boundary_node : boundary_nodes)
			boundary_node = vertex_to_basis[boundary_node];
		std::sort(boundary_nodes.begin(), boundary_nodes.end());

		// --------------------------------------------------------------------

		// L2 Projection
		Eigen::MatrixXd x;
		L2_projection(
			/*is_volume=*/DIM == 3, /*size=*/DIM,
			n_bases_before, bases_before, geom_bases_before, // from
			n_bases, bases, geom_bases,                      // to
			boundary_nodes, obstacle, target_x,
			y, x, /*lump_mass_matrix=*/false);

		// --------------------------------------------------------------------

		set_positions(proposed_rest_positions + utils::unreorder_matrix( //
						  utils::unflatten(x.col(0), DIM), vertex_to_basis, num_vertices));
		set_projected_quantities(utils::unreorder_matrix(
			x.rightCols(n_quantities), vertex_to_basis, num_vertices, DIM));

		write_rest_obj("proposed_rest_mesh.obj");
		write_deformed_obj("proposed_deformed_mesh.obj");
	}

	std::vector<WildRemeshing2D::Tuple> WildRemeshing2D::new_edges_after(
		const std::vector<Tuple> &tris) const
	{
		std::vector<Tuple> new_edges;

		for (auto t : tris)
		{
			for (auto j = 0; j < 3; j++)
			{
				new_edges.push_back(tuple_from_edge(t.fid(*this), j));
			}
		}
		wmtk::unique_edge_tuples(*this, new_edges);
		return new_edges;
	}

} // namespace polyfem::mesh
#include <geogram/basic/attributes.h>
#include <geogram/basic/common.h>
#include <geogram/basic/geometry.h>
#include <geogram/delaunay/delaunay.h>
#include <geogram/mesh/mesh.h>
#include <geogram/mesh/mesh_io.h>
#include <geogram/mesh/mesh_repair.h>
#include <geogram/voronoi/RVD.h>
#include <geogram/voronoi/RVD_callback.h>
#include <geogram/voronoi/RVD_mesh_builder.h>
#include <cassert>

namespace {

using Vertex = std::array<double, 3>;
using Edge = std::array<int, 2>;

// ----------------------------------------------------------------------------

Edge min_max(int a, int b) { return (a < b ? Edge({{a, b}}) : Edge({{b, a}})); };

template <typename T>
void filterDuplicates(std::vector<T>& edges)
{
    std::sort(edges.begin(), edges.end());
    auto it = std::unique(edges.begin(), edges.end());
    edges.resize(std::distance(edges.begin(), it));
    edges.shrink_to_fit();
}

void filterUnreferenced(std::vector<Vertex>& vertices, std::vector<Edge>& edges)
{
    std::vector<Vertex> newVtx;
    std::vector<int> newId(vertices.size(), -1);
    newVtx.reserve(vertices.size());
    int cnt = 0;
    for (auto& edge : edges) {
        for (auto& x : edge) {
            if (newId[x] < 0) {
                newId[x] = cnt++;
                newVtx.push_back(vertices[x]);
            }
            x = newId[x];
        }
    }
    newVtx.shrink_to_fit();
    std::swap(newVtx, vertices);
}

// ----------------------------------------------------------------------------

void voronoiDiagramEdges(const GEO::Mesh& seeds,
                         std::vector<Vertex>& vertices,
                         std::vector<Edge>& edges)
{
    using namespace GEO;

    Delaunay_var delaunay = Delaunay::create(3, "BDEL");
    delaunay->set_vertices(seeds.vertices.nb(), seeds.vertices.point_ptr(0));

    // Retrieve barycenters
    vertices.clear();
    vertices.reserve(delaunay->nb_cells());
    for (index_t c = 0; c < delaunay->nb_cells(); ++c) {
        assert(delaunay->cell_is_finite(c));
        GEO::vec3 v1(delaunay->vertex_ptr(delaunay->cell_vertex(c, 0)));
        GEO::vec3 v2(delaunay->vertex_ptr(delaunay->cell_vertex(c, 1)));
        GEO::vec3 v3(delaunay->vertex_ptr(delaunay->cell_vertex(c, 2)));
        GEO::vec3 v4(delaunay->vertex_ptr(delaunay->cell_vertex(c, 3)));
        GEO::vec3 pt = Geom::tetra_circum_center(v1, v2, v3, v4);
        vertices.push_back({{pt[0], pt[1], pt[2]}});
    }

    edges.clear();
    for (index_t c1 = 0; c1 < delaunay->nb_cells(); ++c1) {
        for (index_t lf = 0; lf < delaunay->cell_size(); ++lf) {
            auto c2 = delaunay->cell_adjacent(c1, lf);
            if (c2 >= 0 && c1 < index_t(c2)) {
                edges.push_back(min_max(c1, c2));
            }
        }
    }
    filterDuplicates(edges);
    filterUnreferenced(vertices, edges);
}

}  // anonymous namespace

// ============================================================================

void voronoi_lattice(const GEO::Mesh& points, GEO::Mesh& lattice)
{
    if (points.vertices.nb() < 4) {
        std::cout << "Cannot create a Voronoi lattice with < 4 points." << std::endl;
        return;
    }

    std::vector<Vertex> vertices;
    std::vector<Edge> edges;
    voronoiDiagramEdges(points, vertices, edges);

    lattice.clear();
    lattice.vertices.create_vertices(vertices.size());
    for (size_t v = 0; v < vertices.size(); ++v) {
        lattice.vertices.point(v) = GEO::vec3(vertices[v].data());
    }
    lattice.edges.create_edges(edges.size());
    for (size_t e = 0; e < edges.size(); ++e) {
        for (size_t lv = 0; lv < 2; ++lv) {
            lattice.edges.set_vertex(e, lv, edges[e][lv]);
        }
    }
}

#pragma once

#include "../Renderer/Vertex.h"

#include <cstdint>
#include <utility>
#include <vector>

using Edge = std::pair<std::uint32_t, std::uint32_t>;

struct MeshData {
	std::vector<Vec3> positions;
	std::vector<Vec3> normals;
	std::vector<Vec2> tex_coords;
	std::vector<Color3> colors;

	std::vector<Edge> edges;
	std::vector<std::uint32_t> tri_indices;

	std::vector<Vertex> render_vertices;
	std::vector<std::uint32_t> render_indices;

	bool has_normals = false;
	bool has_tex_coords = false;
	bool has_colors = false;
};
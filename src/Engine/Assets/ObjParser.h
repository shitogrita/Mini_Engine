#pragma once

#include "MeshData.h"

#include <string>

class ObjParser {
public:
	static bool Parse(const std::string& filename, MeshData& mesh_data);
};
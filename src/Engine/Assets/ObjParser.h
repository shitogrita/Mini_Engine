#pragma once

#include "ImportedMeshData.h"

#include <string>

class ObjParser {
public:
	static bool Parse(const std::string& filename, ImportedMeshData& mesh_data);
};
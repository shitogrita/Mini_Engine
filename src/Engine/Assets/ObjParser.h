#pragma once

#include "Engine/Assets/ImportedMeshData.h"
#include "Engine/Assets/ImportedModelData.h"

#include <string>

class ObjParser {
public:
	static bool Parse(const std::string& filename, ImportedMeshData& mesh_data);
	static bool Parse(const std::string& filename, ImportedModelData& model_data);
};
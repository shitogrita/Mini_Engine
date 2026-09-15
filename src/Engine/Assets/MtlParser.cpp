#include "MtlParser.h"

#include <fstream>
#include <sstream>
#include <stdexcept>


std::vector<ImportedMaterialData> MtlParser::Parse(const std::filesystem::path& path) {
	std::ifstream file(path);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open MTL file: " + path.string());
	}

	std::vector<ImportedMaterialData> materials;

	ImportedMaterialData* current_material = nullptr;

	std::string line;

	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') {
			continue;
		}

		std::istringstream stream(line);

		std::string command;
		stream >> command;

		if (command == "newmtl") {
			materials.emplace_back();

			current_material = &materials.back();

			stream >> current_material->name;
		}
		else if (command == "Kd" && current_material != nullptr) {
			stream
				>> current_material->diffuse_color.x
				>> current_material->diffuse_color.y
				>> current_material->diffuse_color.z;
		}
		else if (command == "Ns" && current_material != nullptr) {
			stream >> current_material->shininess;
		}
		else if (command == "map_Kd" && current_material != nullptr) {
			std::string texture_path;

			stream >> texture_path;

			current_material->diffuse_texture_path =
				path.parent_path() / texture_path;
		}
	}

	return materials;
}
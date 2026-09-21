#include "Engine/Assets/MtlParser.h"

#include <fstream>
#include <sstream>
#include <string>

std::vector<ImportedMaterialData> MtlParser::Parse(const std::filesystem::path& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        return {};
    }

    std::vector<ImportedMaterialData> materials;

    /*
     * Указатель на материал, который сейчас читается.
     *
     * До первого newmtl текущего материала ещё нет.
     */
    ImportedMaterialData* current_material = nullptr;

    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);

        std::string command;
        stream >> command;

        if (command.empty() || command[0] == '#') {
            continue;
        }

        /*
         * newmtl создаёт новый материал.
         *
         * Например:
         * newmtl Wood
         */
        if (command == "newmtl") {
            std::string material_name;
            stream >> material_name;

            if (material_name.empty()) {
                continue;
            }

            materials.emplace_back();
            current_material = &materials.back();
            current_material->name = material_name;

            continue;
        }

        /*
         * Остальные свойства не имеют смысла,
         * пока не был объявлен материал через newmtl.
         */
        if (current_material == nullptr) {
            continue;
        }

        /*
         * Kd — диффузный цвет материала.
         *
         * Например:
         * Kd 0.8 0.5 0.2 - RGB
         */
        if (command == "Kd") {
            stream >> current_material->diffuse_color.x
                   >> current_material->diffuse_color.y
                   >> current_material->diffuse_color.z;

            continue;
        }

        /*
         * Ns — степень зеркального блика.
         *
         * Например:
         * Ns 64
         */
        if (command == "Ns") {
            stream >> current_material->shininess;
            continue;
        }

        /*
         * map_Kd — путь к диффузной текстуре.
         *
         * Путь считается относительно директории MTL-файла.
         *
         * Например:
         * Models/house/house.mtl
         * map_Kd textures/brick.png
         *
         * Получим:
         * Models/house/textures/brick.png
         */
        if (command == "map_Kd") {
            std::string texture_path;
            stream >> texture_path;

            if (!texture_path.empty()) {
                current_material->diffuse_texture_path =
                    (path.parent_path() / texture_path).lexically_normal();
            }

            continue;
        }
    }

    return materials;
}
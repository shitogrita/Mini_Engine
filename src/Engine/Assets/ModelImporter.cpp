#include "Engine/Assets/ModelImporter.h"

#include "Engine/Assets/ImportedMaterialData.h"
#include "Engine/Assets/ImportedModelData.h"
#include "Engine/Assets/ObjParser.h"
#include "Engine/Renderer/Material.h"
#include "Engine/Renderer/Mesh.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Ищет материал по имени среди материалов,
 * прочитанных из MTL-файла.
 *
 * @param materials Материалы импортированной модели.
 * @param name Имя материала из команды usemtl.
 * @return Указатель на найденный материал или nullptr.
 */
static const ImportedMaterialData* FindMaterial(const std::vector<ImportedMaterialData>& materials, const std::string& name) {
    const auto iterator = std::find_if(
        materials.begin(),
        materials.end(),
        [&name](const ImportedMaterialData& material) {
            return material.name == name;
        }
    );

    if (iterator == materials.end()) {
        return nullptr;
    }

    return &(*iterator);
}

/**
 * @brief Преобразует данные материала из MTL
 * в Material движка.
 *
 * Kd используется как базовый цвет.
 * Ns используется как shininess.
 * map_Kd загружается через TextureManager.
 */
static Material CreateMaterial(const ImportedMaterialData* imported_material, TextureManager& texture_manager) {
    Material material;

    if (imported_material == nullptr) {
        return material;
    }

    material.SetColor(imported_material->diffuse_color);
    material.SetShininess(imported_material->shininess);

    if (!imported_material->diffuse_texture_path.empty()) {
        std::shared_ptr<Texture2D> texture = texture_manager.Load(
            imported_material->diffuse_texture_path
        );

        if (texture) {
            material.SetDiffuseTexture(std::move(texture));
            material.SetDiffuseTexturePath(imported_material->diffuse_texture_path);
        }
    }

    return material;
}

std::vector<std::shared_ptr<SceneObject>> ModelImporter::ImportObj(const std::filesystem::path& path, TextureManager& texture_manager) {
    ImportedModelData model_data;

    if (!ObjParser::Parse(path.string(), model_data)) {
        return {};
    }

    auto object = std::make_shared<SceneObject>(path.stem().string());

    object->SetType(SceneObject::Type::ImportedModel);
    object->SetSourcePath(path);

    std::vector<Vec3> bounding_points;

    for (const ImportedMeshPart& part : model_data.meshes) {
        if (part.mesh.render_vertices.empty() || part.mesh.render_indices.empty()) {
            continue;
        }

        auto mesh = std::make_shared<Mesh>(part.mesh);

        const ImportedMaterialData* imported_material = FindMaterial(
            model_data.materials,
            part.material_name
        );

        Material material = CreateMaterial(
            imported_material,
            texture_manager
        );

        object->AddRenderPart(
            part.material_name.empty()
                ? "Material"
                : part.material_name,
            std::move(mesh),
            std::move(material)
        );

        for (const Vertex& vertex : part.mesh.render_vertices) {
            bounding_points.push_back(vertex.position);
        }
    }

    if (!bounding_points.empty()) {
        object->SetBoundingBox(
            BoundingBox::FromPoints(bounding_points)
        );
    }

    if (!object->HasMesh()) {
        return {};
    }

    return {object};
}
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
static const ImportedMaterialData* FindMaterial(
    const std::vector<ImportedMaterialData>& materials,
    const std::string& name) {

    const auto iterator = std::find_if(
        materials.begin(),
        materials.end(),
        [&name](const ImportedMaterialData& material) {
            return material.name == name;
        });

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
static Material CreateMaterial(
    const ImportedMaterialData* imported_material,
    TextureManager& texture_manager) {

    Material material;

    // Если материал отсутствует, оставляем стандартный Material.
    if (imported_material == nullptr) {
        return material;
    }

    // Kd — диффузный цвет материала.
    material.SetColor(imported_material->diffuse_color);

    // Ns — степень зеркального блика.
    material.SetShininess(imported_material->shininess);

    // map_Kd — диффузная текстура материала.
    if (!imported_material->diffuse_texture_path.empty()) {
        material.SetDiffuseTexture(
            texture_manager.Load(imported_material->diffuse_texture_path)
        );
    }

    return material;
}

/**
 * @brief Создаёт BoundingBox для конкретной части модели.
 *
 * Используются render_vertices, потому что они содержат
 * только вершины конкретного ImportedMeshPart.
 *
 * Это важно для моделей с несколькими материалами:
 * каждая часть модели получает собственный BoundingBox,
 * а не BoundingBox всей OBJ-модели.
 */
static BoundingBox CreateBoundingBox(const ImportedMeshData& mesh_data) {
    std::vector<Vec3> points;
    points.reserve(mesh_data.render_vertices.size());

    for (const Vertex& vertex : mesh_data.render_vertices) {
        points.push_back(vertex.position);
    }

    return BoundingBox::FromPoints(points);
}

std::vector<std::shared_ptr<SceneObject>> ModelImporter::ImportObj(
    const std::filesystem::path& path,
    TextureManager& texture_manager) {

    /*
     * ObjParser читает OBJ и связанный с ним MTL.
     *
     * На этом этапе данные ещё находятся на CPU,
     * OpenGL-ресурсы Mesh ещё не созданы.
     */
    ImportedModelData model_data;

    if (!ObjParser::Parse(path.string(), model_data)) {
        return {};
    }

    std::vector<std::shared_ptr<SceneObject>> objects;
    objects.reserve(model_data.meshes.size());

    // Например Models/house.obj -> house.
    const std::string model_name = path.stem().string();

    for (std::size_t i = 0; i < model_data.meshes.size(); ++i) {
        const ImportedMeshPart& part = model_data.meshes[i];

        /*
         * Пустую часть модели создавать не нужно.
         */
        if (part.mesh.render_vertices.empty() ||
            part.mesh.render_indices.empty()) {
            continue;
        }

        /*
         * ImportedMeshData превращается в настоящий Mesh.
         *
         * В конструкторе Mesh создаются OpenGL-ресурсы:
         * VAO, VBO и EBO.
         */
        auto mesh = std::make_shared<Mesh>(part.mesh);

        /*
         * Ищем описание материала по имени,
         * полученному из команды usemtl.
         */
        const ImportedMaterialData* imported_material =
            FindMaterial(model_data.materials, part.material_name);

        /*
         * Создаём runtime-материал движка.
         */
        Material material =
            CreateMaterial(imported_material, texture_manager);

        /*
         * Формируем имя SceneObject.
         *
         * Например:
         *
         * house.obj
         * usemtl Wood
         *
         * ->
         *
         * house_Wood
         */
        std::string object_name = model_name;

        if (!part.material_name.empty()) {
            object_name += "_" + part.material_name;
        } else if (model_data.meshes.size() > 1) {
            object_name += "_" + std::to_string(i);
        }

        /*
         * Создаём SceneObject сразу с готовым Mesh.
         */
        auto object =
            std::make_shared<SceneObject>(object_name, mesh);

        /*
         * GetMaterial() возвращает Material&,
         * поэтому можем записать созданный материал напрямую.
         */
        object->GetMaterial() = material;

        /*
         * BoundingBox рассчитываем именно по render_vertices
         * этой части модели.
         *
         * Поэтому разные части многоматериальной модели
         * получают собственные корректные BoundingBox.
         */
        object->SetBoundingBox(CreateBoundingBox(part.mesh));

        objects.push_back(object);
    }

    return objects;
}
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
 * @brief Находит импортированный материал по имени.
 *
 * Имя приходит из OBJ через директиву usemtl.
 *
 * Оно должно совпасть с именем newmtl,
 * которое было прочитано MtlParser.
 *
 * @param materials Список материалов MTL.
 * @param name Имя материала из usemtl.
 *
 * @return Указатель на материал или nullptr,
 * если материал не найден.
 */
static const ImportedMaterialData* FindImportedMaterial(const std::vector<ImportedMaterialData>& materials, const std::string& name) {
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
 * @brief Преобразует RGB-параметр MTL в scalar strength.
 *
 * Наш текущий Material и basic shader используют
 * AmbientStrength и SpecularStrength как одно число,
 * тогда как MTL хранит Ka и Ks в формате RGB.
 *
 * Пока используется средняя интенсивность каналов.
 *
 * Например:
 *
 * Ks 0.9 0.6 0.3
 *
 * превращается примерно в:
 *
 * SpecularStrength = 0.6
 *
 * В будущем Material можно расширить до полноценного
 * RGB ambient/specular.
 *
 * @param color Цвет MTL.
 *
 * @return Средняя интенсивность в диапазоне [0, 1].
 */
static float MtlColorToStrength(const Vec3& color) {
    return std::clamp(
        (
            color.x +
            color.y +
            color.z
        ) / 3.0f,
        0.0f,
        1.0f
    );
}

/**
 * @brief Создаёт runtime Material из ImportedMaterialData.
 *
 * Выполняется преобразование:
 *
 * Kd     -> Material Color;
 * Ka     -> AmbientStrength;
 * Ks     -> SpecularStrength;
 * Ns     -> Shininess;
 * map_Kd -> Texture2D.
 *
 * Если присутствует map_Kd, базовый Color устанавливается
 * белым, чтобы Kd не затемнял текстуру.
 *
 * Это особенно важно для некоторых MTL,
 * где одновременно встречается:
 *
 * Kd 0 0 0
 * map_Kd texture.jpg
 *
 * @param imported_material Исходный материал MTL.
 * @param texture_manager Менеджер GPU-текстур.
 *
 * @return Готовый Material Renderer.
 */
static Material CreateRuntimeMaterial(const ImportedMaterialData* imported_material, TextureManager& texture_manager) {
    Material material;

    /*
     * Если usemtl ссылается на неизвестный материал,
     * оставляем стандартный Material движка.
     */
    if (imported_material == nullptr) {
        return material;
    }

    const bool has_diffuse_texture =
        !imported_material->
            diffuse_texture_path.
            empty();

    /*
     * Без текстуры используем Kd непосредственно
     * как цвет поверхности.
     *
     * При наличии map_Kd используем белый множитель,
     * чтобы изображение отображалось без затемнения.
     */
    if (has_diffuse_texture) {
        material.SetColor(
            Vec3{
                1.0f,
                1.0f,
                1.0f
            }
        );
    } else {
        material.SetColor(
            imported_material->
                diffuse_color
        );
    }

    /*
     * Ka и Ks являются Vec3 в MTL,
     * а текущий Material использует scalar strength.
     */
    material.SetAmbientStrength(
        MtlColorToStrength(
            imported_material->
                ambient_color
        )
    );

    /*
     * Цвет diffuse уже находится в Material::Color,
     * поэтому коэффициент diffuse оставляем равным 1.
     */
    material.SetDiffuseStrength(
        1.0f
    );

    material.SetSpecularStrength(
        MtlColorToStrength(
            imported_material->
                specular_color
        )
    );

    /*
     * Ns напрямую соответствует параметру
     * shininess текущего shader.
     */
    material.SetShininess(
        imported_material->
            shininess
    );

    if (!has_diffuse_texture) {
        return material;
    }

    /*
     * Путь сохраняется отдельно от Texture2D.
     *
     * Это необходимо для:
     * - Inspector;
     * - Save Scene;
     * - повторной загрузки сцены.
     */
    material.SetDiffuseTexturePath(
        imported_material->
            diffuse_texture_path
    );

    /*
     * MtlParser уже должен был разрешить
     * относительный map_Kd в полный путь.
     */
    std::shared_ptr<Texture2D> texture =
        texture_manager.Load(
            imported_material->
                diffuse_texture_path
        );

    if (texture) {
        material.SetDiffuseTexture(
            std::move(texture)
        );
    }

    return material;
}

/**
 * @brief Импортирует Wavefront OBJ как один SceneObject.
 *
 * OBJ может использовать несколько материалов через usemtl.
 * Каждый материал становится отдельным SceneRenderPart:
 *
 * SceneObject
 * ├── RenderPart + Material A
 * ├── RenderPart + Material B
 * └── RenderPart + Material C
 *
 * Геометрия всех частей остаётся частью одного объекта,
 * поэтому Transform применяется ко всей модели целиком.
 *
 * @param path Путь к OBJ.
 * @param texture_manager Менеджер текстур.
 *
 * @return Список импортированных SceneObject.
 * Сейчас один OBJ создаёт один SceneObject.
 */
std::vector<std::shared_ptr<SceneObject>> ModelImporter::ImportObj(const std::filesystem::path& path, TextureManager& texture_manager) {
    ImportedModelData model_data;

    /*
     * ObjParser одновременно загружает:
     *
     * - geometry;
     * - usemtl;
     * - связанную MTL library.
     */
    if (!ObjParser::Parse(
            path.string(),
            model_data
        )) {
        return {};
    }

    /*
     * Один OBJ представлен одним SceneObject.
     */
    auto object =
        std::make_shared<SceneObject>(
            path.stem().string()
        );

    object->SetType(
        SceneObject::Type::
            ImportedModel
    );

    object->SetSourcePath(
        path
    );

    /*
     * Общий BoundingBox строится из вершин
     * всех material parts.
     */
    std::vector<Vec3> bounding_points;

    for (const ImportedMeshPart& part : model_data.meshes) {
        if (
            part.mesh.render_vertices.empty() ||
            part.mesh.render_indices.empty()
        ) {
            continue;
        }

        /*
         * GPU Mesh создаётся отдельно для каждой
         * части с собственным Material.
         */
        auto mesh =
            std::make_shared<Mesh>(
                part.mesh
            );

        /*
         * Ищем newmtl, соответствующий usemtl
         * текущей части OBJ.
         */
        const ImportedMaterialData* imported_material =
            FindImportedMaterial(
                model_data.materials,
                part.material_name
            );

        Material material =
            CreateRuntimeMaterial(
                imported_material,
                texture_manager
            );

        /*
         * Material name сохраняется в RenderPart,
         * чтобы Inspector позволял переключаться
         * между материалами одной модели.
         */
        object->AddRenderPart(
            part.material_name.empty()
                ? "Material"
                : part.material_name,
            std::move(mesh),
            std::move(material)
        );

        for (const Vertex& vertex : part.mesh.render_vertices) {
            bounding_points.push_back(
                vertex.position
            );
        }
    }

    /*
     * Один BoundingBox описывает всю модель,
     * а не отдельные material parts.
     */
    if (!bounding_points.empty()) {
        object->SetBoundingBox(
            BoundingBox::FromPoints(
                bounding_points
            )
        );
    }

    /*
     * Если ObjParser не создал ни одной
     * пригодной для рендера части,
     * импорт считается неудачным.
     */
    if (!object->HasMesh()) {
        return {};
    }

    return {
        object
    };
}
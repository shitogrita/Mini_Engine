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
 * @brief Находит импортированный материал по его имени.
 *
 * При разборе OBJ директива usemtl связывает часть геометрии
 * с материалом, определённым в MTL-файле директивой newmtl.
 *
 * Метод выполняет поиск соответствующего ImportedMaterialData
 * в уже разобранном списке материалов.
 *
 * Сам список материалов не изменяется.
 *
 * @param materials Материалы, прочитанные из MTL-файла.
 * @param name Имя материала, указанное в usemtl.
 *
 * @return Указатель на найденный ImportedMaterialData.
 * Если материал с указанным именем отсутствует,
 * возвращается nullptr.
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
 * @brief Преобразует RGB-параметр материала MTL
 * в скалярную интенсивность.
 *
 * Формат MTL хранит параметры Ka и Ks как три цветовых канала,
 * тогда как текущий Material Mini Engine использует
 * AmbientStrength и SpecularStrength как одно значение.
 *
 * Для преобразования вычисляется среднее арифметическое
 * компонентов R, G и B:
 *
 * strength = (R + G + B) / 3.
 *
 * Результат дополнительно ограничивается диапазоном [0, 1].
 *
 * Например:
 *
 * Ks 0.9 0.6 0.3
 *
 * преобразуется в:
 *
 * SpecularStrength = 0.6.
 *
 * @param color RGB-значение параметра материала MTL.
 *
 * @return Средняя интенсивность цвета в диапазоне [0, 1].
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
 * @brief Создаёт runtime Material из данных,
 * прочитанных из MTL-файла.
 *
 * ImportedMaterialData является промежуточным представлением
 * материала после разбора MTL.
 *
 * Этот метод преобразует его в Material,
 * непосредственно используемый Renderer.
 *
 * Применяется следующее соответствие:
 *
 * - Kd -> Material Color;
 * - Ka -> AmbientStrength;
 * - Ks -> SpecularStrength;
 * - Ns -> Shininess;
 * - map_Kd -> diffuse Texture2D.
 *
 * Если материал содержит diffuse texture,
 * базовый Color устанавливается равным белому.
 *
 * Это предотвращает нежелательное умножение текстуры
 * на тёмный Kd. Такое поведение важно для MTL-файлов,
 * содержащих, например:
 *
 * Kd 0 0 0
 * map_Kd texture.jpg
 *
 * Если imported_material равен nullptr,
 * возвращается стандартный Material Mini Engine.
 *
 * @param imported_material Материал, прочитанный из MTL,
 * либо nullptr, если материал не найден.
 * @param texture_manager Менеджер загрузки Texture2D.
 *
 * @return Готовый Material для использования Renderer.
 */
static Material CreateRuntimeMaterial(const ImportedMaterialData* imported_material, TextureManager& texture_manager) {
    Material material;

    /**
     * usemtl может ссылаться на отсутствующий
     * или не найденный материал.
     *
     * В этом случае объект получает стандартные
     * параметры Material Mini Engine.
     */
    if (imported_material == nullptr) {
        return material;
    }

    const bool has_diffuse_texture =
        !imported_material->
            diffuse_texture_path.
            empty();

    /**
     * Если diffuse texture отсутствует,
     * Kd используется как базовый цвет поверхности.
     *
     * Если присутствует map_Kd, используется белый цвет,
     * чтобы Texture2D отображалась без дополнительного
     * затемнения значением Kd.
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

    /**
     * MTL хранит Ka как RGB-значение,
     * а Material использует одно скалярное
     * значение AmbientStrength.
     */
    material.SetAmbientStrength(
        MtlColorToStrength(
            imported_material->
                ambient_color
        )
    );

    /**
     * Диффузный цвет уже представлен
     * через Material::Color.
     *
     * Дополнительный коэффициент diffuse
     * поэтому устанавливается равным 1.
     */
    material.SetDiffuseStrength(
        1.0f
    );

    /**
     * MTL хранит Ks как RGB-значение.
     *
     * Для текущего Material оно преобразуется
     * в скалярный SpecularStrength.
     */
    material.SetSpecularStrength(
        MtlColorToStrength(
            imported_material->
                specular_color
        )
    );

    /**
     * Параметр Ns MTL используется
     * как Shininess материала.
     */
    material.SetShininess(
        imported_material->
            shininess
    );

    /**
     * Если diffuse texture не задана,
     * Material уже полностью сформирован.
     */
    if (!has_diffuse_texture) {
        return material;
    }

    /**
     * Путь к Texture2D сохраняется отдельно
     * от самого runtime-ресурса.
     *
     * Он используется Inspector
     * и системой сериализации Scene.
     */
    material.SetDiffuseTexturePath(
        imported_material->
            diffuse_texture_path
    );

    /**
     * MtlParser передаёт путь map_Kd,
     * который затем используется TextureManager
     * для получения runtime Texture2D.
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
 * @brief Импортирует Wavefront OBJ и преобразует его
 * в SceneObject Mini Engine.
 *
 * Метод связывает файловый уровень Assets
 * с runtime-представлением сцены.
 *
 * Последовательность обработки:
 *
 * 1. ObjParser читает OBJ и связанный MTL.
 * 2. Формируется ImportedModelData.
 * 3. Для каждой части геометрии создаётся Mesh.
 * 4. По имени usemtl находится ImportedMaterialData.
 * 5. ImportedMaterialData преобразуется в Material.
 * 6. Mesh и Material добавляются в SceneObject
 *    как SceneRenderPart.
 * 7. По вершинам всех частей вычисляется общий BoundingBox.
 *
 * Один OBJ создаёт один SceneObject.
 *
 * Если OBJ содержит несколько материалов, структура
 * runtime-объекта имеет следующий вид:
 *
 * SceneObject
 * ├── SceneRenderPart -> Mesh + Material A
 * ├── SceneRenderPart -> Mesh + Material B
 * └── SceneRenderPart -> Mesh + Material C
 *
 * Все SceneRenderPart используют Transform
 * родительского SceneObject.
 *
 * @param path Путь к импортируемому OBJ-файлу.
 * @param texture_manager Менеджер загрузки Texture2D.
 *
 * @return Список созданных SceneObject.
 * При успешном импорте текущая реализация возвращает
 * один SceneObject. При ошибке возвращается пустой список.
 */
std::vector<std::shared_ptr<SceneObject>> ModelImporter::ImportObj(const std::filesystem::path& path, TextureManager& texture_manager) {
    ImportedModelData model_data;

    /**
     * ObjParser формирует единое промежуточное
     * представление модели.
     *
     * В него входят:
     *
     * - геометрические части OBJ;
     * - имена usemtl;
     * - материалы связанной MTL library.
     */
    if (!ObjParser::Parse(
            path.string(),
            model_data
        )) {
        return {};
    }

    /**
     * В текущей модели данных один OBJ
     * соответствует одному SceneObject.
     */
    auto object =
        std::make_shared<SceneObject>(
            path.stem().string()
        );

    object->SetType(
        SceneObject::Type::
            ImportedModel
    );

    /**
     * Исходный путь сохраняется внутри SceneObject.
     *
     * Он позволяет связать runtime-объект
     * с моделью, из которой он был импортирован.
     */
    object->SetSourcePath(
        path
    );

    /**
     * BoundingBox должен охватывать всю модель,
     * включая все её material parts.
     *
     * Поэтому позиции вершин каждой части
     * собираются в единый массив.
     */
    std::vector<Vec3> bounding_points;

    for (const ImportedMeshPart& part : model_data.meshes) {
        /**
         * Части без подготовленных вершин
         * или индексов не могут участвовать
         * в индексированном рендеринге.
         */
        if (
            part.mesh.render_vertices.empty() ||
            part.mesh.render_indices.empty()
        ) {
            continue;
        }

        /**
         * Для каждой части модели создаётся
         * собственный runtime Mesh.
         *
         * Разделение соответствует материалам,
         * использованным внутри OBJ.
         */
        auto mesh =
            std::make_shared<Mesh>(
                part.mesh
            );

        /**
         * material_name содержит имя,
         * полученное из usemtl.
         *
         * По нему находится соответствующий
         * материал из MTL library.
         */
        const ImportedMaterialData* imported_material =
            FindImportedMaterial(
                model_data.materials,
                part.material_name
            );

        /**
         * Промежуточные данные MTL преобразуются
         * в runtime Material.
         */
        Material material =
            CreateRuntimeMaterial(
                imported_material,
                texture_manager
            );

        /**
         * Mesh и Material сохраняются внутри
         * отдельного SceneRenderPart.
         *
         * Имя материала также сохраняется,
         * чтобы Inspector мог отображать
         * и переключать части multipart-модели.
         *
         * Если имя отсутствует,
         * используется стандартное "Material".
         */
        object->AddRenderPart(
            part.material_name.empty()
                ? "Material"
                : part.material_name,
            std::move(mesh),
            std::move(material)
        );

        /**
         * Каждая вершина участвует
         * в вычислении общего BoundingBox модели.
         */
        for (const Vertex& vertex : part.mesh.render_vertices) {
            bounding_points.push_back(
                vertex.position
            );
        }
    }

    /**
     * BoundingBox строится один раз
     * для всего SceneObject.
     *
     * Таким образом picking и операции Editor
     * работают со всей импортированной моделью,
     * а не с отдельными material parts.
     */
    if (!bounding_points.empty()) {
        object->SetBoundingBox(
            BoundingBox::FromPoints(
                bounding_points
            )
        );
    }

    /**
     * Если после разбора OBJ не было создано
     * ни одной пригодной для рендеринга части,
     * импорт считается неуспешным.
     */
    if (!object->HasMesh()) {
        return {};
    }

    return {
        object
    };
}
#include "Engine/Scene/SceneSerializer.h"

#include "Engine/Assets/ModelImporter.h"
#include "Engine/Assets/ImportedMeshData.h"
#include "Engine/Renderer/Material.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/PrimitiveGenerator.h"
#include "Engine/Assets/TextureManager.h"
#include "Engine/Scene/BoundingBox.h"
#include "Engine/Scene/SceneObject.h"
#include "Engine/Scene/PointLight.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/**
 * @brief Промежуточное представление Material при чтении сцены.
 *
 * Структура содержит сериализуемые свойства материала,
 * которые могут быть восстановлены после загрузки файла:
 *
 * - основной цвет;
 * - ambient strength;
 * - diffuse strength;
 * - specular strength;
 * - shininess;
 * - путь к diffuse texture.
 *
 * OpenGL-ресурсы и Texture2D непосредственно
 * в файл сцены не сохраняются.
 */
struct SerializedMaterialData {
    Vec3 color{0.67f, 0.76f, 0.91f};
    float ambient = 0.2f;
    float diffuse = 1.0f;
    float specular = 0.5f;
    float shininess = 32.0f;
    std::filesystem::path texture_path;
};

/**
 * @brief Промежуточное представление SceneRenderPart.
 *
 * При сериализации отдельной части multipart-модели
 * сохраняются её имя и Material.
 *
 * Сама геометрия повторно в файл сцены не записывается,
 * поскольку она восстанавливается из исходной модели.
 */
struct SerializedPartData {
    std::string name;
    SerializedMaterialData material;
};

/**
 * @brief Временное представление SceneObject при чтении файла сцены.
 *
 * Структура используется как промежуточный CPU-side объект
 * между разбором текстового файла и созданием runtime SceneObject.
 *
 * Содержит:
 *
 * - имя и тип объекта;
 * - путь к исходной модели;
 * - Transform;
 * - основной Material;
 * - материалы отдельных RenderPart;
 * - данные PointLight.
 *
 * Данные PointLight хранятся отдельно от Material,
 * поскольку источник света является компонентом SceneObject,
 * а не частью геометрии.
 */
struct SerializedObjectData {
    std::string name;
    SceneObject::Type type = SceneObject::Type::Empty;
    std::filesystem::path source_path;
    Transform transform{};
    SerializedMaterialData material;
    std::vector<SerializedPartData> parts;

    bool has_point_light = false;
    Vec3 light_color{1.0f, 1.0f, 1.0f};
    float light_intensity = 1.0f;
    bool light_enabled = true;
};

/**
 * @brief Преобразует SceneObject::Type
 * в строковое представление формата сцены.
 *
 * @param type Тип SceneObject.
 *
 * @return Строковый идентификатор типа объекта.
 */
static const char* SceneObjectTypeToString(SceneObject::Type type) {
    switch (type) {
        case SceneObject::Type::Cube:
            return "CUBE";

        case SceneObject::Type::Plane:
            return "PLANE";

        case SceneObject::Type::Sphere:
            return "SPHERE";

        case SceneObject::Type::ImportedModel:
            return "IMPORTED_MODEL";

        case SceneObject::Type::PointLight:
            return "POINT_LIGHT";

        case SceneObject::Type::Empty:
        default:
            return "EMPTY";
    }
}

/**
 * @brief Преобразует строковый идентификатор
 * из файла сцены в SceneObject::Type.
 *
 * @param value Строковый идентификатор типа.
 * @param type Переменная, в которую записывается результат.
 *
 * @return true, если value содержит известный тип.
 * @return false, если строка не распознана.
 */
static bool SceneObjectTypeFromString(const std::string& value, SceneObject::Type& type) {
    if (value == "CUBE") {
        type = SceneObject::Type::Cube;
        return true;
    }

    if (value == "PLANE") {
        type = SceneObject::Type::Plane;
        return true;
    }

    if (value == "SPHERE") {
        type = SceneObject::Type::Sphere;
        return true;
    }

    if (value == "IMPORTED_MODEL") {
        type = SceneObject::Type::ImportedModel;
        return true;
    }

    if (value == "POINT_LIGHT") {
        type = SceneObject::Type::PointLight;
        return true;
    }

    if (value == "EMPTY") {
        type = SceneObject::Type::Empty;
        return true;
    }

    return false;
}

/**
 * @brief Читает следующий token и проверяет его значение.
 *
 * Функция используется для последовательной проверки
 * структуры текстового формата сцены.
 *
 * Если чтение невозможно или token отличается
 * от ожидаемого значения, возвращается false.
 *
 * @param file Открытый поток файла сцены.
 * @param expected Ожидаемый token.
 *
 * @return true, если следующий token совпал с expected,
 * иначе false.
 */
static bool ReadExpectedToken(std::ifstream& file, const std::string& expected) {
    std::string token;

    if (!(file >> token)) {
        return false;
    }

    return token == expected;
}

/**
 * @brief Записывает компонент PointLight в файл сцены.
 *
 * Сохраняются:
 *
 * - цвет;
 * - интенсивность;
 * - состояние enabled.
 *
 * Позиция не сохраняется внутри PointLight,
 * поскольку пространственное положение источника
 * уже является частью Transform соответствующего SceneObject.
 *
 * @param file Открытый файл сцены.
 * @param light Компонент источника света.
 */
static void WritePointLight(std::ofstream& file, const PointLight& light) {
    const Vec3& color = light.GetColor();

    file << "LIGHT\n";
    file << "COLOR " << color.x << ' ' << color.y << ' ' << color.z << '\n';
    file << "INTENSITY " << light.GetIntensity() << '\n';
    file << "ENABLED " << (light.IsEnabled() ? 1 : 0) << '\n';
    file << "END_LIGHT\n";
}

/**
 * @brief Читает блок PointLight из файла сцены.
 *
 * Ожидается последовательность:
 *
 * LIGHT
 * COLOR
 * INTENSITY
 * ENABLED
 * END_LIGHT
 *
 * После успешного чтения устанавливается
 * SerializedObjectData::has_point_light.
 *
 * @param file Открытый файл сцены.
 * @param data Данные SceneObject, в которые записывается свет.
 *
 * @return true при корректном формате блока,
 * иначе false.
 */
static bool ReadPointLight(std::ifstream& file, SerializedObjectData& data) {
    if (!ReadExpectedToken(file, "LIGHT")) {
        return false;
    }

    if (!ReadExpectedToken(file, "COLOR")) {
        return false;
    }

    if (!(file >> data.light_color.x >> data.light_color.y >> data.light_color.z)) {
        return false;
    }

    if (!ReadExpectedToken(file, "INTENSITY")) {
        return false;
    }

    if (!(file >> data.light_intensity)) {
        return false;
    }

    if (!ReadExpectedToken(file, "ENABLED")) {
        return false;
    }

    int enabled = 0;

    if (!(file >> enabled)) {
        return false;
    }

    if (!ReadExpectedToken(file, "END_LIGHT")) {
        return false;
    }

    data.has_point_light = true;
    data.light_enabled = enabled != 0;

    return true;
}

/**
 * @brief Записывает параметры Material в файл сцены.
 *
 * Сериализуются:
 *
 * - Color;
 * - Ambient Strength;
 * - Diffuse Strength;
 * - Specular Strength;
 * - Shininess;
 * - путь к Diffuse Texture.
 *
 * Путь к texture записывается через std::quoted(),
 * поэтому строка может содержать пробелы.
 *
 * @param file Открытый файл сцены.
 * @param material Материал, который необходимо сохранить.
 */
static void WriteMaterial(std::ofstream& file, const Material& material) {
    const Vec3& color = material.GetColor();

    file << "COLOR "
         << color.x << ' '
         << color.y << ' '
         << color.z << '\n';

    file << "AMBIENT "
         << material.GetAmbientStrength()
         << '\n';

    file << "DIFFUSE "
         << material.GetDiffuseStrength()
         << '\n';

    file << "SPECULAR "
         << material.GetSpecularStrength()
         << '\n';

    file << "SHININESS "
         << material.GetShininess()
         << '\n';

    file << "DIFFUSE_TEXTURE "
         << std::quoted(material.GetDiffuseTexturePath().string())
         << '\n';
}

/**
 * @brief Читает параметры Material из файла сцены.
 *
 * Поля должны располагаться в фиксированной последовательности:
 *
 * COLOR
 * AMBIENT
 * DIFFUSE
 * SPECULAR
 * SHININESS
 * DIFFUSE_TEXTURE
 *
 * @param file Открытый файл сцены.
 * @param material Структура для сохранения прочитанных данных.
 *
 * @return true при успешном чтении всех параметров,
 * иначе false.
 */
static bool ReadMaterial(std::ifstream& file, SerializedMaterialData& material) {
    if (!ReadExpectedToken(file, "COLOR")) {
        return false;
    }

    if (!(file >> material.color.x >> material.color.y >> material.color.z)) {
        return false;
    }

    if (!ReadExpectedToken(file, "AMBIENT")) {
        return false;
    }

    if (!(file >> material.ambient)) {
        return false;
    }

    if (!ReadExpectedToken(file, "DIFFUSE")) {
        return false;
    }

    if (!(file >> material.diffuse)) {
        return false;
    }

    if (!ReadExpectedToken(file, "SPECULAR")) {
        return false;
    }

    if (!(file >> material.specular)) {
        return false;
    }

    if (!ReadExpectedToken(file, "SHININESS")) {
        return false;
    }

    if (!(file >> material.shininess)) {
        return false;
    }

    if (!ReadExpectedToken(file, "DIFFUSE_TEXTURE")) {
        return false;
    }

    std::string texture_path;

    if (!(file >> std::quoted(texture_path))) {
        return false;
    }

    material.texture_path = texture_path;

    return true;
}

/**
 * @brief Применяет сериализованные параметры к runtime Material.
 *
 * Сначала восстанавливаются числовые параметры материала,
 * после чего сбрасывается текущая diffuse texture.
 *
 * Если в сцене сохранён путь к texture,
 * TextureManager пытается загрузить соответствующий Texture2D.
 *
 * @param material Runtime Material, который необходимо настроить.
 * @param data Сериализованные параметры материала.
 * @param texture_manager Менеджер загрузки и кеширования texture.
 */
static void ApplyMaterial(Material& material, const SerializedMaterialData& data, TextureManager& texture_manager) {
    material.SetColor(data.color);
    material.SetAmbientStrength(data.ambient);
    material.SetDiffuseStrength(data.diffuse);
    material.SetSpecularStrength(data.specular);
    material.SetShininess(data.shininess);

    material.ClearDiffuseTexture();

    if (data.texture_path.empty()) {
        return;
    }

    material.SetDiffuseTexturePath(data.texture_path);

    std::shared_ptr<Texture2D> texture = texture_manager.Load(
        data.texture_path
    );

    if (texture) {
        material.SetDiffuseTexture(std::move(texture));
    }
}

/**
 * @brief Вычисляет локальный BoundingBox
 * для восстановленного primitive SceneObject.
 *
 * В первую очередь используются позиции
 * из render_vertices.
 *
 * Если render_vertices отсутствуют,
 * используются позиции из ImportedMeshData::positions.
 *
 * @param object SceneObject,
 * которому необходимо назначить BoundingBox.
 * @param mesh_data Геометрические данные primitive.
 */
static void SetPrimitiveBoundingBox(SceneObject& object, const ImportedMeshData& mesh_data) {
    std::vector<Vec3> points;
    points.reserve(mesh_data.render_vertices.size());

    for (const Vertex& vertex : mesh_data.render_vertices) {
        points.push_back(vertex.position);
    }

    if (points.empty()) {
        points = mesh_data.positions;
    }

    if (!points.empty()) {
        object.SetBoundingBox(
            BoundingBox::FromPoints(points)
        );
    }
}

/**
 * @brief Создаёт primitive SceneObject
 * из сериализованных данных.
 *
 * Поддерживаются:
 *
 * - Cube;
 * - Plane;
 * - Sphere.
 *
 * Геометрия заново генерируется через PrimitiveGenerator,
 * после чего создаются Mesh и SceneObject.
 *
 * Для созданного primitive также восстанавливается
 * локальный BoundingBox.
 *
 * @param data Сериализованные данные объекта.
 *
 * @return Созданный SceneObject
 * или nullptr для неподдерживаемого типа.
 */
static std::shared_ptr<SceneObject> CreatePrimitiveObject(const SerializedObjectData& data) {
    ImportedMeshData mesh_data;

    switch (data.type) {
        case SceneObject::Type::Cube:
            mesh_data = PrimitiveGenerator::CreateCube();
            break;

        case SceneObject::Type::Plane:
            mesh_data = PrimitiveGenerator::CreatePlane();
            break;

        case SceneObject::Type::Sphere:
            mesh_data = PrimitiveGenerator::CreateSphere();
            break;

        default:
            return nullptr;
    }

    auto mesh = std::make_shared<Mesh>(mesh_data);
    auto object = std::make_shared<SceneObject>(data.name, std::move(mesh));

    SetPrimitiveBoundingBox(*object, mesh_data);

    return object;
}

/**
 * @brief Восстанавливает материалы отдельных RenderPart.
 *
 * Материал из файла сопоставляется с существующим
 * SceneRenderPart по имени части.
 *
 * Если соответствующая часть модели не найдена,
 * сериализованный Material пропускается.
 *
 * @param object Runtime SceneObject.
 * @param serialized_parts Сериализованные части модели.
 * @param texture_manager Менеджер загрузки texture.
 */
static void ApplyRenderPartMaterials(SceneObject& object, const std::vector<SerializedPartData>& serialized_parts, TextureManager& texture_manager) {
    std::vector<SceneObject::SceneRenderPart>& render_parts = object.GetRenderParts();

    for (const SerializedPartData& serialized_part : serialized_parts) {
        const auto iterator = std::find_if(
            render_parts.begin(),
            render_parts.end(),
            [&serialized_part](const SceneObject::SceneRenderPart& part) {
                return part.name == serialized_part.name;
            }
        );

        if (iterator == render_parts.end()) {
            continue;
        }

        ApplyMaterial(
            iterator->material,
            serialized_part.material,
            texture_manager
        );
    }
}

/**
 * @brief Создаёт runtime SceneObject
 * из промежуточных сериализованных данных.
 *
 * Способ восстановления зависит от SceneObject::Type:
 *
 * - Cube, Plane, Sphere заново создаются через PrimitiveGenerator;
 * - ImportedModel повторно импортируется из source_path;
 * - PointLight создаётся как SceneObject с компонентом PointLight;
 * - Empty создаётся без геометрии.
 *
 * После создания для объекта восстанавливаются:
 *
 * - имя;
 * - тип;
 * - source path;
 * - Transform;
 * - основной Material;
 * - материалы RenderPart.
 *
 * @param data Сериализованные данные SceneObject.
 * @param texture_manager Менеджер texture.
 *
 * @return Восстановленный SceneObject
 * или nullptr при невозможности восстановления.
 */
static std::shared_ptr<SceneObject> CreateObject(const SerializedObjectData& data, TextureManager& texture_manager) {
    std::shared_ptr<SceneObject> object;

    switch (data.type) {
        case SceneObject::Type::Cube:
        case SceneObject::Type::Plane:
        case SceneObject::Type::Sphere:
            object = CreatePrimitiveObject(data);
            break;

        case SceneObject::Type::ImportedModel: {
            /**
             * ImportedModel невозможно восстановить
             * без пути к исходному OBJ-файлу.
             */
            if (data.source_path.empty()) {
                return nullptr;
            }

            std::vector<std::shared_ptr<SceneObject>> imported_objects =
                ModelImporter::ImportObj(
                    data.source_path,
                    texture_manager
                );

            if (imported_objects.empty()) {
                return nullptr;
            }

            /**
             * Текущая модель импорта использует
             * первый SceneObject из результата ModelImporter.
             */
            object = imported_objects.front();
            break;
        }

        case SceneObject::Type::PointLight: {
            object = std::make_shared<SceneObject>(data.name);

            PointLight light;
            light.SetColor(data.light_color);
            light.SetIntensity(data.light_intensity);
            light.SetEnabled(data.light_enabled);

            object->SetPointLight(light);
            break;
        }

        case SceneObject::Type::Empty:
            object = std::make_shared<SceneObject>(data.name);
            break;
    }

    if (!object) {
        return nullptr;
    }

    /**
     * Восстанавливаем общие свойства SceneObject,
     * независимо от его конкретного типа.
     */
    object->SetName(data.name);
    object->SetType(data.type);
    object->SetSourcePath(data.source_path);

    Transform& transform = object->GetTransform();
    transform.position = data.transform.position;
    transform.rotation = data.transform.rotation;
    transform.scale = data.transform.scale;

    ApplyMaterial(
        object->GetMaterial(),
        data.material,
        texture_manager
    );

    ApplyRenderPartMaterials(
        *object,
        data.parts,
        texture_manager
    );

    return object;
}

/**
 * @brief Сохраняет текущую Scene в текстовый файл.
 *
 * Текущий формат записи:
 *
 * MINI_ENGINE_SCENE 4
 *
 * Для каждого SceneObject сохраняются:
 *
 * - имя;
 * - Type;
 * - source path;
 * - Transform;
 * - PointLight для объектов соответствующего типа;
 * - основной Material;
 * - материалы SceneRenderPart.
 *
 * nullptr-элементы внутри Scene
 * не включаются в OBJECT_COUNT и не записываются.
 *
 * Для Type::PointLight наличие компонента PointLight обязательно.
 * Если тип объекта равен PointLight,
 * но компонент отсутствует, сохранение завершается с false.
 *
 * @param scene Сцена, которую необходимо сохранить.
 * @param path Путь к файлу сцены.
 *
 * @return true при успешной записи.
 * @return false, если файл не удалось открыть
 * или структура объекта не может быть сериализована.
 */
bool SceneSerializer::Save(const Scene& scene, const std::filesystem::path& path) {
    std::ofstream file(path);

    if (!file.is_open()) {
        return false;
    }

    file << "MINI_ENGINE_SCENE 4\n";

    /**
     * В OBJECT_COUNT записываем только реальные SceneObject.
     *
     * nullptr внутри Scene не должен нарушать
     * структуру сериализованного файла.
     */
    std::size_t object_count = 0;

    for (const std::shared_ptr<SceneObject>& object : scene.GetObjects()) {
        if (object) {
            ++object_count;
        }
    }

    file << "OBJECT_COUNT " << object_count << '\n';

    for (const std::shared_ptr<SceneObject>& object : scene.GetObjects()) {
        if (!object) {
            continue;
        }

        const Transform& transform = object->GetTransform();

        file << "OBJECT "
             << std::quoted(object->GetName())
             << '\n';

        file << "TYPE "
             << SceneObjectTypeToString(object->GetType())
             << '\n';

        file << "SOURCE "
             << std::quoted(object->GetSourcePath().string())
             << '\n';

        file << "POSITION "
             << transform.position.x << ' '
             << transform.position.y << ' '
             << transform.position.z << '\n';

        file << "ROTATION "
             << transform.rotation.x << ' '
             << transform.rotation.y << ' '
             << transform.rotation.z << '\n';

        file << "SCALE "
             << transform.scale.x << ' '
             << transform.scale.y << ' '
             << transform.scale.z << '\n';

        /**
         * PointLight является компонентом SceneObject.
         *
         * Его положение уже сохранено выше
         * как часть Transform.
         *
         * Для Type::PointLight компонент обязателен,
         * иначе файл версии 4 был бы структурно неполным.
         */
        if (object->GetType() == SceneObject::Type::PointLight) {
            const PointLight* light = object->GetPointLight();

            if (!light) {
                return false;
            }

            WritePointLight(file, *light);
        }

        file << "MATERIAL\n";

        WriteMaterial(
            file,
            object->GetMaterial()
        );

        file << "END_MATERIAL\n";

        file << "PART_COUNT "
             << object->GetRenderParts().size()
             << '\n';

        for (const SceneObject::SceneRenderPart& part : object->GetRenderParts()) {
            file << "PART "
                 << std::quoted(part.name)
                 << '\n';

            WriteMaterial(
                file,
                part.material
            );

            file << "END_PART\n";
        }

        file << "END_OBJECT\n";
    }

    return true;
}

/**
 * @brief Загружает Scene из текстового файла.
 *
 * Метод поддерживает форматы сцены:
 *
 * - version 3;
 * - version 4.
 *
 * Version 4 содержит отдельный блок PointLight.
 *
 * Version 3 не сохраняла параметры PointLight.
 * Для совместимости объект Type::Empty,
 * имя которого начинается с "Point Light",
 * восстанавливается как PointLight
 * с параметрами света по умолчанию.
 *
 * Загрузка выполняется в несколько этапов:
 *
 * 1. Проверяется заголовок и версия файла.
 * 2. Все объекты разбираются в SerializedObjectData.
 * 3. Из промежуточных данных создаются runtime SceneObject.
 * 4. Только после успешного создания всех объектов
 *    текущая Scene очищается.
 * 5. В Scene добавляются восстановленные объекты.
 *
 * Благодаря такому порядку scene.Clear()
 * не вызывается при ранней ошибке парсинга
 * или создания одного из объектов.
 *
 * @param scene Scene, в которую необходимо загрузить объекты.
 * @param path Путь к файлу сцены.
 * @param texture_manager Менеджер texture,
 * используемый при восстановлении материалов и моделей.
 *
 * @return true при успешной загрузке всей сцены,
 * иначе false.
 */
bool SceneSerializer::Load(Scene& scene, const std::filesystem::path& path, TextureManager& texture_manager) {
    std::ifstream file(path);

    if (!file.is_open()) {
        return false;
    }

    if (!ReadExpectedToken(file, "MINI_ENGINE_SCENE")) {
        return false;
    }

    int version = 0;

    if (!(file >> version)) {
        return false;
    }

    /**
     * Version 4 добавляет сериализацию PointLight.
     *
     * Version 3 продолжаем поддерживать
     * для загрузки ранее сохранённых сцен.
     */
    if (version != 3 && version != 4) {
        return false;
    }

    if (!ReadExpectedToken(file, "OBJECT_COUNT")) {
        return false;
    }

    std::size_t object_count = 0;

    if (!(file >> object_count)) {
        return false;
    }

    /**
     * На первом этапе читаем файл
     * только в промежуточные структуры.
     *
     * Runtime Scene пока не изменяется.
     */
    std::vector<SerializedObjectData> serialized_objects;
    serialized_objects.reserve(object_count);

    for (std::size_t index = 0; index < object_count; ++index) {
        SerializedObjectData object_data;

        if (!ReadExpectedToken(file, "OBJECT")) {
            return false;
        }

        if (!(file >> std::quoted(object_data.name))) {
            return false;
        }

        if (!ReadExpectedToken(file, "TYPE")) {
            return false;
        }

        std::string type_name;

        if (!(file >> type_name)) {
            return false;
        }

        if (!SceneObjectTypeFromString(type_name, object_data.type)) {
            return false;
        }

        /**
         * Scene format v3 не умел сохранять PointLight.
         *
         * Старый serializer записывал такой объект как EMPTY.
         *
         * Если объект старой сцены имеет имя,
         * начинающееся с "Point Light",
         * восстанавливаем его тип и наличие компонента.
         *
         * Color, Intensity и Enabled старого источника
         * восстановить невозможно,
         * поскольку version 3 их не сохраняла.
         */
        if (version == 3 &&
            object_data.type == SceneObject::Type::Empty &&
            object_data.name.rfind("Point Light", 0) == 0) {
            object_data.type = SceneObject::Type::PointLight;
            object_data.has_point_light = true;
        }

        if (!ReadExpectedToken(file, "SOURCE")) {
            return false;
        }

        std::string source_path;

        if (!(file >> std::quoted(source_path))) {
            return false;
        }

        object_data.source_path = source_path;

        if (!ReadExpectedToken(file, "POSITION")) {
            return false;
        }

        if (!(file
            >> object_data.transform.position.x
            >> object_data.transform.position.y
            >> object_data.transform.position.z)) {
            return false;
        }

        if (!ReadExpectedToken(file, "ROTATION")) {
            return false;
        }

        if (!(file
            >> object_data.transform.rotation.x
            >> object_data.transform.rotation.y
            >> object_data.transform.rotation.z)) {
            return false;
        }

        if (!ReadExpectedToken(file, "SCALE")) {
            return false;
        }

        if (!(file
            >> object_data.transform.scale.x
            >> object_data.transform.scale.y
            >> object_data.transform.scale.z)) {
            return false;
        }

        /**
         * В version 4 PointLight
         * хранится отдельным блоком.
         *
         * Для остальных типов
         * следующим блоком сразу является MATERIAL.
         */
        if (version >= 4 && object_data.type == SceneObject::Type::PointLight) {
            if (!ReadPointLight(file, object_data)) {
                return false;
            }
        }

        if (!ReadExpectedToken(file, "MATERIAL")) {
            return false;
        }

        if (!ReadMaterial(file, object_data.material)) {
            return false;
        }

        if (!ReadExpectedToken(file, "END_MATERIAL")) {
            return false;
        }

        if (!ReadExpectedToken(file, "PART_COUNT")) {
            return false;
        }

        std::size_t part_count = 0;

        if (!(file >> part_count)) {
            return false;
        }

        object_data.parts.reserve(part_count);

        for (std::size_t part_index = 0; part_index < part_count; ++part_index) {
            SerializedPartData part_data;

            if (!ReadExpectedToken(file, "PART")) {
                return false;
            }

            if (!(file >> std::quoted(part_data.name))) {
                return false;
            }

            if (!ReadMaterial(file, part_data.material)) {
                return false;
            }

            if (!ReadExpectedToken(file, "END_PART")) {
                return false;
            }

            object_data.parts.push_back(
                std::move(part_data)
            );
        }

        if (!ReadExpectedToken(file, "END_OBJECT")) {
            return false;
        }

        serialized_objects.push_back(
            std::move(object_data)
        );
    }

    /**
     * На втором этапе создаём runtime SceneObject.
     *
     * loaded_objects пока существует отдельно от Scene,
     * поэтому при ошибке существующая сцена
     * ещё не очищается.
     */
    std::vector<std::shared_ptr<SceneObject>> loaded_objects;
    loaded_objects.reserve(serialized_objects.size());

    for (const SerializedObjectData& object_data : serialized_objects) {
        std::shared_ptr<SceneObject> object = CreateObject(
            object_data,
            texture_manager
        );

        if (!object) {
            return false;
        }

        loaded_objects.push_back(
            std::move(object)
        );
    }

    /**
     * Заменяем содержимое Scene
     * только после успешного восстановления
     * всех SceneObject.
     */
    scene.Clear();

    for (std::shared_ptr<SceneObject>& object : loaded_objects) {
        scene.AddObject(
            std::move(object)
        );
    }

    return true;
}
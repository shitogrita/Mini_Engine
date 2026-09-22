#include "Engine/Scene/SceneSerializer.h"

#include "Engine/Assets/ModelImporter.h"
#include "Engine/Assets/ImportedMeshData.h"
#include "Engine/Renderer/Material.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/PrimitiveGenerator.h"
#include "Engine/Assets/TextureManager.h"
#include "Engine/Scene/BoundingBox.h"
#include "Engine/Scene/SceneObject.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct SerializedMaterialData {
    Vec3 color{0.67f, 0.76f, 0.91f};
    float ambient = 0.2f;
    float diffuse = 1.0f;
    float specular = 0.5f;
    float shininess = 32.0f;
    std::filesystem::path texture_path;
};

struct SerializedPartData {
    std::string name;
    SerializedMaterialData material;
};

struct SerializedObjectData {
    std::string name;
    SceneObject::Type type = SceneObject::Type::Empty;
    std::filesystem::path source_path;
    Transform transform{};
    SerializedMaterialData material;
    std::vector<SerializedPartData> parts;
};

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

        case SceneObject::Type::Empty:
        default:
            return "EMPTY";
    }
}

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

    if (value == "EMPTY") {
        type = SceneObject::Type::Empty;
        return true;
    }

    return false;
}

static bool ReadExpectedToken(std::ifstream& file, const std::string& expected) {
    std::string token;

    if (!(file >> token)) {
        return false;
    }

    return token == expected;
}

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

static std::shared_ptr<SceneObject> CreateObject(const SerializedObjectData& data, TextureManager& texture_manager) {
    std::shared_ptr<SceneObject> object;

    switch (data.type) {
        case SceneObject::Type::Cube:
        case SceneObject::Type::Plane:
        case SceneObject::Type::Sphere:
            object = CreatePrimitiveObject(data);
            break;

        case SceneObject::Type::ImportedModel: {
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

            object = imported_objects.front();
            break;
        }

        case SceneObject::Type::Empty:
            object = std::make_shared<SceneObject>(data.name);
            break;
    }

    if (!object) {
        return nullptr;
    }

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

bool SceneSerializer::Save(const Scene& scene, const std::filesystem::path& path) {
    std::ofstream file(path);

    if (!file.is_open()) {
        return false;
    }

    file << "MINI_ENGINE_SCENE 3\n";
    file << "OBJECT_COUNT " << scene.GetObjects().size() << '\n';

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

    if (version != 3) {
        return false;
    }

    if (!ReadExpectedToken(file, "OBJECT_COUNT")) {
        return false;
    }

    std::size_t object_count = 0;

    if (!(file >> object_count)) {
        return false;
    }

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

    scene.Clear();

    for (std::shared_ptr<SceneObject>& object : loaded_objects) {
        scene.AddObject(
            std::move(object)
        );
    }

    return true;
}
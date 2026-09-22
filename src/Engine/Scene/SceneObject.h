#pragma once

#include "Engine/Renderer/Mesh.h"
#include "Engine/Scene/BoundingBox.h"
#include "Engine/Scene/Transform.h"
#include "Engine/Renderer/Material.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class SceneObject {
public:
    /**
     * @brief Тип объекта сцены.
     *
     * Тип используется Editor для определения
     * поведения объекта в Hierarchy, Inspector и Renderer.
     */
    enum class Type {
        Empty,
        Cube,
        Plane,
        Sphere,
        ImportedModel,
        PointLight
    };

    struct SceneRenderPart {
        std::string name;
        std::shared_ptr<Mesh> mesh;
        Material material;
    };

    SceneObject() = default;
    explicit SceneObject(std::string name);
    SceneObject(std::string name, std::shared_ptr<Mesh> mesh);

    const std::string& GetName() const;
    void SetName(std::string name);

    Transform& GetTransform();
    const Transform& GetTransform() const;

    void SetMesh(std::shared_ptr<Mesh> mesh);
    std::shared_ptr<Mesh> GetMesh();
    std::shared_ptr<const Mesh> GetMesh() const;
    bool HasMesh() const;

    void SetBoundingBox(const BoundingBox& bounding_box);
    const BoundingBox& GetBoundingBox() const;

    Material& GetMaterial();
    const Material& GetMaterial() const;

    void AddRenderPart(std::string name, std::shared_ptr<Mesh> mesh, Material material);

    std::vector<SceneRenderPart>& GetRenderParts();
    const std::vector<SceneRenderPart>& GetRenderParts() const;

    bool HasRenderParts() const;

    void SetType(Type type);
    Type GetType() const;

    void SetSourcePath(std::filesystem::path path);
    const std::filesystem::path& GetSourcePath() const;

private:
    std::string name_{"SceneObject"};

    Transform transform_{};

    std::shared_ptr<Mesh> mesh_;

    BoundingBox bounding_box_{};

    Material material_{};

    std::vector<SceneRenderPart> render_parts_;

    Type type_ = Type::Empty;

    std::filesystem::path source_path_;
};
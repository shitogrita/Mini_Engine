add_library(
        mini_engine_core
        STATIC

        Core/Application.cpp
        Core/Application.h
        Core/Window.cpp
        Core/Window.h

        Assets/ImportedMeshData.h
        Assets/ObjParser.cpp
        Assets/ObjParser.h
        Assets/TextureManager.cpp
        Assets/TextureManager.h
        Assets/ImportedMaterialData.h
        Assets/ImportedModelData.h
        Assets/MtlParser.cpp
        Assets/MtlParser.h
        Assets/ModelImporter.cpp
        Assets/ModelImporter.h

        Math/affine_transformation.cpp
        Math/affine_transformation.h
        Math/matrix_types.h
        Math/projection.cpp
        Math/projection.h
        Math/Ray.cpp
        Math/Ray.h

        Renderer/Mesh.cpp
        Renderer/Mesh.h
        Renderer/Renderer.cpp
        Renderer/Renderer.h
        Renderer/Shader.cpp
        Renderer/Shader.h
        Renderer/Vertex.h
        Renderer/PrimitiveGenerator.cpp
        Renderer/PrimitiveGenerator.h
        Renderer/Material.cpp
        Renderer/Material.h
        Renderer/Texture2D.cpp
        Renderer/Texture2D.h

        Platform/OpenGL/Glad/glad.c
        Platform/OpenGL/Glad/glad.h
        Platform/OpenGL/Glad/khrplatform.h
        Platform/OpenGL/OpenGLLoader.cpp
        Platform/OpenGL/OpenGLLoader.h

        Scene/Camera.cpp
        Scene/Camera.h
        Scene/SceneObject.cpp
        Scene/SceneObject.h
        Scene/Scene.cpp
        Scene/Scene.h
        Scene/Transform.cpp
        Scene/Transform.h
        Scene/BoundingBox.cpp
        Scene/BoundingBox.h
        Scene/PointLight.h
        Scene/PointLight.cpp
        Scene/SceneSerializer.cpp
        Scene/SceneSerializer.h

        ThirdParty/stb/stb_image.h
)


target_include_directories(
        mini_engine_core

        PUBLIC
        ${PROJECT_SOURCE_DIR}/src

        PRIVATE
        ${PROJECT_SOURCE_DIR}/src/Engine/Platform/OpenGL/Glad
)


target_compile_definitions(
        mini_engine_core

        PUBLIC
        MINI_ENGINE_RESOURCE_DIR="${PROJECT_SOURCE_DIR}/src/Engine/Resources"
)


target_link_libraries(
        mini_engine_core

        PUBLIC
        glfw
)


if(APPLE)
    target_compile_definitions(
            mini_engine_core

            PUBLIC
            GL_SILENCE_DEPRECATION
    )

    target_link_libraries(
            mini_engine_core

            PUBLIC
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
endif()
add_executable(
        mini_engine_editor

        main.cpp

        EditorWindow.cpp
        EditorWindow.h

        Panels/HierarchyPanel.cpp
        Panels/HierarchyPanel.h

        Panels/InspectorPanel.cpp
        Panels/InspectorPanel.h

        Panels/ProjectPanel.cpp
        Panels/ProjectPanel.h

        Viewport/SceneViewport.cpp
        Viewport/SceneViewport.h
)

target_include_directories(
        mini_engine_editor

        PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(
        mini_engine_editor

        PRIVATE
        mini_engine_core
        Qt6::Widgets
)

set_target_properties(
        mini_engine_editor

        PROPERTIES
        OUTPUT_NAME "MiniEngineEditor"
)
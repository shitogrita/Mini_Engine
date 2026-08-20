#pragma once

#include "Engine/Assets/ImportedMeshData.h"
#include "Engine/Math/matrix_types.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Shader.h"

#include <QOpenGLWidget>
#include <QString>

#include <memory>
#include <optional>


class QLabel;


class SceneViewport final : public QOpenGLWidget {
public:
	explicit SceneViewport(
		QWidget* parent = nullptr
	);

	~SceneViewport() override;

	void SetDisplayedFile(
		const QString& file_path
	);

private:
	void CreateLayout();

	void UploadPendingMesh();

	void CalculateModelFit(
		const ImportedMeshData& mesh_data
	);

protected:
	void initializeGL() override;

	void resizeGL(
		int width,
		int height
	) override;

	void paintGL() override;

private:
	QLabel* title_label_ = nullptr;
	QLabel* content_label_ = nullptr;

	Renderer renderer_;

	std::unique_ptr<Shader> shader_;
	std::unique_ptr<Mesh> mesh_;

	std::optional<ImportedMeshData>
		pending_mesh_data_;

	QString current_file_path_;

	Vec3 background_color_{
		0.12f,
		0.12f,
		0.12f
	};

	Vec3 model_position_{};
	Vec3 model_rotation_{};

	Vec3 model_scale_{
		1.0f,
		1.0f,
		1.0f
	};

	bool gl_initialized_ = false;
};
#pragma once

#include "Engine/Assets/ImportedMeshData.h"
#include "Engine/Math/matrix_types.h"

#include <vector>

class Mesh {
public:
	explicit Mesh(const ImportedMeshData& mesh_data);
	~Mesh();

	// GPU-ресурс нельзя просто скопировать.
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	// Владение GPU-ресурсами можно передать.
	Mesh(Mesh&& other) noexcept;
	Mesh& operator=(Mesh&& other) noexcept;

	void Bind() const;
	unsigned int GetIndexCount() const;

	/**
	 * @brief Загружает Model Matrix экземпляров Mesh
	 * и настраивает instance vertex attributes.
	 *
	 * Attributes 4-7 включаются только после того,
	 * как instance buffer получил реальные данные.
	 */
	void SetInstanceMatrices(const std::vector<Matrix4>& matrices) const;

	/**
	 * @brief Отключает instance vertex attributes.
	 *
	 * После instanced draw Mesh снова становится безопасным
	 * для обычного Draw() / DrawLines().
	 */
	void DisableInstanceAttributes() const;

private:
	unsigned int vao_ = 0;
	unsigned int vbo_ = 0;
	unsigned int ebo_ = 0;
	unsigned int instance_vbo_ = 0;
	unsigned int index_count_ = 0;

	void Destroy();
};
#pragma once

#include "Engine/Assets/ImportedMeshData.h"

class Mesh {
public:
	explicit Mesh(const ImportedMeshData& mesh_data);
	~Mesh();

	// GPU-ресурс нельзя просто скопировать.
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	// владение GPU-ресурсами можно передать.
	Mesh(Mesh&& other) noexcept;
	Mesh& operator=(Mesh&& other) noexcept;

	void Bind() const;
	unsigned int GetIndexCount() const;

private:

	unsigned int vao_ = 0;
	unsigned int vbo_ = 0;
	unsigned int ebo_ = 0;
	unsigned int index_count_ = 0;

	void Destroy();
};
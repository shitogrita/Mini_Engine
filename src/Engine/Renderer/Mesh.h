#pragma once

#include "Engine/Assets/ImportedMeshData.h"

class Mesh {
public:
	explicit Mesh(const ImportedMeshData& mesh_data);
	~Mesh();

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	Mesh(Mesh&& other) noexcept;
	Mesh& operator=(Mesh&& other) noexcept;

	void Draw() const;

private:
	unsigned int vao_ = 0;
	unsigned int vbo_ = 0;
	unsigned int ebo_ = 0;
	unsigned int index_count_ = 0;

	void Destroy();
};
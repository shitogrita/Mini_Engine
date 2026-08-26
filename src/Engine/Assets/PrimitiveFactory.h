#pragma once


class PrimitiveFactory {
public:
	static ImportedMeshData CreatePlane();
	static ImportedMeshData CreateCube();
	static ImportedMeshData CreateCircle(...);
};
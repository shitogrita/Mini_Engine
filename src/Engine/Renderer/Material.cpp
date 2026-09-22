#include "Engine/Renderer/Material.h"

#include <utility>

const Vec3& Material::GetColor() const {
	return color_;
}

void Material::SetColor(const Vec3& color) {
	color_ = color;
}

float Material::GetAmbientStrength() const {
	return ambient_strength_;
}

void Material::SetAmbientStrength(float value) {
	ambient_strength_ = value;
}

float Material::GetDiffuseStrength() const {
	return diffuse_strength_;
}

void Material::SetDiffuseStrength(float value) {
	diffuse_strength_ = value;
}

float Material::GetSpecularStrength() const {
	return specular_strength_;
}

void Material::SetSpecularStrength(float value) {
	specular_strength_ = value;
}

float Material::GetShininess() const {
	return shininess_;
}

void Material::SetShininess(float value) {
	shininess_ = value;
}

void Material::SetDiffuseTexture(std::shared_ptr<Texture2D> texture) {
	diffuse_texture_ = std::move(texture);
}

const std::shared_ptr<Texture2D>& Material::GetDiffuseTexture() const {
	return diffuse_texture_;
}

bool Material::HasDiffuseTexture() const {
	return diffuse_texture_ != nullptr;
}

void Material::ClearDiffuseTexture() {
	diffuse_texture_.reset();
	diffuse_texture_path_.clear();
}

void Material::SetDiffuseTexturePath(std::filesystem::path path) {
	diffuse_texture_path_ = std::move(path);
}

const std::filesystem::path& Material::GetDiffuseTexturePath() const {
	return diffuse_texture_path_;
}
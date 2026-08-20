#pragma once

#include <QWidget>

class QLabel;

class SceneViewport final : public QWidget {
public:
	explicit SceneViewport(QWidget* parent = nullptr);

	void SetDisplayedFile(const QString& file_path);

private:
	void CreateLayout();

	QLabel* title_label_ = nullptr;
	QLabel* content_label_ = nullptr;
};
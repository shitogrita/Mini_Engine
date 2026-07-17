#include "Editor/Viewport/SceneViewport.h"

#include <QFileInfo>
#include <QLabel>
#include <QVBoxLayout>

SceneViewport::SceneViewport(QWidget* parent)
	: QWidget(parent)
{
	CreateLayout();
}

void SceneViewport::CreateLayout()
{
	setMinimumSize(500, 400);

	setStyleSheet(
		R"(
            SceneViewport {
                background-color: #1b1b1b;
                border: 1px solid #363636;
            }
        )"
	);

	QVBoxLayout* layout = new QVBoxLayout(this);

	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	title_label_ = new QLabel("Scene", this);

	title_label_->setFixedHeight(30);
	title_label_->setAlignment(
		Qt::AlignVCenter | Qt::AlignLeft
	);

	title_label_->setContentsMargins(
		10,
		0,
		0,
		0
	);

	title_label_->setStyleSheet(
		R"(
            background-color: #292929;
            border-bottom: 1px solid #3a3a3a;
            font-weight: 600;
        )"
	);

	content_label_ = new QLabel(this);

	content_label_->setAlignment(Qt::AlignCenter);
	content_label_->setText(
		"Scene View\n\n"
		"Open a model using\n"
		"File → Open Model..."
	);

	content_label_->setStyleSheet(
		R"(
            background-color: #1e1e1e;
            color: #909090;
            font-size: 17px;
        )"
	);

	layout->addWidget(title_label_);
	layout->addWidget(content_label_);
}

void SceneViewport::SetDisplayedFile(
	const QString& file_path
)
{
	const QFileInfo file_info(file_path);

	content_label_->setText(
		"Selected model\n\n" +
		file_info.fileName() +
		"\n\n" +
		file_path +
		"\n\n"
		"Rendering is not connected yet."
	);
}
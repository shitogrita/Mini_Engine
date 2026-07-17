#include "Editor/Panels/InspectorPanel.h"

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

InspectorPanel::InspectorPanel(QWidget* parent)
	: QWidget(parent)
{
	CreateLayout();
	ClearSelection();
}

void InspectorPanel::CreateLayout()
{
	QVBoxLayout* layout = new QVBoxLayout(this);

	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(10);

	title_label_ = new QLabel(this);

	title_label_->setStyleSheet(
		"font-size: 15px; font-weight: 600;"
	);

	QFrame* separator = new QFrame(this);

	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);

	transform_label_ = new QLabel(this);

	transform_label_->setText(
		"Transform\n\n"
		"Position    X: 0    Y: 0    Z: 0\n"
		"Rotation    X: 0    Y: 0    Z: 0\n"
		"Scale       X: 1    Y: 1    Z: 1"
	);

	transform_label_->setAlignment(
		Qt::AlignTop | Qt::AlignLeft
	);

	information_label_ = new QLabel(this);

	information_label_->setAlignment(
		Qt::AlignTop | Qt::AlignLeft
	);

	information_label_->setWordWrap(true);

	layout->addWidget(title_label_);
	layout->addWidget(separator);
	layout->addWidget(transform_label_);
	layout->addWidget(information_label_);
	layout->addStretch();
}

void InspectorPanel::SetObjectName(
	const QString& object_name
)
{
	title_label_->setText(object_name);

	transform_label_->show();

	information_label_->setText(
		"Inspector fields are placeholders.\n"
		"Later they will edit data stored in Engine::Scene."
	);
}

void InspectorPanel::ClearSelection()
{
	title_label_->setText("No object selected");

	transform_label_->hide();

	information_label_->setText(
		"Select an object in Hierarchy to inspect it."
	);
}
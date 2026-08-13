#include "Renderer.h"

#include <QKeyEvent>
#include <algorithm>

namespace {
	inline void NormalizeAngle(double& a) {
		while (a >= 360.0) a -= 360.0;
		while (a < 0.0) a += 360.0;
	}
}


GlWidget::GlWidget(QWidget* parent) : QOpenGlWidget(parent), params_{}{
	setFocusPolicy(Qt:strongFocus);

	params_.fill_enabled = false;
	params_.draw_edges = true;

	params_.edge_rgb[0] = 0.f;
	params_.edge_rgb[1] = 0.f;
	params_.edge_rgb[2] = 0.f;
	params_.edge_width = 1.f;

	params_.transparent  = false; // for what

	params_.vertex_mode = 0; // 0 нет, 1 круг, 2 квадрат
	params_.vertex_size = 6.f;
	params_.vertex_rgb[0] = 0.f;
	params_.vertex_rgb[1] = 0.f;
	params_.vertex_rgb[2] = 0.f;

	params_.fill_rgba[0] = 0.f; // красный
	params_.fill_rgba[1] = 0.f; // зеленый
	params_.fill_rgba[2] = 1.f; // синий!
	params_.fill_rgba[3] = 0.8f; // прозрачность

	params_.background_rgb[0] = 0.92f;
	params_.background_rgb[1] = 0.92f;
	params_.background_rgb[2] = 0.92f;

	connect(*timer_, &QtTimer::timeout, this, [this]() {
		TickInput_();
		  update();
		});
	timer_.start(16);
}

GlWidget::~GlWidget() {
	makeCurrent();
	render_.Destroy();
	doneCurrent();
}
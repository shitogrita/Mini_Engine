#include "Editor/EditorWindow.h"
#include "Engine/Tests/SceneUpdateCorrectnessTest.h"

#include <QApplication>
#include <QSurfaceFormat>

#include <iostream>

int main(int argc, char* argv[]) {
	/**
	 * Mini Engine использует OpenGL 3.3 Core.
	 *
	 * Формат необходимо установить до создания QApplication,
	 * чтобы QOpenGLWidget получил правильный OpenGL Context.
	 */
	QSurfaceFormat format;

	format.setRenderableType(
		QSurfaceFormat::OpenGL
	);

	format.setVersion(
		3,
		3
	);

	format.setProfile(
		QSurfaceFormat::CoreProfile
	);

	format.setDepthBufferSize(
		24
	);

	format.setStencilBufferSize(
		8
	);

	QSurfaceFormat::setDefaultFormat(
		format
	);

	QApplication application(
		argc,
		argv
	);

#ifndef NDEBUG
	/**
	 * Проверяем, что SingleThreaded и MultiThreaded
	 * дают одинаковый результат обновления Scene.
	 */
	const bool scene_update_correct =
		SceneUpdateCorrectnessTest::Run();

	std::cout
		<< "SceneUpdate correctness test: "
		<< (
			scene_update_correct
				? "PASSED"
				: "FAILED"
		)
		<< '\n';

	if (!scene_update_correct) {
		return 1;
	}
#endif

	EditorWindow window;

	window.show();

	return application.exec();
}
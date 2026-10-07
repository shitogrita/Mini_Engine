#include "Editor/EditorWindow.h"
#include "Engine/Tests/ThreadPoolTest100.h"

#include <iostream>
#include <QApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>



int main(int argc, char* argv[])
{
	//  проверка многопоточки
	const bool thread_pool_ok = ThreadPoolTest::Run();

	std::cout
		<< "ThreadPool test: "
		<< (thread_pool_ok ? "PASSED" : "FAILED")
		<< '\n';
	//  проверка многопоточки

	QSurfaceFormat format;

	format.setVersion(3, 3);
	format.setProfile(
		QSurfaceFormat::CoreProfile
	);

#ifdef __APPLE__
	format.setOption(
		QSurfaceFormat::DeprecatedFunctions,
		false
	);
#endif

	format.setDepthBufferSize(24);

	QSurfaceFormat::setDefaultFormat(format);


	QApplication application(argc, argv);

	QCoreApplication::setApplicationName(
		"Mini Engine Editor"
	);

	QCoreApplication::setOrganizationName(
		"MiniEngine"
	);


	EditorWindow editor_window;
	editor_window.show();


	return application.exec();
}



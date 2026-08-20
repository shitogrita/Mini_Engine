#include "Editor/EditorWindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char* argv[])
{
	QApplication application(argc, argv);

	QCoreApplication::setApplicationName("Mini Engine Editor");
	QCoreApplication::setOrganizationName("MiniEngine");

	EditorWindow editor_window;
	editor_window.show();

	return application.exec();
}
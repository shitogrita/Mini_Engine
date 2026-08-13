#pragma once

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QSet>
#include <QTimer>


class GlWidget:  public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
	Q_OBJECT

	  public:
		explicit GlWidget(QWidget parent = nullptr);
		~GlWidget();

	  private:

};

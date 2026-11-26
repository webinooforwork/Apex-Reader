#include "main_window.h"
#include <QApplication>
#include <QCommandLineParser>
int main(int argc,char** argv){QApplication app(argc,argv); QApplication::setApplicationName("APEX OFFICE READER"); QApplication::setOrganizationName("APEX"); apex::MainWindow window; window.show(); return app.exec();}

#pragma once
#include <QMainWindow>
class QPlainTextEdit;
class QLabel;
namespace apex { class MainWindow : public QMainWindow { Q_OBJECT public: explicit MainWindow(QWidget* parent=nullptr); private slots: void openFile(); void showCapabilities(); private: void loadPath(const QString&); QPlainTextEdit* view_; QLabel* status_; }; }

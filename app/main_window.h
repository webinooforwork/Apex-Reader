#pragma once
#include <QMainWindow>
class QPlainTextEdit;
class QLabel;
class QStackedWidget;
class QPdfDocument;
class QPdfSearchModel;
class QPdfView;
class QSpinBox;
namespace apex {
class MainWindow : public QMainWindow {
public:
 explicit MainWindow(QWidget* parent=nullptr);
private:
 void openFile();
 void showCapabilities();
 void loadPath(const QString& path);
 void loadPdf(const QString& path);
 QPlainTextEdit* view_{};
 QLabel* status_{};
 QStackedWidget* documentStack_{};
 QPdfDocument* pdfDocument_{};
 QPdfSearchModel* pdfSearch_{};
 QPdfView* pdfView_{};
 QSpinBox* pdfPage_{};
};
}

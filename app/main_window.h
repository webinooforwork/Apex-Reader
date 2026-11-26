#pragma once
#include <QMainWindow>
#include <QString>
class QPlainTextEdit;
class QLabel;
class QStackedWidget;
class QPdfDocument;
class QPdfBookmarkModel;
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
 void showDocumentDna();
 void loadPath(const QString& path);
 void loadPdf(const QString& path);
 QPlainTextEdit* view_{};
 QLabel* status_{};
 QStackedWidget* documentStack_{};
 QPdfDocument* pdfDocument_{};
 QPdfSearchModel* pdfSearch_{};
 QPdfBookmarkModel* pdfBookmarks_{};
 QPdfView* pdfView_{};
 QSpinBox* pdfPage_{};
 QString currentPath_;
 QString documentText_;
 QString documentKind_;
};
}

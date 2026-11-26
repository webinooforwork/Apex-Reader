#pragma once
#include <QMainWindow>
#include <QString>
#include <QPointer>
#include <QThread>
#include "../search/document_index.h"
#include "../engines/spreadsheet/csv_document.h"
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
 ~MainWindow() override;
private:
 void openFile();
 void showCapabilities();
 void showDocumentDna();
 void indexFolder();
 void searchIndexedDocuments();
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
 QPointer<QThread> indexThread_;
 QPointer<IndexTask> indexTask_;
 CsvDocument* csvDocument_{};
 QTableView* csvView_{};
 QLineEdit* csvFind_{};
 QWidget* csvPage_{};
};
}

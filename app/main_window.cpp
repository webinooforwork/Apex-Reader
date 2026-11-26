#include "main_window.h"
#include "../core/capability_registry.h"
#include "../engines/spreadsheet/ods_workbook.h"
#include "../engines/text_document.h"
#include "../engines/word/docx_document.h"
#include "../engines/word/opendocument_text.h"
#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListView>
#include <QListWidget>
#include <QLocale>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableView>
#include <QTreeView>
#include <QTreeWidget>
#include <QVBoxLayout>
#ifdef APEX_HAS_PDF
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfPageNavigator>
#include <QPdfSearchModel>
#include <QPdfView>
#include <QSignalBlocker>
#endif
namespace apex {
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), view_(new QPlainTextEdit(this)),
      status_(new QLabel("Open a supported document", this)),
      documentStack_(new QStackedWidget(this)) {
  setWindowTitle("APEX OFFICE READER");
  resize(1200, 800);
  view_->setReadOnly(true);
  view_->setLineWrapMode(QPlainTextEdit::NoWrap);
  documentStack_->addWidget(view_);
  csvDocument_ = new CsvDocument(this);
  csvPage_ = new QWidget(this);
  auto *csvLayout = new QVBoxLayout(csvPage_);
  auto *csvTools = new QHBoxLayout;
  csvFind_ = new QLineEdit(csvPage_);
  csvFind_->setPlaceholderText("Find cell text");
  csvFind_->setClearButtonEnabled(true);
  auto *findNext = new QPushButton("Find next", csvPage_);
  csvTools->addWidget(csvFind_, 1);
  csvTools->addWidget(findNext);
  csvLayout->addLayout(csvTools);
  csvView_ = new QTableView(csvPage_);
  csvView_->setModel(csvDocument_);
  csvView_->setSelectionBehavior(QAbstractItemView::SelectItems);
  csvView_->setSelectionMode(QAbstractItemView::SingleSelection);
  csvView_->setAlternatingRowColors(true);
  csvView_->setWordWrap(false);
  csvView_->horizontalHeader()->setSectionsMovable(true);
  csvLayout->addWidget(csvView_, 1);
  documentStack_->addWidget(csvPage_);
  connect(findNext, &QPushButton::clicked, this, [this] {
    const auto match =
        csvDocument_->findNext(csvFind_->text(), csvView_->currentIndex());
    if (match.isValid()) {
      csvView_->setCurrentIndex(match);
      csvView_->scrollTo(match, QAbstractItemView::PositionAtCenter);
    } else if (!csvFind_->text().isEmpty())
      status_->setText("No matching cell found in this table.");
  });
  connect(csvFind_, &QLineEdit::returnPressed, findNext, &QPushButton::click);
  xlsxPage_ = new QWidget(this);
  auto *xlsxLayout = new QVBoxLayout(xlsxPage_);
  formulaToggle_ =
      new QCheckBox("Show formulas instead of cached values", xlsxPage_);
  xlsxLayout->addWidget(formulaToggle_);
  xlsxTabs_ = new QTabWidget(xlsxPage_);
  xlsxLayout->addWidget(xlsxTabs_, 1);
  documentStack_->addWidget(xlsxPage_);
  connect(formulaToggle_, &QCheckBox::toggled, this, [this](bool enabled) {
    for (auto *model : spreadsheetModels_)
      model->setShowFormulas(enabled);
  });
#ifdef APEX_HAS_PDF
  pdfDocument_ = new QPdfDocument(this);
  pdfSearch_ = new QPdfSearchModel(this);
  pdfSearch_->setDocument(pdfDocument_);
  pdfBookmarks_ = new QPdfBookmarkModel(this);
  pdfBookmarks_->setDocument(pdfDocument_);
  auto *pdfPage = new QWidget(this);
  auto *pageLayout = new QVBoxLayout(pdfPage);
  pageLayout->setContentsMargins(0, 0, 0, 0);
  auto *controls = new QHBoxLayout;
  auto *search = new QLineEdit(pdfPage);
  search->setPlaceholderText("Search PDF text");
  search->setClearButtonEnabled(true);
  controls->addWidget(search, 1);
  auto *pageLabel = new QLabel("Page", pdfPage);
  controls->addWidget(pageLabel);
  pdfPage_ = new QSpinBox(pdfPage);
  pdfPage_->setMinimum(1);
  pdfPage_->setMaximum(1);
  controls->addWidget(pdfPage_);
  auto *zoom = new QComboBox(pdfPage);
  zoom->addItem("Fit width");
  zoom->addItem("Fit page");
  zoom->addItem("100%");
  controls->addWidget(zoom);
  auto *pageMode = new QComboBox(pdfPage);
  pageMode->addItem("Continuous");
  pageMode->addItem("Single page");
  controls->addWidget(pageMode);
  pageLayout->addLayout(controls);
  auto *body = new QHBoxLayout;
  auto *sideTabs = new QTabWidget(pdfPage);
  sideTabs->setMinimumWidth(230);
  auto *results = new QListView(sideTabs);
  results->setModel(pdfSearch_);
  sideTabs->addTab(results, "Find");
  auto *outline = new QTreeView(sideTabs);
  outline->setModel(pdfBookmarks_);
  outline->setHeaderHidden(true);
  sideTabs->addTab(outline, "Outline");
  body->addWidget(sideTabs);
  pdfView_ = new QPdfView(pdfPage);
  pdfView_->setDocument(pdfDocument_);
  pdfView_->setSearchModel(pdfSearch_);
  pdfView_->setPageMode(QPdfView::PageMode::MultiPage);
  body->addWidget(pdfView_, 1);
  pageLayout->addLayout(body, 1);
  documentStack_->addWidget(pdfPage);
  connect(search, &QLineEdit::textChanged, pdfSearch_,
          &QPdfSearchModel::setSearchString);
  connect(results, &QListView::doubleClicked, this,
          [this](const QModelIndex &index) {
            if (pdfSearch_ && pdfView_)
              pdfView_->pageNavigator()->jump(
                  pdfSearch_->resultAtIndex(index.row()));
          });
  connect(
      outline, &QTreeView::doubleClicked, this,
      [this](const QModelIndex &index) {
        if (!pdfView_)
          return;
        const int page =
            index.data(static_cast<int>(QPdfBookmarkModel::Role::Page)).toInt();
        const QPointF location =
            index.data(static_cast<int>(QPdfBookmarkModel::Role::Location))
                .toPointF();
        const qreal zoom =
            index.data(static_cast<int>(QPdfBookmarkModel::Role::Zoom))
                .toReal();
        pdfView_->pageNavigator()->jump(page, location, zoom);
      });
  connect(pdfPage_, qOverload<int>(&QSpinBox::valueChanged), this,
          [this](int page) {
            if (pdfView_)
              pdfView_->pageNavigator()->jump(page - 1, QPointF());
          });
  connect(pdfView_->pageNavigator(), &QPdfPageNavigator::currentPageChanged,
          this, [this](int page) {
            if (pdfPage_ && page >= 0) {
              const QSignalBlocker blocker(pdfPage_);
              pdfPage_->setValue(page + 1);
            }
          });
  connect(pdfDocument_, &QPdfDocument::pageCountChanged, this,
          [this](int count) {
            if (pdfPage_)
              pdfPage_->setMaximum(qMax(1, count));
          });
  connect(zoom, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int index) {
            if (!pdfView_)
              return;
            if (index == 0)
              pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);
            else if (index == 1)
              pdfView_->setZoomMode(QPdfView::ZoomMode::FitInView);
            else {
              pdfView_->setZoomMode(QPdfView::ZoomMode::Custom);
              pdfView_->setZoomFactor(1.0);
            }
          });
  connect(pageMode, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int index) {
            if (pdfView_)
              pdfView_->setPageMode(index == 0
                                        ? QPdfView::PageMode::MultiPage
                                        : QPdfView::PageMode::SinglePage);
          });
#endif
  auto *root = new QWidget(this);
  auto *layout = new QVBoxLayout(root);
  layout->setContentsMargins(12, 10, 12, 8);
  layout->addWidget(status_);
  layout->addWidget(documentStack_, 1);
  setCentralWidget(root);
  auto *file = menuBar()->addMenu("File");
  auto *open = file->addAction("Open…");
  open->setShortcut(QKeySequence::Open);
  connect(open, &QAction::triggered, this, [this] { openFile(); });
#ifdef APEX_HAS_PDF
  auto *exportText = file->addAction("Export PDF text…");
  connect(exportText, &QAction::triggered, this, [this] {
    if (!pdfDocument_ || pdfDocument_->status() != QPdfDocument::Status::Ready)
      return;
    const auto path = QFileDialog::getSaveFileName(
        this, "Export extracted PDF text", {}, "Text files (*.txt)");
    if (path.isEmpty())
      return;
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QMessageBox::warning(this, "Export failed", out.errorString());
      return;
    }
    for (int page = 0; page < pdfDocument_->pageCount(); ++page) {
      const QByteArray text = pdfDocument_->getAllText(page).text().toUtf8();
      out.write(text);
      out.write("\n\n");
    }
    if (!out.commit())
      QMessageBox::warning(this, "Export failed", out.errorString());
  });
#endif
#ifdef APEX_HAS_PDF
  auto *details = file->addAction("PDF details");
  connect(details, &QAction::triggered, this, [this] {
    if (!pdfDocument_ ||
        pdfDocument_->status() != QPdfDocument::Status::Ready) {
      QMessageBox::information(
          this, "Document details",
          "Open a PDF to inspect its engine-provided metadata.");
      return;
    }
    const auto field = [this](QPdfDocument::MetaDataField key) {
      const auto value = pdfDocument_->metaData(key);
      return value.isValid() && !value.toString().isEmpty()
                 ? value.toString()
                 : QString("(not provided)");
    };
    const QString report =
        QString("Title: %1\nAuthor: %2\nSubject: %3\nKeywords: %4\nCreator: "
                "%5\nProducer: %6\nCreated: %7\nModified: %8\nPages: %9")
            .arg(field(QPdfDocument::MetaDataField::Title),
                 field(QPdfDocument::MetaDataField::Author),
                 field(QPdfDocument::MetaDataField::Subject),
                 field(QPdfDocument::MetaDataField::Keywords),
                 field(QPdfDocument::MetaDataField::Creator),
                 field(QPdfDocument::MetaDataField::Producer),
                 field(QPdfDocument::MetaDataField::CreationDate),
                 field(QPdfDocument::MetaDataField::ModificationDate))
            .arg(pdfDocument_->pageCount());
    QMessageBox::information(this, "PDF metadata", report);
  });
#endif
  auto *library = menuBar()->addMenu("Library");
  auto *index = library->addAction("Index folder…");
  connect(index, &QAction::triggered, this, [this] { indexFolder(); });
  auto *searchIndex = library->addAction("Search indexed documents…");
  searchIndex->setShortcut(QKeySequence("Ctrl+Shift+F"));
  connect(searchIndex, &QAction::triggered, this,
          [this] { searchIndexedDocuments(); });
  auto *inspect = menuBar()->addMenu("Inspect");
  auto *dna = inspect->addAction("Document DNA…");
  dna->setShortcut(QKeySequence("Ctrl+Shift+D"));
  connect(dna, &QAction::triggered, this, [this] { showDocumentDna(); });
  auto *capabilities = file->addAction("Capabilities");
  connect(capabilities, &QAction::triggered, this,
          [this] { showCapabilities(); });
  file->addSeparator();
  auto *exit = file->addAction("Exit");
  connect(exit, &QAction::triggered, this, [this] { close(); });
}
MainWindow::~MainWindow() {
  if (indexTask_)
    indexTask_->cancel();
  if (indexThread_ && indexThread_->isRunning())
    indexThread_->wait();
}
void MainWindow::openFile() {
  const auto path = QFileDialog::getOpenFileName(
      this, "Open document", {},
      "Documents (*.pdf *.docx *.xlsx *.ods *.odt *.odp *.txt *.log "
      "*.md *.csv *.tsv);;All files (*.*)");
  if (!path.isEmpty())
    loadPath(path);
}
void MainWindow::displaySpreadsheet(const QVector<SpreadsheetSheet> &sheets,
                                    const QString &path, const QString &kind,
                                    const QString &searchableText,
                                    int cellCount) {
  while (xlsxTabs_->count() > 0) {
    auto *page = xlsxTabs_->widget(0);
    xlsxTabs_->removeTab(0);
    delete page;
  }
  for (auto *model : spreadsheetModels_)
    delete model;
  spreadsheetModels_.clear();
  for (const auto &sheet : sheets) {
    auto *model = new SpreadsheetTableModel(sheet, xlsxPage_);
    model->setShowFormulas(formulaToggle_->isChecked());
    spreadsheetModels_.append(model);
    auto *table = new QTableView(xlsxTabs_);
    table->setModel(model);
    table->setAlternatingRowColors(true);
    table->setWordWrap(false);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    xlsxTabs_->addTab(table, sheet.name);
  }
  documentText_ = searchableText;
  currentPath_ = path;
  documentKind_ = kind;
  documentStack_->setCurrentWidget(xlsxPage_);
  setWindowTitle(
      QString("%1 — APEX OFFICE READER").arg(QFileInfo(path).fileName()));
  status_->setText(
      QString("%1 • %2 sheets • %3 non-empty cells • %4 • Read-only")
          .arg(QFileInfo(path).fileName())
          .arg(sheets.size())
          .arg(cellCount)
          .arg(kind));
}

void MainWindow::loadPath(const QString &path) {
  const auto ext = QFileInfo(path).suffix().toLower();
#ifdef APEX_HAS_PDF
  if (ext == "pdf") {
    loadPdf(path);
    return;
  }
#endif
  if (ext == "xlsx") {
    XlsxWorkbook workbook;
    if (!workbook.load(path)) {
      QMessageBox::warning(this, "Could not inspect XLSX", workbook.error);
      return;
    }
    displaySpreadsheet(workbook.sheets, path, "XLSX cached values and formulas",
                       workbook.searchableText(), workbook.cellCount);
    return;
  }
  if (ext == "ods") {
    OdsWorkbook workbook;
    if (!workbook.load(path)) {
      QMessageBox::warning(this, "Could not inspect ODS", workbook.error);
      return;
    }
    displaySpreadsheet(workbook.sheets, path, "ODS cell values and formulas",
                       workbook.searchableText(), workbook.cellCount);
    return;
  }
  if (ext == "odt" || ext == "odp") {
    OpenDocumentText document;
    const bool presentation = ext == "odp";
    if (!document.load(path, presentation)) {
      QMessageBox::warning(this, "Could not inspect OpenDocument",
                           document.error);
      return;
    }
    documentText_ = document.text;
    currentPath_ = path;
    documentKind_ =
        presentation ? "ODP slide text extraction" : "ODT text extraction";
    view_->setPlainText(document.text);
    documentStack_->setCurrentWidget(view_);
    setWindowTitle(
        QString("%1 — APEX OFFICE READER").arg(QFileInfo(path).fileName()));
    status_->setText(
        presentation
            ? QString("%1 • %2 slides • Extracted slide text • Read-only")
                  .arg(QFileInfo(path).fileName())
                  .arg(document.slideCount)
            : QString(
                  "%1 • %2 paragraphs • Extracted document text • Read-only")
                  .arg(QFileInfo(path).fileName())
                  .arg(document.paragraphCount));
    return;
  }
  if (ext == "csv" || ext == "tsv") {
    QString error;
    const QChar delimiter = ext == "tsv" ? QChar('\t') : QChar(',');
    if (!csvDocument_->load(path, delimiter, &error)) {
      QMessageBox::warning(this, "Could not read delimited file", error);
      return;
    }
    documentText_ = csvDocument_->rawText();
    currentPath_ = path;
    documentKind_ = ext.toUpper() + " table";
    documentStack_->setCurrentWidget(csvPage_);
    setWindowTitle(
        QString("%1 — APEX OFFICE READER").arg(QFileInfo(path).fileName()));
    status_->setText(
        QString("%1 • %2 rows × %3 columns • Delimited text table • Read-only")
            .arg(QFileInfo(path).fileName())
            .arg(csvDocument_->rowCount())
            .arg(csvDocument_->columnCount()));
    return;
  }
  if (ext == "docx") {
    DocxDocument document;
    if (!document.load(path)) {
      QMessageBox::warning(this, "Could not inspect DOCX", document.error);
      return;
    }
    documentText_ = document.text;
    currentPath_ = path;
    documentKind_ = "DOCX text extraction";
    view_->setPlainText(document.text);
    documentStack_->setCurrentWidget(view_);
    setWindowTitle(
        QString("%1 — APEX OFFICE READER").arg(QFileInfo(path).fileName()));
    status_->setText(
        QString(
            "%1 • DOCX extracted text • %2 paragraphs • %3 words • Read-only")
            .arg(QFileInfo(path).fileName())
            .arg(document.paragraphCount)
            .arg(document.words()));
    return;
  }
  const auto *capability = CapabilityRegistry::find(ext.toStdString());
  if (!capability || capability->support == Support::Unsupported) {
    QMessageBox::information(
        this, "Format not supported",
        QString("%1\n\n%2")
            .arg(capability ? QString::fromStdString(capability->format)
                            : "Unknown format",
                 capability ? QString::fromStdString(capability->note)
                            : "No reader is registered for this extension."));
    return;
  }
  TextDocument document;
  if (!document.load(path)) {
    QMessageBox::warning(this, "Could not open document", document.error);
    return;
  }
  documentText_ = document.text;
  currentPath_ = path;
  documentKind_ = "Plain text";
  view_->setPlainText(document.text);
  documentStack_->setCurrentWidget(view_);
  setWindowTitle(
      QString("%1 — APEX OFFICE READER").arg(QFileInfo(path).fileName()));
  status_->setText(QString("%1 • %2 words • Read-only text view")
                       .arg(QFileInfo(path).fileName())
                       .arg(document.words()));
}
void MainWindow::loadPdf(const QString &path) {
#ifdef APEX_HAS_PDF
  pdfDocument_->close();
  auto error = pdfDocument_->load(path);
  if (error == QPdfDocument::Error::IncorrectPassword) {
    bool ok = false;
    const auto password = QInputDialog::getText(
        this, "Password required",
        "Enter the PDF password:", QLineEdit::Password, {}, &ok);
    if (!ok)
      return;
    pdfDocument_->setPassword(password);
    error = pdfDocument_->load(path);
  }
  if (error != QPdfDocument::Error::None) {
    QMessageBox::warning(this, "Could not open PDF",
                         QString("The PDF engine rejected this file (error "
                                 "%1). No repair or validation was attempted.")
                             .arg(static_cast<int>(error)));
    return;
  }
  currentPath_ = path;
  documentText_.clear();
  documentKind_ = "PDF text extraction";
  documentStack_->setCurrentWidget(pdfView_->parentWidget());
  pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);
  pdfPage_->setValue(1);
  const auto title = QFileInfo(path).fileName();
  setWindowTitle(QString("%1 — APEX OFFICE READER").arg(title));
  status_->setText(QString("%1 • PDFium via Qt PDF • %2 pages • Read-only")
                       .arg(title)
                       .arg(pdfDocument_->pageCount()));
#else
  Q_UNUSED(path);
  QMessageBox::information(this, "PDF engine unavailable",
                           "This build was compiled without Qt PDF. The PDF "
                           "capability is disabled for this build.");
#endif
}

void MainWindow::indexFolder() {
  if (indexThread_ && indexThread_->isRunning()) {
    QMessageBox::information(
        this, "Indexing in progress",
        "Wait for the current folder scan to finish before starting another.");
    return;
  }
  const QString folder =
      QFileDialog::getExistingDirectory(this, "Choose a folder to index");
  if (folder.isEmpty())
    return;
  const QString databasePath = DocumentIndex::defaultDatabasePath();
  auto *thread = new QThread(this);
  auto *task = new IndexTask(folder, databasePath);
  task->moveToThread(thread);
  indexThread_ = thread;
  indexTask_ = task;
  QDialog progress(this);
  progress.setWindowTitle("Index document folder");
  progress.setWindowFlag(Qt::WindowCloseButtonHint, false);
  progress.setMinimumWidth(500);
  auto *layout = new QVBoxLayout(&progress);
  auto *label =
      new QLabel("Preparing supported text and DOCX files…", &progress);
  label->setWordWrap(true);
  layout->addWidget(label);
  auto *bar = new QProgressBar(&progress);
  bar->setRange(0, 0);
  layout->addWidget(bar);
  auto *cancel = new QPushButton("Cancel indexing", &progress);
  layout->addWidget(cancel);
  connect(thread, &QThread::started, task, &IndexTask::run);
  const auto cancelConnection =
      connect(cancel, &QPushButton::clicked, this, [task, label] {
        task->cancel();
        label->setText("Cancelling safely after the current file…");
      });
  connect(task, &IndexTask::progress, &progress,
          [label, bar](int completed, int total, const QString &path) {
            bar->setRange(0, qMax(1, total));
            bar->setValue(completed);
            label->setText(path.isEmpty() ? QStringLiteral("Saving index…")
                                          : QFileInfo(path).fileName());
          });
  connect(
      task, &IndexTask::finished, &progress,
      [&progress, label, cancel, cancelConnection](
          int indexed, int skipped, bool wasCancelled, const QString &error) {
        cancel->setText("Done");
        QObject::disconnect(cancelConnection);
        QString summary =
            QString("Indexed or refreshed %1 files; %2 files were skipped.")
                .arg(indexed)
                .arg(skipped);
        if (wasCancelled)
          summary +=
              QStringLiteral(" Scan cancelled; completed entries were saved.");
        if (!error.isEmpty())
          summary += QStringLiteral("\nDatabase error: ") + error;
        label->setText(summary);
        connect(cancel, &QPushButton::clicked, &progress, &QDialog::accept);
      });
  connect(task, &IndexTask::finished, thread, &QThread::quit);
  connect(thread, &QThread::finished, task, &QObject::deleteLater);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);
  QPointer<QThread> threadGuard(thread);
  thread->start();
  progress.exec();
  if (threadGuard && threadGuard->isRunning())
    threadGuard->wait();
  indexTask_.clear();
  indexThread_.clear();
}

void MainWindow::searchIndexedDocuments() {
  bool ok = false;
  const QString query = QInputDialog::getText(
      this, "Search APEX index",
      "Search words (all terms must match):", QLineEdit::Normal, {}, &ok);
  if (!ok || query.trimmed().isEmpty())
    return;
  const QString databasePath = DocumentIndex::defaultDatabasePath();
  QString error;
  const auto hits = DocumentIndex::search(query, databasePath, &error);
  if (!error.isEmpty()) {
    QMessageBox::information(
        this, "APEX search",
        QString("The local index could not be searched: %1\n\nUse Library → "
                "Index folder to build or refresh it.")
            .arg(error));
    return;
  }
  QDialog results(this);
  results.setWindowTitle(QString("Search results · %1").arg(query));
  results.resize(760, 520);
  auto *layout = new QVBoxLayout(&results);
  auto *list = new QListWidget(&results);
  layout->addWidget(list);
  auto *hint = new QLabel("Double-click a result to open it. Excerpts are "
                          "extracted text and may omit layout.",
                          &results);
  layout->addWidget(hint);
  for (const auto &hit : hits) {
    auto *item = new QListWidgetItem(
        QString("%1   ·   .%2")
            .arg(QFileInfo(hit.path).fileName(), hit.extension),
        list);
    item->setData(Qt::UserRole, hit.path);
    item->setToolTip(hit.excerpt);
  }
  if (hits.isEmpty())
    list->addItem("No indexed documents matched every search term.");
  connect(list, &QListWidget::itemDoubleClicked, &results,
          [this, &results](QListWidgetItem *item) {
            const QString path = item->data(Qt::UserRole).toString();
            if (!path.isEmpty()) {
              results.accept();
              loadPath(path);
            }
          });
  results.exec();
}

void MainWindow::showDocumentDna() {
  if (currentPath_.isEmpty()) {
    QMessageBox::information(this, "Document DNA",
                             "Open a supported document first.");
    return;
  }
  QString content = documentText_;
#ifdef APEX_HAS_PDF
  if (documentStack_->currentWidget() == pdfView_->parentWidget() &&
      pdfDocument_ && pdfDocument_->status() == QPdfDocument::Status::Ready) {
    QStringList pages;
    for (int i = 0; i < pdfDocument_->pageCount(); ++i)
      pages.append(pdfDocument_->getAllText(i).text());
    content = pages.join(QChar('\n'));
  }
#endif
  const QStringList lines = content.split(QRegularExpression("\\r?\\n"));
  QStringList paragraphs;
  QStringList headings;
  QString block;
  for (const auto &line : lines) {
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) {
      if (!block.trimmed().isEmpty())
        paragraphs.append(block.trimmed());
      block.clear();
      continue;
    }
    if (trimmed.startsWith('#')) {
      if (!block.trimmed().isEmpty())
        paragraphs.append(block.trimmed());
      block.clear();
      headings.append(trimmed);
      continue;
    }
    if (!block.isEmpty())
      block += QChar(' ');
    block += trimmed;
  }
  if (!block.trimmed().isEmpty())
    paragraphs.append(block.trimmed());
  QHash<QString, int> frequency;
  qsizetype characters = 0;
  qsizetype longest = 0;
  int shortBlocks = 0, mediumBlocks = 0, longBlocks = 0;
  for (const auto &paragraph : paragraphs) {
    characters += paragraph.size();
    longest = qMax(longest, static_cast<qsizetype>(paragraph.size()));
    ++frequency[paragraph.toCaseFolded()];
    if (paragraph.size() < 160)
      ++shortBlocks;
    else if (paragraph.size() < 600)
      ++mediumBlocks;
    else
      ++longBlocks;
  }
  qsizetype repeated = 0;
  for (auto it = frequency.cbegin(); it != frequency.cend(); ++it)
    if (it.value() > 1)
      repeated += it.value() - 1;
  const qsizetype words =
      content.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
  const int minutes = static_cast<int>((words + 199) / 200);
  QDialog dialog(this);
  dialog.setWindowTitle("APEX Document DNA");
  dialog.resize(760, 620);
  auto *outer = new QVBoxLayout(&dialog);
  auto *title =
      new QLabel(QString("<h2>%1</h2><p>%2 · %3</p>")
                     .arg(QFileInfo(currentPath_).fileName().toHtmlEscaped(),
                          documentKind_.toHtmlEscaped()),
                 &dialog);
  outer->addWidget(title);
  auto *form = new QFormLayout;
  form->addRow(
      "Words",
      new QLabel(QLocale().toString(static_cast<qlonglong>(words)), &dialog));
  form->addRow("Characters", new QLabel(QLocale().toString(
                                            static_cast<qlonglong>(characters)),
                                        &dialog));
  form->addRow(
      "Paragraphs",
      new QLabel(QLocale().toString(static_cast<qlonglong>(paragraphs.size())),
                 &dialog));
  form->addRow(
      "Estimated reading time",
      new QLabel(QString("%1 min at 200 words/min").arg(minutes), &dialog));
  form->addRow("Repeated paragraph instances",
               new QLabel(QLocale().toString(static_cast<qlonglong>(repeated)),
                          &dialog));
  form->addRow(
      "Longest paragraph",
      new QLabel(QString("%1 characters").arg(static_cast<qlonglong>(longest)),
                 &dialog));
  outer->addLayout(form);
  outer->addWidget(new QLabel("Paragraph length profile", &dialog));
  const int maximum =
      qMax(1, qMax(shortBlocks, qMax(mediumBlocks, longBlocks)));
  const QList<QPair<QString, int>> groups{
      {"Short · under 160 chars", shortBlocks},
      {"Medium · 160–599 chars", mediumBlocks},
      {"Long · 600+ chars", longBlocks}};
  for (const auto &item : groups) {
    auto *row = new QHBoxLayout;
    row->addWidget(new QLabel(item.first, &dialog), 1);
    auto *bar = new QProgressBar(&dialog);
    bar->setRange(0, maximum);
    bar->setValue(item.second);
    bar->setFormat(QString::number(item.second));
    row->addWidget(bar, 2);
    outer->addLayout(row);
  }
  auto *tree = new QTreeWidget(&dialog);
  tree->setHeaderLabels({"Document structure and repeated content"});
  auto *root = new QTreeWidgetItem(
      tree, {QString("Content · %1 paragraphs · %2 headings")
                 .arg(static_cast<qlonglong>(paragraphs.size()))
                 .arg(headings.size())});
  for (const auto &heading : headings)
    new QTreeWidgetItem(root, {heading});
  int repeatsShown = 0;
  for (auto it = frequency.cbegin();
       it != frequency.cend() && repeatsShown < 100; ++it)
    if (it.value() > 1) {
      new QTreeWidgetItem(root, {QString("Repeated %1× · %2")
                                     .arg(it.value())
                                     .arg(it.key().left(120))});
      ++repeatsShown;
    }
  root->setExpanded(true);
  tree->setMinimumHeight(180);
  outer->addWidget(tree, 1);
  auto *note = new QLabel("Computed from text exposed by the active reader. "
                          "Layout, images, and unsupported embedded content "
                          "are not included. Reading time is an estimate.",
                          &dialog);
  note->setWordWrap(true);
  outer->addWidget(note);
  dialog.exec();
}

void MainWindow::showCapabilities() {
  QString report = "Format capability registry (current implementation):\n\n";
  for (const auto &c : CapabilityRegistry::all())
    report +=
        QString(".%1 — %2: %3. %4\n")
            .arg(QString::fromStdString(c.extension),
                 QString::fromStdString(CapabilityRegistry::label(c.support)),
                 QString::fromStdString(c.format),
                 QString::fromStdString(c.note));
  QMessageBox::information(this, "APEX format support", report);
}
} // namespace apex

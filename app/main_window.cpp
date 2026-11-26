#include "main_window.h"
#include "../core/capability_registry.h"
#include "../engines/text_document.h"
#include <QAction>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#ifdef APEX_HAS_PDF
#include <QPdfDocument>
#include <QPdfBookmarkModel>
#include <QPdfSearchModel>
#include <QPdfPageNavigator>
#include <QSignalBlocker>
#include <QPdfView>
#endif
namespace apex {
MainWindow::MainWindow(QWidget* parent):QMainWindow(parent),view_(new QPlainTextEdit(this)),status_(new QLabel("Open a supported document",this)),documentStack_(new QStackedWidget(this)) {
 setWindowTitle("APEX OFFICE READER"); resize(1200,800);
 view_->setReadOnly(true); view_->setLineWrapMode(QPlainTextEdit::NoWrap);
 documentStack_->addWidget(view_);
#ifdef APEX_HAS_PDF
 pdfDocument_=new QPdfDocument(this); pdfSearch_=new QPdfSearchModel(this); pdfSearch_->setDocument(pdfDocument_); pdfBookmarks_=new QPdfBookmarkModel(this); pdfBookmarks_->setDocument(pdfDocument_);
 auto* pdfPage=new QWidget(this); auto* pageLayout=new QVBoxLayout(pdfPage); pageLayout->setContentsMargins(0,0,0,0);
 auto* controls=new QHBoxLayout; auto* search=new QLineEdit(pdfPage); search->setPlaceholderText("Search PDF text"); search->setClearButtonEnabled(true); controls->addWidget(search,1);
 auto* pageLabel=new QLabel("Page",pdfPage); controls->addWidget(pageLabel); pdfPage_=new QSpinBox(pdfPage); pdfPage_->setMinimum(1); pdfPage_->setMaximum(1); controls->addWidget(pdfPage_);
 auto* zoom=new QComboBox(pdfPage); zoom->addItem("Fit width"); zoom->addItem("Fit page"); zoom->addItem("100%"); controls->addWidget(zoom);
 auto* pageMode=new QComboBox(pdfPage); pageMode->addItem("Continuous"); pageMode->addItem("Single page"); controls->addWidget(pageMode); pageLayout->addLayout(controls);
 auto* body=new QHBoxLayout; auto* sideTabs=new QTabWidget(pdfPage); sideTabs->setMinimumWidth(230); auto* results=new QListView(sideTabs); results->setModel(pdfSearch_); sideTabs->addTab(results,"Find"); auto* outline=new QTreeView(sideTabs); outline->setModel(pdfBookmarks_); outline->setHeaderHidden(true); sideTabs->addTab(outline,"Outline"); body->addWidget(sideTabs); pdfView_=new QPdfView(pdfPage); pdfView_->setDocument(pdfDocument_); pdfView_->setSearchModel(pdfSearch_); pdfView_->setPageMode(QPdfView::PageMode::MultiPage); body->addWidget(pdfView_,1); pageLayout->addLayout(body,1); documentStack_->addWidget(pdfPage);
 connect(search,&QLineEdit::textChanged,pdfSearch_,&QPdfSearchModel::setSearchString);
 connect(results,&QListView::doubleClicked,this,[this](const QModelIndex& index){if(pdfSearch_&&pdfView_)pdfView_->pageNavigator()->jump(pdfSearch_->resultAtIndex(index.row()));});
 connect(outline,&QTreeView::doubleClicked,this,[this](const QModelIndex& index){if(!pdfView_)return;const int page=index.data(static_cast<int>(QPdfBookmarkModel::Role::Page)).toInt();const QPointF location=index.data(static_cast<int>(QPdfBookmarkModel::Role::Location)).toPointF();const qreal zoom=index.data(static_cast<int>(QPdfBookmarkModel::Role::Zoom)).toReal();pdfView_->pageNavigator()->jump(page,location,zoom);});
 connect(pdfPage_,qOverload<int>(&QSpinBox::valueChanged),this,[this](int page){if(pdfView_)pdfView_->pageNavigator()->jump(page-1,QPointF());});
 connect(pdfView_->pageNavigator(),&QPdfPageNavigator::currentPageChanged,this,[this](int page){if(pdfPage_&&page>=0){const QSignalBlocker blocker(pdfPage_);pdfPage_->setValue(page+1);}});
 connect(pdfDocument_,&QPdfDocument::pageCountChanged,this,[this](int count){if(pdfPage_)pdfPage_->setMaximum(qMax(1,count));});
 connect(zoom,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){if(!pdfView_)return; if(index==0)pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth); else if(index==1)pdfView_->setZoomMode(QPdfView::ZoomMode::FitInView); else {pdfView_->setZoomMode(QPdfView::ZoomMode::Custom);pdfView_->setZoomFactor(1.0);}});
 connect(pageMode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){if(pdfView_)pdfView_->setPageMode(index==0?QPdfView::PageMode::MultiPage:QPdfView::PageMode::SinglePage);});
#endif
 auto* root=new QWidget(this); auto* layout=new QVBoxLayout(root); layout->setContentsMargins(12,10,12,8); layout->addWidget(status_); layout->addWidget(documentStack_,1); setCentralWidget(root);
 auto* file=menuBar()->addMenu("File"); auto* open=file->addAction("Open…"); open->setShortcut(QKeySequence::Open); connect(open,&QAction::triggered,this,[this]{openFile();});
#ifdef APEX_HAS_PDF
 auto* exportText=file->addAction("Export PDF text…"); connect(exportText,&QAction::triggered,this,[this]{if(!pdfDocument_||pdfDocument_->status()!=QPdfDocument::Status::Ready)return; const auto path=QFileDialog::getSaveFileName(this,"Export extracted PDF text",{},"Text files (*.txt)"); if(path.isEmpty())return; QSaveFile out(path); if(!out.open(QIODevice::WriteOnly|QIODevice::Text)){QMessageBox::warning(this,"Export failed",out.errorString());return;} for(int page=0;page<pdfDocument_->pageCount();++page){const QByteArray text=pdfDocument_->getAllText(page).text().toUtf8();out.write(text);out.write("\n\n");} if(!out.commit())QMessageBox::warning(this,"Export failed",out.errorString());});
#endif
#ifdef APEX_HAS_PDF
 auto* details=file->addAction("PDF details"); connect(details,&QAction::triggered,this,[this]{if(!pdfDocument_||pdfDocument_->status()!=QPdfDocument::Status::Ready){QMessageBox::information(this,"Document details","Open a PDF to inspect its engine-provided metadata.");return;}const auto field=[this](QPdfDocument::MetaDataField key){const auto value=pdfDocument_->metaData(key);return value.isValid()&&!value.toString().isEmpty()?value.toString():QString("(not provided)");};const QString report=QString("Title: %1\nAuthor: %2\nSubject: %3\nKeywords: %4\nCreator: %5\nProducer: %6\nCreated: %7\nModified: %8\nPages: %9").arg(field(QPdfDocument::MetaDataField::Title),field(QPdfDocument::MetaDataField::Author),field(QPdfDocument::MetaDataField::Subject),field(QPdfDocument::MetaDataField::Keywords),field(QPdfDocument::MetaDataField::Creator),field(QPdfDocument::MetaDataField::Producer),field(QPdfDocument::MetaDataField::CreationDate),field(QPdfDocument::MetaDataField::ModificationDate)).arg(pdfDocument_->pageCount());QMessageBox::information(this,"PDF metadata",report);});
#endif
 auto* capabilities=file->addAction("Capabilities"); connect(capabilities,&QAction::triggered,this,[this]{showCapabilities();}); file->addSeparator(); auto* exit=file->addAction("Exit"); connect(exit,&QAction::triggered,this,[this]{close();});
}
void MainWindow::openFile(){const auto path=QFileDialog::getOpenFileName(this,"Open document",{},"Documents (*.pdf *.txt *.log *.md *.csv *.tsv);;All files (*.*)");if(!path.isEmpty())loadPath(path);}
void MainWindow::loadPath(const QString& path){const auto ext=QFileInfo(path).suffix().toLower();
#ifdef APEX_HAS_PDF
 if(ext=="pdf"){loadPdf(path);return;}
#endif
 const auto* capability=CapabilityRegistry::find(ext.toStdString());if(!capability||capability->support==Support::Unsupported){QMessageBox::information(this,"Format not supported",QString("%1\n\n%2").arg(capability?QString::fromStdString(capability->format):"Unknown format",capability?QString::fromStdString(capability->note):"No reader is registered for this extension."));return;}
 TextDocument document;if(!document.load(path)){QMessageBox::warning(this,"Could not open document",document.error);return;}view_->setPlainText(document.text);documentStack_->setCurrentWidget(view_);setWindowTitle(QString("%1 — APEX OFFICE READER").arg(QFileInfo(path).fileName()));status_->setText(QString("%1 • %2 words • Read-only text view").arg(QFileInfo(path).fileName()).arg(document.words()));}
void MainWindow::loadPdf(const QString& path){
#ifdef APEX_HAS_PDF
 pdfDocument_->close();auto error=pdfDocument_->load(path);if(error==QPdfDocument::Error::IncorrectPassword){bool ok=false;const auto password=QInputDialog::getText(this,"Password required","Enter the PDF password:",QLineEdit::Password,{},&ok);if(!ok)return;pdfDocument_->setPassword(password);error=pdfDocument_->load(path);}if(error!=QPdfDocument::Error::None){QMessageBox::warning(this,"Could not open PDF",QString("The PDF engine rejected this file (error %1). No repair or validation was attempted.").arg(static_cast<int>(error)));return;}documentStack_->setCurrentIndex(1);pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);pdfPage_->setValue(1);const auto title=QFileInfo(path).fileName();setWindowTitle(QString("%1 — APEX OFFICE READER").arg(title));status_->setText(QString("%1 • PDFium via Qt PDF • %2 pages • Read-only").arg(title).arg(pdfDocument_->pageCount()));
#else
 Q_UNUSED(path);QMessageBox::information(this,"PDF engine unavailable","This build was compiled without Qt PDF. The PDF capability is disabled for this build.");
#endif
}
void MainWindow::showCapabilities(){QString report="Format capability registry (current implementation):\n\n";for(const auto& c:CapabilityRegistry::all())report+=QString(".%1 — %2: %3. %4\n").arg(QString::fromStdString(c.extension),QString::fromStdString(CapabilityRegistry::label(c.support)),QString::fromStdString(c.format),QString::fromStdString(c.note));QMessageBox::information(this,"APEX format support",report);}
}

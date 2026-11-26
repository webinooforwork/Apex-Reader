#include "main_window.h"
#include "../core/capability_registry.h"
#include "../engines/text_document.h"
#include <QFileDialog>
#include <QAction>
#include <QKeySequence>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
namespace apex {
MainWindow::MainWindow(QWidget* parent):QMainWindow(parent),view_(new QPlainTextEdit(this)),status_(new QLabel("Open a supported text document",this)) { setWindowTitle("APEX OFFICE READER"); resize(1100,760); view_->setReadOnly(true); view_->setLineWrapMode(QPlainTextEdit::NoWrap); auto* root=new QWidget(this); auto* layout=new QVBoxLayout(root); layout->setContentsMargins(12,12,12,8); layout->addWidget(status_); layout->addWidget(view_); setCentralWidget(root); auto* file=menuBar()->addMenu("File"); auto* open=file->addAction("Open…"); open->setShortcut(QKeySequence::Open); connect(open,&QAction::triggered,this,&MainWindow::openFile); file->addAction("Capabilities",this,&MainWindow::showCapabilities); file->addSeparator(); file->addAction("Exit",this,&QWidget::close); }
void MainWindow::openFile(){const auto p=QFileDialog::getOpenFileName(this,"Open document"); if(!p.isEmpty())loadPath(p);}
void MainWindow::loadPath(const QString& path){const auto ext=QFileInfo(path).suffix().toStdString(); const auto* cap=CapabilityRegistry::find(ext); if(!cap||cap->support==Support::Unsupported){QMessageBox::information(this,"Format not supported",QString("%1\n\n%2").arg(cap?QString::fromStdString(cap->format):"Unknown format",cap?QString::fromStdString(cap->note):"No reader is registered for this extension."));return;} TextDocument doc; if(!doc.load(path)){QMessageBox::warning(this,"Could not open document",doc.error);return;} view_->setPlainText(doc.text); setWindowTitle(QString("%1 — APEX OFFICE READER").arg(QFileInfo(path).fileName())); status_->setText(QString("%1 • %2 words • Read-only").arg(QFileInfo(path).fileName()).arg(doc.words())); }
void MainWindow::showCapabilities(){QString text="Format capability registry (current implementation):\n\n"; for(const auto& c:CapabilityRegistry::all()) text+=QString(".%1 — %2: %3. %4\n").arg(QString::fromStdString(c.extension),QString::fromStdString(CapabilityRegistry::label(c.support)),QString::fromStdString(c.format),QString::fromStdString(c.note)); QMessageBox::information(this,"APEX format support",text);}
}

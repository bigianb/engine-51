#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "../../a51lib/Bitmap.h"
#include "../../a51lib/DFSFile.h"
#include "../../a51lib/RigidGeom.h"

#include <iostream>
#include <sstream>

#include <QDragEnterEvent>
#include <QMimeData>
#include <QtCore/QDir>


DfsTreeModel::DfsTreeModel(DFSFile* dfsFile, QObject* parent)
    : QAbstractItemModel(parent)
{
    this->dfsFile = dfsFile;
}

DfsTreeModel::~DfsTreeModel()
{
}

QModelIndex DfsTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    return createIndex(row, column);
}

QModelIndex DfsTreeModel::parent(const QModelIndex& index) const
{
    return QModelIndex{};
}

int DfsTreeModel::rowCount(const QModelIndex& parent) const
{
    if (dfsFile != nullptr && parent.column() > 0 || parent.isValid()) {
        return 0;
    }

    return dfsFile->numFiles();
}

int DfsTreeModel::columnCount(const QModelIndex& parent) const
{
    return 2;
}

QVariant DfsTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return {};
    }
    if (index.column() == 0){
        return dfsFile->getFilename(index.row()).c_str();
    }

    return dfsFile->getFileSize(index.row());
}

QVariant DfsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (section == 0){
        return QVariant("Filename");
    } else {
        return "size";
    }
}

void DfsTreeModel::doBeginResetModel()
{
    beginResetModel();
}

void DfsTreeModel::doEndResetModel()
{
    endResetModel();
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    bool ok = connect(ui->treeView, &QAbstractItemView::clicked, this, &MainWindow::treeItemClicked);

    dfsFile = new DFSFile();
    dfsTreeModel = new DfsTreeModel(dfsFile);
    ui->treeView->setModel(dfsTreeModel);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete dfsTreeModel;
    delete dfsFile;
}

void MainWindow::treeItemClicked(const QModelIndex &index) {
    int entryNo = index.row();
    //qDebug() << "Clicked row:" << entryNo;
    auto extension = dfsFile->getFileExtension(entryNo);

    uint8_t* fileData = dfsFile->getFileData(entryNo);
    int fileLen = dfsFile->getFileSize(entryNo);

    if (extension == ".TXT" || extension == ".VSH" || extension == ".INFO"){
        
        int i=0;
        std::ostringstream ss;
        while (i < fileLen && fileData[i]){
            ss.put(fileData[i++]);
        }
        ui->plainTextEdit->setPlainText(ss.str().c_str());
    } else if (extension == ".RIGIDGEOM"){
        RigidGeom rigidGeom;
        rigidGeom.readFile(fileData, fileLen);
        std::ostringstream ss;
        rigidGeom.describe(ss);
        ui->plainTextEdit->setPlainText(ss.str().c_str());
        ui->modelPage->setGeom(rigidGeom);
    } else if (extension == ".XBMP"){
        Bitmap bitmap;
        bitmap.readFile(fileData, fileLen);
        std::ostringstream ss;
        bitmap.describe(ss);
        ui->plainTextEdit->setPlainText(ss.str().c_str());
    } else {
        ui->plainTextEdit->setPlainText("Can't parse this format yet.");
    }
}

static QString getFilenameFromMimeData(const QMimeData* mimeData)
{
    QString filename;
    if (mimeData->hasUrls()) {
        const QList<QUrl> urlList = mimeData->urls();
        if (urlList.size() >= 1) {
            filename = QDir::toNativeSeparators(urlList.front().toLocalFile());
        }
    }

    return filename;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    QString filename = getFilenameFromMimeData(event->mimeData());
    if (filename.toLower().endsWith("dfs")){
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    QString filename = getFilenameFromMimeData(event->mimeData());
    if (filename.toLower().endsWith("dfs")){
        dfsTreeModel->doBeginResetModel();
        dfsFile->read(filename.toStdString());
        dfsTreeModel->doEndResetModel();
        event->acceptProposedAction();
    }
}

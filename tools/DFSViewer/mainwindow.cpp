#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "../../a51lib/Bitmap.h"
#include "../../a51lib/DFSFile.h"
#include "../../a51lib/RigidGeom.h"
#include "../../a51lib/animation/animData.h"
#include "../../a51lib/Playsurface.h"

#include "gltfExporter.h"
#include <iostream>
#include <sstream>
#include <fstream>

#include <QDragEnterEvent>
#include <QMimeData>
#include <QtCore/QDir>
#include <QFileDialog>

#include "../../a51lib/gltf/json.hpp"
using nlohmann::json;

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
    return createIndex(row, column, row);
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
    return 3;
}

QVariant DfsTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return {};
    }
    const int dfsItemNo = index.internalId();
    if (index.column() == 0) {
        return dfsFile->getBaseFilename(dfsItemNo).c_str();
    } else if (index.column() == 1) {
        return dfsFile->getFileExtension(dfsItemNo).c_str();
    }
    return dfsFile->getFileSize(dfsItemNo);
}

QVariant DfsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (section == 0) {
        return QVariant("Filename");
    } else if (section == 1) {
        return "Ext";
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
    ok = connect(ui->actionToolbarExportFile, &QAction::triggered, this, &MainWindow::exportTriggered);
    ok = connect(ui->actionToolbarExportAll, &QAction::triggered, this, &MainWindow::exportAllTriggered);
    ok = connect(ui->actionToolbarExtractFile, &QAction::triggered, this, &MainWindow::extractFileTriggered);

    ok = connect(ui->searchText, &QLineEdit::textChanged, this, &MainWindow::searchRegularExpressionChanged);

    dfsFile = new DFSFile();
    dfsTreeModel = new DfsTreeModel(dfsFile);
    proxyModel = new QSortFilterProxyModel(nullptr);

    proxyModel->setSourceModel(dfsTreeModel);
    ui->treeView->setModel(proxyModel);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete dfsTreeModel;
    delete dfsFile;
}

void MainWindow::extractFileTriggered()
{
    auto              currentIndex = ui->treeView->selectionModel()->currentIndex();
    const QModelIndex sourceIdx = proxyModel->mapToSource(currentIndex);
    int               entryNo = sourceIdx.internalId();

    uint8_t* fileData = dfsFile->getFileData(entryNo);
    int      fileLen = dfsFile->getFileSize(entryNo);

    auto extension = dfsFile->getFileExtension(entryNo);
    auto origFilename = dfsFile->getBaseFilename(entryNo);

    QString fileName = QFileDialog::getSaveFileName(this, tr("Export File"),
                                                    origFilename.c_str());

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write((const char*)fileData, fileLen);
        file.close();
    }
}

void MainWindow::exportAllTriggered()
{
    QString exportDir = QFileDialog::getExistingDirectory(this, tr("Export Directory"),
                                                          "",
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    qDebug() << "Exporting to:" << exportDir;
    if (exportDir.isEmpty()) {
        return;
    }
    for (int entryNo = 0; entryNo < dfsFile->numFiles(); ++entryNo) {
        exportFile(entryNo, exportDir);
    }
}

void exportJSON(Playsurface playSurface, QString filename)
{
    json zones;
    for (auto& zi : playSurface.zones) {
        if (zi.numSurfaces > 0) {
            json zone;
            for (auto& surface : zi.surfaces) {
                json surf;
                surf["geomName"] = playSurface.geoms.at(surface.GeomNameIndex);

                json localToWorld;
                for (int r = 0; r < 4; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        localToWorld.push_back(surface.L2W.cells[r][c]);
                    }
                }
                surf["localToWorld"] = localToWorld;
                zone["surfaces"].push_back(surf);
            }
            zones.push_back(zone);
        }
    }
    json output;
    output["zones"] = zones;
    auto          outputString = output.dump(2);
    std::ofstream outputFile(filename.toStdString());
    if (outputFile.is_open()) {
        outputFile << outputString << std::endl;
    }
}

void MainWindow::exportFile(int entryNo, QString exportDir)
{
    auto extension = dfsFile->getFileExtension(entryNo);
    auto origFilename = dfsFile->getBaseFilename(entryNo);
    if (extension == ".XBMP") {
        QString fileName;
        if (exportDir.isEmpty()) {
            fileName = QFileDialog::getSaveFileName(this, tr("Export Png"),
                                                    origFilename.c_str(),
                                                    tr("PNG Files (*.png)"));
        } else {
            fileName = QDir::toNativeSeparators(exportDir + "/" + origFilename.c_str() + ".png");
        }

        uint8_t*   fileData = dfsFile->getFileData(entryNo);
        int        fileLen = dfsFile->getFileSize(entryNo);
        Bitmap     bitmap;
        const bool oldVersion = dfsFile->getVersion() == 1;
        bitmap.readFile(fileData, fileLen, oldVersion);
        bitmap.convertFormat(Bitmap::FMT_32_ARGB_8888);
        auto image = QImage(bitmap.data.pixelData, bitmap.width, bitmap.height, bitmap.physicalWidth * 4, QImage::Format_ARGB32);
        image.save(fileName, "PNG");
    } else if (extension == ".RIGIDGEOM") {
        QString fileName;
        if (exportDir.isEmpty()) {
            fileName = QFileDialog::getSaveFileName(this, tr("Export GLTF"),
                                                    origFilename.c_str(),
                                                    tr("GLTF Files (*.gltf)"));
        } else {
            fileName = QDir::toNativeSeparators(exportDir + "/" + origFilename.c_str() + ".gltf");
        }

        RigidGeom rigidGeom;
        uint8_t*  fileData = dfsFile->getFileData(entryNo);
        int       fileLen = dfsFile->getFileSize(entryNo);
        rigidGeom.readFile(fileData, fileLen);
        exportGLTF(rigidGeom, fileName);
    } else if (extension == ".PLAYSURFACE") {
        QString fileName;
        if (exportDir.isEmpty()) {
            fileName = QFileDialog::getSaveFileName(this, tr("Export JSON"),
                                                    origFilename.c_str(),
                                                    tr("JSON Files (*.json)"));
        } else {
            fileName = QDir::toNativeSeparators(exportDir + "/" + origFilename.c_str() + ".json");
        }
        Playsurface playSurface;
        uint8_t*    fileData = dfsFile->getFileData(entryNo);
        int         fileLen = dfsFile->getFileSize(entryNo);
        playSurface.readFile(fileData, fileLen);
        exportJSON(playSurface, fileName);
    }
}

void MainWindow::exportTriggered()
{
    auto              currentIndex = ui->treeView->selectionModel()->currentIndex();
    const QModelIndex sourceIdx = proxyModel->mapToSource(currentIndex);
    int               entryNo = sourceIdx.internalId();
    exportFile(entryNo, "");
}

void MainWindow::treeItemClicked(const QModelIndex& index)
{
    const QModelIndex sourceIdx = proxyModel->mapToSource(index);
    int               entryNo = sourceIdx.internalId();
    //qDebug() << "Clicked row:" << entryNo;
    auto extension = dfsFile->getFileExtension(entryNo);

    uint8_t* fileData = dfsFile->getFileData(entryNo);
    int      fileLen = dfsFile->getFileSize(entryNo);

    bool exportable = false;

    if (extension == ".TXT" || extension == ".PSH" || extension == ".VSH" || extension == ".INFO") {

        int                i = 0;
        std::ostringstream ss;
        while (i < fileLen && fileData[i]) {
            ss.put(fileData[i++]);
        }
        ui->plainTextEdit->setPlainText(ss.str().c_str());
    } else if (extension == ".ANIM") {
        AnimGroup animData;
        animData.readFile(fileData, fileLen);
        std::ostringstream ss;
        animData.describe(ss);
        ui->plainTextEdit->setPlainText(ss.str().c_str());
    } else if (extension == ".RIGIDGEOM") {
        RigidGeom rigidGeom;
        rigidGeom.readFile(fileData, fileLen);
        std::ostringstream ss;
        rigidGeom.describe(ss);
        ui->plainTextEdit->setPlainText(ss.str().c_str());
        ui->modelPage->setGeom(rigidGeom);
        exportable = true;
    } else if (extension == ".XBMP") {
        ui->imageLabel->clear();
        const bool oldVersion = dfsFile->getVersion() == 1;
        labelBitmap.readFile(fileData, fileLen, oldVersion);
        std::ostringstream ss;
        labelBitmap.describe(ss);
        ui->plainTextEdit->setPlainText(ss.str().c_str());
        labelBitmap.convertFormat(Bitmap::FMT_32_ARGB_8888);
        setBitmap(labelBitmap, ui->imageLabel);
        exportable = true;
    } else if (extension == ".PLAYSURFACE") {
        Playsurface playSurface;
        playSurface.readFile(fileData, fileLen);
        std::ostringstream ss;
        playSurface.describe(ss);
        ui->plainTextEdit->setPlainText(ss.str().c_str());
        exportable = true;
    } else {
        ui->plainTextEdit->setPlainText("Can't parse this format yet.");
    }

    ui->actionToolbarExportFile->setEnabled(exportable);
    ui->actionToolbarExtractFile->setEnabled(true);
}

void MainWindow::setBitmap(Bitmap& bitmap, QLabel* label)
{
    // Qimage does not take a copy of the data.
    // cost cast is required to ensure QImage does not modidy the buffer.
    labelImage = QImage((const uint8_t*)bitmap.getPixelData(0), bitmap.width, bitmap.height, bitmap.physicalWidth * 4, QImage::Format_ARGB32);
    label->setPixmap(QPixmap::fromImage(labelImage));
    label->resize(label->pixmap().size());
}

static inline QColor textColor(const QPalette& palette)
{
    return palette.color(QPalette::Active, QPalette::Text);
}

static void setTextColor(QWidget* w, const QColor& c)
{
    auto palette = w->palette();
    if (textColor(palette) != c) {
        palette.setColor(QPalette::Active, QPalette::Text, c);
        w->setPalette(palette);
    }
}

void MainWindow::searchRegularExpressionChanged()
{
    QString pattern = ui->searchText->text();

    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption | QRegularExpression::CaseInsensitiveOption;
    QRegularExpression                 regularExpression(pattern, options);

    if (regularExpression.isValid()) {
        ui->searchText->setToolTip(QString());
        proxyModel->setFilterRegularExpression(regularExpression);
        setTextColor(ui->searchText, textColor(style()->standardPalette()));
    } else {
        ui->searchText->setToolTip(regularExpression.errorString());
        proxyModel->setFilterRegularExpression(QRegularExpression());
        setTextColor(ui->searchText, Qt::red);
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
    if (filename.toLower().endsWith("dfs")) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    QString filename = getFilenameFromMimeData(event->mimeData());
    if (filename.toLower().endsWith("dfs")) {
        ui->treeView->setSortingEnabled(false);
        dfsTreeModel->doBeginResetModel();
        dfsFile->read(filename.toStdString());
        dfsTreeModel->doEndResetModel();
        ui->treeView->setSortingEnabled(true);
        event->acceptProposedAction();
        ui->actionToolbarExportAll->setEnabled(true);
    }
}

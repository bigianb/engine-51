#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "../../a51lib/Bitmap.h"
#include "../../a51lib/DFSFile.h"
#include "../../a51lib/RigidGeom.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "../../a51lib/gltf/tiny_gltf.h"

#include <iostream>
#include <sstream>

#include <QDragEnterEvent>
#include <QMimeData>
#include <QtCore/QDir>
#include <QFileDialog>

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
    if (index.column() == 0) {
        return dfsFile->getFilename(index.row()).c_str();
    }

    return dfsFile->getFileSize(index.row());
}

QVariant DfsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (section == 0) {
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
    ok = connect(ui->actionToolbarExportFile, &QAction::triggered, this, &MainWindow::exportTriggered);

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

void MainWindow::exportTriggered()
{
    auto currentIndex = ui->treeView->selectionModel()->currentIndex();
    int  entryNo = currentIndex.row();
    auto extension = dfsFile->getFileExtension(entryNo);
    auto origFilename = dfsFile->getBaseFilename(entryNo);
    if (extension == ".XBMP") {
       
        QString fileName = QFileDialog::getSaveFileName(this, tr("Export Png"),
                                                        origFilename.c_str(),
                                                        tr("PNG Files (*.png)"));

        uint8_t* fileData = dfsFile->getFileData(entryNo);
        int      fileLen = dfsFile->getFileSize(entryNo);
        Bitmap   bitmap;
        bitmap.readFile(fileData, fileLen);
        auto image = QImage(bitmap.pixelData, bitmap.width, bitmap.height, bitmap.physicalWidth * 4, QImage::Format_ARGB32);
        image.save(fileName, "PNG");
    } else if (extension == ".RIGIDGEOM") {
        QString   fileName = QFileDialog::getSaveFileName(this, tr("Export GLTF"),
                                                          origFilename.c_str(),
                                                          tr("GLTF Files (*.gltf)"));

        RigidGeom rigidGeom;
        uint8_t*  fileData = dfsFile->getFileData(entryNo);
        int       fileLen = dfsFile->getFileSize(entryNo);
        rigidGeom.readFile(fileData, fileLen);
        exportGLTF(rigidGeom, fileName);
    }
}

// TODO: this just exports a triangle for now.
void MainWindow::exportGLTF(RigidGeom& rigidGeom, QString fileName)
{
    // Create a model with a single mesh and save it as a gltf file
    tinygltf::Model      m;
    tinygltf::Scene      scene;
    tinygltf::Mesh       mesh;
    tinygltf::Primitive  primitive;
    tinygltf::Node       node;
    tinygltf::Buffer     buffer;
    tinygltf::BufferView bufferView1;
    tinygltf::BufferView bufferView2;
    tinygltf::Accessor   accessor1;
    tinygltf::Accessor   accessor2;
    tinygltf::Asset      asset;

    // This is the raw data buffer.
    buffer.data = {
        // 6 bytes of indices and two bytes of padding
        0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
        // 36 bytes of floating point numbers
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f,
        0x00, 0x00, 0x00, 0x00};

    // "The indices of the vertices (ELEMENT_ARRAY_BUFFER) take up 6 bytes in the
    // start of the buffer.
    bufferView1.buffer = 0;
    bufferView1.byteOffset = 0;
    bufferView1.byteLength = 6;
    bufferView1.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

    // The vertices take up 36 bytes (3 vertices * 3 floating points * 4 bytes)
    // at position 8 in the buffer and are of type ARRAY_BUFFER
    bufferView2.buffer = 0;
    bufferView2.byteOffset = 8;
    bufferView2.byteLength = 36;
    bufferView2.target = TINYGLTF_TARGET_ARRAY_BUFFER;

    // Describe the layout of bufferView1, the indices of the vertices
    accessor1.bufferView = 0;
    accessor1.byteOffset = 0;
    accessor1.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
    accessor1.count = 3;
    accessor1.type = TINYGLTF_TYPE_SCALAR;
    accessor1.maxValues.push_back(2);
    accessor1.minValues.push_back(0);

    // Describe the layout of bufferView2, the vertices themself
    accessor2.bufferView = 1;
    accessor2.byteOffset = 0;
    accessor2.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    accessor2.count = 3;
    accessor2.type = TINYGLTF_TYPE_VEC3;
    accessor2.maxValues = {1.0, 1.0, 0.0};
    accessor2.minValues = {0.0, 0.0, 0.0};

    // Build the mesh primitive and add it to the mesh
    primitive.indices = 0;                // The index of the accessor for the vertex indices
    primitive.attributes["POSITION"] = 1; // The index of the accessor for positions
    primitive.material = 0;
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    mesh.primitives.push_back(primitive);

    // Other tie ups
    node.mesh = 0;
    scene.nodes.push_back(0); // Default scene

    // Define the asset. The version is required
    asset.version = "2.0";
    asset.generator = "tinygltf";

    // Now all that remains is to tie back all the loose objects into the
    // our single model.
    m.scenes.push_back(scene);
    m.meshes.push_back(mesh);
    m.nodes.push_back(node);
    m.buffers.push_back(buffer);
    m.bufferViews.push_back(bufferView1);
    m.bufferViews.push_back(bufferView2);
    m.accessors.push_back(accessor1);
    m.accessors.push_back(accessor2);
    m.asset = asset;

    // Create a simple material
    tinygltf::Material mat;
    mat.pbrMetallicRoughness.baseColorFactor = {1.0f, 0.9f, 0.9f, 1.0f};
    mat.doubleSided = true;
    m.materials.push_back(mat);

    // Save it to a file
    tinygltf::TinyGLTF gltf;
    gltf.WriteGltfSceneToFile(&m, fileName.toStdString(),
                              true,   // embedImages
                              true,   // embedBuffers
                              true,   // pretty print
                              false); // write binary
}

void MainWindow::treeItemClicked(const QModelIndex& index)
{
    int entryNo = index.row();
    //qDebug() << "Clicked row:" << entryNo;
    auto extension = dfsFile->getFileExtension(entryNo);

    uint8_t* fileData = dfsFile->getFileData(entryNo);
    int      fileLen = dfsFile->getFileSize(entryNo);

    bool exportable = false;

    if (extension == ".TXT" || extension == ".VSH" || extension == ".INFO") {

        int                i = 0;
        std::ostringstream ss;
        while (i < fileLen && fileData[i]) {
            ss.put(fileData[i++]);
        }
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
        Bitmap bitmap;
        bitmap.readFile(fileData, fileLen);
        std::ostringstream ss;
        bitmap.describe(ss);
        ui->plainTextEdit->setPlainText(ss.str().c_str());
        setBitmap(bitmap, ui->imageLabel);
        exportable = true;
    } else {
        ui->plainTextEdit->setPlainText("Can't parse this format yet.");
    }

    ui->actionToolbarExportFile->setEnabled(exportable);
}

void MainWindow::setBitmap(Bitmap& bitmap, QLabel* label)
{
    // TODO: is this safe? It assumes that the label takes a copy of the pixmap data.
    // We may need to ensure that the bitmap lifetime is longer than it being displayed in the label.
    labelImage = QImage(bitmap.pixelData, bitmap.width, bitmap.height, bitmap.physicalWidth * 4, QImage::Format_ARGB32);
    label->setPixmap(QPixmap::fromImage(labelImage));
    label->resize(label->pixmap().size());
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
        dfsTreeModel->doBeginResetModel();
        dfsFile->read(filename.toStdString());
        dfsTreeModel->doEndResetModel();
        event->acceptProposedAction();
    }
}

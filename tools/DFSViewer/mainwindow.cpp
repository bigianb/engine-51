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
        bitmap.convertFormat(Bitmap::FMT_32_ARGB_8888);
        auto image = QImage(bitmap.data.pixelData, bitmap.width, bitmap.height, bitmap.physicalWidth * 4, QImage::Format_ARGB32);
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
    
    int numMeshes = rigidGeom.getNumMeshes();
    for (int meshNo=0; meshNo < numMeshes; ++meshNo){

        int numVertices = rigidGeom.getNumVertices(meshNo);
        float* vertexData = rigidGeom.getVerticesPUV(meshNo);

        // This is the raw data buffer.
        // Must be a better way to do this
        const unsigned char* pcvd = (unsigned char*)vertexData;
        tinygltf::Buffer     buffer;
        buffer.data = std::vector<unsigned char>(pcvd, pcvd + numVertices * 12);
        delete[] vertexData;
        m.buffers.push_back(buffer);

        tinygltf::BufferView bufferView;
        bufferView.buffer = meshNo;
        bufferView.byteOffset = 0;
        bufferView.byteLength = numVertices * 12;
        bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

        // Describe the layout of bufferView2, the vertices themself
        tinygltf::Accessor   accessor;
        accessor.bufferView = meshNo;
        accessor.byteOffset = 0;
        accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        accessor.count = numVertices;
        accessor.type = TINYGLTF_TYPE_VEC3;
        const BBox& bbox = rigidGeom.getBoundingBox(meshNo);
        accessor.maxValues = {bbox.max.x, bbox.max.y, bbox.max.z};
        accessor.minValues = {bbox.min.x, bbox.min.y, bbox.min.z};

        // Build the mesh primitive and add it to the mesh
        tinygltf::Primitive  primitive;
        primitive.attributes["POSITION"] = meshNo; // The index of the accessor for positions
        primitive.material = 0;
        primitive.mode = TINYGLTF_MODE_TRIANGLES;
        tinygltf::Mesh mesh;
        mesh.primitives.push_back(primitive);
        m.meshes.push_back(mesh);

        // Other tie ups
        tinygltf::Node node;
        node.mesh = meshNo;
        scene.nodes.push_back(meshNo); // Default scene
        m.nodes.push_back(node);

        // Now all that remains is to tie back all the loose objects into the
        // our single model.
        
        m.bufferViews.push_back(bufferView);
        m.accessors.push_back(accessor);
    }
    m.scenes.push_back(scene);

    // Define the asset. The version is required
    tinygltf::Asset asset;
    asset.version = "2.0";
    asset.generator = "DFSViewer";
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
        ui->imageLabel->clear();
        labelBitmap.readFile(fileData, fileLen);
        std::ostringstream ss;
        labelBitmap.describe(ss);
        ui->plainTextEdit->setPlainText(ss.str().c_str());
        labelBitmap.convertFormat(Bitmap::FMT_32_ARGB_8888);
        setBitmap(labelBitmap, ui->imageLabel);
        exportable = true;
    } else {
        ui->plainTextEdit->setPlainText("Can't parse this format yet.");
    }

    ui->actionToolbarExportFile->setEnabled(exportable);
}

void MainWindow::setBitmap(Bitmap& bitmap, QLabel* label)
{
    // Qimage does not take a copy of the data.
    // cost cast is required to ensure QImage does not modidy the buffer.
    labelImage = QImage((const uint8_t*)bitmap.getPixelData(0), bitmap.width, bitmap.height, bitmap.physicalWidth * 4, QImage::Format_ARGB32);
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

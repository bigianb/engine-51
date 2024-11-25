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
    } else if (section == 1){
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

void MainWindow::exportTriggered()
{
    auto currentIndex = ui->treeView->selectionModel()->currentIndex();
    const QModelIndex sourceIdx = proxyModel->mapToSource(currentIndex);
    int entryNo = sourceIdx.internalId();

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
        QString fileName = QFileDialog::getSaveFileName(this, tr("Export GLTF"),
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
    tinygltf::Model m;
    tinygltf::Scene scene;

    int numMeshes = rigidGeom.getNumMeshes();
    int accessorIdx = 0;
    int viewIdx = 0;
    int materialIdx = 0;
    int textureIdx = 0;

    for (int texNo = 0; texNo < rigidGeom.getNumTextures(); ++texNo) {
        tinygltf::Image image;
        // TODO: Uppercase and change the extension to png.
        std::string tfn = rigidGeom.getTextureFilename(texNo);
        tfn.replace(tfn.end() - 4, tfn.end(), "png");
        for (auto& c : tfn) {
            c = toupper(c);
        }
        image.uri = tfn;
        m.images.push_back(image);
    }

    int nodeMeshIdx = 0;
    for (int meshNo = 0; meshNo < numMeshes; ++meshNo) {
        Mesh& mesh = rigidGeom.meshes[meshNo];
        for (int submeshIdx = mesh.iSubMesh; submeshIdx < mesh.iSubMesh + mesh.nSubMeshes; ++submeshIdx) {

            int    numVertices = rigidGeom.getNumSubmeshVertices(submeshIdx);
            float* vertexData = rigidGeom.getSubmeshVerticesPUV(submeshIdx);

            // This is the raw data buffer.
            // Must be a better way to do this
            const unsigned char* pcvd = (unsigned char*)vertexData;
            tinygltf::Buffer     buffer;
            buffer.data = std::vector<unsigned char>(pcvd, pcvd + numVertices * 20);
            delete[] vertexData;
            m.buffers.push_back(buffer);

            tinygltf::BufferView posBufferView;
            posBufferView.buffer = nodeMeshIdx;
            posBufferView.byteOffset = 0;
            posBufferView.byteLength = numVertices * 12;
            posBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

            tinygltf::BufferView uvBufferView;
            uvBufferView.buffer = nodeMeshIdx;
            uvBufferView.byteOffset = numVertices * 12;
            uvBufferView.byteLength = numVertices * 8;
            uvBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

            m.bufferViews.push_back(posBufferView);
            int posBufferViewIdx = viewIdx++;

            m.bufferViews.push_back(uvBufferView);
            int uvBufferViewIdx = viewIdx++;

            // Describe the layout of posBufferView, the vertices themself
            tinygltf::Accessor posAccessor;
            posAccessor.bufferView = posBufferViewIdx;
            posAccessor.byteOffset = 0;
            posAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            posAccessor.count = numVertices;
            posAccessor.type = TINYGLTF_TYPE_VEC3;
            const BBox& bbox = rigidGeom.getBoundingBox(meshNo);
            posAccessor.maxValues = {bbox.max.x, bbox.max.y, bbox.max.z};
            posAccessor.minValues = {bbox.min.x, bbox.min.y, bbox.min.z};

            tinygltf::Accessor uvAccessor;
            uvAccessor.bufferView = uvBufferViewIdx;
            uvAccessor.byteOffset = 0;
            uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            uvAccessor.count = numVertices;
            uvAccessor.type = TINYGLTF_TYPE_VEC2;
            uvAccessor.maxValues = {1.0, 1.0}; // TODO: Build correct min/max from data
            uvAccessor.minValues = {-1.0, -1.0};

            m.accessors.push_back(posAccessor);
            int posAccessorId = accessorIdx++;

            m.accessors.push_back(uvAccessor);
            int uvAccessorId = accessorIdx++;

            // Create a simple material
            tinygltf::Material mat;
            mat.pbrMetallicRoughness.baseColorFactor = {1.0f, 0.9f, 0.9f, 1.0f};
            mat.pbrMetallicRoughness.baseColorTexture.index = textureIdx;

            mat.doubleSided = true;
            m.materials.push_back(mat);
            int theMaterialIdx = materialIdx++;

            tinygltf::Texture texture;
            int               meshMatIdx = rigidGeom.subMeshes[submeshIdx].iMaterial;
            texture.source = rigidGeom.materials[meshMatIdx].iTexture;
            m.textures.push_back(texture);
            textureIdx++;

            // Build the mesh primitive and add it to the mesh
            tinygltf::Primitive primitive;
            primitive.attributes["POSITION"] = posAccessorId;
            primitive.attributes["TEXCOORD_0"] = uvAccessorId;
            primitive.material = theMaterialIdx;
            primitive.mode = TINYGLTF_MODE_TRIANGLES;
            tinygltf::Mesh mesh;
            mesh.primitives.push_back(primitive);
            m.meshes.push_back(mesh);

            // Other tie ups
            tinygltf::Node node;
            node.mesh = nodeMeshIdx;
            scene.nodes.push_back(nodeMeshIdx++); // Default scene
            m.nodes.push_back(node);
        }
    }
    m.scenes.push_back(scene);

    // Define the asset. The version is required
    tinygltf::Asset asset;
    asset.version = "2.0";
    asset.generator = "DFSViewer";
    m.asset = asset;

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
    const QModelIndex sourceIdx = proxyModel->mapToSource(index);
    int entryNo = sourceIdx.internalId();
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

static inline QColor textColor(const QPalette &palette)
{
    return palette.color(QPalette::Active, QPalette::Text);
}

static void setTextColor(QWidget *w, const QColor &c)
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
    QRegularExpression regularExpression(pattern, options);

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
    }
}

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <iostream>

#include <QDragEnterEvent>
#include <QMimeData>
#include <QtCore/QDir>

DfsTreeItem::DfsTreeItem(QVariantList data, DfsTreeItem* parent)
    : m_itemData(std::move(data))
    , m_parentItem(parent)
{
}

void DfsTreeItem::appendChild(std::unique_ptr<DfsTreeItem>&& child)
{
    m_childItems.push_back(std::move(child));
}

DfsTreeItem* DfsTreeItem::child(int row)
{
    return row >= 0 && row < childCount() ? m_childItems.at(row).get() : nullptr;
}

int DfsTreeItem::childCount() const
{
    return int(m_childItems.size());
}

int DfsTreeItem::row() const
{
    if (m_parentItem == nullptr) {
        return 0;
    }
    const auto it = std::find_if(
        m_parentItem->m_childItems.cbegin(), m_parentItem->m_childItems.cend(),
        [this](const std::unique_ptr<DfsTreeItem>& treeItem) {
            return treeItem.get() == this;
        });

    if (it != m_parentItem->m_childItems.cend()) {
        return std::distance(m_parentItem->m_childItems.cbegin(), it);
    }
    Q_ASSERT(false); // should not happen
    return -1;
}

int DfsTreeItem::columnCount() const
{
    return int(m_itemData.count());
}

QVariant DfsTreeItem::data(int column) const
{
    return m_itemData.value(column);
}

DfsTreeItem* DfsTreeItem::parentItem()
{
    return m_parentItem;
}

DfsTreeModel::DfsTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
    , rootItem(std::make_unique<DfsTreeItem>(QVariantList{tr("Name"), tr("Size")}))
{
}

DfsTreeModel::~DfsTreeModel()
{
}

QModelIndex DfsTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) {
        return {};
    }

    DfsTreeItem* parentItem = parent.isValid()
                                  ? static_cast<DfsTreeItem*>(parent.internalPointer())
                                  : rootItem.get();

    if (auto* childItem = parentItem->child(row)) {
        return createIndex(row, column, childItem);
    }
    return {};
}

QModelIndex DfsTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return {};
    }

    auto* childItem = static_cast<DfsTreeItem*>(index.internalPointer());
    DfsTreeItem* parentItem = childItem->parentItem();

    return parentItem != rootItem.get()
               ? createIndex(parentItem->row(), 0, parentItem)
               : QModelIndex{};
}

int DfsTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0) {
        return 0;
    }

    const DfsTreeItem* parentItem = parent.isValid()
                                        ? static_cast<const DfsTreeItem*>(parent.internalPointer())
                                        : rootItem.get();

    return parentItem->childCount();
}

int DfsTreeModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return static_cast<DfsTreeItem*>(parent.internalPointer())->columnCount();
    }
    return rootItem->columnCount();
}

QVariant DfsTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return {};
    }

    const auto* item = static_cast<const DfsTreeItem*>(index.internalPointer());
    return item->data(index.column());
}

Qt::ItemFlags DfsTreeModel::flags(const QModelIndex& index) const
{
    return index.isValid()
               ? QAbstractItemModel::flags(index)
               : Qt::ItemFlags(Qt::NoItemFlags);
}

QVariant DfsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return orientation == Qt::Horizontal && role == Qt::DisplayRole
               ? rootItem->data(section)
               : QVariant{};
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    dfsTreeModel = new DfsTreeModel();
    ui->treeView->setModel(dfsTreeModel);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete dfsTreeModel;
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

    std::cout << "Got drag from " << qUtf8Printable(filename) << std::endl;

    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{

    event->acceptProposedAction();
}

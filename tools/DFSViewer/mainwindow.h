#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore/qabstractitemmodel.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class DfsTreeItem
{
public:
    explicit DfsTreeItem(QVariantList data, DfsTreeItem* parentItem = nullptr);

    void appendChild(std::unique_ptr<DfsTreeItem>&& child);

    DfsTreeItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    DfsTreeItem* parentItem();

private:
    std::vector<std::unique_ptr<DfsTreeItem>> m_childItems;
    QVariantList m_itemData;
    DfsTreeItem* m_parentItem;
};

class DfsTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(DfsTreeModel)

    explicit DfsTreeModel(QObject* parent = nullptr);
    ~DfsTreeModel() override;

    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

private:
    static void setupModelData(const QList<QStringView>& lines, DfsTreeItem* parent);

    std::unique_ptr<DfsTreeItem> rootItem;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    DfsTreeModel* dfsTreeModel;
    Ui::MainWindow* ui;
};
#endif // MAINWINDOW_H

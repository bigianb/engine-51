#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QtCore/qabstractitemmodel.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class DFSFile;

class DfsTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(DfsTreeModel)

    explicit DfsTreeModel(DFSFile* dfsFile, QObject* parent = nullptr);
    ~DfsTreeModel() override;

    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

    void doBeginResetModel();
    void doEndResetModel();
    
private:
    DFSFile* dfsFile;
};

class Bitmap;
class RigidGeom;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

public slots:
    void treeItemClicked(const QModelIndex &index);
    void exportTriggered();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void setBitmap(Bitmap& bitmap, QLabel* label);
    void exportGLTF(RigidGeom& rigidGeom, QString fileName);

    QImage labelImage;

    DfsTreeModel* dfsTreeModel;
    DFSFile* dfsFile;
    Ui::MainWindow* ui;
};
#endif // MAINWINDOW_H

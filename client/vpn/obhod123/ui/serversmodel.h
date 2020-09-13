#ifndef SERVERSMODEL_H
#define SERVERSMODEL_H

#include <QSortFilterProxyModel>
#include <QAbstractTableModel>
#include <QImage>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QSettings>
#include <QTimer>

#include "backend/settings.h"

enum ServerRole {
    ServerIdRole = Qt::UserRole + 1,
    ServerRegionRole = Qt::UserRole + 2,
    RowIsServerRole = Qt::UserRole + 3,     // display server if true, otherwise display region string with span
};

class RowHoverDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    RowHoverDelegate(QTableView *tableView, QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;

protected slots:
    void onItemEntered(const QModelIndex &index);

private:
    QTableView *m_tableView;
    int m_hoveredRow;
};

class FilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit FilterModel(QObject *parent = 0);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

    void setOnlyFavorites(bool showOnlyFav) {showOnlyFavorites = showOnlyFav; invalidate();}
    void setOnlyRecommended(bool showOnlyRec) {showOnlyRecommended = showOnlyRec; invalidate();}

public slots:
    void updateFilter() {
        QSettings s;
    }

private:
    bool showOnlyFavorites;
    bool showOnlyRecommended;
};

class ServerModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ServerModel(QObject *parent = 0);
    ~ServerModel();

    QModelIndex parent(const QModelIndex &child) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const Q_DECL_OVERRIDE;
    bool hasChildren(const QModelIndex &parent) const Q_DECL_OVERRIDE;

    //   QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QSize span(const QModelIndex &index) const Q_DECL_OVERRIDE;

    void resetModel();
private:
    QImage favorites_enabled;
    QImage favorites_disabled;
    QImage favorites_hover;

    QTimer timer;
};

#endif // SERVERSMODEL_H

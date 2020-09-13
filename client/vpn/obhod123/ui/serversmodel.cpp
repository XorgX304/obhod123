#include <QFont>
#include <QApplication>

#include "flags.h"
#include "serversmodel.h"
#include "../backend/pinger.h"
#include "../backend/api_http.h"

RowHoverDelegate::RowHoverDelegate(QTableView *tableView, QObject *parent)
    : QStyledItemDelegate(parent),
      m_tableView(tableView),
      m_hoveredRow(-1)
{
    // mouse tracking have to be true, otherwise itemEntered won't emit
    m_tableView->setMouseTracking(true);
    connect(m_tableView, &QTableView::entered, this, &RowHoverDelegate::onItemEntered);
}

void RowHoverDelegate::onItemEntered(const QModelIndex &index)
{
    m_hoveredRow = index.row();
    m_tableView->viewport()->update();  // force update
}

void RowHoverDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    if (index.data(RowIsServerRole).toBool()) {
        if (index.row() == m_hoveredRow) {
            opt.state |= QStyle::State_MouseOver;
        }
    }
    else {
        opt.state = opt.state & ~QStyle::State_MouseOver;
    }

    QStyledItemDelegate::paint(painter, opt, index);
}

FilterModel::FilterModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
    showOnlyFavorites = false;
}

bool FilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    //if (! index0.isValid()) return false;
//    qDebug() << index0.model();
//    qDebug() << index0.model()->rowCount();

    //if (!index0.data(RowIsServerRole).toBool()) return true;  // regions always visible
    // check if servers in region present
    if (!index0.data(RowIsServerRole).toBool()) {
        int serversCountInRegion = 0;
        for (int i = 0; i < index0.model()->rowCount(); ++i) {
            QModelIndex tmp_idx  = index0.sibling(i, 0);
            if (! tmp_idx.data(RowIsServerRole).toBool()) continue; // prevent recursive call

            QString tmp_region = tmp_idx.data(ServerRegionRole).toString();
            if (tmp_region != index0.data(ServerRegionRole).toString()) continue;

            if (filterAcceptsRow(i, sourceParent)) serversCountInRegion++;
        }

        if (serversCountInRegion > 0) return true;
        else return false;
    }


    //QString protocol = q_settings.value("protocol").toString();
    QString serverId = index0.data(ServerIdRole).toString();

    //const QJsonObject &server = API_HTTP::Instance().getServerById(serverId);

    if (showOnlyFavorites) {
        if (! Settings::favoritesServers().contains(serverId)) return false;
    }
    if (showOnlyRecommended) {
        if (Pinger::Instance().getPing(index0.data(ServerIdRole).toString()) > 200) return false;
    }

    return true;
}

ServerModel::ServerModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    favorites_enabled = QImage(":/images/favorites_enabled.png");
    favorites_disabled = QImage(":/images/favorites_disabled.png");
    favorites_hover = QImage(":/images/favorites_hover.png");

    connect(&API_HTTP::Instance(), &API_HTTP::serverListChanged, this, &ServerModel::resetModel);

    timer.setInterval(10000);   // update ping data
    connect(&timer, &QTimer::timeout, [&](){
        beginResetModel();
        endResetModel();
    });
}

ServerModel::~ServerModel()
{
    timer.stop();
}

QModelIndex ServerModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

QModelIndex ServerModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

QModelIndex ServerModel::sibling(int row, int column, const QModelIndex &) const
{
    return index(row, column);
}

bool ServerModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.model() == this || !parent.isValid())
        return rowCount(parent) > 0 && columnCount(parent) > 0;
    return false;
}

int ServerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    int regionsCount = API_HTTP::Instance().getRegions().size();

    int serverCount = 0;
    for (const QString &region : API_HTTP::Instance().getRegions()) {
        serverCount += API_HTTP::Instance().getServersInRegion(region).size();
    }

    return regionsCount + serverCount;
}

int ServerModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
}

#ifdef Q_OS_WIN
QVariant ServerModel::data(const QModelIndex &index, int role) const
{
    bool isServerRow = false;
    QJsonObject server;
    QString region;


    // int pos = 1;  // first row is Auto Select
    int pos = 0;  // Auto Select no more displayed
    const QStringList &regions = API_HTTP::Instance().getRegions();

    for (const QString &r : regions) {
        const QJsonArray servers = API_HTTP::Instance().getServersInRegion(r);
        if (pos == index.row()) {
            isServerRow = false;
            region = r;
            break;
        }
        pos++;

        for (const QJsonValue &s : servers) {
            if (index.row() == pos){
                isServerRow = true;
                server = s.toObject();
            }
            pos++;
        }
    }

    if (role == RowIsServerRole)    return isServerRow;
    if (role == ServerIdRole)       return server.value("id").toString();
    if (role == ServerRegionRole)   {
        if (isServerRow)  return server.value("region").toString();
        else return region;
    }

    if (role == Qt::FontRole) {
        QFont f = qApp->font();

        if (index.column() == 0) {
            f.setPixelSize(25);
            return f;
        }
        if (index.column() == 1) {
            f.setFamily("Lucida Console");
            f.setPixelSize(50);
            return f;
        }
    }

    double ping = Pinger::Instance().getPing(server.value("id").toString());
    if (role == Qt::TextColorRole && index.column() == 1) {

        QColor color;

        if (ping < 150) {
            color.setBlueF(0);
            color.setGreenF(1);
            color.setRedF(0);
            //color.setRedF(1);
        }
        else if (ping > 700) return QColor(Qt::red);
        else if (ping > 300) {
            color.setBlueF(0);
            color.setGreenF((700 - ping) / 700.0);
            color.setRedF(1);
        }
        else if (ping > 150 && ping < 300) {
            color.setBlueF(0);
            color.setGreenF(1);
            color.setRedF((ping - 150) / 300.0 + 0.5);
        }

        //qDebug() << color;

        return color;
    }
     if (role == Qt::TextColorRole && index.column() == 2  && isServerRow) {
         //return QColor(107, 124, 147);
         return QColor(120, 120, 120);
     }
     if (role == Qt::TextColorRole && index.column() == 0  && !isServerRow) {
         return QColor(74, 74, 74);
     }

    if (role == Qt::TextAlignmentRole && index.column() == 0) {
        return Qt::AlignCenter;
    }

    if (role == Qt::DecorationRole) {
        if (index.column() == 0) {
            if (isServerRow) return Flags::Instance().getFlag(server.value("flag").toString());
        }
        else if (index.column() == 3) {
            if (Settings::favoritesServers().contains(server.value("id").toString())) return favorites_enabled;
            else return favorites_disabled;
        }
    }

    if (role == Qt::DisplayRole) {
        //if (index.row() == 0) return "Auto Select";
        if (index.column() == 0) {
            if (!isServerRow) return region;
        }
        else if (index.column() == 2) {
            if (isServerRow) return server.value("name").toString();
//            if (isServerRow) return QString::number(ping, 'f', 0)
//                    + "ms, " + server.value("name").toString()
//                    + ", ID:" + server.value("id").toString();
        }
        if (index.column() == 1) {
            return "•";
        }
    }

    return QVariant();

}
#endif

#ifdef Q_OS_MAC
QVariant ServerModel::data(const QModelIndex &index, int role) const
{
    bool isServerRow = false;
    QJsonObject server;
    QString region;


    // int pos = 1;  // first row is Auto Select
    int pos = 0;  // Auto Select no more displayed
    const QStringList &regions = API_HTTP::Instance().getRegions();

    for (const QString &r : regions) {
        const QJsonArray servers = API_HTTP::Instance().getServersInRegion(r);
        if (pos == index.row()) {
            isServerRow = false;
            region = r;
            break;
        }
        pos++;

        for (const QJsonValue &s : servers) {
            if (index.row() == pos){
                isServerRow = true;
                server = s.toObject();
            }
            pos++;
        }
    }

    if (role == RowIsServerRole)    return isServerRow;
    if (role == ServerIdRole)       return server.value("id").toString();
    if (role == ServerRegionRole)   {
        if (isServerRow)  return server.value("region").toString();
        else return region;
    }

    if (role == Qt::FontRole) {
        QFont f = qApp->font();

        if (index.column() == 0) {
            f.setPixelSize(13);
            return f;
        }
        if (index.column() == 1) {
            f.setFamily("Lucida Console");
            f.setPixelSize(40);
            return f;
        }
        if (index.column() == 2) {
            f.setPixelSize(16);
            return f;
        }
    }

    double ping = Pinger::Instance().getPing(server.value("id").toString());
    if (role == Qt::TextColorRole && index.column() == 1) {

        QColor color;

        if (ping < 150) {
            color.setBlueF(0);
            color.setGreenF(1);
            color.setRedF(0);
            //color.setRedF(1);
        }
        else if (ping > 700) return QColor(Qt::red);
        else if (ping > 300) {
            color.setBlueF(0);
            color.setGreenF((700 - ping) / 700.0);
            color.setRedF(1);
        }
        else if (ping > 150 && ping < 300) {
            color.setBlueF(0);
            color.setGreenF(1);
            color.setRedF((ping - 150) / 300.0 + 0.5);
        }

        //qDebug() << color;

        return color;
    }
     if (role == Qt::TextColorRole && index.column() == 2  && isServerRow) {
         return QColor(107, 124, 147);
     }
     if (role == Qt::TextColorRole && index.column() == 0  && !isServerRow) {
         return QColor(107, 124, 147, 100);
     }

    if (role == Qt::TextAlignmentRole && index.column() == 0) {
        return Qt::AlignVCenter;
    }

    if (role == Qt::DecorationRole) {
        if (index.column() == 0) {
            if (isServerRow) return Flags::Instance().getFlag(server.value("flag").toString());
        }
        else if (index.column() == 3) {
            if (Settings::favoritesServers().contains(server.value("id").toString())) return favorites_enabled;
            else return favorites_disabled;
        }
    }

    if (role == Qt::DisplayRole) {
        //if (index.row() == 0) return "Auto Select";
        if (index.column() == 0) {
            if (!isServerRow) return region.toUpper();
        }
        else if (index.column() == 2) {
            if (isServerRow) return server.value("name").toString();
//            if (isServerRow) return QString::number(ping, 'f', 0)
//                    + "ms, " + server.value("name").toString()
//                    + ", ID:" + server.value("id").toString();
        }
        if (index.column() == 1) {
            return "•";
        }
    }

    return QVariant();
}
#endif

QSize ServerModel::span(const QModelIndex &index) const
{
    if (index.column() != 0) return QSize(1, 1);
    if (! index.data(RowIsServerRole).toBool() ) return QSize(4, 1);
    return QSize(1, 1);
}

void ServerModel::resetModel() {
    beginResetModel();
//    qDebug().noquote() << "Reset model" ;
    endResetModel();
}

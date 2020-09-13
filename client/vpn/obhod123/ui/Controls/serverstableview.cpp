#include <QScrollBar>
#include <QHeaderView>

#include "serverstableview.h"
#include "../../backend/api_http.h"

ServersTableView::ServersTableView(QWidget *parent) :
    QTableView(parent)
{
    connect(&API_HTTP::Instance(), &API_HTTP::serverListChanged, this, &ServersTableView::updateTable);
    updateTable();

    verticalScrollBar()->setSingleStep(1);
    verticalScrollBar()->setPageStep(1);

    setVerticalStepsPerItem(10);
//     QScrollBar *vsb = new QScrollBar(Qt::Vertical, this);
//     vsb->setSingleStep(1);
//     vsb->setPageStep(2);

//     connect(vsb, &QScrollBar::valueChanged, [&](int value){
//          qDebug() << "QScrollBar::valueChanged" << value;
//     });

//     setVerticalScrollBar(vsb);

//     setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
//     setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

void ServersTableView::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);

    connect(model, &QAbstractItemModel::dataChanged, [&](const QModelIndex& /*topLeft*/, const QModelIndex& /*bottomRight*/, const QVector<int>& /*roles*/) {
        setModelSpan();
    });

    connect(model, &QAbstractItemModel::layoutChanged, [&](const QList<QPersistentModelIndex>& /*parents*/, QAbstractItemModel::LayoutChangeHint /*hint*/) {
        setModelSpan();
    });

    connect(model, &QAbstractItemModel::modelReset, [&]() {
        setModelSpan();
    });

    setModelSpan();
}

void ServersTableView::updateTable()
{
    qDebug().noquote() << "ServersTableView::updateTable()";

    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    //const QStringList& regions = API_HTTP::Instance().getRegions();

    verticalHeader()->setDefaultSectionSize(50);
    horizontalHeader()->setDefaultSectionSize(50);

    setColumnWidth(0,40);
    setColumnWidth(1,30);
    setColumnWidth(2,230);
    setColumnWidth(3,25);
}

void ServersTableView::setModelSpan()
{
    qDebug().noquote() << "ServersTableView::setModelSpan()";
    if (model()) {
        clearSpans();
        verticalHeader()->setDefaultSectionSize(50);
        for(int col = 0; col < model()->columnCount(); col++){
            for (int row = 0; row < model()->rowCount(); row++){
                QSize span = model()->span(model()->index(row, col));
                if(span != QSize(1,1)) {
                    setSpan(row, col, span.height(), span.width());
                    #ifdef Q_OS_WIN
                        setRowHeight(row, 70);
                    #endif
                    #ifdef Q_OS_MAC
                        setRowHeight(row, 50);
                    #endif
                }
            }
        }
    }
}

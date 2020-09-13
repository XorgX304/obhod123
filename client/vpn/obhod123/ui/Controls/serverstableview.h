#ifndef SERVERSTABLEVIEW_H
#define SERVERSTABLEVIEW_H

#include <QTableView>

class ServersTableView : public QTableView
{
    Q_OBJECT

public:
    ServersTableView(QWidget *parent = Q_NULLPTR);
    void setModel(QAbstractItemModel *model);

public slots:
    void updateTable();
    void setModelSpan();
};

#endif // SERVERSTABLEVIEW_H

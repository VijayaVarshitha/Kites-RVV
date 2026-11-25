#pragma once
#include "processor/processor_manager.h"
#include <QAbstractTableModel>
#include <QTimer>
#include <QString>
#include <QVector>


namespace Kites
{
class VMStateTableModel : public QAbstractTableModel
{
    Q_OBJECT
  public:
    explicit VMStateTableModel(QObject *parent = nullptr, ProcessorManager * = nullptr);
    ~VMStateTableModel() = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

  private:
    ProcessorManager *m_vmManager = nullptr;

    struct Row
    {
        QString section;
        QString metric;
        QString value;
    };
    QVector<Row> m_rows; // cached on every VM state change
    void refreshRows();

    QTimer m_refreshTimer; // coalesces per-clock updates into one refresh every 100 ms

  public slots:
    void vmStateChangedSlot(const QMap<QString, QVariant> &processorState);
    void refreshNow();     // rebuild the table immediately (run finished, tab shown, reset)
    void requestRefresh(); // rebuild soon; safe to call on every clock
};
} // namespace kites


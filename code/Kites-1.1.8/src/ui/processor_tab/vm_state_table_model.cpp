#include "vm_state_table_model.h"

namespace Kites
{
VMStateTableModel::VMStateTableModel(QObject *parent, ProcessorManager *vmManager)
    : QAbstractTableModel(parent), m_vmManager(vmManager)
{
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(100);
    connect(&m_refreshTimer, &QTimer::timeout, this, &VMStateTableModel::refreshNow);

    if (m_vmManager)
    {
        // The processor emits these from the run thread; the default (queued) connection
        // delivers them on the GUI thread, where the table is rebuilt.
        connect(m_vmManager, &ProcessorManager::processorStateChangedSignal, this,
                &VMStateTableModel::requestRefresh);
        connect(m_vmManager, &ProcessorManager::runFinishedSignal, this, &VMStateTableModel::refreshNow);
        connect(m_vmManager, &ProcessorManager::processorPausedAtBreakpointSignal, this,
                &VMStateTableModel::refreshNow);
    }
    refreshRows();
}

void VMStateTableModel::refreshNow()
{
    m_refreshTimer.stop();
    beginResetModel();
    refreshRows();
    endResetModel();
}

void VMStateTableModel::requestRefresh()
{
    if (!m_refreshTimer.isActive())
        m_refreshTimer.start();
}

void VMStateTableModel::refreshRows()
{
    m_rows.clear();
    if (!m_vmManager)
        return;

    m_rows.push_back({QStringLiteral("Program"), QStringLiteral("Program counter"),
                      QStringLiteral("0x%1").arg(static_cast<qulonglong>(m_vmManager->getProgramCounter()),
                                                 8, 16, QLatin1Char('0'))});
    // Full performance report: execution time, throughput, instruction mix, vector work,
    // memory traffic, pipeline hazards and cache behaviour (see ProcessorBase::GetPerformanceReport).
    for (const auto &metric : m_vmManager->getPerformanceReport())
        m_rows.push_back({QString::fromStdString(metric.section), QString::fromStdString(metric.name),
                          QString::fromStdString(metric.value)});
}

int VMStateTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_rows.size());
}

int VMStateTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3; // Section, Metric, Value
}

QVariant VMStateTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0: return QStringLiteral("Section");
        case 1: return QStringLiteral("Metric");
        case 2: return QStringLiteral("Value");
        default: break;
        }
    }
    return QVariant{};
}

QVariant VMStateTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant{};

    const Row &row = m_rows[index.row()];
    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case 0:
            // Show each section name once, on its first row, to keep the table readable.
            if (index.row() > 0 && m_rows[index.row() - 1].section == row.section)
                return QString();
            return row.section;
        case 1: return row.metric;
        case 2: return row.value;
        default: break;
        }
    }
    if (role == Qt::TextAlignmentRole && index.column() == 2)
        return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
    return QVariant{};
}

void VMStateTableModel::vmStateChangedSlot(const QMap<QString, QVariant> &processorState)
{
    Q_UNUSED(processorState);
    refreshNow();
}
} // namespace Kites

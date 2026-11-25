#include "registermodel.h"
#include "common/register_names.h"
#include <QApplication>
#include <QBrush>
#include <QDebug>
#include <QPalette>
#include <QColor>
#include <algorithm>
#include <bit>
#include <cstring>

namespace Kites
{

RegisterModel::RegisterModel(QObject *parent, RegisterFile *regfile) : QAbstractTableModel(parent)
{
    m_currentRegisterFile = regfile;
    if (m_currentRegisterFile)
    {
        connect(m_currentRegisterFile, &RegisterFile::updateRegister, this,
                &RegisterModel::updateRegisterValue);
        connect(m_currentRegisterFile, &RegisterFile::updateFRegister, this,
                &RegisterModel::updateFRegisterValue);
        connect(m_currentRegisterFile, &RegisterFile::updateVectorRegister, this,
                &RegisterModel::updateVRegisterValue);
        connect(m_currentRegisterFile, &RegisterFile::registerResetSignal, this,
                &RegisterModel::registerResetSlot);
    }
}

int RegisterModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 96;
}

int RegisterModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
} // Register name, alias, value

void RegisterModel::setDisplayBase(Base base)
{
    beginResetModel();
    m_displayBase = base;
    endResetModel();
}

QVariant RegisterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant{};

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return QString("Register");
        case 1:
            return QString("Alias");
        case 2:
            return QString("Value");
        }
    }

    return QVariant{};
}

QVariant RegisterModel::data(const QModelIndex &index, int role) const
{
    if (!m_currentRegisterFile || !index.isValid() || index.row() < 0 || index.row() >= 96)
        return QVariant{};

    if (role == Qt::TextAlignmentRole)
        return Qt::AlignCenter;

    const size_t row = static_cast<size_t>(index.row());
    if (role == Qt::BackgroundRole)
    {
        if (row == m_highlightedRegisterIndex)
            return QBrush(QColor("#b13232fd"));
        return QVariant{};
    }

    if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
        return QVariant{};

    switch (index.column())
    {
    case 0:
        if (row < 32)
            return QString("x%1").arg(row);
        if (row < 64)
            return QString("f%1").arg(row - 32);
        return QString("v%1").arg(row - 64);

    case 1:
        if (row < 32)
            return QString::fromLatin1(register_names::kGprAliases[row]);
        if (row < 64)
            return QString::fromLatin1(register_names::kFprAliases[row - 32]);
        return QString{};

    case 2:
        if (row < 32)
        {
            const uint64_t value = m_currentRegisterFile->ReadGpr(row);
            switch (m_displayBase)
            {
            case Base::Binary:
                return QString("0b%1").arg(QString::number(value, 2));
            case Base::Decimal:
                return QString::number(static_cast<int64_t>(value));
            case Base::Hexadecimal:
                return QString("0x%1").arg(QString::number(value, 16).toUpper());
            }
        }
        else if (row < 64)
        {
            const uint64_t raw = m_currentRegisterFile->ReadFpr(row - 32);
            switch (m_displayBase)
            {
            case Base::Binary:
                return QString("0b%1").arg(QString::number(raw, 2));
            case Base::Decimal:
            {
                const uint32_t upper32 = static_cast<uint32_t>(raw >> 32U);
                if (upper32 == 0)
                {
                    const uint32_t lower32 = static_cast<uint32_t>(raw & 0xFFFFFFFFU);
                    float fval = 0.0F;
                    std::memcpy(&fval, &lower32, sizeof(float));
                    return QString::number(static_cast<double>(fval), 'g', 8);
                }
                double dval = 0.0;
                std::memcpy(&dval, &raw, sizeof(double));
                return QString::number(dval, 'g', 16);
            }
            case Base::Hexadecimal:
                return QString("0x%1").arg(QString::number(raw, 16).toUpper());
            }
        }
        else
        {
            const auto value = m_currentRegisterFile->ReadVpr(row - 64);
            QString result;
            const size_t sew = m_currentRegisterFile->ReadSew();
            if (sew >= 8 && sew <= 64 && sew % 8 == 0)
            {
                const size_t laneBytes = sew / 8;
                const size_t laneCount = std::min(value.size() / laneBytes,
                                                  m_currentRegisterFile->ReadVl());
                result = "[";
                for (size_t lane = 0; lane < laneCount; ++lane)
                {
                    uint64_t laneValue = 0;
                    for (size_t byte = 0; byte < laneBytes; ++byte)
                        laneValue |= uint64_t{value[lane * laneBytes + byte]} << (byte * 8U);

                    if (lane > 0)
                        result += ", ";
                    switch (m_displayBase)
                    {
                    case Base::Binary:
                        result += "0b" + QString::number(laneValue, 2).rightJustified(
                                                   static_cast<int>(sew), QChar('0'));
                        break;
                    case Base::Decimal:
                    {
                        // Vector lanes are displayed as signed integers in decimal mode.
                        // The register stores raw bits; interpret the top bit according
                        // to the active SEW so, for example, e32 0xFFFFFFFB displays -5.
                        int64_t signedLane = 0;
                        if (sew == 64)
                        {
                            signedLane = std::bit_cast<int64_t>(laneValue);
                        }
                        else
                        {
                            const uint64_t signBit = uint64_t{1} << (sew - 1U);
                            if ((laneValue & signBit) != 0)
                                signedLane = static_cast<int64_t>(laneValue) -
                                             static_cast<int64_t>(uint64_t{1} << sew);
                            else
                                signedLane = static_cast<int64_t>(laneValue);
                        }
                        result += QString::number(signedLane);
                        break;
                    }
                    case Base::Hexadecimal:
                        result += "0x" + QString::number(laneValue, 16).rightJustified(
                                                   static_cast<int>(sew / 4), QChar('0')).toUpper();
                        break;
                    }
                }
                return result + "]";
            }

            // Keep the byte-oriented fallback for an unset/unsupported SEW.
            switch (m_displayBase)
            {
            case Base::Binary:
                for (int i = static_cast<int>(value.size()) - 1; i >= 0; --i)
                    result += QString("%1").arg(static_cast<unsigned int>(value[static_cast<size_t>(i)]),
                                                  8, 2, QChar('0'));
                return QString("0b") + result;
            case Base::Decimal:
                result = "[";
                for (int i = static_cast<int>(value.size()) - 1; i >= 0; --i)
                {
                    result += QString::number(value[static_cast<size_t>(i)]);
                    if (i > 0)
                        result += ", ";
                }
                return result + "]";
            case Base::Hexadecimal:
                for (int i = static_cast<int>(value.size()) - 1; i >= 0; --i)
                    result += QString("%1").arg(static_cast<unsigned int>(value[static_cast<size_t>(i)]),
                                                  2, 16, QChar('0')).toUpper();
                return QString("0x") + result;
            }
        }
        break;
    default:
        return QVariant{};
    }
    return QVariant{};
}

void RegisterModel::changeRegisterFile(RegisterFile *regfile)
{
    beginResetModel();
    if (m_currentRegisterFile)
        disconnect(m_currentRegisterFile, nullptr, this, nullptr);

    m_currentRegisterFile = regfile;
    if (m_currentRegisterFile)
    {
        connect(m_currentRegisterFile, &RegisterFile::updateRegister, this,
                &RegisterModel::updateRegisterValue);
        connect(m_currentRegisterFile, &RegisterFile::updateFRegister, this,
                &RegisterModel::updateFRegisterValue);
        connect(m_currentRegisterFile, &RegisterFile::updateVectorRegister, this,
                &RegisterModel::updateVRegisterValue);
        connect(m_currentRegisterFile, &RegisterFile::registerResetSignal, this,
                &RegisterModel::registerResetSlot);
    }
    endResetModel();
}

void RegisterModel::registerResetSlot()
{
    m_highlightedRegisterIndex = static_cast<size_t>(-1);
    beginResetModel();
    endResetModel();
}

void RegisterModel::updateRegisterValue(size_t regIndex, uint64_t value)
{
    //QModelIndex index = this->index(static_cast<int>(regIndex), 2);
    m_highlightedRegisterIndex = regIndex; // Update highlighted register index
    // emit dataChanged(index,index);
    beginResetModel();
    endResetModel();
}

void RegisterModel::updateFRegisterValue(size_t regIndex, uint64_t value)
{
    Q_UNUSED(value);
    m_highlightedRegisterIndex = regIndex + 32;
    beginResetModel();
    endResetModel();
}

void RegisterModel::updateVRegisterValue(size_t regIndex)
{
    m_highlightedRegisterIndex = regIndex + 64;
    beginResetModel();
    endResetModel();
}
} // namespace Kites

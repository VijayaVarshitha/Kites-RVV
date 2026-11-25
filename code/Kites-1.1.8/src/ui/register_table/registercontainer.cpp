#include "registercontainer.h"
#include "ui_registercontainer.h"
#include "processor/registers.h"
#include "ui/common/collapsible_section.h"
#include <QHeaderView>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QAbstractItemView>
#include <QVBoxLayout>
#include <array>

namespace Kites
{
namespace
{
// Filters a RegisterModel down to a contiguous row range, so General
// Purpose / Floating Point registers can be shown in separate collapsible
// sections while sharing the same underlying model.
class RegisterRangeProxyModel : public QSortFilterProxyModel
{
  public:
    RegisterRangeProxyModel(int firstRow, int lastRowExclusive, QObject *parent = nullptr)
        : QSortFilterProxyModel(parent), m_firstRow(firstRow), m_lastRowExclusive(lastRowExclusive)
    {
    }

  protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        Q_UNUSED(sourceParent);
        return sourceRow >= m_firstRow && sourceRow < m_lastRowExclusive;
    }

  private:
    int m_firstRow;
    int m_lastRowExclusive;
};
} // namespace

RegisterContainer::RegisterContainer(QWidget *parent, RegisterFile *regfile)
    : QWidget(parent), ui(new Ui::RegisterContainer),
      m_registerModel(new RegisterModel(this, regfile)), m_currentRegisterFile(regfile)
{
    ui->setupUi(this);
    setupRegisterTable();

    ui->displayType->setCurrentText("Hexadecimal");
    connect(ui->displayType, &QComboBox::currentTextChanged, this,
            [this](const QString &text)
            {
                Base base = Base::Hexadecimal;
                if (text == "Binary")
                    base = Base::Binary;
                else if (text == "Decimal/Float")
                    base = Base::Decimal;
                setDisplayBase(base);
            });
    setDisplayBase(Base::Hexadecimal);
}

QSortFilterProxyModel *RegisterContainer::makeRangeProxy(int firstRow, int lastRowExclusive)
{
    auto *proxy = new RegisterRangeProxyModel(firstRow, lastRowExclusive, this);
    proxy->setSourceModel(m_registerModel);
    return proxy;
}

void RegisterContainer::setupRegisterTable()
{
    auto *sectionsLayout = new QVBoxLayout(ui->sectionsContainer);
    sectionsLayout->setContentsMargins(0, 0, 0, 0);
    sectionsLayout->setSpacing(4);

    struct SectionSpec
    {
        QString title;
        int firstRow;
        int lastRowExclusive;
    };
    const std::array<SectionSpec, 3> sections = {
        SectionSpec{"General Purpose", 0, 32},
        SectionSpec{"Floating Point", 32, 64},
        SectionSpec{"Vector Registers", 64, 96},
    };

    for (const SectionSpec &spec : sections)
    {
        auto *section = new CollapsibleSection(spec.title, ui->sectionsContainer);

        auto *tableView = new QTableView(section);
        auto *rangeModel = makeRangeProxy(spec.firstRow, spec.lastRowExclusive);
        tableView->setModel(rangeModel);
        tableView->verticalHeader()->setVisible(false);
        tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        if (spec.firstRow == 64)
        {
            // Keep all vector lanes on one line and allow horizontal scrolling.
            tableView->setWordWrap(false);
            tableView->setTextElideMode(Qt::ElideNone);
            tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }

        auto* header = tableView->horizontalHeader();
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(2, QHeaderView::Stretch);
        if (spec.firstRow == 64)
            header->setSectionResizeMode(2, QHeaderView::ResizeToContents);

        section->setContentWidget(tableView);
        sectionsLayout->addWidget(section);
    }

    auto *configurationSection =
        new CollapsibleSection("Vector Configuration", ui->sectionsContainer);

    // Use the same table presentation as the GPR/FPR/vector register sections.
    // The VTYPE value is formatted using the shared Display Type selector.
    m_vectorConfigurationTable = new QTableView(configurationSection);
    m_vectorConfigurationModel = new QStandardItemModel(4, 3, m_vectorConfigurationTable);
    m_vectorConfigurationModel->setHorizontalHeaderLabels(
        {QStringLiteral("Field"), QStringLiteral("Description"), QStringLiteral("Value")});

    const QStringList fields = {
        QStringLiteral("VL"), QStringLiteral("SEW"),
        QStringLiteral("LMUL"), QStringLiteral("VTYPE")};
    const QStringList descriptions = {
        QStringLiteral("Active vector length"),
        QStringLiteral("Standard element width"),
        QStringLiteral("Vector length multiplier"),
        QStringLiteral("Encoded vector type")};
    for (int row = 0; row < fields.size(); ++row)
    {
        m_vectorConfigurationModel->setItem(row, 0, new QStandardItem(fields[row]));
        m_vectorConfigurationModel->setItem(row, 1, new QStandardItem(descriptions[row]));
        m_vectorConfigurationModel->setItem(row, 2, new QStandardItem(QString{}));
    }

    m_vectorConfigurationTable->setModel(m_vectorConfigurationModel);
    m_vectorConfigurationTable->verticalHeader()->setVisible(false);
    m_vectorConfigurationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_vectorConfigurationTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_vectorConfigurationTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_vectorConfigurationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_vectorConfigurationTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_vectorConfigurationTable->setFocusPolicy(Qt::NoFocus);
    m_vectorConfigurationTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_vectorConfigurationTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_vectorConfigurationTable->resizeRowsToContents();
    m_vectorConfigurationTable->setFixedHeight(
        m_vectorConfigurationTable->horizontalHeader()->sizeHint().height() +
        4 * m_vectorConfigurationTable->verticalHeader()->defaultSectionSize() + 4);

    configurationSection->setContentWidget(m_vectorConfigurationTable);
    sectionsLayout->addWidget(configurationSection);

    if (m_currentRegisterFile)
    {
        connect(m_currentRegisterFile, &RegisterFile::updateVectorConfiguration, this,
                &RegisterContainer::updateVectorConfigurationDisplay);
        connect(m_currentRegisterFile, &RegisterFile::registerResetSignal, this,
                &RegisterContainer::refreshVectorConfigurationDisplay);
    }
    refreshVectorConfigurationDisplay();
}

void RegisterContainer::setRegisterFile(RegisterFile *regfile)
{
    if (m_currentRegisterFile)
        disconnect(m_currentRegisterFile, nullptr, this, nullptr);
    m_currentRegisterFile = regfile;
    m_registerModel->changeRegisterFile(regfile);
    if (m_currentRegisterFile)
    {
        connect(m_currentRegisterFile, &RegisterFile::updateVectorConfiguration, this,
                &RegisterContainer::updateVectorConfigurationDisplay);
        connect(m_currentRegisterFile, &RegisterFile::registerResetSignal, this,
                &RegisterContainer::refreshVectorConfigurationDisplay);
    }
    refreshVectorConfigurationDisplay();
}

void RegisterContainer::refreshVectorConfigurationDisplay()
{
    if (!m_currentRegisterFile || !m_vectorConfigurationModel)
        return;
    updateVectorConfigurationDisplay(m_currentRegisterFile->ReadVl(),
                                     m_currentRegisterFile->ReadVtype(),
                                     m_currentRegisterFile->ReadSew(),
                                     m_currentRegisterFile->ReadLmul());
}

void RegisterContainer::setDisplayBase(Base base)
{
    m_displayBase = base;
    m_registerModel->setDisplayBase(base);
    refreshVectorConfigurationDisplay();
}

QString RegisterContainer::formatVtype(uint64_t vtype) const
{
    QString formatted;
    switch (m_displayBase)
    {
    case Base::Binary:
        formatted = QStringLiteral("0b") + QString::number(static_cast<qulonglong>(vtype), 2);
        break;
    case Base::Decimal:
        formatted = QString::number(static_cast<qulonglong>(vtype), 10);
        break;
    case Base::Hexadecimal:
        formatted = QStringLiteral("0x") +
                    QString::number(static_cast<qulonglong>(vtype), 16).toUpper();
        break;
    }

    if ((vtype & (uint64_t{1} << 31U)) != 0)
        formatted += QStringLiteral(" (vill)");
    return formatted;
}

void RegisterContainer::updateVectorConfigurationDisplay(size_t vl, uint64_t vtype,
                                                          size_t sew, size_t lmul)
{
    if (!m_vectorConfigurationModel)
        return;

    m_vectorConfigurationModel->item(0, 2)->setText(QString::number(vl));
    m_vectorConfigurationModel->item(1, 2)->setText(QStringLiteral("%1-bit").arg(sew));
    m_vectorConfigurationModel->item(2, 2)->setText(QStringLiteral("m%1").arg(lmul));
    m_vectorConfigurationModel->item(3, 2)->setText(formatVtype(vtype));
}

RegisterContainer::~RegisterContainer()
{
    delete ui;
}

} // namespace Kites

#ifndef REGISTERCONTAINER_H
#define REGISTERCONTAINER_H

#include "registermodel.h"
#include <QWidget>
#include <cstddef>
#include <cstdint>
#include <memory>

class QSortFilterProxyModel;
class QStandardItemModel;
class QTableView;

namespace Kites
{
namespace Ui
{
class RegisterContainer;
}
class CollapsibleSection;

class RegisterContainer : public QWidget
{
    Q_OBJECT

  public:
    explicit RegisterContainer(QWidget *parent = nullptr, RegisterFile *regfile = nullptr);
    ~RegisterContainer();
    void setRegisterFile(RegisterFile *regfile);
    // private slots:
    //     void updateRegisterValue(size_t regIndex, uint64_t value);
  private:
    void setupRegisterTable();
    QSortFilterProxyModel *makeRangeProxy(int firstRow, int lastRowExclusive);
    void refreshVectorConfigurationDisplay();
    void setDisplayBase(Base base);
    QString formatVtype(uint64_t vtype) const;

    Ui::RegisterContainer *ui;
    RegisterModel *m_registerModel;
    RegisterFile *m_currentRegisterFile = nullptr;
    QTableView *m_vectorConfigurationTable = nullptr;
    QStandardItemModel *m_vectorConfigurationModel = nullptr;
    Base m_displayBase = Base::Hexadecimal;

  private slots:
    void updateVectorConfigurationDisplay(size_t vl, uint64_t vtype, size_t sew, size_t lmul);
};
} // namespace Kites
#endif // REGISTERCONTAINER_H

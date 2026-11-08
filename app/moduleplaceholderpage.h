#ifndef MODULEPLACEHOLDERPAGE_H
#define MODULEPLACEHOLDERPAGE_H

#include <QWidget>

class QLabel;
namespace Ui { class ModulePlaceholderPage; }

/// Temporary page for modules that are not implemented yet.
class ModulePlaceholderPage : public QWidget
{
    Q_OBJECT

public:
    explicit ModulePlaceholderPage(QWidget *parent = 0);
    ~ModulePlaceholderPage();
    void setModule(const QString &moduleId);

signals:
    void backRequested();

private:
    Ui::ModulePlaceholderPage *ui;
};

#endif // MODULEPLACEHOLDERPAGE_H

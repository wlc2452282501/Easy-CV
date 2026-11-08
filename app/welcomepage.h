#ifndef WELCOMEPAGE_H
#define WELCOMEPAGE_H

#include <QWidget>

namespace Ui { class WelcomePage; }

/// Designer-built entry page for the three application modules.
class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(QWidget *parent = 0);
    ~WelcomePage();

signals:
    void moduleRequested(const QString &moduleId);

private:
    Ui::WelcomePage *ui;
};

#endif // WELCOMEPAGE_H

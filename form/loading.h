#ifndef LOADING_H
#define LOADING_H

#include <QFrame>

class QLabel;

namespace Ui {
class Loading;
}

class Loading : public QFrame
{
    Q_OBJECT

public:
    explicit Loading(QWidget *parent);
    ~Loading();

signals:
    void started();
    void stoped();
    void cancel();

public slots:
    QObject *start(QString msg = "", bool bCancel = false, int nValue = -1);
    void stop();
    void onResize(const QSize &size);
    void setCancel(const bool &bEnable);
    void setText(const QString &text);
    void setValue(const int &value);

private slots:
    void updateValue();

public:
    QObject *object = nullptr;
    std::atomic_int value{ -1 };

private:
    Ui::Loading *ui;
    QLabel *m_labelValue = nullptr;
    QTimer *m_timer = nullptr;
    int m_value = -1;
};
extern Loading *qLoading;

struct LoadingProgress {
    int max{0};
    int n{0};
    int p{0};
    bool add(const int &m = 100) {
        if (max <= 0) return false;
        auto v = (++n) * m / max;
        if (p == v) return false;
        p = v;
        return true;
    }
};

#endif // LOADING_H

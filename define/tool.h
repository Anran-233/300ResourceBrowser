#ifndef TOOL_H
#define TOOL_H

#include <QDebug>
#include <QThread>
#include <QKeyEvent>
#include <QWidget>
#include <QProgressDialog>

namespace Tool {
    /// 转换为QT内部编码 (返回值:QT文本) (data 数据)
    QString FromText(const QByteArray& data);
    /// 读取文件数据 (返回值:是否成功) (path 文件路径, data 读取数据)
    bool ReadFileData(const QString &path, QByteArray &data);
    /// 写入文件数据 (返回值:是否成功) (path 文件路径, data 写入数据, mkpath 是否自动创建前置目录)
    bool WriteFileData(const QString &path, const QByteArray &data, bool mkpath = true);
    /// 获取文件MD5 (返回值:MD5) (path 文件路径)
    QByteArray GetFileMD5(const QString &path);
}

/**
 * @brief 作用域善后处理
 */
class CActionScope {
    std::function<void()> m_deleteFun = nullptr; ///< 清理函数
public:
    /// 传入(deleteFun 作用域结束时执行此函数)
    CActionScope(std::function<void()> deleteFun) : m_deleteFun(deleteFun) {}
    /// 传入(initFun 初始化函数, deleteFun 作用域结束时执行此函数)
    CActionScope(std::function<void()> initFun, std::function<void()> deleteFun) : m_deleteFun(deleteFun) { if (initFun) initFun(); }
    /// 设置清理函数(deleteFun 作用域结束时执行此函数)
    void set(std::function<void()> deleteFun) { m_deleteFun = deleteFun; }
    /// 立即执行清理函数(bClear 执行后是否删除清理函数)
    void execute(bool bClear = true) { if (m_deleteFun) m_deleteFun(); if (bClear) m_deleteFun = nullptr; }
    /// 删除执行清理函数
    void clear() { m_deleteFun = nullptr; }
    /// 作用域结束时执行清理函数
    ~CActionScope() { if (m_deleteFun) m_deleteFun(); }
};
#define ActionScopeName2(name,num) name##num
#define ActionScopeName1(name,num) ActionScopeName2(name,num)
#define ActionScope CActionScope ActionScopeName1(scope, __LINE__)

/**
 * @brief 轻量级线程对象
 */
class CThread : public QObject
{
    Q_OBJECT
public:
    CThread(QObject *parent = nullptr);
    ~CThread();
    void start();
signals:
    void started();
    void finished(QVariant result = {});
    void message(int state, QVariant msg);
private:
    QThread* m_thread = nullptr;
};

/**
 * @brief 悬浮提示
 */
class CTip : public QWidget
{
    Q_OBJECT
public:
    explicit CTip(QWidget *parent = nullptr);
    ~CTip() {}
signals:
    void setText(const QString& strTip);
    void restart(int nTimeout = 1000);
public slots:
    void updateSize(const QSize& size);
    void tip(const QString& strTip, int nTimeout = 1000);
};

/**
 * @brief 事件过滤器（屏蔽Dialog自带的Esc按键事件）
 */
class CEventFilter : public QObject {
    bool eventFilter(QObject *, QEvent *event) override;
public:
    static CEventFilter *Instance();
};

/**
 * @brief 轻量级Http业务处理
 */
class Http : public QObject {
    Q_OBJECT
public:
    Http(QObject* parent = nullptr);
    ~Http();
    /// 设置链接
    Http *url(const QString& strUrl);
    /// 在链接尾部添加随机参数
    Http *addRandom();
    /// 是否正在执行任务
    bool isRunning() const;
    /// 返回数据
    QByteArray &data();
    /// get请求
    void get();
    /// 下载文件 (strSavePath 保存路径)
    void download(const QString& strSavePath);
    /// 停止当前下载任务
    void stop();
    /// 下载文件(非阻塞等待)
    static QByteArray DownloadData(const QString &url, bool bRandom = true, std::atomic_bool *bStop = nullptr);

signals:
    /// 下载进度 (progress 当前进度, progressMax 总进度)
    void progressChanged(quint32 progress, quint32 progressMax);
    /// 操作完成 (error 是否出错[0.正常][1.中止][-1.失败])
    void finished(int error);
private:
    QString m_strUrl;
    QByteArray m_data;
    std::atomic_bool m_running{false};
    std::atomic_bool m_result{false};
    static std::mutex m_mutex;
};

#endif // TOOL_H

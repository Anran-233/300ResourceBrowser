#include "tool.h"

#include <QCryptographicHash>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>
#include <QTextCodec>
#include <Urlmon.h>
#include <QEventLoop>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QGuiApplication>
#include <QFileInfo>
#include <QDir>
#pragma comment(lib, "Urlmon.lib")

QString Tool::FromText(const QByteArray &data)
{
    // 读取3字节用于判断
    const char& c1 = data.size() > 0 ? data[0] : '\0';
    const char& c2 = data.size() > 1 ? data[1] : '\0';
    const char& c3 = data.size() > 2 ? data[2] : '\0';
    if (c1 == '\xFF' && c2 == '\xFE') {
        return QTextCodec::codecForName("UTF-16LE")->toUnicode(data);
    }
    else if (c1 == '\xFE' && c2 == '\xFF') {
        return QTextCodec::codecForName("UTF-16BE")->toUnicode(data);
    }
    if (c1 == '\xEF' && c2 == '\xBB' && c3 == '\xBF') {
        return QTextCodec::codecForName("UTF-8")->toUnicode(data);
    }
    //尝试用utf8转换,如果可用字符数大于0,则表示是ansi编码
    QTextCodec::ConverterState state;
    QString text = QTextCodec::codecForName("UTF-8")->toUnicode(data.constData(), data.size(), &state);
    if (state.invalidChars > 0) return QString::fromLocal8Bit(data);
    else return text;
}

bool Tool::ReadFileData(const QString &path, QByteArray &data)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    data = file.readAll();
    file.close();
    return true;
}

bool Tool::WriteFileData(const QString &path, const QByteArray &data, bool mkpath)
{
    if (mkpath) QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(data);
    file.close();
    return true;
}

QByteArray Tool::GetFileMD5(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    QByteArray md5 = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5).toHex();
    file.close();
    return md5;
}

CThread::CThread(QObject *parent) : m_thread(new QThread)
{
    if (parent) connect(parent, &QObject::destroyed, m_thread, [=]{ delete this; }, Qt::DirectConnection);
    connect(m_thread, &QThread::started, this, &CThread::started);
    this->moveToThread(m_thread);
}

CThread::~CThread()
{
    this->disconnect();
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
        delete m_thread;
    }
}

void CThread::start()
{
    if (m_thread->isRunning()) return;
    m_thread->start();
}

CTip::CTip(QWidget *parent) : QWidget(parent)
{
    setAutoFillBackground(false);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    installEventFilter(CEventFilter::Instance());

    // UI
    move(0, 0);
    resize(parent->width(), 30);
    QLabel* label = new QLabel(this);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(u8"font: 12px \"微软雅黑\";"
                         "border-radius:8px;"
                         "background-color: rgba(0, 0, 0, 0.5);"
                         "color: rgba(255, 255, 255, 1);");
    connect(this, &CTip::setText, label, &QLabel::setText);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5,2,5,2);
    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(label);
    this->setLayout(layout);

    // 淡出效果
    QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect(this);
    opacity->setOpacity(1);
    label->setGraphicsEffect(opacity);
    QPropertyAnimation* animation = new QPropertyAnimation(opacity, "opacity");
    animation->setDuration(500);
    animation->setStartValue(1);
    animation->setEndValue(0);
    connect(animation, &QPropertyAnimation::finished, this, [=] { this->setVisible(0); });
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=] { timer->stop(), animation->start(); });
    connect(this, &CTip::restart, this, [=](int nTimeout){
        animation->stop();
        opacity->setOpacity(1);
        this->show();
        timer->start(nTimeout);
    });
}

void CTip::updateSize(const QSize& size)
{
    resize(size.width(), 30);
}

void CTip::tip(const QString& strTip, int nTimeout)
{
    emit setText("   " + strTip + "   ");
    emit restart(nTimeout);
}

bool CEventFilter::eventFilter(QObject *obj, QEvent *event) {
    return QObject::eventFilter(obj, event);
}

CEventFilter *CEventFilter::Instance() {
    static CEventFilter d;
    return &d;
}

Http::Http(QObject *parent) : QObject(parent)
{

}

Http::~Http()
{
    stop();
}

Http *Http::url(const QString &strUrl)
{
    m_strUrl = strUrl;
    return this;
}

Http *Http::addRandom()
{
    static quint8 count = 0;
    if (count >= 0xff) count = 0;
    m_strUrl += QString("?T=%1&N=%2").arg(time(0)).arg(++count);
    return this;
}

bool Http::isRunning() const
{
    return m_running;
}

QByteArray &Http::data()
{
    return this->m_data;
}

// 回调下载进度
class DownloadProgress : public IBindStatusCallback {
public:
    DownloadProgress(Http* http) : m_http(http) {}
    HRESULT __stdcall QueryInterface(const IID&,void**) { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef(void) { return 1; }
    ULONG STDMETHODCALLTYPE Release(void) { return 1; }
    HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD, IBinding*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE GetPriority(LONG*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE OnLowResource(DWORD) { return S_OK; }
    virtual HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT, LPCWSTR) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD*, BINDINFO*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD, DWORD, FORMATETC*, STGMEDIUM*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID, IUnknown*) { return E_NOTIMPL; }
    virtual HRESULT __stdcall OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG, LPCWSTR) {
        emit m_http->progressChanged(ulProgress, ulProgressMax);
        if (m_http->isRunning()) return S_OK;
        else return E_ABORT;
    }
    Http* m_http = nullptr;
};

std::mutex Http::m_mutex;
void Http::get()
{
    if (m_running) return; // 同时只能执行一个任务
    m_running = true;
    m_data.clear();
    CThread* thread = new CThread(this);
    connect(thread, &CThread::started, thread, [=]{
        std::lock_guard<std::mutex> locker(m_mutex);
        m_result = false;
        while(true) {
            // 下载数据
            DownloadProgress progress(this);
            IStream* stream = nullptr;
            ActionScope([&stream]{ if (stream) stream->Release();});
            if (S_OK != URLOpenBlockingStreamW(nullptr, m_strUrl.toStdWString().c_str(), &stream, 0, &progress)) break;
            if (!m_running) return;
            // 获取数据大小
            STATSTG stat = {};
            if (S_OK != stream->Stat(&stat, STATFLAG_NONAME)) break;
            // 将数据装入容器
            DWORD bytesRead;
            m_data.resize(stat.cbSize.QuadPart);
            auto r = stream->Read(m_data.data(), stat.cbSize.QuadPart, &bytesRead);
            if (S_OK != r) break;
            if (bytesRead < stat.cbSize.QuadPart) m_data.resize(bytesRead);
            // 任务成功
            m_result = true;
            break;
        }
        emit thread->finished((bool)m_result);
    });
    connect(thread, &CThread::finished, this, [=](QVariant result){
        delete thread;
        if (m_running){
            m_running = false;
            emit finished(result.toBool() ? 0 : -1);
        }
        else emit finished(1);
    });
    thread->start();
}

void Http::download(const QString &strSavePath)
{
    m_running = true;
    CThread* thread = new CThread(this);
    connect(thread, &CThread::started, thread, [=]{
        std::lock_guard<std::mutex> locker(m_mutex);
        DownloadProgress progress(this);
        if (S_OK != URLDownloadToFileW(NULL, m_strUrl.toStdWString().c_str(), strSavePath.toStdWString().c_str(), 0, &progress))
            emit thread->finished(false);
        else emit thread->finished(true);
    });
    connect(thread, &CThread::finished, this, [=](QVariant result){
        delete thread;
        if (m_running){
            m_running = false;
            emit finished(result.toBool() ? 0 : -1);
        }
        else emit finished(1);
    });
    thread->start();
}

void Http::stop()
{
    m_running = false;
}

QByteArray Http::DownloadData(const QString &url, bool bRandom, std::atomic_bool *bStop)
{
    Http http;
    http.url(url);
    if (bRandom) http.addRandom();
    http.get();
    QEventLoop loop;
    while (http.m_running) {
        if (bStop) if (*bStop) return {};
        loop.processEvents();
    }
    if (http.m_result) {
        QByteArray data;
        std::swap(data, http.m_data);
        return data;
    }
    else return {};
}

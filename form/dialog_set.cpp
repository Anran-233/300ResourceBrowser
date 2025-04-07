#include "dialog_set.h"
#include "ui_dialog_set.h"
#include "cpatch.h"
#include "config.h"
#include "tool.h"

#include "Shlobj.h"
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QProgressDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QProcess>

/// 获取应用关联状态 (返回值: 0.未关联 1.已关联 2.已关联(其他程序)) (strClass 关联名, strExt 拓展名(.ext))
int GetApplicationAssociation(const QString& strClass, const QString& strExt) {
    QSettings settingsrClasses("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    const QString strName = settingsrClasses.value(strExt + "/.", "").toString();
    if (strName == "") return 0; // 0.未关联
    if (strName != strClass) return 2; // 2.已关联(其他程序)
    const QString strOpenPath = settingsrClasses.value(strName + "/Shell/Open/Command/.").toString();
    const QString strAppPath = "\"" + qApp->applicationFilePath().replace('/', '\\') + "\" \"%1\"";
    if (strOpenPath != strAppPath) return 2; // 2.已关联(其他程序)

    QSettings settingsFileExts("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts", QSettings::NativeFormat);
    if (!settingsFileExts.childGroups().contains(strExt)) return 1; // 1.已关联
    if (settingsFileExts.contains(strExt + "/UserChoice/ProgId")) {
        const QString strOpenName = settingsFileExts.value(strExt + "/UserChoice/ProgId").toString();
        if (strOpenName == strClass) return 1; // 1.已关联
        else return 2; // 2.已关联(其他程序)
    }
    else {
        const QString MRUList = settingsFileExts.value(strExt + "/OpenWithList/MRUList").toString();
        if (MRUList == "") return 1; // 1.已关联
        const QString strOpenName = settingsFileExts.value(strExt + "/OpenWithList/" + MRUList.left(1)).toString();
        const QString strAppName = qApp->applicationFilePath().split('/').back();
        if (strOpenName == strAppName) return 1; // 1.已关联
        else return 2; // 2.已关联(其他程序)
    }
}

/// 设置应用关联 (strClass 关联名, strExt 拓展名(.ext), strIcon 图标路径, strInfo 说明信息)
void SetApplicationAssociation(const QString& strClass, const QString& strExt, const QString& strIcon, const QString& strInfo) {
    const QString strAppPath = "\"" + qApp->applicationFilePath().replace('/', '\\') + "\" \"%1\"";
    QSettings settingsrClasses("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    settingsrClasses.setValue(strExt + "/.", strClass);
    settingsrClasses.setValue(strClass + "/.", strInfo);
    settingsrClasses.setValue(strClass + "/DefaultIcon/.", strIcon);
    settingsrClasses.setValue(strClass + "/Shell/Open/Command/.", strAppPath);
    settingsrClasses.sync();

    QSettings settingsFileExts("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts", QSettings::NativeFormat);
    if (settingsFileExts.childGroups().contains(strExt))  {
        settingsFileExts.remove(strExt);
        settingsFileExts.sync();
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

/// 移除应用关联 (strClass 关联名, strExt 拓展名(.ext))
void RemoveApplicationAssociation(const QString& strClass, const QString& strExt) {
    QSettings settingsrClasses("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    if (settingsrClasses.childGroups().contains(strExt))  {
        settingsrClasses.remove(strExt);
        settingsrClasses.remove(strClass);
        settingsrClasses.sync();
    }

    QSettings settingsFileExts("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts", QSettings::NativeFormat);
    if (settingsFileExts.childGroups().contains(strExt))  {
        settingsFileExts.remove(strExt);
        settingsFileExts.sync();
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

/// 更新文件关联 (value 文件关联值[000])
void UpdateApplicationAssociation(const QString &value)
{
    if (value.size() < 3) return;
    for (auto i = 0; i < 3; ++i) {
        if (value[i] != '1') continue;
        const QString strExt = i == 0 ? ".jmp" : i == 1 ? ".bank" : ".dat";
        const QString strInfo = i == 0 ? u8"Jmp资源包" : i == 1 ? u8"Bank语音" : u8"Dat数据库";
        const QString strIcon = QString("%1,%2").arg(qApp->applicationFilePath().replace('/', '\\')).arg(i + 1);
        SetApplicationAssociation("300ResourceBrowser" + strExt, strExt, strIcon, strInfo);
    }
}

/// 下载更新 (parent 父级窗口, progress 进度对话框, obj 下载信息, type 更新源, index 操作阶段)
void DownloadUpdate(QWidget* parent, QProgressDialog* progress, const QJsonObject& obj, const int& type, int index) {
    static const auto IsAppNameVersion = [](const QString &appName)->bool {
        int pos = appName.lastIndexOf('v', -1, Qt::CaseInsensitive);
        if (pos < 0) return false;
        QString version = appName.mid(pos + 1, appName.size() - pos - 5);
        if (version.isEmpty()) return false;
        if (version.back() == u')') version = version.mid(0, version.length() - 1);
        auto vs = version.split('(');
        if (vs.size() > 2) return false;
        if (vs.size() == 2) for (auto &c : vs[1]) if (c < u'0' || c >u'9') return false;
        vs = vs[0].split('.');
        if (vs.size() > 2) return false;
        for (auto &v : vs) for (auto &c : v) if (c < u'0' || c >u'9') return false;
        return true;
    };
    // 1.检测本地是否有缓存更新文件
    if (index == 1) {
        progress->setLabelText(u8"检测本地缓存文件...");
        CThread* thread = new CThread;
        QObject::connect(thread, &CThread::started, thread, [=]{
            bool result = false;
            while (!result) {
                QFileInfo fileInfo(qApp->applicationDirPath() + "/BConfig/300ResourceBrowser.exe");
                if (!fileInfo.exists()) break;
                if (fileInfo.size() != obj["fileLen"].toInt(-1)) break;
                QFile file(fileInfo.filePath());
                if (!file.open(QIODevice::ReadOnly)) break;
                const QByteArray& md5 = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5).toHex();
                file.close();
                if (obj["fileMd5"].toString() != md5) break;
                result = true;
            }
            emit thread->finished(result);
        });
        QObject::connect(thread, &CThread::finished, parent, [=](QVariant result){
            delete thread;
            if (result.toBool()) DownloadUpdate(parent, progress, {}, type, 3); // 本地已下载文件，直接调用更新替换程序
            else DownloadUpdate(parent, progress, obj, type, 2); // 本地未找到缓存文件，继续下载
        });
        thread->start();
    }
    // 2.下载新版本程序
    else if (index == 2) {
        CActionScope error([=]{DownloadUpdate(parent, progress, {}, type, -1);});
        const QJsonArray array = obj["url"].toArray();
        if (array.size() <= type) return;
        const QString url = array.at(type).toString();
        if (url.isEmpty()) return;
        progress->setLabelText(u8"准备下载最新版本...");
        progress->setCancelButtonText(u8"取消");
        Http* http = new Http(parent);
        QObject::connect(progress, &QProgressDialog::rejected, parent, [=]{
            if (QMessageBox::Yes == QMessageBox::question(parent, u8"提示", u8"是否取消下载更新？")) {
                progress->setLabelText(u8"正在取消更新...");
                progress->setCancelButton(nullptr);
                progress->setValue(0);
                progress->setMaximum(0);
                http->stop();
            }
            progress->show();
        });
        QObject::connect(http, &Http::progressChanged, parent, [=](quint32 n, quint32 max){
            if (!http->isRunning()) return;
            if (!max) return;
            if (n != max) {
                const int& value = n * 100 / max;
                if (progress->maximum() != 100) progress->setMaximum(100);
                if (progress->value() != value) progress->setValue(value);
                progress->setLabelText(QString(u8"正在下载最新版本 (%1/%2kb)").arg(n / 1024).arg(max / 1024));
            }
        });
        QObject::connect(http, &Http::finished, parent, [=](int error){
            if (error == -1) {
                delete progress;
                delete http;
                QMessageBox::critical(parent, u8"错误", u8"更新失败！");
                return;
            }
            if (error == 1) {
                delete progress;
                delete http;
                QMessageBox::information(parent, u8"提示", u8"更新取消！");
                return;
            }
            // 解压下载数据
            progress->setLabelText(u8"正在解压下载数据...");
            progress->setCancelButton(nullptr);
            progress->setValue(0);
            progress->setMaximum(0);
            CThread* thread = new CThread;
            QObject::connect(thread, &CThread::started, thread, [=]{
                bool result = false;
                while (!result) {
                    // 检查下载数据长度
                    if (http->data().size() != obj["compLen"].toInt()) break;
                    // 检查下载数据MD5
                    const QByteArray& compMd5 = QCryptographicHash::hash(http->data(), QCryptographicHash::Md5).toHex();
                    if (compMd5 != obj["compMd5"].toString()) break;
                    // 解压数据
                    QByteArray fileData;
                    if (Zlib::UncompGz((uchar*)http->data().data(), http->data().size(), fileData)) break;
                    // 检查文件数据长度
                    if (fileData.size() != obj["fileLen"].toInt()) break;
                    // 检查文件数据MD5
                    const QByteArray& fileMd5 = QCryptographicHash::hash(fileData, QCryptographicHash::Md5).toHex();
                    if (fileMd5 != obj["fileMd5"].toString()) break;
                    // 写入文件
                    QFile file("BConfig/300ResourceBrowser.exe");
                    if (!file.open(QIODevice::WriteOnly)) break;
                    file.write(fileData);
                    file.close();
                    // 成功
                    result = true;
                }
                emit thread->finished(result);
            });
            QObject::connect(thread, &CThread::finished, parent, [=](QVariant result){
                delete http;
                delete thread;
                if (result.toBool()) DownloadUpdate(parent, progress, {}, type, 3); // 调用更新替换程序
                else DownloadUpdate(parent, progress, {}, type, -1);
            });
            thread->start();
        });
        error.clear();
        http->url(url)->addRandom()->get();
    }
    // 3.调用更新替换程序
    else if (index == 3) {
        delete progress;
        const QString strOpenPath = qApp->applicationDirPath().replace('/', '\\').append("\\BConfig\\update.exe");
        const QString strSourcePath = qApp->applicationDirPath().replace('/', '\\').append("\\BConfig\\300ResourceBrowser.exe");
        const QString strDeletePath = qApp->applicationFilePath().replace('/', '\\');
        QString strDestPath = strDeletePath;
        QString strInfo("000");
        QString strAppName = strDeletePath.split('\\').back();
        if (IsAppNameVersion(strAppName)) {
            const QStringList& versions = QString(APP_VERSION).split('.');
            strDestPath = QString("%1v%2.%3%4.exe")
                    .arg(strDeletePath.left(strDeletePath.lastIndexOf('v', -1, Qt::CaseInsensitive)),
                         versions[1], versions[2], versions[3] == "0" ? "" : ("(" + versions[3] + ")"));
            const QString strExts[3] {".jmp", ".bank", ".dat"}; // 确认文件关联是否需要更新
            for (int i = 0; i < 3; ++i) strInfo[i] = GetApplicationAssociation("300ResourceBrowser" + strExts[i], strExts[i]) == 1 ? '1' : '0';
        }
        QProcess::startDetached(strOpenPath, QStringList() << strSourcePath << strDeletePath << strDestPath << strInfo);
        qApp->quit();
    }
    // -1.更新失败
    else if (index == -1) {
        delete progress;
        QMessageBox::critical(parent, u8"错误", u8"更新失败！");
    }
}

/// 获取下载信息 (parent 父级窗口, progress 进度对话框, type 更新源)
void GetDownloadInfo(QWidget* parent, const int& type) {
    static const char* urls[3] = {
        "https://gitee.com/anran_233/300ResourceBrowser/raw/main/update/download.json",  // 0.Gitee
        "https://github.com/Anran-233/300ResourceBrowser/raw/main/update/download.json", // 1.Github
        "https://Anran233.com/app/300hero/300ResourceBrowser/update/download.json"       // 2.备用服务器
    };
    Http* http = new Http(parent);
    QProgressDialog* progress = new QProgressDialog(u8"正在获取下载信息...", nullptr, 0, 0, parent, Qt::Dialog | Qt::FramelessWindowHint);
    progress->installEventFilter(CEventFilter::Instance());
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->show();
    QObject::connect(http, &Http::finished, parent, [=](int error){
        while (error == 0) {
            const QJsonObject obj = QJsonDocument::fromJson(http->data()).object();
            if (!obj.contains("version")) break;
            if (!obj.contains("fileLen")) break;
            if (!obj.contains("fileMd5")) break;
            if (!obj.contains("compLen")) break;
            if (!obj.contains("compMd5")) break;
            if (!obj.contains("url")) break;
            delete http;
            DownloadUpdate(parent, progress, obj, type, 1);
            return;
        }
        QMessageBox::critical(parent, u8"错误", u8"下载更新失败！");
        delete progress;
        delete http;
    });
    http->url(urls[qMax(qMin(type, 2), 0)])->addRandom()->get();
}

/// 检测更新 (parent 窗口指针, type 更新源, bSuccessMsg 成功是否弹窗)
void CheckUpdate(QWidget* parent, const int& type, bool bSuccessMsg) {
    static const char* urls[3] = {
        "https://gitee.com/anran_233/300ResourceBrowser/raw/main/update/version.txt",    // 0.Gitee
        "https://github.com/Anran-233/300ResourceBrowser/raw/main/update/version.txt",  // 1.Github
        "https://Anran233.com/app/300hero/300ResourceBrowser/update/version.txt"        // 2.备用服务器
    };
    /* 判断是否为新版本 */ static auto isNewVersion = [](const QByteArray& version)->bool{
        if (version == APP_VERSION) return false;   // 版本相同
        const auto& newVersions = version.split('.');
        if (newVersions.size() < 4) return false;   // 新版本格式有误
        const auto& oldVersions = QByteArray(APP_VERSION).split('.');
        for (int i = 0; i < 4; ++i) {
            if (oldVersions[i] == newVersions[i]) continue; // 此段版本号相同
            const unsigned int& oldNum = oldVersions[i].toUInt();
            const unsigned int& newNum = newVersions[i].toUInt();
            if (oldNum == newNum) continue; // 此段版本号相同
            return oldNum < newNum; // 当前版本是否小于最新版本
        }
        return false; // 版本相同
    };

    QProgressDialog* progress = new QProgressDialog(u8"正在检测最新版本...", nullptr, 0, 0, parent, Qt::Dialog | Qt::FramelessWindowHint);
    progress->installEventFilter(CEventFilter::Instance());
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(300);
    progress->show();

    Http* http = new Http(parent);
    QObject::connect(http, &Http::finished, parent, [=](int error){
        delete progress;
        if (error == 0) {
            auto list = http->data().split('.');
            if (isNewVersion(http->data())) {
                if (QMessageBox::Yes == QMessageBox::question(parent, u8"提示", u8"检测到新版本！是否进行更新？\n当前：v" APP_VERSION "  最新：v" + http->data())) {
                    GetDownloadInfo(parent, type);
                }
            }
            else {
                if (bSuccessMsg) QMessageBox::information(parent, u8"提示", u8"当前程序已是最新版本！\n版本：v" APP_VERSION);
                qConfig.set_strUpdateLast(QString::number(time(0))); // 记录本次检测更新时间
            }
        }
        else QMessageBox::critical(parent, u8"错误", u8"检测更新失败！");
        delete http;
    });
    http->url(urls[qMax(qMin(type, 2), 0)])->addRandom()->get();
}

Dialog_Set::Dialog_Set(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_Set)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    Init();
}

Dialog_Set::~Dialog_Set()
{
    delete ui;
}

void Dialog_Set::Init()
{
    ui->label_text->clear();

    // 目录设置
    ui->line_gamePath->setText(qConfig.strGamePath);
    ui->line_exportPath->setText(qConfig.strExportPath);
    ui->line_alonePath->setText(qConfig.strAlonePath);
    connect(ui->Button_find, &QPushButton::clicked, this, &Dialog_Set::onAutoFind);
    connect(ui->Button_gamePath, &QPushButton::clicked, this, [this]{ onSetPath(0); });
    connect(ui->Button_exportPath, &QPushButton::clicked, this, [this]{ onSetPath(1); });
    connect(ui->Button_alonePath, &QPushButton::clicked, this, [this]{ onSetPath(2); });

    // 导出模式
    ui->comboBox_export_mode->setCurrentIndex(qBound(0, qConfig.nExportMode, 3));

    // 略缩图预览器
    ui->comboBox_preview->setCurrentIndex(qConfig.bPreview ? 1 : 0);
    ui->comboBox_preview_size->setCurrentIndex(qBound(0, qConfig.nPreviewSize, 2));
    ui->comboBox_preview_mode->setCurrentIndex(qConfig.nPreviewMode ? 1 : 0);

    // 文件关联(jmp, bank, dat)
    QLabel* link_labels[3] {ui->label_link_jmp, ui->label_link_bank, ui->label_link_dat};
    QPushButton* link_buttons[3] {ui->Button_link_jmp, ui->Button_link_bank, ui->Button_link_dat};
    const QString strExts[3] {".jmp", ".bank", ".dat"};
    for (int i = 0; i < 3; ++i) {
        const int& link = GetApplicationAssociation("300ResourceBrowser" + strExts[i], strExts[i]);
        link_labels[i]->setText(link == 0 ? u8"未关联" : link == 1 ? u8"已关联" : u8"已关联(其他程序)");
        link_buttons[i]->setText(link == 0 ? u8"关联" : link == 1 ? u8"取消" : u8"关联");
    }
    connect(ui->Button_link_jmp, &QPushButton::clicked, this, [this]{ onSetLink(0); });
    connect(ui->Button_link_bank, &QPushButton::clicked, this, [this]{ onSetLink(1); });
    connect(ui->Button_link_dat, &QPushButton::clicked, this, [this]{ onSetLink(2); });

    // 自动更新
    ui->comboBox_update_interval->setCurrentIndex(qBound(0, qConfig.nUpdateInterval, 3));
    ui->comboBox_update_source->setCurrentIndex(qBound(0, qConfig.nUpdateSource, 2));
    connect(ui->Button_update, &QPushButton::clicked, this, &Dialog_Set::onUpdate);

    // 获取项目
    connect(ui->Button_github, &QPushButton::clicked, this, []{QDesktopServices::openUrl(QUrl("https://github.com/Anran-233/300ResourceBrowser"));});
    connect(ui->Button_gitee, &QPushButton::clicked, this, []{QDesktopServices::openUrl(QUrl("https://gitee.com/anran_233/300ResourceBrowser"));});

    // 确认取消
    connect(ui->Button_OK, &QPushButton::clicked, this, &Dialog_Set::onOK);
    connect(ui->Button_OUT, &QPushButton::clicked, this, &Dialog_Set::onOUT);
}

void Dialog_Set::onOK()
{
    // 目录设置
    qConfig.set_strGamePath(ui->line_gamePath->text());
    qConfig.set_strExportPath(ui->line_exportPath->text());
    qConfig.set_strAlonePath(ui->line_alonePath->text());

    // 导出模式
    qConfig.set_nExportMode(ui->comboBox_export_mode->currentIndex());

    // 略缩图预览器
    qConfig.set_bPreview(ui->comboBox_preview->currentIndex());
    qConfig.set_nPreviewSize(ui->comboBox_preview_size->currentIndex());
    qConfig.set_nPreviewMode(ui->comboBox_preview_mode->currentIndex());

    // 自动更新
    qConfig.set_nUpdateInterval(ui->comboBox_update_interval->currentIndex());
    qConfig.set_nUpdateSource(ui->comboBox_update_source->currentIndex());

    this->close();
}

void Dialog_Set::onOUT()
{
    this->close();
}

void Dialog_Set::onAutoFind()
{
    ui->label_text->setText("");
    // 从注册表搜索
    QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\", QSettings::NativeFormat);
    for (auto& strRegItem : settings.childGroups()) {
        settings.beginGroup(strRegItem);
        const QString& strDisplayName = settings.value("DisplayName").toString();
        if((!strDisplayName.isEmpty())&&(strDisplayName.contains("300", Qt::CaseSensitive))) {
            const QString& strPath = settings.value("UninstallString").toString();
            if(strDisplayName.contains("300battle", Qt::CaseSensitive)) {
                int nPos = strPath.lastIndexOf("\\");
                nPos = strPath.lastIndexOf("\\", nPos-1);
                ui->line_gamePath->setText(strPath.mid(1, nPos).append("300Hero").replace("\\", "/"));
                return;
            }
            else if(strDisplayName == u8"300英雄")  {
                int nPos = strPath.lastIndexOf("\\");
                ui->line_gamePath->setText(strPath.mid(1, nPos-1).replace("\\", "/"));
                return;
            }
        }
        settings.endGroup();
    }
    // 从特定位置搜索
    for (auto& my_info : QDir::drives()) {
        const QString& strPath = my_info.absoluteFilePath();
        if (QDir().exists(strPath + "JumpGame/300Hero")) {
            ui->line_gamePath->setText(strPath + "JumpGame/300Hero");
            return;
        }
        else if (QDir().exists(strPath + u8"Program Files (x86)/Jump/300英雄")) {
            ui->line_gamePath->setText(strPath + u8"Program Files (x86)/Jump/300英雄");
            return;
        }
    }
    ui->label_text->setText(u8"自动搜索失败！");
}

void Dialog_Set::onSetPath(int index)
{
    const char* strTitle[3]{ u8"请设置300英雄游戏路径", u8"请设置本地导出路径", u8"请设置单独导出路径" };
    QLineEdit* lineEdit = index == 0 ? ui->line_gamePath : index == 1 ? ui->line_exportPath : ui->line_alonePath;
    const QString& strPath = QFileDialog::getExistingDirectory(this, strTitle[index], lineEdit->text());
    if (!strPath.isEmpty()) lineEdit->setText(strPath);
}

void Dialog_Set::onSetLink(int index)
{
    QLabel* label = index == 0 ? ui->label_link_jmp : index == 1 ? ui->label_link_bank : ui->label_link_dat;
    QPushButton* button = index == 0 ? ui->Button_link_jmp : index == 1 ? ui->Button_link_bank : ui->Button_link_dat;
    const QString& strExt = index == 0 ? ".jmp" : index == 1 ? ".bank" : ".dat";
    if (button->text() == u8"关联") {
        const QString& strInfo = index == 0 ? u8"Jmp资源包" : index == 1 ? u8"Bank语音" : u8"Dat数据库";
        const QString& strIcon = QString("%1,%2").arg(QCoreApplication::applicationFilePath().replace('/', '\\')).arg(index + 1);
        SetApplicationAssociation("300ResourceBrowser" + strExt, strExt, strIcon, strInfo);
        label->setText(u8"已关联");
        button->setText(u8"取消");
    }
    else {
        RemoveApplicationAssociation("300ResourceBrowser" + strExt, strExt);
        label->setText(u8"未关联");
        button->setText(u8"关联");
    }
}

void Dialog_Set::onUpdate()
{
    CheckUpdate(this, ui->comboBox_update_source->currentIndex(), true);
}

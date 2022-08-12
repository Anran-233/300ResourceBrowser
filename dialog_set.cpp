#include "dialog_set.h"
#include "ui_dialog_set.h"

#include "windows.h"

#include <QFileDialog>
#include <QSettings>

Dialog_Set::Dialog_Set(QString strAppPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_Set)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    m_strAppPath = strAppPath;
    Init();
}

Dialog_Set::~Dialog_Set()
{
    delete ui;
}

// 初始化
void Dialog_Set::Init()
{
    ui->label_text->clear();

    wchar_t lcGamePath[520];
    wchar_t lcExportPath[520];
    wchar_t lcAlonePath[520];
    int nExportMode;
    int nPreviewMode;
    GetPrivateProfileString(L"配置", L"游戏路径", L"", lcGamePath, 520, L"BConfig/Config.ini");
    GetPrivateProfileString(L"配置", L"导出路径", L"BExport", lcExportPath, 520, L"BConfig/Config.ini");
    GetPrivateProfileString(L"配置", L"单独导出", L"BAlone", lcAlonePath, 520, L"BConfig/Config.ini");
    nExportMode = GetPrivateProfileInt(L"配置", L"导出模式", 1, L"BConfig/Config.ini");
    nPreviewMode = GetPrivateProfileInt(L"配置", L"预览图片", 1, L"BConfig/Config.ini");

    ui->line_gamePath->setText(QString::fromWCharArray(lcGamePath));
    ui->line_exportPath->setText(QString::fromWCharArray(lcExportPath));
    ui->line_alonePath->setText(QString::fromWCharArray(lcAlonePath));
    if (nExportMode == 1)
    {
        ui->Overlay_YES->setChecked(1);
        ui->Overlay_NO->setChecked(0);
    }
    else if (nExportMode == 0)
    {
        ui->Overlay_YES->setChecked(0);
        ui->Overlay_NO->setChecked(1);
    }
    if (nPreviewMode == 2) {
        ui->Preview_direct->setChecked(1);
        ui->Preview_YES->setChecked(0);
        ui->Preview_NO->setChecked(0);
    }
    else if (nPreviewMode == 1) {
        ui->Preview_direct->setChecked(0);
        ui->Preview_YES->setChecked(1);
        ui->Preview_NO->setChecked(0);
    }
    else if (nPreviewMode == 0) {
        ui->Preview_direct->setChecked(0);
        ui->Preview_YES->setChecked(0);
        ui->Preview_NO->setChecked(1);
    }

    // 确认文件关联状态
    ui->label_fileOpen->setText(QString::fromWCharArray(L"未关联"));
    ui->Button_fileOpen->setText(QString::fromWCharArray(L"设置关联"));
    QSettings settingRegClasses("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    if (settingRegClasses.value(".jmp/.", "errer").toString() == "300ResourceBrowser.open.jmp")
    {
        ui->label_fileOpen->setText(QString::fromWCharArray(L"已关联"));
        ui->Button_fileOpen->setText(QString::fromWCharArray(L"取消关联"));
        QSettings settingRegJmp("HKEY_CURRENT_USER\\Software\\Classes\\300ResourceBrowser.open.jmp\\Shell\\Open\\Command\\", QSettings::NativeFormat);
        QString strPath = settingRegJmp.value(".", "errer").toString();
        if (strPath != QString("\"" + m_strAppPath + "\" \"%1\""))
        {
            ui->label_fileOpen->setText(QString::fromWCharArray(L"已关联 ( 非本程序 )"));
            ui->Button_fileOpen->setText(QString::fromWCharArray(L"设置关联"));
        }
    }
}

// 点击【取消】按钮
void Dialog_Set::on_Button_OUT_clicked()
{
    this->close();
}

// 点击【确定】按钮
void Dialog_Set::on_Button_OK_clicked()
{
    QString strGamePath = ui->line_gamePath->text();
    QString strExportPath = ui->line_exportPath->text();
    QString strAlonePath = ui->line_alonePath->text();
    int nExportMode = 1;
    int nPreviewMode = 1;
    if      (ui->Overlay_YES->isChecked() == 1)     nExportMode = 1;
    else if (ui->Overlay_NO->isChecked() == 1)      nExportMode = 0;
    if      (ui->Preview_direct->isChecked() == 1)  nPreviewMode = 2;
    else if (ui->Preview_YES->isChecked() == 1)     nPreviewMode = 1;
    else if (ui->Preview_NO->isChecked() == 1)      nPreviewMode = 0;
    WritePrivateProfileString(L"配置", L"游戏路径", strGamePath.toStdWString().c_str(), L"BConfig/Config.ini");
    WritePrivateProfileString(L"配置", L"导出路径", strExportPath.toStdWString().c_str(), L"BConfig/Config.ini");
    WritePrivateProfileString(L"配置", L"单独导出", strAlonePath.toStdWString().c_str(), L"BConfig/Config.ini");
    WritePrivateProfileString(L"配置", L"导出模式", QString::number(nExportMode).toStdWString().c_str(), L"BConfig/Config.ini");
    WritePrivateProfileString(L"配置", L"预览图片", QString::number(nPreviewMode).toStdWString().c_str(), L"BConfig/Config.ini");

    emit SendUpdateSet();
    this->close();
}

// 点击【自动搜索】按钮
void Dialog_Set::on_Button_find_clicked()
{
    // 从注册表搜索
    QString strReg = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
    QSettings settings(strReg, QSettings::NativeFormat);
    QStringList strlRegGroups = settings.childGroups();
    foreach (QString strRegItem , strlRegGroups)
    {
        settings.beginGroup(strRegItem);
        QString strDisplayName = settings.value("DisplayName").toString();
        if((!strDisplayName.isEmpty())&&(strDisplayName.contains("300", Qt::CaseSensitive)))
        {
            QString strPath = settings.value("UninstallString").toString();
            if(strDisplayName.contains("300battle", Qt::CaseSensitive))
            {
                int nPos = strPath.lastIndexOf("\\");
                nPos = strPath.lastIndexOf("\\", nPos-1);
                ui->line_gamePath->setText(strPath.mid(1, nPos).append("300Hero").replace("\\", "/"));
                return;
            }
            else if(strDisplayName == L"300英雄")
            {
                int nPos = strPath.lastIndexOf("\\");
                ui->line_gamePath->setText(strPath.mid(1, nPos-1).replace("\\", "/"));
                return;
            }
        }
        settings.endGroup();
    }
    // 从特定位置搜索
    foreach (QFileInfo my_info, QDir::drives())
    {
        QString strPath(my_info.absoluteFilePath());
        QDir dirDir;
        if (dirDir.exists(QString(strPath).append("JumpGame/300Hero")))
        {
            ui->line_gamePath->setText(QString(strPath).append("JumpGame/300Hero"));
            return;
        }
        else if (dirDir.exists(QString(strPath).append(QString::fromWCharArray(L"Program Files (x86)/Jump/300英雄"))))
        {
            ui->line_gamePath->setText(QString(strPath).append(QString::fromWCharArray(L"Program Files (x86)/Jump/300英雄")));
            return;
        }
    }
    ui->label_text->setText(QString::fromWCharArray(L"自动搜索失败！"));
}

// 点击【选取游戏路径】按钮
void Dialog_Set::on_Button_gamePath_clicked()
{
    QString strGamePath = QFileDialog::getExistingDirectory(this, QString::fromWCharArray(L"请设置300英雄游戏路径"));
    if (!strGamePath.isEmpty()) ui->line_gamePath->setText(strGamePath);
}

// 点击【选取导出路径】按钮
void Dialog_Set::on_Button_exportPath_clicked()
{
    QString strExportPath = QFileDialog::getExistingDirectory(this, QString::fromWCharArray(L"请设置本地导出路径"));
    if (!strExportPath.isEmpty()) ui->line_exportPath->setText(strExportPath);
}

// 点击【选取单独导出路径】按钮
void Dialog_Set::on_Button_alone_clicked()
{
    QString strAlonePath = QFileDialog::getExistingDirectory(this, QString::fromWCharArray(L"请设置单独导出路径"));
    if (!strAlonePath.isEmpty()) ui->line_alonePath->setText(strAlonePath);
}

// 点击【关联文件类型】按钮
void Dialog_Set::on_Button_fileOpen_clicked()
{
    if (ui->Button_fileOpen->text() == QString::fromWCharArray(L"设置关联"))
    {
        QString strIcoPath = "\"" + QDir::currentPath().replace('/', '\\') + "\\BConfig\\ico_jmpFile.ico\"";
        QSettings settingRegClasses("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
        settingRegClasses.setValue("/300ResourceBrowser.open.jmp/Shell/Open/Command/.", "\"" + m_strAppPath + "\" \"%1\"");
        settingRegClasses.setValue("/300ResourceBrowser.open.jmp/.", QString::fromWCharArray(L"300英雄 资源数据包"));
        settingRegClasses.setValue("/300ResourceBrowser.open.jmp/DefaultIcon/.", QDir::currentPath().replace('/', '\\') + "\\BConfig\\ico_jmpFile.ico");
        settingRegClasses.setValue("/.jmp/.", "300ResourceBrowser.open.jmp");
        settingRegClasses.sync();
        ui->label_fileOpen->setText(QString::fromWCharArray(L"已关联"));
        ui->Button_fileOpen->setText(QString::fromWCharArray(L"取消关联"));
    }
    else
    {
        QSettings().remove("HKEY_CURRENT_USER\\Software\\Classes\\300ResourceBrowser.open.jmp");
        QSettings("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat).setValue("/.jmp/.", "");
        ui->label_fileOpen->setText(QString::fromWCharArray(L"未关联"));
        ui->Button_fileOpen->setText(QString::fromWCharArray(L"设置关联"));
    }
}

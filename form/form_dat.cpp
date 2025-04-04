#include "form_dat.h"
#include "ui_form_dat.h"
#include "tool.h"

#include <QProgressDialog>
#include <QTimer>
#include <QMessageBox>
#include <QClipboard>

// 读取dat数据(返回0为正常运行)(datData 原始数据, datDataLen 原始数据长度, datList 用来存放解析后的DAT数据表)
int ReadDatData(const char *datData, const int &datDataLen, DATLIST &datList)
{
    const int &&int_max{ 0x3fffffff };
    int &&index{ 0 };
    datList.clear();

    /* 是否读取到结尾 */ auto lam_isend = [&](const int &i) -> bool {
        if (i < datDataLen) return false;
        if (i != datDataLen) index = int_max;
        return true;
    };
    /* 获取数据类型 */ auto lam_rtype = [](const char &data_c) -> int {
        char &&type_c = data_c  &0x07;
        if (type_c == 0x00) return 1;
        if (type_c == 0x05) return 2;
        if (type_c == 0x02) return 3;
        return 0;
    };
    /* 读取列号 */ auto lam_rcolumn = [&]() -> long long {
        int &&n{ 0 };
        long long &&nColumn{ 0 };
        do {
            if (lam_isend(index + n)) return 0;
            if (n == 0) nColumn |= (long long)(datData[index] & 0x78) >> 3;
            else        nColumn |= (long long)(datData[index + n] & 0x7f) << (7 * n - 3);
            n += 1;
        } while ((datData[index + n - 1] & 0x80) != 0);
        index += n;
        return nColumn;
    };
    /* 读取vint数据 */ auto lam_rvint = [&]() -> long long {
        int &&n{ 0 };
        long long &&nValue{ 0 };
        do {
            if (lam_isend(index + n)) return 0;
            nValue |= (long long)(datData[index + n] & 0x7f) << (7 * n);
            n += 1;
        } while ((datData[index + n - 1] & 0x80) != 0);
        index += n;
        return nValue;
    };
    /* 读取float数据 */ auto lam_rfloat = [&]() -> float {
        if (lam_isend(index + 3)) return 0.0f;
        float &&f{ 0.0f };
        for (int &&i = 0; i < 4; i++, index++) *((char*)&f + i) = datData[index];
        return f;
    };
    /* 读取str数据 */ auto lam_rstr = [&]() -> QString {
        int &&nStrLen = (int)lam_rvint();
        const char *data = &datData[index];
        index += nStrLen;
        if (lam_isend(index - 1)) return QString("");
        return QString::fromLocal8Bit(data, nStrLen);
    };

    // 读取行索引表
    QVector<int> lineIndexList;
    while (!lam_isend(index))
    {
        if (datData[index++] != char(0x0a)) return -1;
        int nLineLen = (int)lam_rvint();
        lineIndexList.push_back(index);
        lineIndexList.push_back(index += nLineLen);
    }
    if (index == int_max) return -1;

    // 读取每行数据
    int &&nColumnMax{ 0 };
    datList.resize(lineIndexList.size() / 2);
    for (int &&nRowNum{ 0 }; nRowNum < datList.size(); nRowNum++)
    {
        index = lineIndexList[nRowNum * 2];
        int &indexEnd = lineIndexList[nRowNum * 2 + 1];
        while (index < indexEnd)
        {
            // 读取单个数据
            int &&type = lam_rtype(datData[index]);
            int nColumnNum = (int)lam_rcolumn();
            if (nColumnNum > datList[nRowNum].size())
            {
                datList[nRowNum].resize(nColumnNum);
                datList[nRowNum].back().type = type;
                if (type == 1)      datList[nRowNum].back().Int = lam_rvint();
                else if (type == 2) datList[nRowNum].back().Float = lam_rfloat();
                else if (type == 3) datList[nRowNum].back().Str = lam_rstr();
            }
            else index = int_max;
        }
        if (datList[nRowNum].size() > nColumnMax) nColumnMax = datList[nRowNum].size();
        if (lam_isend(index)) break;
    }
    if (index == int_max) datList.clear();
    else for (auto &datLine : datList) datLine.resize(nColumnMax);
    return index == int_max ? -1 : 0;
}

Form_dat::Form_dat(QWidget *parent) :
    ui(new Ui::Form_dat)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->tableWidget->clear();
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setColumnCount(0);
    ui->label->setText(u8"正在解析数据...");
    ui->button_void->setVisible(false);
    ui->button_lookup->setVisible(false);
    ui->lineEdit->setVisible(false);
    ui->lineEdit->setText("");
    connect(ui->button_void, &QPushButton::clicked, this, [this]{
        bool bHidden = ui->button_void->text() == u8"隐藏空列";
        for (int &&column{ 0 }, &&nColumnSize{ ui->tableWidget->columnCount() }; column < nColumnSize; ++column) {
            if (ui->tableWidget->horizontalHeaderItem(column)->text().right(2) == u8"空值")
                ui->tableWidget->setColumnHidden(column, bHidden);
        }
        ui->button_void->setText(bHidden ? u8"显示空列" : u8"隐藏空列");
    });
    connect(ui->button_lookup, &QPushButton::clicked, this, [this] {
        const QString &strText = ui->lineEdit->text();
        if (strText.trimmed().isEmpty()) return;
        ui->lineEdit->setEnabled(false);
        ui->button_lookup->setEnabled(false);
        int column = (std::max)(-1, ui->tableWidget->currentColumn()) + 1;
        int row = (std::max)(0, ui->tableWidget->currentRow());
        emit sigLookup(row, column);
    });
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, [this]{ ui->button_lookup->clicked(); });
    connect(ui->tableWidget, &QTableWidget::itemDoubleClicked, this, [](QTableWidgetItem *item){ QApplication::clipboard()->setText(item->text()); });
    connect(this, &Form_dat::sigTabelHead, this, &Form_dat::onTableHead, Qt::QueuedConnection);
    connect(this, &Form_dat::sigRender, this, &Form_dat::onRender, Qt::QueuedConnection);
    connect(this, &Form_dat::sigLookup, this, &Form_dat::onLookup, Qt::QueuedConnection);
    if (parent) connect(parent, &QObject::destroyed, this, [=]{ delete this; });
}

Form_dat::~Form_dat()
{
    delete ui;
}

void Form_dat::load(QWidget *parent, const SDataInfo &sDataInfo)
{
    // 创建进度条
    QProgressDialog *progressBar = new QProgressDialog(u8"正在加载...", nullptr, 0, 0, parent, Qt::Dialog | Qt::FramelessWindowHint);
    progressBar->installEventFilter(CEventFilter::Instance());
    progressBar->setWindowModality(Qt::WindowModal);
    progressBar->setMinimumDuration(0);
    progressBar->show();

    // 创建线程
    CThread *thread = new CThread(this);
    connect(thread, &CThread::started, thread, [=]{
        // 读取数据
        const QByteArray &data = CPatch::ReadData(sDataInfo);
        // 解析dat
        if (ReadDatData(data.data(), data.size(), m_data))  emit thread->finished(QString(u8"数据解析失败"));
        else if (m_data.size() == 0)                        emit thread->finished(QString(u8"数据为空"));
        else if (m_data[0].size() == 0)                     emit thread->finished(QString(u8"数据为空"));
        else emit thread->finished(QString());
    });
    connect(thread, &CThread::finished, this, [=](QVariant result){
        ActionScope([=]{ delete progressBar; delete thread; });
        // 是否失败
        const QString &error = result.toString();
        if (error.size()) {
            this->setWindowTitle(sDataInfo.patchPath.split('\\').back());
            ui->label->setText(error);
            return;
        }
        // 成功解析dat
        ui->label->setText(u8"正在渲染数据...");
        this->setWindowTitle(QString(u8"%1 (行%2 列%3 总计%4)")
                             .arg(sDataInfo.patchPath.split('\\').back())
                             .arg(m_data.size()).arg(m_data[0].size()).arg(m_data.size() * m_data[0].size()));
        // 显示界面
        show();
        // 渲染数据
        emit sigTabelHead();
    });
    thread->start();
}

void Form_dat::onTableHead()
{
    QFont font(u8"微软雅黑");
    font.setPixelSize(12);
    QFontMetrics fm(font);
    const int &nRowSize = m_data.size(), &nColumnSize = m_data[0].size();
    int nColumnWidthMax = 500;
    QStringList columnNames;                // 列名
    QVector<int> columnWidths(nColumnSize); // 列宽
    for (int &&column{ 0 }; column < nColumnSize; ++column) {
        int type = 0;   // 列类型
        for (int &&row{ 0 }; row < nRowSize; ++row) {
            if (m_data[row][column].type > type) type = m_data[row][column].type;
            if (m_data[row][column].type == 3) break;
        }
        if (type == 0)      columnNames << QString::number(column + 1) + u8":空值";
        else if (type == 1) columnNames << QString::number(column + 1) + u8":整数";
        else if (type == 2) columnNames << QString::number(column + 1) + u8":小数";
        else if (type == 3) columnNames << QString::number(column + 1) + u8":字符";
        columnWidths[column] = fm.horizontalAdvance(columnNames.back());   // 设置最小列宽
        if (type != 0) for (int &&row{ 0 }; row < nRowSize; ++row) {
            if (m_data[row][column].type == 0) continue;
            else if (m_data[row][column].type == 1) {
                if(m_data[row][column].Int <= 99999  &&m_data[row][column].Int > -9999) continue;
                int &&nWidth = QString::number(m_data[row][column].Int).size() * 7; // 7px是字体(微软雅黑, 12px)中数字字符的固定宽度
                if (nWidth > columnWidths[column]) columnWidths[column] = nWidth;
            }
            else if (m_data[row][column].type == 2) {
                int &&nWidth = QString::number(m_data[row][column].Float).size() * 7; // 7px是字体(微软雅黑, 12px)中数字字符的固定宽度
                if (nWidth > columnWidths[column]) columnWidths[column] = nWidth;
            }
            else {
                int &&nWidth = fm.horizontalAdvance(m_data[row][column].Str);
                if (nWidth > columnWidths[column]) columnWidths[column] = nWidth;
            }
            if (columnWidths[column] > nColumnWidthMax) {
                columnWidths[column] = nColumnWidthMax;
                break;
            }
        }
        columnWidths[column] = (std::min)(columnWidths[column] + 10, nColumnWidthMax);   // 加上内填充宽度
    }
    ui->tableWidget->setColumnCount(nColumnSize);
    ui->tableWidget->setHorizontalHeaderLabels(columnNames);
    for (int &&column{ 0 }; column < nColumnSize; ++column) ui->tableWidget->setColumnWidth(column, columnWidths[column]);
    emit sigRender();
}

void Form_dat::onRender()
{
    const int &nRowSize = m_data.size();
    const int &nColumnSize = m_data[0].size();
    int row = ui->tableWidget->rowCount();
    const int &rowend = qMin(nRowSize, row + 128);
    while (row < rowend) {
        ui->tableWidget->setRowCount(row + 1);
        for (int &&column{ 0 }; column < nColumnSize; ++column) {
            if (m_data[row][column].type == 1)      m_data[row][column].Str = QString::number(m_data[row][column].Int);
            else if (m_data[row][column].type == 2) m_data[row][column].Str = QString::number(m_data[row][column].Float);
            ui->tableWidget->setItem(row, column, new QTableWidgetItem(m_data[row][column].Str));
        }
        row++;
    }
    if (row < nRowSize) {
        ui->label->setText(QString(u8"正在加载数据：%1 / %2 行").arg(row).arg(nRowSize));
        emit sigRender();   // 继续渲染
    }
    else {  // 完成渲染
        ui->label->setVisible(false);
        ui->button_void->setVisible(true);
        ui->button_lookup->setVisible(true);
        ui->lineEdit->setVisible(true);
    }
}

void Form_dat::onLookup(int row, int column)
{
    const QString &strText = ui->lineEdit->text();
    const int &nColumnSize = ui->tableWidget->columnCount();
    const int &nRowSize = ui->tableWidget->rowCount();
    const int &rowend = qMin(nRowSize, row + 256);
    while (row < rowend) {
        for (; column < nColumnSize; ++column){
            if (m_data[row][column].Str.contains(strText, Qt::CaseInsensitive)) {
                ui->tableWidget->setCurrentItem(ui->tableWidget->item(row, column), QItemSelectionModel::ClearAndSelect);
                ui->lineEdit->setEnabled(true);
                ui->button_lookup->setEnabled(true);
                return;
            }
        }
        column = 0;
    }
    if (row < nRowSize) emit sigLookup(row, column); // 继续查找
    else {  // 查找到末尾
        ui->lineEdit->setEnabled(true);
        ui->button_lookup->setEnabled(true);
        QMessageBox::about(this, u8"查找", u8"已经搜索到末尾！");
    }
}

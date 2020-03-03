#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qftp.h"
#include <QTextCodec>
#include <QFile>
#include <QTreeWidgetItem>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //初次打开软件默认返回上一级按钮不可用
    ui->cdToParentButton->setEnabled(false);

    ui->progressBar->setValue(0);

    //如果我们鼠标双击了这个目录，则会发出这个信号
    connect(ui->fileList, &QTreeWidget::itemActivated,
            this, &MainWindow::processItem);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//命令开始
void MainWindow::ftpCommandStarted(int)
{
    int id = ftp->currentCommand();
    switch (id)
    {
    case QFtp::ConnectToHost :
        ui->label->setText(tr("正在连接到服务器…"));
        break;
    case QFtp::Login :
        ui->label->setText(tr("正在登录…"));
        break;
    case QFtp::Get :
        ui->label->setText(tr("正在下载…"));
        break;
    case QFtp::Close :
        ui->label->setText(tr("正在关闭连接…"));
    }
}

//跟进执行不同的命令，来判断执行结果，如果执行出错会返回一个bool值
//跟进bool判断需要显示的结果，errorString如果错误，则记录错误信息
void MainWindow::ftpCommandFinished(int, bool error)
{
    if(ftp->currentCommand() == QFtp::ConnectToHost) {
        if (error)
            ui->label->setText(tr("连接服务器出现错误：%1").arg(ftp->errorString()));
        else ui->label->setText(tr("连接到服务器成功"));
    } else if (ftp->currentCommand() == QFtp::Login) {
        if (error)
            ui->label->setText(tr("登录出现错误：%1").arg(ftp->errorString()));
        else {
            ui->label->setText(tr("登录成功"));
            ftp->list();
        }
    } else if (ftp->currentCommand() == QFtp::Get) {
        if(error)
            ui->label->setText(tr("下载出现错误：%1").arg(ftp->errorString()));
        else {
            ui->label->setText(tr("已经完成下载"));
            file->close();
        }
        ui->downloadButton->setEnabled(true);
    } else if (ftp->currentCommand() == QFtp::List) {
        if (isDirectory.isEmpty())
        {
            ui->fileList->addTopLevelItem(
                        new QTreeWidgetItem(QStringList()<< tr("<empty>")));
            ui->fileList->setEnabled(false);
            ui->label->setText(tr("该目录为空"));
        }
    } else if (ftp->currentCommand() == QFtp::Close) {
        ui->label->setText(tr("已经关闭连接"));
    }
}

// 连接按钮
void MainWindow::on_connectButton_clicked()
{
    //清理工作
    ui->fileList->clear();
    currentPath.clear();
    isDirectory.clear();
    ui->progressBar->setValue(0);
    //创建FTP对象
    ftp = new QFtp(this);
    //连接信号和槽：当指令 开始 的时候，刷新指令的显示状态
    connect(ftp, &QFtp::commandStarted, this,
            &MainWindow::ftpCommandStarted);
    //连接信号和槽：当指令 完成 的时候，刷新指令的显示状态
    connect(ftp, &QFtp::commandFinished,
            this, &MainWindow::ftpCommandFinished);
    //连接信号和槽：读取到列表信息的时候，通知保存列表信息
    connect(ftp, &QFtp::listInfo, this, &MainWindow::addToList);
    //连接信号和槽：当有下载进度的时候，更新进度条显示
    connect(ftp, &QFtp::dataTransferProgress,
            this, &MainWindow::updateDataTransferProgress);

    //获取用户输入的FTP地址，用户名，密码
    QString ftpServer = ui->ftpServerLineEdit->text();
    QString userName = ui->userNameLineEdit->text();
    QString passWord = ui->passWordLineEdit->text();
    //连接到ftp服务器，指定地址，端口（ftp端口默认是21所以可以写死）
    ftp->connectToHost(ftpServer, 21);
    //使用密码登录ftp服务器
    ftp->login(userName, passWord);
}

//获取列表信息
void MainWindow::addToList(const QUrlInfo &urlInfo)
{
    // 注意：因为服务器上文件使用UTF-8编码，所以要进行编码转换，这样显示中文才不会乱码
    //编码转换
    QString name = QString::fromUtf8(urlInfo.name().toLatin1());
    QString owner = QString::fromUtf8(urlInfo.owner().toLatin1());
    QString group = QString::fromUtf8(urlInfo.group().toLatin1());

    //定义一个树形部件，创建一个树形条目，下面是为部件设置“字段”（类似一个表格）
    QTreeWidgetItem *item = new QTreeWidgetItem;
    //第1 字段是：文件名称
    item->setText(0, name);
    //第2 字段是：文件大小
    item->setText(1, QString::number(urlInfo.size()));
    //第3 字段是：拥有者
    item->setText(2, owner);
    //第4 字段是：用户组
    item->setText(3, group);
    //第5 字段是：最后修改日期
    item->setText(4, urlInfo.lastModified().toString("yyyy-MM-dd"));

    //设置文件图标：判断当前info，如果是目录，则文件名称前面显示目录的图片，
    //如果是文件则文件名前显示文件的图片
    QPixmap pixmap(urlInfo.isDir() ? "../myFTP/dir.png" : "../myFTP/file.png");
    //把上面的图标设置为图标
    item->setIcon(0, pixmap);

    //把是不是目录的信息，保存到哈希表中isDirectory中
    //（QHash<QString, bool> isDirectory;）是一个键值对
    //为了是后面用户点击了这个文件，需要根据是目录还是文件做相应的操作
    isDirectory[name] = urlInfo.isDir();

    //把上面当前这个条目的内容，添加到属性目录中（当前是一条一条获取的，也是一条一条添加的）
    ui->fileList->addTopLevelItem(item);
    if (!ui->fileList->currentItem()) {
        ui->fileList->setCurrentItem(ui->fileList->topLevelItem(0));
        ui->fileList->setEnabled(true);
    }
}

//双击后对当条目进行处理，如果是目录则进入内部，如果是文件，则不处理
void MainWindow::processItem(QTreeWidgetItem *item, int)
{

    qDebug()<<"***item激活提示*****";   //验证是单击进入，还是双击进入
    // 如果这个文件是个目录，则打开
    if (isDirectory.value(item->text(0))) {
        // 注意：因为目录名称可能是中文，在使用ftp命令cd()前需要先进行编码转换
        QString name = QLatin1String(item->text(0).toUtf8());
        ui->fileList->clear();
        isDirectory.clear();
        //记录生成当前的最新目录
        currentPath += "/";
        currentPath += name;
        //进入新的文件夹
        ftp->cd(name);
        qDebug()<<"********当前进入的路径："<<currentPath;
        //获取内容列表
        ftp->list();
        //设置返回上级按钮为可用
        ui->cdToParentButton->setEnabled(true);
    }
}

// 返回上级目录按钮
void MainWindow::on_cdToParentButton_clicked()
{
    if(!ui->cdToParentButton->isEnabled())
    {
        qDebug()<<"**********返回上一级文件夹不可用***********";
        return;
    }
    //清理内容
    ui->fileList->clear();
    isDirectory.clear();
    //按斜线分割，找到上一级目录
    currentPath = currentPath.left(currentPath.lastIndexOf('/'));
    qDebug()<<"********当前返回的路径："<<currentPath;
    //如果上一级目录为空，则进入根目录，否则切换到最新目录
    if (currentPath.isEmpty()) {
        ui->cdToParentButton->setEnabled(false);
        ftp->cd("/");
    } else {
        ftp->cd(currentPath);
    }
    ftp->list();
}

// 下载按钮
void MainWindow::on_downloadButton_clicked()
{
    // 注意：因为文件名称可能是中文，所以在使用get()函数前需要进行编码转换
    //获取选择当前选中的文件的名称，并针对中文进行转码
    QString fileName = ui->fileList->currentItem()->text(0);
    QString name = QLatin1String(fileName.toUtf8());

    if (isDirectory.value(name)) {
        qDebug()<<"********下载的是目录："<<name;
        ui->label->setText("下载出现错误：不支持下载目录!");
        return;
    }
    qDebug()<<"********下载的是文件："<<name;

    //创建一个文件，关联文件名
    file = new QFile(fileName);
    if (!file->open(QIODevice::WriteOnly)) {
        delete file;
        return;
    }
    //开始下载，设置下载按钮不可用
    ui->downloadButton->setEnabled(false);
    //下载文件
    ftp->get(name, file);
}

//更新进度条
void MainWindow::updateDataTransferProgress(qint64 readBytes,qint64 totalBytes)
{
    ui->progressBar->setMaximum(totalBytes);
    ui->progressBar->setValue(readBytes);
}

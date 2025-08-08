#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QProcess>
#include <QSettings>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <functional>
#include <thread>
#include <chrono>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_model(new CharModel)
    , m_newCharData()
    , m_pidList()
{
    ui->setupUi(this);
    setWindowFlags(Qt::Widget);

    ui->charsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->charsTableView->verticalHeader()->hide();
    ui->charsTableView->setModel(m_model);

    setupConnections();
    readSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupConnections()
{
    connect(ui->elementClientButton, &QPushButton::clicked, this, &MainWindow::openFileDialog);
    connect(ui->accountLineEdit, &QLineEdit::editingFinished, this, &MainWindow::updateCharAccount);
    connect(ui->passwordLineEdit, &QLineEdit::editingFinished, this, &MainWindow::updateCharPassword);
    connect(ui->charLineEdit, &QLineEdit::editingFinished, this, &MainWindow::updateCharName);
    connect(ui->addCharButton, &QPushButton::clicked, this, &MainWindow::addChar);
    connect(ui->clearFormButton, &QPushButton::clicked, this, &MainWindow::clearForm);
    connect(ui->logCharButton, &QPushButton::clicked, this, &MainWindow::logSelectedChar);
    connect(ui->deleteCharButton, &QPushButton::clicked, this, &MainWindow::deleteChar);
    connect(ui->logAllCharsButton, &QPushButton::clicked, this, &MainWindow::logAllChars);
    connect(ui->actionBug, &QAction::triggered, this, &MainWindow::reportBug);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::readSettings()
{
    QSettings settings;

    ui->unfreezeCheckbox->setChecked(settings.value("unfreeze", false).toBool());
    ui->elementClientLineEdit->setText(settings.value("elementClientPath", "").toString());

    int charCount = settings.value("charCount", 0).toInt();
    for(int i = 0; i < charCount; ++i)
    {
        QStringList charInfo = settings.value(QString("char%1").arg(i), QStringList()).toStringList();
        CharData charData;
        charData.setAccount(charInfo.at(0));
        charData.setPassword(charInfo.at(1));
        charData.setCharName(charInfo.at(2));
        m_model->pushCharData(charData);
    }
}

void MainWindow::writeSettings()
{
    QSettings settings;

    settings.setValue("unfreeze", ui->unfreezeCheckbox->isChecked());
    settings.setValue("elementClientPath", ui->elementClientLineEdit->text());

    int charCount = m_model->rowCount();
    settings.setValue("charCount", charCount);
    for(int i = 0; i < charCount; ++i)
    {
        settings.setValue(QString("char%1").arg(i), m_model->getCharData(i).toList());
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

bool MainWindow::isElementClientPathSet() {
    return !ui->elementClientLineEdit->text().isEmpty();
}

void MainWindow::launchClient(const CharData& charData) {
    QStringList parameters{
        "startbypatcher",
        QString("user:%1").arg(charData.getAccount()),
        QString("pwd:%1").arg(charData.getPassword()),
        QString("role:%1").arg(charData.getCharName()),
        ui->unfreezeCheckbox->isChecked() ? QString("rendernofocus") : ""
    };

    qint64 pid;
    QProcess* newProc = new QProcess();
    QString execPath = ui->elementClientLineEdit->text();
    QFileInfo fileInfo(execPath);
    newProc->startDetached(execPath, parameters, fileInfo.absolutePath(), &pid);
    m_pidList.push_back(pid);
}

// ------------ SLOTS ------------
void MainWindow::openFileDialog()
{
    QString elementClientPath = QFileDialog::getOpenFileName(this,
                                                             tr("Cari ElementClient"), "C:/",
                                                             tr("ElementClient (ElementClient.exe)"));
    ui->elementClientLineEdit->setText(elementClientPath);
}

void MainWindow::updateCharAccount()
{
    m_newCharData.setAccount(ui->accountLineEdit->text());
}

void MainWindow::updateCharPassword()
{
    m_newCharData.setPassword(ui->passwordLineEdit->text());
}

void MainWindow::updateCharName()
{
    m_newCharData.setCharName(ui->charLineEdit->text());
}

void MainWindow::addChar()
{
    if(m_newCharData.getAccount() == "" || m_newCharData.getPassword() == "")
    {
        ui->statusbar->showMessage("ERROR: Akun/Sandi kosong", 2000);
        return;
    }
    else if(m_newCharData.getCharName() == "")
    {
        ui->statusbar->showMessage("PERINGATAN: Nama Char Kosong (opsional)", 2000);
    }

    m_model->pushCharData(m_newCharData);
    m_newCharData = CharData();
    ui->accountLineEdit->clear();
    ui->passwordLineEdit->clear();
    ui->charLineEdit->clear();

    ui->statusbar->showMessage("Akun ditambahkan!", 2000);
}

void MainWindow::clearForm()
{
    m_newCharData = CharData();
    ui->accountLineEdit->clear();
    ui->passwordLineEdit->clear();
    ui->charLineEdit->clear();
}

void MainWindow::logSelectedChar()
{
    if(!isElementClientPathSet())
    {
        ui->statusbar->showMessage("Path ELEMENTCLIENT tidak disediakan!", 2000);
        return;
    }

    QItemSelectionModel *select = ui->charsTableView->selectionModel();
    if(select->hasSelection())
    {
        QModelIndex selectedIndex = select->selectedRows().first();
        const CharData &data = m_model->getCharData(selectedIndex.row());

        launchClient(data);
    }
}

void MainWindow::deleteChar()
{
    QItemSelectionModel *select = ui->charsTableView->selectionModel();
    if(select->hasSelection())
    {
        QModelIndex selectedIndex = select->selectedRows().first();
        m_model->deleteCharData(selectedIndex.row());
    }
}

void MainWindow::logAllChars()
{
    if(!isElementClientPathSet())
    {
        ui->statusbar->showMessage("Path ELEMENTCLIENT tidak disediakan!", 2000);
        return;
    }

    int charCount = m_model->rowCount();
    for(int i = 0; i < charCount; ++i)
    {
        const CharData &data = m_model->getCharData(i);

        // Bind the launchClient function with the current char's data
        auto threadFunc = std::bind(&MainWindow::launchClient, this, data);

        std::thread launchClientThread([&threadFunc]() {
            threadFunc();
            // Add a delay after launching the client to avoid system hiccups
            std::this_thread::sleep_for(std::chrono::milliseconds(1300));
        });

        // Wait the thread to finish
        launchClientThread.join();
    }
}

void MainWindow::reportBug()
{
    QDesktopServices::openUrl(QUrl("https://github.com/hrace009/PWCI-Auto-Login/issues"));
}

void MainWindow::showAbout()
{
    QMessageBox::information(
                this,
                "Tentang",
                "Kode Sumber Tersedia di https://github.com/hrace009/PWCI-Auto-Login"
                "\n\nLisensi LGPL 3.0");
}

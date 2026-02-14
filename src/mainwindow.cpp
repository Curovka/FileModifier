#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QDateTime>
#include <QStandardPaths>
#include <QRegularExpressionValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_scanTimer(new QTimer(this))
    , m_workerThread(new QThread(this))
    , m_processor(new FileProcessor(this))
    , m_processing(false)
{
    ui->setupUi(this);
    setWindowTitle("File Modifier");

    // Перенос обработчика в отдельный поток
    m_processor->moveToThread(m_workerThread);
    m_workerThread->start();

    setupConnections();
    loadSettings();
    updateControls();

    ui->progressBar->setValue(0);
    ui->logText->setReadOnly(true);

    // Валидатор XOR ключа
    QRegularExpression hexRegex("^[0-9A-Fa-f]{16}$");
    ui->xorKeyEdit->setValidator(new QRegularExpressionValidator(hexRegex, this));

    if (ui->fileMaskEdit->text().isEmpty()) { ui->fileMaskEdit->setText("*.txt,*.bin,*.dat"); }
    if (ui->xorKeyEdit->text().isEmpty()) { ui->xorKeyEdit->setText("0123456789ABCDEF"); }
}

MainWindow::~MainWindow()
{
    saveSettings();
    m_workerThread->quit();
    m_workerThread->wait();

    delete ui;
}

void MainWindow::setupConnections() {
    // кнопки Обзор (для исходной и целевой папки), Старт/Стоп, Обработать сейчас
    connect(ui->sourceBrowseBtn, &QPushButton::clicked, this, &MainWindow::onSourceBrowseClicked);
    connect(ui->targetBrowseBtn, &QPushButton::clicked, this, &MainWindow::onTargetBrowseClicked);
    connect(ui->startStopBtn, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(ui->processNowBtn, &QPushButton::clicked, this, &MainWindow::onProcessNowClicked);

    // для единого формата hex кода 
    // "0123456789ABCDEF" -> "0123456789ABCDEF"
    // "0123456789abcdef" -> "0123456789ABCDEF"
    connect(ui->xorKeyEdit, &QLineEdit::textChanged, this, &MainWindow::onXorKeyChanged);

    // логи, прогресс выполнения
    connect(m_processor, &FileProcessor::processingStarted, this, &MainWindow::onProcessingStarted);
    connect(m_processor, &FileProcessor::progressChanged, this, &MainWindow::onProcessingProgress);
    connect(m_processor, &FileProcessor::processingFinished, this, &MainWindow::onProcessingFinished);
    connect(m_processor, &FileProcessor::fileFound, this, &MainWindow::onFileFound);
    connect(m_processor, &FileProcessor::errorOccurred, this, &MainWindow::onErrorOccurred);

    // Таймер
    connect(m_scanTimer, &QTimer::timeout, this, &MainWindow::checkForNewFiles);

    // Сигнал запуска обработки → слот FileProcessor
    connect(this, &MainWindow::startProcessing, m_processor, &FileProcessor::processFiles);
}

void MainWindow::loadSettings() {
    QSettings settings("FileModifier", "Settings");

    ui->fileMaskEdit->setText(settings.value("FileMasks", "*.txt,*.bin").toString());
    ui->sourcePathEdit->setText(settings.value("SourcePath", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString());
    ui->targetPathEdit->setText(settings.value("TargetPath", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString());
    ui->deleteSourceCheck->setChecked(settings.value("DeleteSource", false).toBool());
    ui->conflictCombo->setCurrentIndex(settings.value("ConflictAction", 0).toInt());
    ui->modeCombo->setCurrentIndex(settings.value("ProcessingMode", 0).toInt());
    ui->intervalSpin->setValue(settings.value("ScanInterval", 5).toInt());
    ui->xorKeyEdit->setText(settings.value("XorKey", "0123456789ABCDEF").toString());
    ui->recursiveCheck->setChecked(settings.value("Recursive", true).toBool());
}

void MainWindow::saveSettings() {
    QSettings settings("FileModifier", "Settings");

    settings.setValue("FileMasks", ui->fileMaskEdit->text());
    settings.setValue("SourcePath", ui->sourcePathEdit->text());
    settings.setValue("TargetPath", ui->targetPathEdit->text());
    settings.setValue("DeleteSource", ui->deleteSourceCheck->isChecked());
    settings.setValue("ConflictAction", ui->conflictCombo->currentIndex());
    settings.setValue("ProcessingMode", ui->modeCombo->currentIndex());
    settings.setValue("ScanInterval", ui->intervalSpin->value());
    settings.setValue("XorKey", ui->xorKeyEdit->text());
    settings.setValue("Recursive", ui->recursiveCheck->isChecked());
}

void MainWindow::updateControls() {
    bool enabled = !m_processing;

    ui->fileMaskEdit->setEnabled(enabled);
    ui->sourcePathEdit->setEnabled(enabled);
    ui->targetPathEdit->setEnabled(enabled);
    ui->sourceBrowseBtn->setEnabled(enabled);
    ui->targetBrowseBtn->setEnabled(enabled);
    ui->deleteSourceCheck->setEnabled(enabled);
    ui->conflictCombo->setEnabled(enabled);
    ui->modeCombo->setEnabled(enabled);
    ui->intervalSpin->setEnabled(enabled && ui->modeCombo->currentIndex() == 1);
    ui->xorKeyEdit->setEnabled(enabled);
    ui->recursiveCheck->setEnabled(enabled);
    ui->processNowBtn->setEnabled(enabled);

    ui->startStopBtn->setText(m_processing ? "Стоп" : "Старт");
    ui->startStopBtn->setStyleSheet(m_processing ? "background-color: #ff4757;" : "background-color: #2ed573;");
}

void MainWindow::onSourceBrowseClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите исходную папку", ui->sourcePathEdit->text());
    if (!dir.isEmpty()) { ui->sourcePathEdit->setText(dir); }
}

void MainWindow::onTargetBrowseClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите целевую папку", ui->targetPathEdit->text());
    if (!dir.isEmpty()) { ui->targetPathEdit->setText(dir); }
}

void MainWindow::onStartStopClicked() {
    if (m_processing) {
        // Остановка
        stopWatching();
        m_processor->stopProcessing();
        m_processing = false;
        ui->logText->appendPlainText(QDateTime::currentDateTime().toString() + " - Обработка остановлена");
        
    } else {
        // Проверка настроек перед стартом
        if (ui->xorKeyEdit->text().length() != 16) {
            QMessageBox::warning(this, "Ошибка", "XOR ключ должен содержать ровно 16 hex-символов (8 байт)");
            return;
        }

        if (!QDir(ui->sourcePathEdit->text()).exists()) {
            QMessageBox::warning(this, "Ошибка", "Исходная папка не существует");
            return;
        }

        if (!QDir(ui->targetPathEdit->text()).exists()) { QDir().mkpath(ui->targetPathEdit->text()); }

        // Формирование опций
        FileProcessor::ProcessingOptions options;
        options.fileMasks = ui->fileMaskEdit->text().split(',', Qt::SkipEmptyParts);
        options.sourcePath = ui->sourcePathEdit->text();
        options.targetPath = ui->targetPathEdit->text();
        options.deleteSource = ui->deleteSourceCheck->isChecked();
        options.conflictAction = static_cast<FileProcessor::ConflictResolution>(ui->conflictCombo->currentIndex());
        options.xorKey = QByteArray::fromHex(ui->xorKeyEdit->text().toUtf8());
        options.recursive = ui->recursiveCheck->isChecked();

        m_processor->setOptions(options);
        m_processing = true;

        if (ui->modeCombo->currentIndex() == 0) {
            // Разовый запуск
            ui->logText->appendPlainText(QDateTime::currentDateTime().toString() + " - Запуск разовой обработки...");
            emit startProcessing();

        } else {
            // Режим наблюдения
            startWatching();
            ui->logText->appendPlainText(QDateTime::currentDateTime().toString() + " - Запущено наблюдение за файлами...");
        }
    }

    updateControls();
}

void MainWindow::onProcessNowClicked() {
    if (!m_processing) { onStartStopClicked(); }
}

void MainWindow::onXorKeyChanged() {
    QString text = ui->xorKeyEdit->text().toUpper();
    ui->xorKeyEdit->setText(text);
}

void MainWindow::startWatching() {
    stopWatching();
    if (ui->modeCombo->currentIndex() == 1) { m_scanTimer->start(ui->intervalSpin->value() * 1000); }
}

void MainWindow::stopWatching() {
    m_scanTimer->stop();
}

void MainWindow::checkForNewFiles() {
    if (m_processing && !m_processor->isProcessing()) {
        ui->logText->appendPlainText(QDateTime::currentDateTime().toString() + " - Проверка новых файлов...");
        emit startProcessing();
    }
}

void MainWindow::onProcessingStarted() {
    ui->progressBar->setValue(0);
    ui->statusLabel->setText("Обработка файлов...");
}

void MainWindow::onProcessingProgress(int percent) {
    ui->progressBar->setValue(percent);
    ui->statusLabel->setText(QString("Прогресс: %1%").arg(percent));
}

void MainWindow::onProcessingFinished(const QString &result) {
    ui->progressBar->setValue(100);
    ui->statusLabel->setText("Готово");
    ui->logText->appendPlainText(QDateTime::currentDateTime().toString() + " - " + result);

    QTimer::singleShot(1500, this, [this]() { ui->progressBar->setValue(0); });
    
    if (ui->modeCombo->currentIndex() == 0) {
        m_processing = false;
        updateControls();
    }
}

void MainWindow::onFileFound(const QString &file) {
    ui->logText->appendPlainText(QDateTime::currentDateTime().toString() + " - Найден файл: " + QFileInfo(file).fileName());
}

void MainWindow::onErrorOccurred(const QString &error) {
    ui->logText->appendPlainText(QDateTime::currentDateTime().toString() + " - ОШИБКА: " + error);
    QMessageBox::warning(this, "Ошибка обработки", error);
}
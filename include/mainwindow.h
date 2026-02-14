#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QThread>
#include "fileprocessor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void startProcessing();   // Сигнал для запуска FileProcessor в рабочем потоке

private slots:
    // Слоты UI
    void onSourceBrowseClicked();
    void onTargetBrowseClicked();
    void onStartStopClicked();
    void onProcessNowClicked();
    void onXorKeyChanged();

    // Слоты обработки сигналов FileProcessor
    void onProcessingStarted();
    void onProcessingProgress(int percent);
    void onProcessingFinished(const QString &result);
    void onFileFound(const QString &file);
    void onErrorOccurred(const QString &error);

    // Таймер для режима наблюдения
    void checkForNewFiles();

private:
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void updateControls();
    void startWatching();
    void stopWatching();

    Ui::MainWindow *ui;
    QTimer *m_scanTimer;            // Таймер для периодической проверки
    QThread *m_workerThread;
    FileProcessor *m_processor;
    bool m_processing;             // Флаг активности
};

#endif // MAINWINDOW_H
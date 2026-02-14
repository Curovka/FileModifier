#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <QObject>
#include <QFile>
#include <QDir>
#include <atomic>

class FileProcessor : public QObject {
    Q_OBJECT

public:
    enum ConflictResolution {
        Overwrite,
        Rename
    };

    struct ProcessingOptions {
        QStringList fileMasks;
        QString sourcePath;
        QString targetPath;
        bool deleteSource = false;
        ConflictResolution conflictAction = Rename;
        QByteArray xorKey;
        bool recursive = true;

        ProcessingOptions() = default;
    };

    explicit FileProcessor(QObject *parent = nullptr);
    ~FileProcessor() = default;
    
    void setOptions(const ProcessingOptions &options);
    bool isProcessing() const noexcept { return m_isProcessing; }

public slots:
    void processFiles();
    void stopProcessing();

signals:
    void processingStarted();
    void progressChanged(int percent);          // 0..100
    void processingFinished(const QString &result);
    void fileFound(const QString &file);
    void errorOccurred(const QString &error);

private:
    static constexpr qint64 BUFFER_SIZE = 1024 * 1024;      // 1 MiB
    static constexpr int MAX_RENAME_ATTEMPTS = 1000;       // макс. число переименований

    QStringList findFiles();
    QString generateTargetPath(const QString &sourceFile, int counter = 0);
    QString resolveConflict(const QString &sourceFile, const QString &targetPath);
    bool processFile(const QString &sourcePath, const QString &targetPath);
    QByteArray xorData(const QByteArray &data);

    ProcessingOptions m_options;
    std::atomic<bool> m_stopRequested;
    std::atomic<bool> m_isProcessing;
};

#endif // FILEPROCESSOR_H
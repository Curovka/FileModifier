#include "fileprocessor.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QDateTime>

FileProcessor::FileProcessor(QObject *parent)
    : QObject(parent)
    , m_stopRequested(false)
    , m_isProcessing(false)
{

}

void FileProcessor::setOptions(const ProcessingOptions &options) {
    m_options = options;
}

void FileProcessor::processFiles() {
    // Защита от повторного входа
    if (m_isProcessing.exchange(true)) {
        emit errorOccurred("Обработка уже выполняется");
        return;
    }

    m_stopRequested = false;
    emit processingStarted();

    try {
        QStringList files = findFiles();
        if (files.isEmpty()) {
            m_isProcessing = false;
            emit processingFinished("Файлы не найдены");
            return;
        }

        int processed = 0;
        int total = files.size();

        for (const QString &sourceFile : files) {
            if (m_stopRequested) { break; }

            emit fileFound(sourceFile);

            QString targetFile = generateTargetPath(sourceFile);
            targetFile = resolveConflict(sourceFile, targetFile);

            if (processFile(sourceFile, targetFile)) {
                ++processed;

                if (m_options.deleteSource) {
                    if (!QFile::remove(sourceFile)) {
                        emit errorOccurred(QString("Не удалось удалить исходный файл: %1").arg(sourceFile));
                    }
                }
            }

            int progress = (processed * 100) / total;
            emit progressChanged(progress);

            if (m_stopRequested) break;
        }

        QString result = m_stopRequested ? "Обработка прервана пользователем" : QString("Обработка завершена. Успешно: %1 из %2 файлов").arg(processed).arg(total);

        emit processingFinished(result);

    } catch (const std::exception &e) {
        emit errorOccurred(QString("Исключение: %1").arg(e.what()));
    } catch (...) {
        emit errorOccurred("Неизвестное исключение");
    }

    m_isProcessing = false;
}

void FileProcessor::stopProcessing() {
    m_stopRequested = true;
}

QStringList FileProcessor::findFiles() {
    QStringList result;

    QDirIterator::IteratorFlags flags = m_options.recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;

    QDirIterator it(m_options.sourcePath, m_options.fileMasks, QDir::Files, flags);

    while (it.hasNext() && !m_stopRequested) {
        result.append(it.next());
    }

    return result;
}

QString FileProcessor::generateTargetPath(const QString &sourceFile, int counter) {
    QFileInfo sourceInfo(sourceFile);
    QString relativePath = QDir(m_options.sourcePath).relativeFilePath(sourceFile);
    QString baseName = sourceInfo.completeBaseName();
    QString suffix = sourceInfo.suffix();

    // Формируем имя файла с учётом счётчика
    QString fileName;
    if (counter <= 0) {
        fileName = sourceInfo.fileName();
    
    } else {
        if (suffix.isEmpty()) { fileName = QString("%1_%2").arg(baseName).arg(counter); }
        else { fileName = QString("%1_%2.%3").arg(baseName).arg(counter).arg(suffix); }

    }

    // Полный путь в целевой папке с сохранением структуры
    QString targetDir = QDir(m_options.targetPath).filePath(QFileInfo(relativePath).path());
    return QDir(targetDir).filePath(fileName);
}

QString FileProcessor::resolveConflict(const QString &sourceFile, const QString &targetPath) {
    if (m_options.conflictAction == Overwrite || !QFile::exists(targetPath)) {
        return targetPath;
    }

    // Режим переименования
    int counter = 1;
    QString newPath;

    do {
        newPath = generateTargetPath(sourceFile, counter++);
    } while (QFile::exists(newPath) && counter <= MAX_RENAME_ATTEMPTS);

    // Если вдруг все имена заняты, то временная метка (позже можно исправить)
    if (QFile::exists(newPath)) {
        QFileInfo fi(targetPath);
        QString base = fi.completeBaseName();
        QString ext = fi.suffix();
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        newPath = QString("%1/%2_%3.%4").arg(fi.path()).arg(base).arg(timestamp).arg(ext);
    }

    return newPath;
}

bool FileProcessor::processFile(const QString &sourcePath, const QString &targetPath) {
    QFile sourceFile(sourcePath);
    QFile targetFile(targetPath);

    if (!sourceFile.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("Не удалось открыть файл: %1").arg(sourcePath));
        return false;
    }

    // Создание целевой директории с проверкой
    QDir targetDir;
    if (!targetDir.mkpath(QFileInfo(targetPath).path())) {
        emit errorOccurred(QString("Не удалось создать папку: %1").arg(QFileInfo(targetPath).path()));
        sourceFile.close();
        return false;
    }

    if (!targetFile.open(QIODevice::WriteOnly)) {
        emit errorOccurred(QString("Не удалось создать файл: %1").arg(targetPath));
        sourceFile.close();
        return false;
    }

    QByteArray buffer;

    while (!sourceFile.atEnd() && !m_stopRequested) {
        buffer = sourceFile.read(BUFFER_SIZE);
        if (buffer.isEmpty()) break;

        QByteArray processed = xorData(buffer);

        if (targetFile.write(processed) != processed.size()) {
            emit errorOccurred(QString("Ошибка записи в файл: %1").arg(targetPath));
            sourceFile.close();
            targetFile.close();
            targetFile.remove();
            return false;
        }
    }

    sourceFile.close();
    targetFile.close();

    if (m_stopRequested) {
        targetFile.remove();
        return false;
    }

    return true;
}

QByteArray FileProcessor::xorData(const QByteArray &data) {
    QByteArray result = data;
    if (m_options.xorKey.isEmpty()) return result;

    const char *key = m_options.xorKey.constData();
    int keyLength = m_options.xorKey.size();

    for (int i = 0; i < result.size(); ++i) {
        result[i] = result[i] ^ key[i % keyLength];
    }

    return result;
}
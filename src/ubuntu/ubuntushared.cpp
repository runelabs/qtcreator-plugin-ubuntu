#include "ubuntushared.h"

bool readFile(QString fileName, QByteArray *data, QString *errorMessage)  {
    Utils::FileReader reader;
    if (!reader.fetch(fileName, errorMessage)) return false;
    *data = reader.data();
    return true;
}

void printToOutputPane(QString msg) {
    QString timestamp = QDateTime::currentDateTime().toString(QString::fromLatin1("HH:mm:ss"));
    Core::MessageManager::write(QString(QLatin1String("[%0] %1")).arg(timestamp).arg(msg),Core::MessageManager::NoModeSwitch);
}

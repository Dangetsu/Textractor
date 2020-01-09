#include "extension.h"
#include "network.h"
#include "../GUI/host/util.h"
#include <QStringList>
#include <QFile>
#include <QCryptographicHash>

bool ProcessSentence(std::wstring& sentence, SentenceInfo sentenceInfo)
{
	DWORD processId = sentenceInfo["process id"];
	auto moduleName = Util::GetModuleFilename(processId);
	if (!moduleName) 
	{
		return false;
	}
	QString moduleNameQString = QString::fromStdWString(moduleName.value());

	QFile file(moduleNameQString);
	if (!file.open(QIODevice::ReadOnly))
	{
		return false;
	}

	QByteArray fileData = file.readAll();
	QByteArray fileMd5 = QCryptographicHash::hash(fileData, QCryptographicHash::Md5).toHex();
	//QString::fromStdString(fileMd5.toStdString()).toStdWString();

	//if (HttpRequest httpRequest{ L"Mozilla/5.0 Textractor", L"vnr.aniclan.com", L"GET", FormatString(L"/connection.php?go=allcomments&md5=%s")})
	//{
	//	httpRequest.response
	//}

	std::string cSentence = QByteArray::fromStdString(QString::fromStdWString(sentence).toStdString());
	QByteArray sentenceMd5 = QCryptographicHash::hash(QByteArray::fromStdString(cSentence), QCryptographicHash::Md5).toHex();
	
	sentence += L"\n VNR executed";
	return true;
}
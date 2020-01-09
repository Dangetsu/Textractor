#include "extension.h"
#include "network.h"
#include "../GUI/host/util.h"
#include <QStringList>
#include <QFile>
#include <QCryptographicHash>
#include <qtcommon.h>
#include <pugixml.hpp>

const QString CACHE_FILE = QString("VRN %1 Cache.txt");

static class CacheService
{
public:
	void saveCache(std::wstring fileMd5, std::wstring key, std::wstring value)
	{
		std::wstring fileIndex = concateIndex(fileMd5, key);
		QTextFile file(CACHE_FILE.arg(fileIndex), QIODevice::WriteOnly | QIODevice::Truncate);
		translationCache->insert({ fileIndex, value });
		file.write(S(value).toUtf8());
	}
	std::wstring getCache(std::wstring fileMd5, std::wstring key)
	{
		std::wstring index = concateIndex(fileMd5, key);
		if (translationCache->count(index) > 0) 
		{
			return translationCache->at(index);
		}
		return L"";
	}
private:
	Synchronized<std::map<std::wstring, std::wstring>> translationCache;

	std::wstring concateIndex(std::wstring fileMd5, std::wstring key)
	{
		return fileMd5 + key;
	}
} cacheService;

std::wstring convertStringToWstring(std::string string)
{
	return QString::fromStdString(string).toStdWString();
}

QByteArray convertWStringToQByteArray(std::wstring string)
{
	return QByteArray::fromStdString(QString::fromStdWString(string).toStdString());
}

std::wstring calcMd5Hash(QByteArray string)
{
	return convertStringToWstring(QCryptographicHash::hash(string, QCryptographicHash::Md5).toHex().toStdString());
}

bool ProcessSentence(std::wstring& sentence, SentenceInfo sentenceInfo)
{
	if (sentenceInfo["text number"] == 0) return false;

	DWORD processId = sentenceInfo["process id"];
	auto moduleName = Util::GetModuleFilename(processId);
	if (!moduleName) return false;

	QString moduleNameQString = QString::fromStdWString(moduleName.value());

	QFile file(moduleNameQString);
	if (!file.open(QIODevice::ReadOnly)) return false;

	std::wstring initialSentence = sentence;

	QByteArray fileData = file.readAll();
	std::wstring fileMd5 = calcMd5Hash(fileData);

	std::wstring response = cacheService.getCache(fileMd5, L"game");
	if (response == L"") {
		std::wstring httpQuery = FormatString(L"/connection.php?go=allcomments&md5=%s", fileMd5);
		if (HttpRequest httpRequest{ L"Mozilla/5.0 Textractor", L"vnr.aniclan.com", L"GET", httpQuery.c_str() })
		{
			response = httpRequest.response;
			cacheService.saveCache(fileMd5, L"game", response);
		}
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_string(response.c_str());
	if (result) {
		std::wstring sentenceMd5 = calcMd5Hash(convertWStringToQByteArray(initialSentence));
		for (pugi::xml_node comment : doc.child(L"grimoire").child(L"comments").children())
		{
			std::wstring commentId = comment.attribute(L"id").value();
			std::wstring commentText = comment.child_value(L"text");
			std::wstring commentContextHash = comment.child_value(L"contextHash");
			if (sentenceMd5 == commentContextHash) {
				sentence += FormatString(L"\n %s", commentText);
				break;
			}
		}
	}

	sentence += L"\n VNR executed";
	return true;
}
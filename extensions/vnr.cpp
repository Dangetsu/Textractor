#include "extension.h"
#include "network.h"
#include "defs.h"
#include "../GUI/host/util.h"
#include <QStringList>
#include <QFile>
#include <QTimer>
#include <QCryptographicHash>
#include <qtcommon.h>
#include <pugixml.hpp>

extern const char* SELECT_LANGUAGE;
extern const char* SELECT_LANGUAGE_MESSAGE;

const std::wstring EMPTY_WSTRING = L"";
const char* SETTINGS_GROUP = "VNR";
const char* SETTINGS_LANGUAGE = u8"Language";
QSettings settings(CONFIG_FILE, QSettings::IniFormat);

QStringList languages
{
	"English: en",
	"Afrikaans: af",
	"Arabic: ar",
	"Albanian: sq",
	"Belarusian: be",
	"Bengali: bn",
	"Bosnian: bs",
	"Bulgarian: bg",
	"Catalan: ca",
	"Chinese(Simplified): zh-CH",
	"Chinese(Traditional): zh-TW",
	"Croatian: hr",
	"Czech: cs",
	"Danish: da",
	"Dutch: nl",
	"Esperanto: eo",
	"Estonian: et",
	"Filipino: tl",
	"Finnish: fi",
	"French: fr",
	"Galician: gl",
	"German: de",
	"Greek: el",
	"Hebrew: iw",
	"Hindi: hi",
	"Hungarian: hu",
	"Icelandic: is",
	"Indonesian: id",
	"Irish: ga",
	"Italian: it",
	"Japanese: ja",
	"Klingon: tlh",
	"Korean: ko",
	"Latin: la",
	"Latvian: lv",
	"Lithuanian: lt",
	"Macedonian: mk",
	"Malay: ms",
	"Maltese: mt",
	"Norwegian: no",
	"Persian: fa",
	"Polish: pl",
	"Portuguese: pt",
	"Romanian: ro",
	"Russian: ru",
	"Serbian: sr",
	"Slovak: sk",
	"Slovenian: sl",
	"Somali: so",
	"Spanish: es",
	"Swahili: sw",
	"Swedish: sv",
	"Thai: th",
	"Turkish: tr",
	"Ukranian: uk",
	"Urdu: ur",
	"Vietnamese: vi",
	"Welsh: cy",
	"Yiddish: yi",
	"Zulu: zu"
};

Synchronized<std::wstring> translateLanguage = L"en";

BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		settings.beginGroup(SETTINGS_GROUP);
		if (settings.contains(SETTINGS_LANGUAGE)) translateLanguage->assign(S(settings.value(SETTINGS_LANGUAGE).toString()));
		else QTimer::singleShot(0, []
			{
				QString language = QInputDialog::getItem(
					nullptr,
					SELECT_LANGUAGE,
					QString(SELECT_LANGUAGE_MESSAGE).arg(SETTINGS_GROUP),
					languages,
					0,
					false,
					nullptr,
					Qt::WindowCloseButtonHint
				);
				translateLanguage->assign(S(language.split(": ")[1]));
				settings.setValue(SETTINGS_LANGUAGE, S(translateLanguage->c_str()));
			});
	}
	break;
	}
	return TRUE;
}

static class CacheService
{
public:
	void saveCache(std::wstring fileMd5, std::wstring key, std::wstring value)
	{
		std::wstring fileIndex = _concateIndex(fileMd5, key);
		translationCache->insert({ fileIndex, value });
	}
	std::wstring getCache(std::wstring fileMd5, std::wstring key)
	{
		std::wstring index = _concateIndex(fileMd5, key);
		if (translationCache->count(index) > 0) 
		{
			return translationCache->at(index);
		}
		return EMPTY_WSTRING;
	}
private:
	Synchronized<std::map<std::wstring, std::wstring>> translationCache;

	std::wstring _concateIndex(std::wstring fileMd5, std::wstring key)
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

std::vector<std::wstring> explode(std::wstring string, std::wstring delimiter)
{
	std::vector<std::wstring> parts;
	size_t pos = 0;
	while ((pos = string.find(delimiter)) != std::wstring::npos) {
		parts.push_back(string.substr(0, pos));
		string.erase(0, pos + delimiter.length());
	}
	parts.push_back(string); // append last item
	return parts;
}

std::wstring extractTranslationsFromXmlByContext(std::wstring xmlText, std::wstring searchContext)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_string(xmlText.c_str());
	if (!result) {
		return EMPTY_WSTRING;
	}

	std::wstring translations;
	for (pugi::xml_node comment : doc.child(L"grimoire").child(L"comments").children())
	{
		std::wstring commentLanguage = comment.child_value(L"language");
		int commentId = std::stoi(comment.attribute(L"id").value());
		std::wstring commentText = comment.child_value(L"text");
		std::wstring commentContext = comment.child_value(L"context");
		int commentContextSize = std::stoi(comment.child(L"context").attribute(L"size").value());
		if (commentContextSize > 1) {
			std::vector<std::wstring> parts = explode(commentContext, L"||");
			commentContext = parts.back();
		}

		std::wstring searchContextWithoutWhitespaces = searchContext;
		searchContextWithoutWhitespaces.erase(std::remove(searchContextWithoutWhitespaces.begin(), searchContextWithoutWhitespaces.end(), L'　'), searchContextWithoutWhitespaces.end());
		if ((searchContext == commentContext || searchContextWithoutWhitespaces == commentContext) && commentLanguage == translateLanguage->c_str())
		{
			translations += FormatString(L"\n %s", commentText);
		}
	}

	return translations;
}

std::wstring calcFileMd5ByProcessId(DWORD processId) {
	auto moduleName = Util::GetModuleFilename(processId);
	if (!moduleName) return EMPTY_WSTRING;

	QString moduleNameQString = QString::fromStdWString(moduleName.value());
	QFile file(moduleNameQString);
	if (!file.open(QIODevice::ReadOnly)) return EMPTY_WSTRING;

	QByteArray fileData = file.readAll();
	return calcMd5Hash(fileData);
}

bool ProcessSentence(std::wstring& sentence, SentenceInfo sentenceInfo)
{
	if (sentenceInfo["text number"] == 0) return false;

	std::wstring fileMd5 = calcFileMd5ByProcessId(sentenceInfo["process id"]);
	if (fileMd5 == EMPTY_WSTRING) {
		return false;
	}

	std::wstring response = cacheService.getCache(fileMd5, L"game");
	if (response == EMPTY_WSTRING) {
		std::wstring httpQuery = FormatString(L"/connection.php?go=allcomments&md5=%s", fileMd5);
		if (HttpRequest httpRequest{ L"Mozilla/5.0 Textractor", L"vnr.aniclan.com", L"GET", httpQuery.c_str() })
		{
			response = httpRequest.response;
			cacheService.saveCache(fileMd5, L"game", response);
		}
	}

	sentence += extractTranslationsFromXmlByContext(response, sentence);
	sentence += L"\n VNR executed for " + fileMd5;
	return true;
}
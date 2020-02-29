#include "extension.h"
#include "network.h"
#include "util.h"
#include <ctime>
#include <QStringList>

extern const wchar_t* TRANSLATION_ERROR;

extern Synchronized<std::wstring> translateTo;

const char* TRANSLATION_PROVIDER = "Yandex";
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

std::wstring GetTranslationUri(const std::wstring& text)
{
	const wchar_t* transTo = translateTo->c_str();
	return FormatString(L"/dictionary/translate?brandID=yandex&statLang=%s&targetLang=%s&url=http://vnr.aniclan.com/&locale=ru&text=%s", transTo, transTo, Escape(text));
}

std::pair<bool, std::wstring> Translate(const std::wstring& text)
{
	std::wstring url = GetTranslationUri(text);
	if (HttpRequest httpRequest{
		L"Mozilla/5.0 Textractor",
		L"api.browser.yandex.ru",
		L"GET",
		url.c_str()
		})
		// Response formatted as JSON: translation starts with text":" and ends with ","
		if (std::wsmatch results; std::regex_search(httpRequest.response, results, std::wregex(L"text\":\"(.+)\"\,\"ts"))) return { true, results[1] };
		else return { false, TRANSLATION_ERROR };
	else return { false, FormatString(L"%s (code=%u)", TRANSLATION_ERROR, httpRequest.errorCode) };
}

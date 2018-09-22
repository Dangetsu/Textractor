// textthread.cc
// 8/24/2013 jichi
// Branch IHF/TextThread.cpp, rev 133

#include "textthread.h"
#include "host.h"
#include "const.h"

TextThread::TextThread(ThreadParam tp, DWORD status) : tp(tp), status(status), name(Host::GetHookName(tp.pid, tp.hook)) {}

TextThread::~TextThread()
{
	SetEvent(deletionEvent);
	flushThread.join();
	CloseHandle(deletionEvent);
	LOCK(ttMutex);
}

std::wstring TextThread::GetStore()
{
	LOCK(ttMutex);
	return storage;
}

void TextThread::Flush()
{
	std::wstring sentence;
	{
		LOCK(ttMutex);
		if (buffer.size() < 400 && (GetTickCount() - timestamp < flushDelay || buffer.size() == 0)) return; // TODO: let user change delay before sentence is flushed
		if (status & USING_UNICODE) sentence = std::wstring((wchar_t*)buffer.data(), buffer.size() / 2);
		else sentence = ToWString(buffer.data(), status & USING_UTF8 ? CP_UTF8 : SHIFT_JIS);
		buffer.clear();
	}
	AddSentence(sentence);
}

void TextThread::AddSentence(std::wstring sentence)
{
	// Dispatch to extensions occurs here. Don't hold mutex! Extensions might take a while!
	if (Output) sentence = Output(this, sentence);
	LOCK(ttMutex);
	storage.append(sentence);
}

void TextThread::AddText(const BYTE* data, int len)
{
	LOCK(ttMutex);
	buffer.insert(buffer.end(), data, data + len);
	timestamp = GetTickCount();
}

// EOF

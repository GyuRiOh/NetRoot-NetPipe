
#define _WINSOCKAPI_
#include <Windows.h>
#include "Parser.h"
#include "SystemLogger.h"
#include "Crash.h"


void server_baby::Parser::Initialize()
{
	LoadFile();
}

void server_baby::Parser::LoadFile(const char* fileName)
{
	FILE* stream;
	if (fopen_s(&stream, fileName, "r") == 0)
	{
		fseek(stream, 0, SEEK_END);
		int size = ftell(stream);

		fseek(stream, 0, SEEK_SET);
		fread(buffer_, size, 1, stream);
		fclose(stream);
	}
	else
	{
		SystemLogger::GetInstance()->LogText(L"Parser", LEVEL_ERROR, L"fopen failed");
		CrashDump::Crash();
	}

}

bool server_baby::Parser::GetValue(const char* key, int* value)
{
	char* buf = buffer_;
	char word1[128] = { 0 };
	char word2[128] = { 0 };
	char word3[128] = { 0 };
	int length = 0;

	while (FindNextWord(&buf, &length))
	{
		memset(word1, 0, sizeof(word1));
		memcpy(word1, buf, length);

		buf += length + 1;
		if (strcmp(key, word1) != 0)
			continue;

		if (!FindNextWord(&buf, &length))
			break;

		memcpy(word2, buf, length);

		buf += length + 1;
		if (strcmp("=", word2) != 0)
			continue;

		if (!FindNextWord(&buf, &length))
			break;

		memcpy(word3, buf, length);
		*value = atoi(word3);

		return true;
	}
	

	return false;
}

bool server_baby::Parser::GetString(const char* key, char* value)
{

	char* buf = buffer_;
	char word1[128] = { 0 };
	char word2[128] = { 0 };
	char word3[128] = { 0 };
	char word4[512] = { 0 };
	int length = 0;

	while (FindNextWord(&buf, &length))
	{
		memset(word1, 0, sizeof(word1));
		memcpy(word1, buf, length);

		buf += length + 1;
		if (strcmp(key, word1) != 0)
			continue;

		if (!FindNextWord(&buf, &length))
			break;

		memcpy(word2, buf, length);
		buf += length + 1;
		if (strcmp("=", word2) != 0)
			continue;

		if (!FindStringWord(&buf, &length))
			break;

		memcpy(value, buf, length);
		return true;
	}

	return false;
}

//스페이스, 탭, 엔터, 주석 처리
bool server_baby::Parser::SkipNoneCommand(char character)
{
	switch (character)
	{
	case 0x00:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0d:
	case 0x20:
	case L',':
		break;
	default:
		return false;
	}

	return true;
}

//다음 단어 얻기
bool server_baby::Parser::FindNextWord(char** bufferStart, int* length)
{
	int loopCnt = 0;
	char* cursor = *bufferStart;

	while (1)
	{
		if (SkipNoneCommand(*cursor))
			break;

		cursor++;
		loopCnt++;

	}

	if (!loopCnt)
		return false;

	*length = loopCnt;
	return true;
}

//문자열 얻기
bool server_baby::Parser::FindStringWord(char** bufferStart, int* length)
{
	int loopCnt = 0;
	char* cursor = *bufferStart;
	
	for (;;)
	{
		if ((*cursor) == '\"')
			break;

		if (SkipNoneCommand(*cursor))
			return false;

		cursor++;
	}

	cursor++;
	*bufferStart = cursor;

	for(;;)
	{

		if ((*cursor) == '"')
			break;

		if (SkipNoneCommand(*cursor))
			return false;

		cursor++;
		loopCnt++;

	}

	*length = loopCnt;
	return true;

}

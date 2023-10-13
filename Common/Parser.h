
#ifndef  __PARSER__
#define  __PARSER__
#include <iostream>
#include "Singleton.h"

namespace server_baby
{
	class Parser : public Singleton<Parser>
	{
		enum en_Parser
		{
			eBUFFER_DEFAULT = 8192,
		};

		explicit Parser();
	public:
		~Parser(){}

		void Initialize();
		void Destroy(){}
		void LoadFile(const char* fileName = "Config.ini");
		bool GetValue(const char* key, int* value);
		bool GetString(const char* key, char* value);
		char* GetBuffer() 
		{
			return buffer_;
		}
	
	private:
		bool SkipNoneCommand(char character);
		bool FindNextWord(char** bufferStart, int* length);
		bool FindStringWord(char** bufferStart, int* length);

	private:
		char buffer_[eBUFFER_DEFAULT];

	};
}
#endif
#pragma once
#include <iostream>
#include <map>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <filesystem>
#include <atomic>
static inline std::string trim(const std::string& s)
{
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");

	if (start == std::string::npos)
		return "";

	return s.substr(start, end - start + 1);
}
static inline std::string toLower(std::string s)
{
	for (char& c : s)
	{
		c = static_cast<char>(std::tolower((unsigned char)c));
	}
	return s;
}
static inline bool startWith(const std::string& word, const std::string& prefix)
{
	if (word.size() < prefix.size())
		return false;

	return word.compare(0, prefix.size(), prefix) == 0;
}
static inline std::chrono::system_clock::time_point parseTime(const std::string& s)
{
	std::tm tm{};
	std::istringstream ss(s);
	ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

	return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}
static inline bool isTimeInRange(const std::string& logTime,
	const std::string& start,
	const std::string& end)
{
	auto logTp = parseTime(logTime);
	auto startTp = parseTime(start);
	auto endTp = parseTime(end);

	return logTp >= startTp && logTp <= endTp;
}
static inline bool endWith(const std::string& word, const std::string& suffix)
{
	if (word.size() < suffix.size())
		return false;

	return word.compare(word.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static inline std::string normalize(std::string word)
{
	
	std::transform(word.begin(), word.end(), word.begin(), ::tolower);

	if (word.size() > 5 && endWith(word, "ing"))
		return word.substr(0, word.size() - 3);

	if (word.size() > 4 && endWith(word, "ed"))
		return word.substr(0, word.size() - 2);

	if (word.size() > 4 && endWith(word, "s"))
		return word.substr(0, word.size() - 1);

	return word;
}
static inline bool containsKeyword(const std::string& word, const std::string& keyword)
{
	return word.find(keyword) != std::string::npos;
}
static inline bool isDelimiter(char c)
{
		return c == ' ' ||
			c == ',' ||
			c == '.' ||
			c == '[' ||
			c == ']' ||
			c == '(' ||
			c == ')' ||
			c == '{' ||
			c == '}' ||
			c == '\t' ||
			c == '\n';
}

static inline char detectDelimiter(const std::string& line)
{
	int comma = 0;
	int semicolon = 0;
	int tab = 0;

	bool inQuotose = false;
	for (size_t i = 0; i < line.size(); i++)
	{
		char c = line[i];
		if (c == '"')
		{
			if(inQuotose && i + 1 < line.size() && line[i + 1] == '"')
			{
				i++;
			}
			else
			{
				inQuotose = !inQuotose;
			}
			
		}
		else if (!inQuotose)
		{
			if (c == ',') comma++;
			if (c == ';') semicolon++;
			if (c == '\t') tab++;
		}
	}
	if (semicolon > comma && semicolon > tab) return ';';
	if (tab > comma && tab > semicolon) return '\t';

	return ',';
}
struct LogEntry
{
	std::string timestamp;
	std::string level;
	std::string message;


	std::unordered_map<std::string, std::string> extraFields;
};
struct ProcessingStats
{
	uint64_t linesRead = 0;
	uint64_t linesProcessed = 0;
	uint64_t linesSkipped = 0;
	uint64_t parseErrors = 0;

	void Merge(const ProcessingStats& other)
	{
		linesRead += other.linesRead;
		linesProcessed += other.linesProcessed;
		linesSkipped += other.linesSkipped;
		parseErrors += other.parseErrors;
	}
};
struct AnalysisResult
{
	std::map<std::string, int> levelCount;
	std::map<std::string, int> keywordsCount;

	ProcessingStats stats;
	

	void Merge(const AnalysisResult& other)
	{
		for (const auto& [k, v] : other.levelCount)
			levelCount[k] += v;

		for (const auto& [k, v] : other.keywordsCount)
			keywordsCount[k] += v;

		stats.Merge(other.stats);
	}
	
};
class LogParser
{
public:
	virtual void ParseFile(const std::string& filenamem, std::function <void(const std::string&)> online, const std::atomic<bool>* stopRequested) = 0;

	virtual ~LogParser() = default;
};
class LogSearch
{
public:
	virtual void Init(const std::string& line) { };
	virtual LogEntry ParseLine(const std::string& line) = 0;
	virtual std::unique_ptr<LogSearch> Clone()  const = 0;
	virtual bool HasHeader() 
	{
		return false;
	}
	virtual ~LogSearch() = default;
};
class LogFilter
{
public:
	virtual std::unique_ptr<LogFilter> Clone() const = 0;
	virtual bool FilterByLevel(const  LogEntry& entry) = 0;
	virtual ~LogFilter() = default;
};
class IAnalyzer
{
public:
	virtual void Proces(const LogEntry& entry) = 0;
	virtual AnalysisResult GetResukt() = 0;
	virtual void Merge(const IAnalyzer& other) = 0;
	virtual std::unique_ptr<IAnalyzer> Clone() const = 0;
	virtual ~IAnalyzer() = default;
};
class IOutput
{
public:
	virtual void write(const AnalysisResult& result) = 0;
	virtual ~IOutput() = default;
};
class IConvertor
{
public:
	virtual std::string Converte(const LogEntry& entry) = 0;
	virtual std::unique_ptr<IConvertor> Clone() const = 0;
	virtual ~IConvertor() = default;
};
class IConvertOutput
{
public:
	virtual ~IConvertOutput() = default;
	
	virtual void Write(const std::string& data) = 0;
};

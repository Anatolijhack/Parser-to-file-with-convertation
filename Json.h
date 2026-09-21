#pragma once
#include "LogPipeline.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <set>
using json = nlohmann::json;
class JsonParse : public LogParser
{
public:
	void ParseFile(const std::string& filenamem, std::function <void(const std::string&)> online, const std::atomic<bool>* stopRequested) override;
};
class JsonLineParse : public  LogSearch
{
public:
	LogEntry ParseLine(const std::string& line) override;
	std::unique_ptr<LogSearch> Clone() const override;
};
class JsonFilterbyLevel : public LogFilter
{
private:
	std::string  level;
	std::string startTime;
	std::string endTime;
public:
	JsonFilterbyLevel(std::string level,std::string startTime, std::string endTime);
	std::unique_ptr<LogFilter> Clone() const override;
	bool FilterByLevel(const LogEntry& entry) override;

};
class JsonAnalyzer : public IAnalyzer
{
private:
	AnalysisResult result;
	std::set<std::string> keyword;
public:
	JsonAnalyzer(std::set<std::string> keyword);
	void Proces(const LogEntry& entry) override;
	AnalysisResult GetResukt() override;
	std::unique_ptr<IAnalyzer> Clone() const override;
	void Merge(const IAnalyzer& other) override;
};
class JsonOutput : public IOutput
{
public:
	void write(const AnalysisResult& result) override;
};

class JsonConverter : public IConvertor
{
public:
	std::string Converte(const LogEntry& entry) override
	{
		nlohmann::json j;

		j["timestamp"] = entry.timestamp;
		j["level"] = entry.level;
		j["message"] = entry.message;

		for (const auto& [key, value] : entry.extraFields)
			j[key] = value;

		return j.dump();
	}

	std::unique_ptr<IConvertor> Clone() const override
	{
		return std::make_unique<JsonConverter>(*this);
	}
};
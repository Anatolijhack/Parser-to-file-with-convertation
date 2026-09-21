#pragma once
#include "LogPipeline.h"
#include <fstream>
#include <set>


class ParseCSV : public LogParser
{
public:
	void ParseFile(const std::string& name, std::function<void(const std::string&)> online, const std::atomic<bool>* stopRequested) override;
};
class ParseLineCsv : public LogSearch
{
public:
	std::unordered_map<std::string, size_t> headerMap;
	void Init(const std::string& line) override;
	std::unique_ptr<LogSearch> Clone()  const override;
	LogEntry ParseLine(const std::string& line) override;
	bool HasHeader();
};
class CsvFilter : public LogFilter
{
private:
	std::string startTime;
	std::string endTime;
	std::string level;
public:
	
	CsvFilter(const std::string level,std::string startTime, std::string endTime);
	std::unique_ptr<LogFilter> Clone() const override;
	bool FilterByLevel(const  LogEntry& entry) override;
	
};
class CsvAnalyzer : public IAnalyzer
{
private:
	AnalysisResult result;
	std::set<std::string> keyword;
public:
	CsvAnalyzer(std::set<std::string> keywords);
	void Proces(const LogEntry& entry) override;
	std::unique_ptr<IAnalyzer> Clone() const override;
	void Merge(const IAnalyzer& other) override;
	AnalysisResult GetResukt() override;
};

class CsvConverter : public IConvertor
{
private:
	char delimiter = ',';
public:
	CsvConverter(char c) : delimiter(c) {};
	std::unique_ptr<IConvertor> Clone() const override
	{
		return std::make_unique<CsvConverter>(*this);
	}
	std::string Converte(const LogEntry& entry) override;
};
class CsvOutput : public IOutput
{
public:

	void write(const AnalysisResult& result) override;
};

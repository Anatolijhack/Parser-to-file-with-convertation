#pragma once
#include "Parser.h"
#include "memory"
#include "ThrreadPool.h"
#include <set>
#include <atomic>
static std::mutex cout_mtx;
//class LogPipeline
//{
//private:
//	std::mutex mtx;
//	std::mutex output_mtx;
//	std::unique_ptr<LogParser> source;
//	std::unique_ptr<LogSearch> parser;
//	std::unique_ptr<LogFilter> filter;
//	std::unique_ptr<IAnalyzer> analyze;
//	std::unique_ptr<IOutput> output;
//public:
//	LogPipeline(std::unique_ptr<LogParser> p, std::unique_ptr<LogSearch> s, std::unique_ptr<LogFilter> f, std::unique_ptr<IAnalyzer> a, std::unique_ptr<IOutput> o);
//	void run(const std::string& filename, ThreadPool& pool);
//
//};
 

class PipelineError : public std::runtime_error
{
public:
    PipelineError(
        const std::string& filename,
        size_t batch,
        const std::string& message)
        : std::runtime_error(
            "Pipeline error in file '" +
            filename +
            "', batch " +
            std::to_string(batch) +
            ": " +
            message)
    {
    }
};

class LogPipeline
{
public:
    LogPipeline(
        std::unique_ptr<LogParser> p,
        std::unique_ptr<LogSearch> s,
        std::unique_ptr<LogFilter> f,
        std::set<std::string> keywords_,
        std::unique_ptr<IOutput> o);

    
    IOutput* getOutput()
    {
        return output.get();
    }
    AnalysisResult run(const std::string& filename, ThreadPool& pool, const std::atomic<bool>* stopRequested);

private:
    std::set<std::string> keywords_;
    std::unique_ptr<LogParser> source;
    std::unique_ptr<LogSearch> parser;
    std::unique_ptr<LogFilter> filter;
    std::unique_ptr<IAnalyzer> analyze;
    std::unique_ptr<IOutput> output;
};
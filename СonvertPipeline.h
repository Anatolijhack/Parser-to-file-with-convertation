#pragma once
#include "Parser.h"
#include <fstream>
#include "ThrreadPool.h"

class FileConvertOutput : public IConvertOutput
{
private:
    std::ofstream file;

public:
    explicit FileConvertOutput(const std::string& filename)
    {
        file.open(filename);

        if (!file.is_open())
            throw std::runtime_error("Output file isn't open");
    }

    void Write(const std::string& data) override
    {
        file << data << '\n';
    }
};


class ConvertePipeline
{
public:
    ConvertePipeline(
        std::unique_ptr<LogParser> p,
        std::unique_ptr<LogSearch> s,
        std::unique_ptr<IConvertor> c,
        std::unique_ptr<IConvertOutput> o);

    IConvertOutput* getOutput()
    {
        return output.get();
    }

    void run(
        const std::string& filename,
        ThreadPool& pool,
        const std::atomic<bool>* stopRequested);

private:
    std::unique_ptr<LogParser> source;
    std::unique_ptr<LogSearch> parser;
    std::unique_ptr<IConvertor> converter;
    std::unique_ptr<IConvertOutput> output;
};
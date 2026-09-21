#include "Txt.h"
#include "Csv.h"
#include "Json.h"
#include <filesystem>
#include <csignal>
#include <atomic>

// остальные твои include
namespace fs = std::filesystem;
std::atomic<bool> stopRequested{ false };

void SignalHandler(int)
{
    stopRequested.store(true);
}

std::unique_ptr<LogPipeline> createPipeline(const std::filesystem::path& path)
{
    std::string ext = path.extension().string();

    std::set<std::string> keywords = { "error", "connection","database"};

    if (ext == ".csv")
    {
        return std::make_unique<LogPipeline>(
            std::make_unique<ParseCSV>(),
            std::make_unique<ParseLineCsv>(),
            std::make_unique<CsvFilter>(
                "ERROR",
                "2025-06-01 10:00:00",
                "2025-06-01 12:00:00"
            ),
            keywords,
            std::make_unique<CsvOutput>()   // ✔ вернули
        );
    }
    else if (ext == ".log")
    {
        return std::make_unique<LogPipeline>(
            std::make_unique<TxtParser>(),
            std::make_unique<LineParser>(),
            std::make_unique<LevelFilter>(
                "ERROR",
                "2025-06-01 10:00:00",
                "2025-06-01 12:00:00"
            ),
            keywords,
            std::make_unique<ConsoleOutput>()   // ✔ вернули
        );
    }
    else if (ext == ".json")
    {
        return std::make_unique<LogPipeline>(
            std::make_unique<JsonParse>(),
            std::make_unique<JsonLineParse>(),
            std::make_unique<JsonFilterbyLevel>(
                "ERROR",
                "2025-06-01 10:00:00",
                "2025-06-01 12:00:00"
            ),
            keywords,
            std::make_unique<JsonOutput>()   // ✔ вернули
        );
    }

    return nullptr;
}

int main()
{

    std::signal(SIGINT, SignalHandler);

    ThreadPool pool(8);
    
    fs::path dir = LR"(D:\Мои проект\ParsFile by me test)";

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {

        if (stopRequested.load())
        {
            std::cout << "Shutdown requested.\n";
            break;
        }


        if (!entry.is_regular_file()) continue; const fs::path& path = entry.path(); const std::string ext = path.extension().string();

        if (ext != ".csv" && ext != ".log" && ext != ".json") { continue; };

        auto pipeline = createPipeline(path); if (!pipeline) { std::cerr << "Unsupported file: " << path << '\n'; continue; }

        AnalysisResult result = pipeline->run(path.string(), pool,&stopRequested);

        std::cout << "File: " << entry.path().string() << '\n';
        std::cout << "Lines read: "
            << result.stats.linesRead << '\n';
        std::cout << "Lines processed: "
            << result.stats.linesProcessed << '\n';
        std::cout << "Lines skipped: "
            << result.stats.linesSkipped << '\n';
        std::cout << "Parse errors: "
            << result.stats.parseErrors << '\n';

        pipeline->getOutput()->write(result);
    }


    pool.wait(); return 0;

}
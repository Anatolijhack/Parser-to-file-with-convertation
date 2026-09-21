#include "LogPipeline.h"
#include "ThrreadPool.h"
#include <optional>
#include "Txt.h"

//LogPipeline::LogPipeline(std::unique_ptr<LogParser> p, std::unique_ptr<LogSearch> s, std::unique_ptr<LogFilter> f, std::unique_ptr<IAnalyzer> a, std::unique_ptr<IOutput> o) : analyze(std::move(a)), source(std::move(p)), filter(std::move(f)), parser(std::move(s)),output(std::move(o)) {}
//
//void LogPipeline::run(const std::string& filename, ThreadPool& pool)
//{
//    const size_t BATCH_SIZE = 200;
//
//    std::vector<std::string> buffer;
//    buffer.reserve(BATCH_SIZE);
//
//    bool firstLine = true;
//
//
//
//    std::vector<std::future<void>> futures;
//
//    source->ParseFile(filename, [&](const std::string& line)
//        {
//
//            if (firstLine && parser->HasHeader())
//            {
//                parser->Init(line);
//                firstLine = false;
//                return;
//            }
//
//            firstLine = false;
//            buffer.push_back(line);
//
//            if (buffer.size() >= BATCH_SIZE)
//            {
//                auto batch = std::move(buffer);
//                buffer.clear();
//
//                futures.emplace_back(
//                    pool.submit(0, [this, batch = std::move(batch)]() mutable
//                        {
//                           
//                            auto localAnalyzer = analyze->Clone();
//
//                            for (auto& line : batch)
//                            {
//                                auto entry = parser->ParseLine(line);
//
//                                if (!filter->FilterByLevel(entry))
//                                    continue;
//
//                                localAnalyzer->Proces(entry);
//                            }
//
//                           
//                            {
//                                std::lock_guard<std::mutex> lock(mtx);
//                                analyze->Merge(*localAnalyzer);
//                            }
//                        })
//                );
//            }
//            
//        });
//
//     //🔥 хвост
//    if (!buffer.empty())
//    {
//        auto batch = std::move(buffer);
//
//        futures.emplace_back(
//            pool.submit(0, [this, batch = std::move(batch)]() mutable
//                {
//
//                    auto localAnalyzer = analyze->Clone();
//
//                    for (auto& line : batch)
//                    {
//                        auto entry = parser->ParseLine(line);
//
//                        if (!filter->FilterByLevel(entry))
//                            continue;
//
//                        localAnalyzer->Proces(entry);
//                    }
//
//
//                    {
//                        std::lock_guard<std::mutex> lock(mtx);
//                        analyze->Merge(*localAnalyzer);
//                    }
//                })
//        );
//    }
//    for (auto& f : futures)
//    {
//        f.get();
//    }
//    {
//        std::lock_guard<std::mutex> lock(cout_mtx);
//        output->write(analyze->GetResukt());
//    }
//    
//}
 



LogPipeline::LogPipeline(
    std::unique_ptr<LogParser> p,
    std::unique_ptr<LogSearch> s,
    std::unique_ptr<LogFilter> f,
    std::set<std::string> keywords_,
    std::unique_ptr<IOutput> o)
    : source(std::move(p)),
    parser(std::move(s)),
    filter(std::move(f)),
    keywords_(std::move(keywords_)),
    output(std::move(o))
{}

AnalysisResult processBatch(
    std::vector<std::string> batch,
    LogSearch* parser,
    LogFilter* filter,
    std::set<std::string> keywords)
{
    auto parserLocal = parser->Clone();
    auto filterLocal = filter->Clone();

    Analyzer analyzer(keywords);

    for (const auto& line : batch)
    {
        auto entry = parserLocal->ParseLine(line);

        if (!filterLocal->FilterByLevel(entry))
            continue;

        analyzer.Proces(entry);
    }

    return analyzer.GetResukt();
}

//AnalysisResult LogPipeline::run(const std::string& filename, ThreadPool& pool)
//{
//    const size_t BATCH_SIZE = 200;
//    const size_t MAX_QUEUE = 10;
//
//    std::vector<std::string> buffer;
//    buffer.reserve(BATCH_SIZE);
//
//    std::deque<std::future<AnalysisResult>> futures;
//
//    AnalysisResult finalResult;
//
//    bool firstLine = true;
//
//    LogSearch* parserPtr = parser.get();
//
//    // 🔥 забираем готовые задачи
//    auto drain_ready = [&]()
//        {
//            while (!futures.empty())
//            {
//                auto& f = futures.front();
//
//                if (f.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
//                    break;
//
//                finalResult.Merge(f.get());
//                futures.pop_front();
//            }
//        };
//
//    // 🔥 жёсткое ограничение
//    auto wait_slot = [&]()
//        {
//            while (futures.size() >= MAX_QUEUE)
//            {
//                // сначала пробуем не блокируясь
//                drain_ready();
//
//                if (futures.size() < MAX_QUEUE)
//                    break;
//
//                // иначе ждём первый
//                finalResult.Merge(futures.front().get());
//                futures.pop_front();
//            }
//        };
//
//    source->ParseFile(filename, [&](const std::string& line)
//        {
//            if (firstLine && parserPtr->HasHeader())
//            {
//                parserPtr->Init(line);
//                firstLine = false;
//                return;
//            }
//
//            firstLine = false;
//            buffer.push_back(line);
//
//            if (buffer.size() == BATCH_SIZE)
//            {
//                wait_slot(); // 🔥 ограничение
//
//                auto batch = std::move(buffer);
//                buffer.clear();
//
//                auto parserClone = std::shared_ptr<LogSearch>(parser->Clone());
//                auto filterClone = std::shared_ptr<LogFilter>(filter->Clone());
//                auto keywordsCopy = keywords_;
//
//                futures.emplace_back(
//                    pool.submit(0,
//                        [batch = std::move(batch),
//                        parser = parserClone,
//                        filter = filterClone,
//                        keywordsCopy]() mutable
//                        {
//                            Analyzer analyzer(keywordsCopy);
//
//                            for (const auto& line : batch)
//                            {
//                                auto entry = parser->ParseLine(line);
//
//                                if (!filter->FilterByLevel(entry))
//                                    continue;
//
//                                analyzer.Proces(entry);
//                            }
//
//                            return analyzer.GetResukt();
//                        })
//                );
//            }
//        });
//
//    // tail
//    if (!buffer.empty())
//    {
//        wait_slot();
//
//        auto parserClone = std::shared_ptr<LogSearch>(parser->Clone());
//        auto filterClone = std::shared_ptr<LogFilter>(filter->Clone());
//        auto keywordsCopy = keywords_;
//
//        futures.emplace_back(
//            pool.submit(0,
//                [batch = std::move(buffer),
//                parser = parserClone,
//                filter = filterClone,
//                keywordsCopy]() mutable
//                {
//                    Analyzer analyzer(keywordsCopy);
//
//                    for (const auto& line : batch)
//                    {
//                        auto entry = parser->ParseLine(line);
//
//                        if (!filter->FilterByLevel(entry))
//                            continue;
//
//                        analyzer.Proces(entry);
//                    }
//
//                    return analyzer.GetResukt();
//                })
//        );
//    }
//
//    // финальный drain
//    while (!futures.empty())
//    {
//        finalResult.Merge(futures.front().get());
//        futures.pop_front();
//    }
//
//    return finalResult;
//}
AnalysisResult LogPipeline::run(
    const std::string& filename,
    ThreadPool& pool, const std::atomic<bool>* stopRequested)
{
    const size_t BATCH_SIZE = 200;
    const size_t MAX_QUEUE = 10;

    struct PendingBatch
    {
        size_t id;
        std::future<AnalysisResult> future;
    };

    std::vector<std::string> buffer;
    buffer.reserve(BATCH_SIZE);

    std::deque<PendingBatch> futures;

    AnalysisResult finalResult;

    bool firstLine = true;
    size_t nextBatchId = 0;

    LogSearch* parserPtr = parser.get();

    // Получить результат batch-а и добавить контекст ошибки.
    auto get_result =
        [&](PendingBatch& pending) -> AnalysisResult
        {
            try
            {
                return pending.future.get();
            }
            catch (const std::exception& e)
            {
                throw PipelineError(
                    filename,
                    pending.id,
                    e.what());
            }
            catch (...)
            {
                throw PipelineError(
                    filename,
                    pending.id,
                    "unknown exception");
            }
        };

    // Забрать все готовые batch-и с начала очереди.
    auto drain_ready = [&]()
        {
            while (!futures.empty())
            {
                auto& pending = futures.front();

                if (pending.future.wait_for(
                    std::chrono::seconds(0))
                    != std::future_status::ready)
                {
                    break;
                }

                finalResult.Merge(
                    get_result(pending));

                futures.pop_front();
            }
        };

    // Не позволяем накопить слишком много ожидающих задач.
    auto wait_slot = [&]()
        {
            while (futures.size() >= MAX_QUEUE)
            {
                // Сначала пробуем забрать уже готовые.
                drain_ready();

                if (futures.size() < MAX_QUEUE)
                    break;

                // Если готовых нет — ждём самый старый batch.
                finalResult.Merge(
                    get_result(futures.front()));

                futures.pop_front();
            }
        };

    // Создание и отправка batch-а.
    auto submit_batch =
        [&](std::vector<std::string> batch)
        {
            if (stopRequested &&
                stopRequested->load())
            {
                return;
            }

            wait_slot();

            if (stopRequested &&
                stopRequested->load())
            {
                return;
            }


            const size_t batchId = nextBatchId++;

            auto parserClone =
                std::shared_ptr<LogSearch>(
                    parser->Clone());

            auto filterClone =
                std::shared_ptr<LogFilter>(
                    filter->Clone());

            auto keywordsCopy = keywords_;

            auto future = pool.submit(
                0,
                [batch = std::move(batch),
                parser = parserClone,
                filter = filterClone,
                keywordsCopy]() mutable
                {
                    Analyzer analyzer(keywordsCopy);

                    AnalysisResult result;

                    for (const auto& line : batch)
                    {
                        result.stats.linesRead++;

                        try
                        {
                            auto entry = parser->ParseLine(line);

                            if (!filter->FilterByLevel(entry))
                            {
                                result.stats.linesSkipped++;
                                continue;
                            }

                            analyzer.Proces(entry);
                            result.stats.linesProcessed++;
                        }
                        catch (...)
                        {
                            result.stats.parseErrors++;
                        }
                    }

                    result.Merge(analyzer.GetResukt());

                    return result;
                });

            futures.push_back(
                PendingBatch{
                    batchId,
                    std::move(future)
                });
        };

    std::exception_ptr parseException;

    try
    {
        source->ParseFile(
            filename,
            [&](const std::string& line)
            {
                if (stopRequested &&
                    stopRequested->load())
                {
                    return;
                }

                if (firstLine && parserPtr->HasHeader())
                {
                    parserPtr->Init(line);
                    firstLine = false;
                    return;
                }

                firstLine = false;

                buffer.push_back(line);

                if (buffer.size() == BATCH_SIZE)
                {
                    submit_batch(std::move(buffer));

                    buffer.clear();
                    buffer.reserve(BATCH_SIZE);
                }
            }, stopRequested);
    }
    catch (...)
    {
        // ParseFile мог упасть во время чтения файла.
        // Сохраняем исключение, но не бросаем сразу:
        // уже отправленные batch-и должны завершиться.
        parseException = std::current_exception();
    }

    // Если чтение файла закончилось нормально
    // и остался неполный batch — отправляем его.
    if (!buffer.empty() &&
        !parseException &&
        !(stopRequested && stopRequested->load()))
    {
        submit_batch(std::move(buffer));
        buffer.clear();
    }

    // Дожидаемся ВСЕХ уже отправленных batch-ей.
    while (!futures.empty())
    {
        auto pending = std::move(futures.front());
        futures.pop_front();

        try
        {
            finalResult.Merge(
                pending.future.get());
        }
        catch (const std::exception& e)
        {
            // Если worker упал — это ошибка pipeline.
            // Но сначала очередь уже отправленных задач
            // будет постепенно обработана этим циклом.
            //
            // Здесь сохраняем ошибку и продолжаем ждать остальные.
            if (!parseException)
            {
                parseException = std::make_exception_ptr(
                    PipelineError(
                        filename,
                        pending.id,
                        e.what()));
            }
        }
        catch (...)
        {
            if (!parseException)
            {
                parseException = std::make_exception_ptr(
                    PipelineError(
                        filename,
                        pending.id,
                        "unknown exception"));
            }
        }
    }

    // Если ParseFile упал — отдаём исходную ошибку.
    if (parseException)
    {
        std::rethrow_exception(parseException);
    }

    return finalResult;
}



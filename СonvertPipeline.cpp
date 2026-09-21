#include "ÑonvertPipeline.h"

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
ConvertePipeline::ConvertePipeline(
    std::unique_ptr<LogParser> p,
    std::unique_ptr<LogSearch> s,
    std::unique_ptr<IConvertor> c,
    std::unique_ptr<IConvertOutput> o)
    : source(std::move(p)),
    parser(std::move(s)),
    converter(std::move(c)),
    output(std::move(o))
{
}

 void ConvertePipeline::run(
        const std::string & filename,
        ThreadPool & pool,
        const std::atomic<bool>*stopRequested)
    {
        const size_t BATCH_SIZE = 200;
        const size_t MAX_QUEUE = 10;

        struct PendingBatch
        {
            size_t id;
            std::future<std::vector<std::string>> future;
        };

        std::vector<std::string> buffer;
        buffer.reserve(BATCH_SIZE);

        std::deque<PendingBatch> futures;

        bool firstLine = true;
        size_t nextBatchId = 0;

        LogSearch* parserPtr = parser.get();

        auto get_result =
            [&](PendingBatch& pending)
            -> std::vector<std::string>
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

        auto write_result =
            [&](const std::vector<std::string>& result)
            {
                for (const auto& line : result)
                {
                    output->Write(line);
                }
            };

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

                    auto result = get_result(pending);

                    write_result(result);

                    futures.pop_front();
                }
            };

        auto wait_slot = [&]()
            {
                while (futures.size() >= MAX_QUEUE)
                {
                    drain_ready();

                    if (futures.size() < MAX_QUEUE)
                        break;

                    auto result =
                        get_result(futures.front());

                    write_result(result);

                    futures.pop_front();
                }
            };

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

                auto converterClone =
                    std::shared_ptr<IConvertor>(
                        converter->Clone());

                auto future = pool.submit(
                    0,
                    [batch = std::move(batch),
                    parser = parserClone,
                    converter = converterClone]()
                    {
                        std::vector<std::string> result;

                        result.reserve(batch.size());

                        for (const auto& line : batch)
                        {
                            auto entry =
                                parser->ParseLine(line);

                            result.push_back(
                                converter->Converte(entry));
                        }

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

                    if (firstLine &&
                        parserPtr->HasHeader())
                    {
                        parserPtr->Init(line);

                        firstLine = false;

                        return;
                    }

                    firstLine = false;

                    buffer.push_back(line);

                    if (buffer.size() == BATCH_SIZE)
                    {
                        submit_batch(
                            std::move(buffer));

                        buffer.clear();
                        buffer.reserve(BATCH_SIZE);
                    }
                },
                stopRequested);
        }
        catch (...)
        {
            parseException =
                std::current_exception();
        }

        if (!buffer.empty() &&
            !parseException &&
            !(stopRequested &&
                stopRequested->load()))
        {
            submit_batch(
                std::move(buffer));

            buffer.clear();
        }

        while (!futures.empty())
        {
            auto pending =
                std::move(futures.front());

            futures.pop_front();

            try
            {
                auto result =
                    pending.future.get();

                write_result(result);
            }
            catch (const std::exception& e)
            {
                if (!parseException)
                {
                    parseException =
                        std::make_exception_ptr(
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
                    parseException =
                        std::make_exception_ptr(
                            PipelineError(
                                filename,
                                pending.id,
                                "unknown exception"));
                }
            }
        }

        if (parseException)
        {
            std::rethrow_exception(
                parseException);
        }
    }


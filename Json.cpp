#include "Json.h"
#include <iostream>

void JsonParse::ParseFile(
    const std::string& filename,
    std::function<void(const std::string&)> online, const std::atomic<bool>* stopRequested)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "JSON open error\n";
        return;
    }

    std::string buffer;
    buffer.reserve(64 * 1024);

    char ch;

    int depth = 0;
    bool inString = false;
    bool escape = false;
    bool started = false;

    while (file.get(ch))
    {
        // Проверяем, не попросили ли нас остановиться
        if (stopRequested &&
            stopRequested->load())
        {
            break;
        }

        buffer.push_back(ch);

        if (ch == '"' && !escape)
            inString = !inString;

        if (!inString)
        {
            if (ch == '{') depth++;
            if (ch == '}') depth--;
        }

        escape = (ch == '\\' && !escape);

        
        if (depth == 0 && !buffer.empty())
        {
            
            if (buffer.front() == '{')
            {
                online(buffer);
            }

            buffer.clear();
        }
    }

    // ===== tail =====
    if (!buffer.empty())
    {
        online(buffer);
    }
}
LogEntry JsonLineParse::ParseLine(const std::string& line)
{
    try
    {
        json j = json::parse(line);
        LogEntry entry;

        for (auto& [key, value] : j.items())
        {
            if (!value.is_string()) continue;

            std::string val = value.get<std::string>();

            if (key == "timestamp")
                entry.timestamp = val;
            else if (key == "level")
                entry.level = val;
            else if (key == "message")
                entry.message = val;
            else
                entry.extraFields[key] = val; 
        }

        return entry;
    }
    catch (const json::parse_error& e)
    {
        std::cerr << "JSON parse error: "
            << e.what() << '\n';

        return {};
    }
}
JsonFilterbyLevel::JsonFilterbyLevel(std::string level,std::string startTime, std::string endTime)
{
    this->level = level;
    this->endTime = endTime;
    this->startTime = startTime;
}
bool JsonFilterbyLevel::FilterByLevel(const LogEntry& entry)
{
    if (!level.empty() && entry.level != level)
        return false;

    if (!startTime.empty())
    {
        if (!isTimeInRange(entry.timestamp, startTime, endTime))
            return false;
    }

    return true;
 }
void JsonAnalyzer::Proces(const LogEntry& entry)
{
    //result.levelCount[entry.level]++;

    //std::string word;
    //const std::string& text = entry.message;
    //for (char c : text)
    //{
    //    if (!isDelimiter(c))
    //    {
    //        word += std::tolower((unsigned char)c);
    //    }
    //    else
    //    {
    //        if (!word.empty())
    //        {
    //            std::string norm = normalize(word);

    //            bool matched = false;

    //            for (const auto& key : keyword)
    //            {
    //                if (startWith(norm, key) || containsKeyword(norm, key))
    //                {
    //                    result.keywordsCount[key]++;
    //                    matched = true;
    //                    break;
    //                }
    //            }
    //            if (!matched)
    //            {
    //                result.keywordsCount[norm]++;
    //            }

    //            word.clear();
    //        }
    //    }
    //}
    //if (!word.empty())
    //{
    //    std::string norm = normalize(word);

    //    bool matched = false;

    //    for (const auto& key : keyword)
    //    {
    //        if (startWith(norm, key) || containsKeyword(norm, key))
    //        {
    //            result.keywordsCount[key]++;
    //            matched = true;
    //            break;
    //        }
    //    }

    //    if (!matched)
    //    {
    //        result.keywordsCount[norm]++;
    //    }
    //}
    result.levelCount[entry.level]++;

    const std::string& text = entry.message;

    std::vector<std::string> words;
    std::string word;

    // ===== 1. SPLIT =====
    for (char c : text)
    {
        if (!isDelimiter(c))
        {
            word += std::tolower((unsigned char)c);
        }
        else
        {
            if (!word.empty())
            {
                words.push_back(normalize(word));
                word.clear();
            }
        }
    }

    if (!word.empty())
    {
        words.push_back(normalize(word));
    }

    // ===== 2. ОБРАБОТКА СЛОВ (твоя логика) =====
    for (const auto& w : words)
    {
        bool matched = false;

        for (const auto& key : keyword)
        {
            if (startWith(w, key) || containsKeyword(w, key))
            {
                result.keywordsCount[key]++;
                matched = true;
                break;
            }
        }

        if (!matched)
        {
            result.keywordsCount[w]++;
        }
    }

    // ===== 3. BIGRAM (НОВОЕ 🔥) =====
    for (size_t i = 0; i + 1 < words.size(); i++)
    {
        std::string bigram = words[i] + "_" + words[i + 1];

        result.keywordsCount[bigram]++;
    }
}
std::unique_ptr<LogSearch> JsonLineParse::Clone() const
{
    return std::make_unique<JsonLineParse>(*this);
}
std::unique_ptr<LogFilter>  JsonFilterbyLevel::Clone() const
{
    return std::make_unique<JsonFilterbyLevel>(*this);
}
JsonAnalyzer::JsonAnalyzer(std::set<std::string> keywords)
{
    this->keyword = keywords;
}
AnalysisResult JsonAnalyzer::GetResukt()
{
    return result;
}
std::unique_ptr<IAnalyzer> JsonAnalyzer::Clone() const
{
    return std::make_unique<JsonAnalyzer>(*this);
}
void JsonAnalyzer::Merge(const IAnalyzer& other)
{
    const auto& o = static_cast<const JsonAnalyzer&>(other);

    for (const auto& [k, v] : o.result.levelCount)
        result.levelCount[k] += v;

    for (const auto& [k, v] : o.result.keywordsCount)
        result.keywordsCount[k] += v;
}
void JsonOutput::write(const AnalysisResult& result)
{
    if (result.levelCount.empty() && result.keywordsCount.empty())
        return; 
    std::lock_guard<std::mutex> lock(cout_mtx);

    std::cout << "{\n";

    std::cout << "  \"levels\": {\n";
    for (auto it = result.levelCount.begin(); it != result.levelCount.end(); ++it)
    {
        std::cout << "    \"" << it->first << "\": " << it->second;

        if (std::next(it) != result.levelCount.end())
            std::cout << ",";

        std::cout << "\n";
    }
    std::cout << "  },\n";

    std::cout << "  \"keywords\": {\n";
    for (auto it = result.keywordsCount.begin(); it != result.keywordsCount.end(); ++it)
    {
        std::cout << "    \"" << it->first << "\": " << it->second;

        if (std::next(it) != result.keywordsCount.end())
            std::cout << ",";

        std::cout << "\n";
    }
    std::cout << "  }\n";

    std::cout << "}\n";
}
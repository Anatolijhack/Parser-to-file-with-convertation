#include "Csv.h"
#include <string>
#include <vector>


static std::vector<std::string> CSVsplit(const std::string& line, char delimiter)
{
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        char c = line[i];

        if (c == '"')
        {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"')
            {
                current += '"';
                ++i;
            }
            else
            {
                inQuotes = !inQuotes;
            }
        }
        else if (c == delimiter && !inQuotes)
        {
            result.push_back(current);
            current.clear();
        }
        else
        {
            current += c;
        }
    }

    result.push_back(current);
    return result;
}
static std::string escapeCSV(const std::string& s) {
    std::string escaped = "\"";
    for (char c : s) {
        if (c == '"') escaped += "\"\""; // двойные кавычки внутри строки
        else escaped += c;
    }
    escaped += "\"";
    return escaped;
}
void ParseCSV::ParseFile(const std::string& name, std::function<void(const std::string&)> online, const std::atomic<bool>* stopRequested)
{
    std::ifstream file(name);
    if (!file.is_open())
    {
        std::cout << "СSV isn't open" << std::endl;
    }

    std::string line;

    while (std::getline(file, line))
    {
        // Проверяем, не попросили ли нас остановиться
        if (stopRequested &&
            stopRequested->load())
        {
            break;
        }
        online(line);
    }
}

LogEntry ParseLineCsv::ParseLine(const std::string& line)
{
    auto values = CSVsplit(line, detectDelimiter(line));

    LogEntry entry;

    for (const auto& [name, index] : headerMap)
    {
        if (index >= values.size()) continue;

        const std::string& val = values[index];

        if (name == "level")
            entry.level = val;
        else if (name == "message")
            entry.message = val;
        else if (name == "timestamp")
            entry.timestamp = val;
        else
            entry.extraFields[name] = val;
    }

    return entry;
}
void ParseLineCsv::Init(const std::string& line)
{
    auto headers =
        CSVsplit(line, detectDelimiter(line));

    headerMap.clear();

    for (size_t i = 0; i < headers.size(); ++i)
    {
        headerMap[headers[i]] = i;
    }
}
std::unique_ptr<LogSearch> ParseLineCsv::Clone() const
{
    return std::make_unique<ParseLineCsv>(*this);
}
bool ParseLineCsv::HasHeader()
{
    return true;
}
CsvFilter::CsvFilter(std::string level, std::string startTime, std::string endTime)
{
    this->level = level;
    this->endTime = endTime;
    this->startTime = startTime;

}

bool CsvFilter::FilterByLevel(const LogEntry& entry)
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
std::unique_ptr<LogFilter> CsvFilter::Clone() const
{
    return std::make_unique<CsvFilter>(*this);
}
CsvAnalyzer::CsvAnalyzer(std::set<std::string> keywords)
{
    this->keyword = keywords;
}std::unique_ptr<IAnalyzer>  CsvAnalyzer::Clone() const
{
    return std::make_unique<CsvAnalyzer>(*this);
}
void CsvAnalyzer::Merge(const IAnalyzer& other)
{
    const auto& o = static_cast<const CsvAnalyzer&>(other);

    for (const auto& [k, v] : o.result.levelCount)
        result.levelCount[k] += v;

    for (const auto& [k, v] : o.result.keywordsCount)
        result.keywordsCount[k] += v;
}
void CsvAnalyzer::Proces(const LogEntry& entry)
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
AnalysisResult CsvAnalyzer::GetResukt()
{
    return result;
}
void CsvOutput::write(const AnalysisResult& result)
{
    std::lock_guard<std::mutex> lock(cout_mtx);
    std::cout << "LEVEL,COUNT\n";
    for (const auto& pair : result.levelCount)
    {
        auto& level = pair.first;
        auto& count = pair.second;

        std::cout << "LEVEL" << escapeCSV(level) << "," << count << "\n";
    }

    std::cout << "\n";


    std::cout << "WORD,COUNT\n";
    for (const auto& pair : result.keywordsCount)
    {
        auto& word = pair.first;
        auto& count = pair.second;
        std::cout << "KEYWORD" << escapeCSV(word) << ',' << count << "\n";
    }
}

std::string CsvConverter::Converte(const LogEntry& entry)
{
    std::string result =
        escapeCSV(entry.timestamp) + delimiter +
        escapeCSV(entry.level) + delimiter +
        escapeCSV(entry.message);

    for (const auto& [key, value] : entry.extraFields)
    {
        result += delimiter;
        result += escapeCSV(value);
    }

    return result;
}
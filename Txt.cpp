#include "Txt.h"
#include <fstream>
#include <string>


void TxtParser::ParseFile(const std::string& filenamem, std::function<void(const std::string&)> online, const std::atomic<bool>* stopRequested)
{
	std::ifstream file(filenamem);
	if (!file.is_open())
	{
		std::cerr << "Ошибка открытия" << std::endl;
		return;
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
LogEntry LineParser::ParseLine(const std::string& line)
{
	LogEntry e;

	
	if (line.find("ERROR") != std::string::npos)
		e.level = "ERROR";
	else if (line.find("INFO") != std::string::npos)
		e.level = "INFO";
	else if (line.find("WARNING") != std::string::npos)
		e.level = "WARNING";
	else
		e.level = "UNKNOWN";

	
	size_t t = line.find("202");
	if (t != std::string::npos)
	{
		e.timestamp = line.substr(t, 19); 
	}
	else
	{
		e.timestamp = "";
	}

	
	e.message = line;

	return e;
}
std::unique_ptr<LogSearch> LineParser::Clone() const
{
	return std::make_unique<LineParser>(*this);
}
LevelFilter::LevelFilter(std::string level, std::string startTime, std::string endTime)
{
	this->level = std::move(level);
	this->endTime = endTime;
	this->startTime = startTime;
}
bool LevelFilter::FilterByLevel(const LogEntry& entry)
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
std::unique_ptr<LogFilter> LevelFilter::Clone() const
{
	return std::make_unique<LevelFilter>(*this);
}
void ConsoleOutput::write(const AnalysisResult& result)
{
	std::lock_guard<std::mutex> lock(cout_mtx);

	
	for (auto& pair : result.levelCount)
	{
		auto lvl = pair.first;
		auto count = pair.second;

		std::cout << lvl << ":" << count << "\n";
	}
	
	for (auto& pair : result.keywordsCount)
	{
		auto word = pair.first;
		auto count = pair.second;
		std::cout << word << ":" << count << "\n";
	}
}
std::unique_ptr<IAnalyzer>  Analyzer::Clone() const
{
	return std::make_unique<Analyzer>(*this);
}
void Analyzer::Merge(const IAnalyzer& other)
{
	const auto& o = static_cast<const Analyzer&>(other);

	for (const auto& [k, v] : o.result.levelCount)
		result.levelCount[k] += v;

	for (const auto& [k, v] : o.result.keywordsCount)
		result.keywordsCount[k] += v;
}
Analyzer::Analyzer(const std::set<std::string>& keyword)
{
	this->keyword = keyword;
}
void Analyzer::Proces(const LogEntry& entry)
{
	
	
	//result.levelCount[entry.level]++;

	//std::string word;
	//const std::string& text = entry.message; 
	//for (char c : text)
	//{
	//	if (!isDelimiter(c))
	//	{
	//		word += std::tolower((unsigned char)c);
	//	}
	//	else
	//	{
	//		if (!word.empty())
	//		{
	//			std::string norm = normalize(word);

	//			bool matched = false;

	//			for (const auto& key : keyword)
	//			{
	//				if (startWith(norm, key) || containsKeyword(norm, key))
	//				{
	//					result.keywordsCount[key]++;
	//					matched = true;
	//					break;
	//				}
	//			}
	//			if (!matched)
	//			{
	//				result.keywordsCount[norm]++;
	//			}

	//			word.clear();
	//		}
	//	}
	//}
	//if (!word.empty())
	//{
	//	std::string norm = normalize(word);

	//	bool matched = false;

	//	for (const auto& key : keyword)
	//	{
	//		if (startWith(norm, key) || containsKeyword(norm, key))
	//		{
	//			result.keywordsCount[key]++;
	//			matched = true;
	//			break;
	//		}
	//	}

	//	if (!matched)
	//	{
	//		result.keywordsCount[norm]++;
	//	}
	//}
	result.levelCount[entry.level]++;

	const std::string& text = entry.message;

	std::vector<std::string> words;
	std::string word;

	
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
AnalysisResult Analyzer::GetResukt()
{
	return result;
}

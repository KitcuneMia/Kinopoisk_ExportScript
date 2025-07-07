#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <filesystem>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

std::string cleanHtmlEntities(const std::string& text) {
    std::string cleaned = text;
    cleaned = std::regex_replace(cleaned, std::regex("&nbsp;"), " ");
    cleaned = std::regex_replace(cleaned, std::regex("&amp;"), "&");
    cleaned = std::regex_replace(cleaned, std::regex("&quot;"), "\"");
    cleaned = std::regex_replace(cleaned, std::regex("&lt;"), "<");
    cleaned = std::regex_replace(cleaned, std::regex("&gt;"), ">");
    return cleaned;
}

void processHtmlFile(const std::string& filename, std::ofstream& csvFile) {
    std::ifstream htmlFile(filename);
    if (!htmlFile.is_open()) {
        std::cerr << "Не удалось открыть файл: " << filename << std::endl;
        return;
    }

    std::string line, htmlContent;
    while (std::getline(htmlFile, line)) {
        htmlContent += line + "\n";
    }
    htmlFile.close();

    std::regex itemRegex(R"(<div class="item(?:\s+even)?">([\s\S]*?)<\/div>\s*<div class="clear"></div>)");
    std::regex numRegex(R"(<div class="num">\s*(\d+)\s*<\/div>)");
    std::regex nameRusRegex(R"(<div class="nameRus">\s*<a[^>]*>([^<]+)<\/a>)");
    std::regex nameEngRegex(R"(<div class="nameEng">\s*([^<]*)\s*<\/div>)");
    std::regex ratingRegex(R"(<div[^>]*class="[^"]*myVote[^"]*"[^>]*>\s*(\d+)\s*<\/div>)");

    auto itemsBegin = std::sregex_iterator(htmlContent.begin(), htmlContent.end(), itemRegex);
    auto itemsEnd = std::sregex_iterator();

    for (auto it = itemsBegin; it != itemsEnd; ++it) {
        std::string itemBlock = (*it)[1].str();

        std::smatch matchNum, matchRus, matchEng, matchRating;

        std::string num = "";
        std::string nameRus = "";
        std::string nameEng = "";
        std::string rating = "";

        if (std::regex_search(itemBlock, matchNum, numRegex)) {
            num = matchNum[1];
        }

        if (std::regex_search(itemBlock, matchRus, nameRusRegex)) {
            nameRus = cleanHtmlEntities(matchRus[1]);
        }

        if (std::regex_search(itemBlock, matchEng, nameEngRegex)) {
            nameEng = cleanHtmlEntities(matchEng[1]);
        }

        if (std::regex_search(itemBlock, matchRating, ratingRegex)) {
            rating = matchRating[1];
        }

        nameRus = std::regex_replace(nameRus, std::regex("\""), "\"\"");
        nameEng = std::regex_replace(nameEng, std::regex("\""), "\"\"");

        csvFile << num << ";\"" << nameRus << "\";\"" << nameEng << "\";" << rating << "\n";
    }

    std::cout << "Файл обработан: " << filename << std::endl;
}

int extractPageNumber(const std::string& filename) {
    std::smatch match;
    if (std::regex_search(filename, match, std::regex(R"(kinopoisk_page(\d+)\.html)"))) {
        return std::stoi(match[1]);
    }
    return -1;
}

int main() {
    setlocale(LC_ALL, "Ru");

    std::vector<std::string> htmlFiles;

    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find("kinopoisk_page") == 0 && filename.find(".html") != std::string::npos) {
                htmlFiles.push_back(filename);
            }
        }
    }

    std::sort(htmlFiles.begin(), htmlFiles.end(), [](const std::string& a, const std::string& b) {
        return extractPageNumber(a) < extractPageNumber(b);
        });

    std::ofstream csvFile("films.csv");
    if (!csvFile.is_open()) {
        std::cerr << "Не удалось создать файл CSV!" << std::endl;
        return 1;
    }

    csvFile << "Number;Ru;Eng;MyRating\n";

    for (const auto& filename : htmlFiles) {
        processHtmlFile(filename, csvFile);
    }

    csvFile.close();
    std::cout << "Таблица сохранена в файле films.csv" << std::endl;

    std::cin.get();

    return 0;
}

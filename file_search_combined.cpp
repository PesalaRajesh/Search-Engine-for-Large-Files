#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <regex>
#include <algorithm>
using namespace std;

mutex printMutex;

// Convert string to lowercase
string toLowerCase(const string& str) {
    string lowerStr = str;
    transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

// Search function for keyword (used by each thread)
void searchChunkKeyword(const vector<string>& lines, size_t startLine, const string& keywordLower, bool caseSensitive) {
    for (size_t i = 0; i < lines.size(); ++i) {
        string line = lines[i];
        string targetLine = caseSensitive ? line : toLowerCase(line);
        if (targetLine.find(keywordLower) != string::npos) {
            lock_guard<mutex> lock(printMutex);
            cout << "Line " << startLine + i + 1 << ": " << line << endl;
        }
    }
}

// Search function for regex (used by each thread)
void searchChunkRegex(const vector<string>& lines, size_t startLine, const regex& pattern) {
    for (size_t i = 0; i < lines.size(); ++i) {
        if (regex_search(lines[i], pattern)) {
            lock_guard<mutex> lock(printMutex);
            cout << "Line " << startLine + i + 1 << ": " << lines[i] << endl;
        }
    }
}

// Multi-threaded file search (Unified Handler)
void searchInFile(const string& filePath, const string& keyword, bool caseSensitive, int numThreads, bool useRegex) {
    ifstream inputFile(filePath);
    if (!inputFile.is_open()) {
        cerr << "Error opening file: " << filePath << endl;
        return;
    }

    vector<string> lines;
    string line;
    while (getline(inputFile, line)) {
        lines.push_back(line);
    }
    inputFile.close();

    size_t totalLines = lines.size();
    size_t chunkSize = totalLines / numThreads;

    vector<thread> threads;

    if (useRegex) {
        regex pattern(keyword);
        for (int i = 0; i < numThreads; ++i) {
            size_t start = i * chunkSize;
            size_t end = (i == numThreads - 1) ? totalLines : (i + 1) * chunkSize;
            vector<string> chunk(lines.begin() + start, lines.begin() + end);

            threads.emplace_back(searchChunkRegex, chunk, start, pattern);
        }
    } else {
        string keywordLower = caseSensitive ? keyword : toLowerCase(keyword);
        for (int i = 0; i < numThreads; ++i) {
            size_t start = i * chunkSize;
            size_t end = (i == numThreads - 1) ? totalLines : (i + 1) * chunkSize;
            vector<string> chunk(lines.begin() + start, lines.begin() + end);

            threads.emplace_back(searchChunkKeyword, chunk, start, keywordLower, caseSensitive);
        }
    }

    for (auto& t : threads) {
        t.join();
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage:" << endl;
        cerr << "  Keyword Search: " << argv[0] << " <file path> keyword <keyword> <case-sensitive: 0/1> <num-threads>" << endl;
        cerr << "  Regex Search:   " << argv[0] << " <file path> regex <pattern> <num-threads>" << endl;
        return 1;
    }

    string filePath = argv[1];
    string searchType = argv[2];

    if (searchType == "keyword") {
        if (argc != 6) {
            cerr << "Incorrect arguments for keyword search." << endl;
            return 1;
        }
        string keyword = argv[3];
        bool caseSensitive = (stoi(argv[4]) != 0);
        int numThreads = stoi(argv[5]);

        searchInFile(filePath, keyword, caseSensitive, numThreads, false);
    }
    else if (searchType == "regex") {
        if (argc != 5) {
            cerr << "Incorrect arguments for regex search." << endl;
            return 1;
        }
        string pattern = argv[3];
        int numThreads = stoi(argv[4]);

        searchInFile(filePath, pattern, false, numThreads, true);
    }
    else {
        cerr << "Invalid search type. Use 'keyword' or 'regex'." << endl;
        return 1;
    }

    return 0;
}

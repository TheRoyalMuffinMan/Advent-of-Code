#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <utility>
#include <iostream>
#include <getopt.h>
#include <functional>
#include <unordered_set>
#include <unordered_map>

// Set to 0 to disable debugging
#define DDEBUG 1

typedef struct {
    std::unordered_map<std::int32_t, std::unordered_set<std::int32_t>> order;
    std::vector<std::vector<std::int32_t>> prints;
} InputData;

std::int32_t checkIfCanPrint(std::size_t index, 
                     std::vector<std::int32_t>& pages,
                     std::unordered_map<std::int32_t, std::unordered_set<std::int32_t>>& order) {
    for (std::size_t i = index + 1; i < pages.size(); i++) {
        if (order[pages[index]].find(pages[i]) != order[pages[index]].end()) {
            return i;
        } 
    }
    return -1;
}

std::int32_t solvePartOne(std::unique_ptr<InputData>& data) {
    std::int32_t answer = 0;
    for (auto print : data->prints) {
        bool isPrintable = true;
        for (std::size_t i = 0; i < print.size(); i++) {
            if (checkIfCanPrint(i, print, data->order) != -1) {
                isPrintable = false;
                break;
            } 
        }
        if (isPrintable) {
            answer += print[print.size() / 2];
        }
    } 
    std::cout << "Part 1 Answer: " << answer << std::endl;

    return answer;
}

void solvePartTwo(std::unique_ptr<InputData>& data, std::int32_t prevAnswer) {
    std::int32_t answer = 0;
    for (auto print : data->prints) {
        for (std::size_t i = 0; i < print.size(); i++) {
            std::int32_t j = -1; 
            while ((j = checkIfCanPrint(i, print, data->order)) != -1) {
                std::swap(print[i], print[j]); 
            } 
        }
        answer += print[print.size() / 2];
    } 
    std::cout << "Part 2 Answer: " << answer - prevAnswer << std::endl;
}

void usage(char* argv[]) {
    std::cout << "Usage: " << argv[0] <<" [OPTIONS]" << std::endl
              << "Options:" << std::endl
              << "  -f, --file <file>    Advent of code sample file" << std::endl;

    return;
}

std::string parseArgs(std::int32_t& argc, char* argv[]) {
    struct option longOptions[] = {{"file", required_argument, NULL, 'f'}};
    std::string filename;
    char flag;

    while ((flag = getopt_long(argc, argv, "f:", longOptions, NULL)) != -1) {
        switch (flag) {
            case 'f':
                filename = std::string(optarg);
                break;
            default:
                usage(argv);
        }
    }

    if (filename.empty()) {
        usage(argv);
        std::exit(EXIT_FAILURE);
    }

    return filename;
}

void parseFile(std::string& filename, 
               std::unique_ptr<InputData>& data, 
               std::function<void(std::string, std::unique_ptr<InputData>&)> func) {

    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error: Couldn't open file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    while (std::getline(file, line)) {
        func(line, data);
    }

    return;
}

std::vector<std::int32_t> split(std::string line, char delim) {
    std::vector<std::int32_t> result;
    std::stringstream ss(line);
    std::string item;

    while (getline(ss, item, delim)) {
        result.push_back(std::stoi(item));
    }
    
    return result;
}

void customParse(std::string line, std::unique_ptr<InputData>& data) {
    std::vector<std::int32_t> result;
    if (line.find('|') != std::string::npos) {
        result = split(line, '|');
        data->order[result[1]].insert(result[0]);
    }
    if (line.find(',') != std::string::npos) {
        result = split(line, ',');
        data->prints.push_back(result);
    }
    return;
}

int main(std::int32_t argc, char* argv[]) {
    std::unique_ptr<InputData> data = std::make_unique<InputData>();
    std::string filename = parseArgs(argc, argv);
    parseFile(filename, data, customParse);

    // Day 1: 
    int32_t prevAnswer = solvePartOne(data);

    // Day 2: 
    solvePartTwo(data, prevAnswer);
}

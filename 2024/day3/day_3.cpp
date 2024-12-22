#include <regex>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <iostream>
#include <getopt.h>
#include <functional>

// Set to 0 to disable debugging
#define DDEBUG 1

typedef struct {
    std::string code;
} InputData;

void solvePartOne(std::unique_ptr<InputData>& data) {
    std::int32_t answer = 0;
    std::string const code = data->code;
    std::regex const pattern(R"(mul\((\d+),(\d+)\))");

    for (std::sregex_iterator it{code.begin(), code.end(), pattern}, end{}; it != end; it++) {
        std::int32_t left = std::stoi(it->str(1));
        std::int32_t right = std::stoi(it->str(2));
        answer += left * right;
    }

    std::cout << "Part 1 Answer: " << answer << std::endl;
}

void solvePartTwo(std::unique_ptr<InputData>& data) {

    std::cout << "Part 2 Answer: " << 1 << std::endl;
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
        exit(EXIT_FAILURE);
    }

    return filename;
}

void parseFile(std::string& filename, 
               std::unique_ptr<InputData>& data, 
               std::function<void(std::istringstream, std::unique_ptr<InputData>&)> func) {

    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error: Couldn't open file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    while (std::getline(file, line)) {
        func(std::istringstream(line), data);
    }

    return;
}

void dayOneParse(std::istringstream iss, std::unique_ptr<InputData>& data) {
    std::string line;
    while (iss >> line) {
        data->code += line;
    }
    return;
}

int main(std::int32_t argc, char* argv[]) {
    std::unique_ptr<InputData> data = std::make_unique<InputData>();
    std::string filename = parseArgs(argc, argv);
    parseFile(filename, data, dayOneParse);

    // Day 1: Find safe levels
    solvePartOne(data);

    // Day 2: Find safe levels but can remove one bad level
    solvePartTwo(data);
}
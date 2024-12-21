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
    std::vector<std::vector<std::int32_t>> mat;
} InputData;

void solvePartOne(std::unique_ptr<InputData>& data) {
    std::int32_t safeLevel = 0;
    for (std::size_t i = 0; i < data->mat.size(); i++) {
        bool isSafe = true;
        std::int32_t state = -1; // 0 => decreasing, 1 => increasing
        for (std::size_t j = 0; j < data->mat[i].size() - 1; j++) {

            if (state == -1) {
                if (data->mat[i][j] == data->mat[i][j + 1]) {
                    isSafe = false;
                    break;
                }
                state = data->mat[i][j] < data->mat[i][j + 1];
            }

            if (state && data->mat[i][j] > data->mat[i][j + 1]) {
                isSafe = false;
                break;
            }

            if (!state && data->mat[i][j] < data->mat[i][j + 1]) {
                isSafe = false;
                break;
            }

            std::int32_t diff = std::abs(data->mat[i][j] - data->mat[i][j + 1]);
            if (diff < 1 || diff > 3) {
                isSafe = false;
                break;
            }
        }

        safeLevel += isSafe;
    }

    std::cout << "Part 1 Answer: " << safeLevel << std::endl;
}

void solvePartTwo(std::unique_ptr<InputData>& data) {
    std::int32_t safeLevel = 0;
    for (std::size_t i = 0; i < data->mat.size(); i++) {
        bool isSafe = true, levelRemoved = false;
        std::int32_t state = -1; // 0 => decreasing, 1 => increasing
        for (std::size_t j = 0; j < data->mat[i].size() - 1; j++) {
            if (state == -1) {
                if (data->mat[i][j] == data->mat[i][j + 1]) {
                    if (!levelRemoved) {
                        levelRemoved = true;
                        continue;
                    }
                    isSafe = false;
                    break;
                }
                state = data->mat[i][j] < data->mat[i][j + 1];
            }

            if (state && data->mat[i][j] > data->mat[i][j + 1]) {
                if (!levelRemoved) {
                    levelRemoved = true;
                    continue;
                }
                isSafe = false;
                break;
            }

            if (!state && data->mat[i][j] < data->mat[i][j + 1]) {
                if (!levelRemoved) {
                    levelRemoved = true;
                    continue;
                }
                isSafe = false;
                break;
            }

            std::int32_t diff = std::abs(data->mat[i][j] - data->mat[i][j + 1]);
            if (diff < 1 || diff > 3) {
                if (!levelRemoved) {
                    levelRemoved = true;
                    continue;
                }
                isSafe = false;
                break;
            }
        }

        safeLevel += isSafe;
    }

    std::cout << "Part 2 Answer: " << safeLevel << std::endl;
}

void usage(char* argv[]) {
    std::cout << "Usage: " << argv[0] <<" [OPTIONS]" << std::endl
              << "Options:" << std::endl
              << "  -f, --file <file>    Advent of code sample file" << std::endl
              << "  -debug, --debug      Sets debug flag" << std::endl;

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
    std::vector<std::int32_t> row;
    std::int32_t num = 0;
    while (iss >> num) {
        row.push_back(num);
    }
    data->mat.push_back(row);
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
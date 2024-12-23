#include <regex>
#include <queue>
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
    std::vector<std::string> grid;
} InputData;
typedef struct {
    std::int32_t row;
    std::int32_t col;
    char current;
} Element;
std::vector<std::pair<std::int32_t, std::int32_t>> ADJACENT = {
    {0, 1}, {0, -1}, {1, 0}, {-1, 0},
    {1, 1}, {-1, 1}, {1, -1}, {-1, -1} 
};
std::unordered_map<char, char> NEXT_CHAR = {
    {'X', 'M'}, {'M', 'A'}, {'A', 'S'}, {'S', '!'}
};

std::int32_t foundXMAS(std::unique_ptr<InputData>& data, Element start) {
    std::int32_t answer = 0;
    std::queue<Element> positions;
    std::unordered_set<std::string> seen;
    positions.push(start);
    auto isValid = [](std::unique_ptr<InputData>& data, std::int32_t row, std::int32_t col) {
        return row >= 0 && row < data->grid.size() && col >= 0 && col < data->grid[0].size();
    };
    std::cout << start.row << ", " << start.col << ", " << start.current << std::endl;

    while (!positions.empty()) {
        Element top = positions.front();
        std::string lookup = std::to_string(top.row) + std::to_string(top.col);
        positions.pop();

        if (seen.find(lookup) == seen.end()) {
            continue;
        }
        seen.insert(lookup);

        for (auto adj : ADJACENT) {
            std::int32_t rowF = top.row + adj.first;
            std::int32_t colF = top.col + adj.second;

            // Check if the next character is valid
            if (isValid(data, rowF, colF) && NEXT_CHAR.find(data->grid[rowF][colF]) != NEXT_CHAR.end()) {
                if (NEXT_CHAR[data->grid[rowF][colF]] == '!') {
                    answer++;
                    continue;
                }
                positions.push({rowF, colF, data->grid[rowF][colF]});
            }
        }
    }

    return answer;
}

void solvePartOne(std::unique_ptr<InputData>& data) {
    std::vector<std::pair<std::int32_t, std::int32_t>> X_locations;
    std::int32_t answer = 0;

    // Locate the X positions in the grid
    for (std::size_t row = 0; row < data->grid.size(); row++) {
        for (std::size_t col = 0; col < data->grid[row].size(); col++) {
            if (data->grid[row][col] == 'X') {
                X_locations.push_back({row, col});
            }
        }
    }

    // Run search X->M->A->S
    for (auto start : X_locations) {
        answer += foundXMAS(data, {start.first, start.second, 'X'});
    }

    std::cout << "Part 1 Answer: " << answer << std::endl;
}

void solvePartTwo(std::unique_ptr<InputData>& data) {
    std::int32_t answer = 0;

    std::cout << "Part 2 Answer: " << answer << std::endl;
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
    std::string row;
    while (iss >> row) {
        data->grid.push_back(row);
    }
    return;
}

int main(std::int32_t argc, char* argv[]) {
    std::unique_ptr<InputData> data = std::make_unique<InputData>();
    std::string filename = parseArgs(argc, argv);
    parseFile(filename, data, dayOneParse);

    // Day 1: 
    solvePartOne(data);

    // Day 2: 
    solvePartTwo(data);
}
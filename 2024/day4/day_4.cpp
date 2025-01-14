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
#define XMAS_LENGTH 4

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

auto isValid = [](std::unique_ptr<InputData>& data, std::int32_t row, std::int32_t col) {
    return row >= 0 && row < data->grid.size() && col >= 0 && col < data->grid[0].size();
};

std::int32_t foundXMAS(std::unique_ptr<InputData>& data, Element start) {
    std::int32_t answer = 0;

    for (auto adj : ADJACENT) {
        std::int32_t nR = start.row, nC = start.col;
        char prev = start.current;
        for (std::size_t dist = 1; dist < XMAS_LENGTH; dist++) {
            nR += adj.first, nC += adj.second;
            if (!isValid(data, nR, nC)) break; 
            if (dist == XMAS_LENGTH - 1 && data->grid[nR][nC] == 'S') answer++;
            if (data->grid[nR][nC] != NEXT_CHAR[prev]) break;
            prev = data->grid[nR][nC];
        }
    }

    return answer;
}

std::int32_t checkXMAS(std::unique_ptr<InputData>& data, Element middle) {
    std::unordered_set<std::string> xmas = {"MAS", "SAM"};
    std::string left = "", right = "";

    if (isValid(data, middle.row - 1, middle.col - 1) && isValid(data, middle.row - 1, middle.col + 1)) {
        left += data->grid[middle.row - 1][middle.col - 1], right += data->grid[middle.row - 1][middle.col + 1];
    }
    left += middle.current, right += middle.current; 
    if (isValid(data, middle.row + 1, middle.col - 1) && isValid(data, middle.row + 1, middle.col + 1)) {
        right += data->grid[middle.row + 1][middle.col - 1], left += data->grid[middle.row + 1][middle.col + 1];
    }

    if (left.size() != XMAS_LENGTH - 1 || right.size() != XMAS_LENGTH - 1 || xmas.find(left) == xmas.end() || xmas.find(right) == xmas.end()) {
        return 0;
    }

    return 1;
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
    std::vector<std::pair<std::int32_t, std::int32_t>> A_locations;
    std::int32_t answer = 0;


    // Locate the A positions in the grid
    for (std::size_t row = 0; row < data->grid.size(); row++) {
        for (std::size_t col = 0; col < data->grid[row].size(); col++) {
            if (data->grid[row][col] == 'A') {
                A_locations.push_back({row, col});
            }
        }
    }

    // Run X-MAS check at each A 
    for (auto start : A_locations) {
        answer += checkXMAS(data, {start.first, start.second, 'A'});
    }

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

#include <iostream>
#include "config.hpp"

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Please specify a config file." << std::endl;
        exit(0);
    }

    Config conf = Config(argv[1]);

    return 0;
}
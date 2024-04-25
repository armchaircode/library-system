#pragma once

#include <vector>
#include <unordered_map>
#include <string>

struct Book {
    inline static std::vector<std::string> info {
        "isbn",
        "title",
        "author",
        "publisher"
    };
    std::unordered_map<std::string, std::string> data;
    unsigned int quantity;
    float rating;
};

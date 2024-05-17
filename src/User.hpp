#pragma once

#include <string>
#include <vector>

enum class UserClass{
    ADMIN,
    NORMAL
};

struct User {
    std::string email;
    std::string username;
    UserClass type;
};

typedef std::vector<User> Users;

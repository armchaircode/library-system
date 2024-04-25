#pragma once

#include "ftxui/screen/screen.hpp"
#include "User.hpp"

#include <memory>

class App {
 // Main app
    public:
        int run();
        std::unique_ptr<User> active_user;
    private:
        void login();
};

inline std::unique_ptr<App> ap;

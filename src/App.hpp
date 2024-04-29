#pragma once

#include "User.hpp"

#include <memory>
#include <filesystem>

// Main app
class App {
    public:
        int run();
        std::string session;
        std::filesystem::path session_file;
    private:
        void login();
        std::unique_ptr<User> active_user;
};

inline std::unique_ptr<App> ap;

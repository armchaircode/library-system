#pragma once

#include "User.hpp"

#include <ftxui/component/screen_interactive.hpp>
#include <memory>
#include <filesystem>

// Main app
class App {
    public:
        int run();
        bool newSession = false;
        std::filesystem::path session_file;
    private:
        void login();
        void home();
        void attemptRestore();
        std::unique_ptr<User> active_user;
        inline static ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();
        void clearSession();
        void clearSessionFile();
        void writeSessionFile(const std::size_t session);
        void saveSession();

        std::vector<std::thread> dumped_threads;
};

inline std::unique_ptr<App> ap;

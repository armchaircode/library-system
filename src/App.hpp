#pragma once

#include "Book.hpp"
#include "User.hpp"

#include <ftxui/component/component.hpp>
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
        int entryMenuSize = 70;
        void login();
        void home();
        void adminHome();
        void normalHome();
        void attemptRestore();
        std::unique_ptr<User> active_user;
        inline static ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();
        void clearSession();
        void clearSessionFile();
        void writeSessionFile(const std::size_t session);
        void saveSession();
        ftxui::Component bookDetail(const BookStack& books, const int& selector);
        ftxui::Component userDetail(const Users& users, const int& selector);
        bool isSearchResult(const Book& book, const std::string& searchString);
        bool isSearchResult(const User& usr, const std::string& searchString);
};

inline std::unique_ptr<App> ap;

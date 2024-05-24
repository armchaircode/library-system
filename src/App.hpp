#pragma once

#include "Book.hpp"
#include "User.hpp"

#include "ftxui/component/screen_interactive.hpp"

#include <memory> // unique_ptr
#include <filesystem> // path

// Main app
class App {
    public:
        int run();

        bool newSession = false;
        std::filesystem::path session_file;
    private:
        void login();

        void attemptRestore();

        void home();
        void adminHome();
        void normalHome();

        void clearSession();
        void clearSessionFile();
        void writeSessionFile(const std::size_t session);
        void saveSession();

        ftxui::Component bookDetail(const BookStack& books, const int& selector);
        ftxui::Component userDetail(const Users& users, const int& selector);

        ftxui::Component label(const std::string txt);

        bool isSearchResult(const BookPtr& book, const std::string& searchString);
        bool isSearchResult(const UserPtr& usr, const std::string& searchString);

        ftxui::Component accountMgmtScreen(std::string& new_password, bool& password_change_success, bool& deleting_account);

        int entryMenuSize = 70;
        inline static ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();
        std::unique_ptr<User> active_user;
};

inline std::unique_ptr<App> ap;

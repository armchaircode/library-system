#include "ftxui/screen/screen.hpp"
#include <algorithm>
#include <memory>

class App {
 // Main app
    public:
        int run();
};

inline std::unique_ptr<App> ap;

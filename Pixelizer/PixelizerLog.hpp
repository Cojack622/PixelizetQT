#pragma once
#include <string>
#include <fstream>
#include <stdexcept>

namespace Pixelizer {
    class PixLogger {
    private:
        std::ofstream fileOut;

    public:
        explicit PixLogger(const std::string& filename)
        {
            fileOut.open(filename);
            if (!fileOut.is_open()) {
                throw std::runtime_error("Failed to open log file: " + filename);
            }
        }

        PixLogger(const PixLogger&) = delete;
        PixLogger& operator=(const PixLogger&) = delete;

        PixLogger(PixLogger&&) = default;
        PixLogger& operator=(PixLogger&&) = default;

        void Log(const std::string& message) {
            fileOut << message << '\n';
            fileOut.flush(); // optional
        }
    };
}

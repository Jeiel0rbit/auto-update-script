#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

// Color definitions for console output on Linux systems.
#define COLOR_RESET   "\033[0m"
#define COLOR_INFO    "\033[33m" // Yellow
#define COLOR_LOG     "\033[32m" // Green
#define COLOR_ERROR   "\033[31m" // Red
#define COLOR_WARNING "\033[90m" // Gray

/**
 * @brief Executes a shell command and displays the output in real-time with colors.
 * @param cmd The command to be executed.
 * @param prefix A prefix for each log line (e.g., "[UPDATE] ").
 * @param color The color to be used for the standard output.
 * @param stop_on_error If true, stops execution upon detecting an error in the output.
 * @return int 0 on success, or a non-zero error code.
 */
int exec_cmd_realtime(const std::string &cmd, const std::string &prefix, const std::string &color, bool stop_on_error = false) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << COLOR_ERROR << "[ERROR] Failed to execute command: " << cmd << COLOR_RESET << std::endl;
        return -1;
    }

    char buffer[256];
    bool error_detected = false;

    // Keywords to be ignored in the apt output for a cleaner log.
    const std::vector<std::string> ignore_keywords = {
        "Hit:", "Ign:", "Get:", "Reading package lists...", "Building dependency tree...", "Reading state information..."
    };

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);

        // Remove the newline from the end, if it exists.
        if (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
        if (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
        
        // Skip empty lines.
        if (line.empty()) {
            continue;
        }

        // Check if the line should be ignored.
        bool should_ignore = false;
        for (const auto& keyword : ignore_keywords) {
            if (line.find(keyword) == 0) { // Check if the line starts with the keyword.
                should_ignore = true;
                break;
            }
        }
        if (should_ignore) {
            continue;
        }

        // Handle specific apt warnings.
        if (line.find("WARNING: apt does not have a stable CLI interface.") != std::string::npos) {
            std::cout << COLOR_WARNING << prefix << line << COLOR_RESET << std::endl;
            continue;
        }

        // Handle generic warnings.
        if (line.find("warning") != std::string::npos || line.find("Warning") != std::string::npos) {
            std::cout << COLOR_WARNING << prefix << line << COLOR_RESET << std::endl;
            continue;
        }

        // Handle errors.
        if (line.find("error") != std::string::npos || line.find("failed") != std::string::npos ||
            line.find("Error") != std::string::npos || line.find("Failed") != std::string::npos ||
            line.find("E:") == 0) {
            error_detected = true;
            std::cerr << COLOR_ERROR << prefix << line << COLOR_RESET << std::endl;
            if (stop_on_error) break;
            continue;
        }

        // Print the standard output with the defined color.
        std::cout << color << prefix << line << COLOR_RESET << std::endl;
    }

    int ret_code = pclose(pipe);

    if (ret_code != 0) {
        std::cerr << COLOR_ERROR << prefix << "[ERROR] Command returned exit code " << (ret_code / 256) << COLOR_RESET << std::endl;
        return ret_code;
    }

    if (error_detected) {
        std::cerr << COLOR_ERROR << prefix << "[ERROR] An error was detected in the command output." << COLOR_RESET << std::endl;
        return -2; // Custom error code for error detected in log.
    }

    return 0;
}

/**
 * @brief Prints a visual separator in the console to organize the output.
 * @param title The title to be displayed in the separator.
 */
void print_separator(const std::string &title) {
    std::string line(50, '=');
    std::cout << "\n" << COLOR_INFO << line << std::endl;
    std::cout << "[ " << title << " ]" << std::endl;
    std::cout << line << COLOR_RESET << "\n" << std::endl;
}

/**
 * @brief Detects if the Linux distribution is Ubuntu or Debian by reading the /etc/os-release file.
 * @return std::string "ubuntu", "debian", or an empty string if it's neither.
 */
std::string detect_linux_distro() {
    std::ifstream file("/etc/os-release");
    if (!file.is_open()) {
        return "";
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("ID=ubuntu") != std::string::npos) {
            return "ubuntu";
        }
        if (line.find("ID=debian") != std::string::npos) {
            return "debian";
        }
    }
    return "";
}

int main() {
    std::string distro = detect_linux_distro();

    if (distro != "ubuntu" && distro != "debian") {
        std::cerr << COLOR_ERROR << "[ERROR] Unsupported Linux distribution. This script only works on Ubuntu or Debian." << COLOR_RESET << std::endl;
        return 1;
    }

    std::cout << COLOR_INFO << "[INFO] Detected system: Linux (" << distro << ")" << COLOR_RESET << std::endl;

    // --- UPDATE STEP ---
    print_separator("UPDATING PACKAGE LIST (UPDATE)");
    int ret = exec_cmd_realtime("sudo apt-get update 2>&1", "[UPDATE] ", COLOR_LOG);
    if (ret != 0) {
        std::cerr << COLOR_ERROR << "The update step failed. Aborting." << COLOR_RESET << std::endl;
        return 1;
    }

    // --- UPGRADE STEP ---
    print_separator("UPGRADING INSTALLED PACKAGES (UPGRADE)");
    ret = exec_cmd_realtime("sudo apt-get upgrade -y 2>&1", "[UPGRADE] ", COLOR_LOG);
    if (ret != 0) {
        std::cerr << COLOR_ERROR << "The upgrade step failed. Aborting." << COLOR_RESET << std::endl;
        return 1;
    }

    std::cout << "\n" << COLOR_LOG << "[SUCCESS] The system has been updated successfully." << COLOR_RESET << std::endl;

    return 0;
}

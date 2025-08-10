#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#if defined(__ANDROID__) || defined(__linux__)
#define COLOR_RESET   "\033[0m"
#define COLOR_INFO    "\033[33m"  // yellow
#define COLOR_LOG     "\033[32m"  // green
#define COLOR_ERROR   "\033[31m"  // red
#define COLOR_WARNING "\033[90m"  // gray
#else
#define COLOR_RESET   ""
#define COLOR_INFO    ""
#define COLOR_LOG     ""
#define COLOR_ERROR   ""
#define COLOR_WARNING ""
#endif

// Executes a shell command, prints colored real-time output, and detects errors.
int exec_cmd_realtime(const std::string &cmd, const std::string &prefix, const std::string &color, bool stop_on_error = false) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << COLOR_ERROR << "[ERROR] Failed to execute command: " << cmd << COLOR_RESET << std::endl;
        return -1;
    }

    char buffer[256];
    bool error_detected = false;

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);

        if (line.find("Hit:") != std::string::npos) continue;
        if (line.find("Ign:") != std::string::npos) continue;
        if (line.find("Get:") != std::string::npos) continue;
        if (line.find("Reading") != std::string::npos) continue;
        if (line.find("Building") != std::string::npos) continue;
        if (line.find("Waiting") != std::string::npos) continue;

        if (line.find("WARNING: apt does not have a stable CLI interface.") != std::string::npos) {
            std::cout << COLOR_WARNING << prefix << line.substr(0, line.size()-1) << COLOR_RESET << std::endl;
            continue;
        }

        if (line.find("warning") != std::string::npos || line.find("Warning") != std::string::npos) {
            std::cout << COLOR_WARNING << prefix << line.substr(0, line.size()-1) << COLOR_RESET << std::endl;
            continue;
        }

        if (line.find("error") != std::string::npos || line.find("failed") != std::string::npos ||
            line.find("Error") != std::string::npos || line.find("Failed") != std::string::npos) {
            error_detected = true;
            std::cerr << COLOR_ERROR << prefix << line.substr(0, line.size()-1) << COLOR_RESET << std::endl;
            if (stop_on_error) break;
            continue;
        }

        if (!line.empty() && line.back() == '\n')
            line.pop_back();

        std::cout << color << prefix << line << COLOR_RESET << std::endl;
    }

    int ret_code = pclose(pipe);

    if (ret_code != 0) {
        std::cerr << COLOR_ERROR << prefix << "[ERROR] Command returned code " << ret_code << COLOR_RESET << std::endl;
        return ret_code;
    }

    if (error_detected) {
        std::cerr << COLOR_ERROR << prefix << "[ERROR] Detected error in command output." << COLOR_RESET << std::endl;
        return -2;
    }

    return 0;
}

// Prints a colored ASCII separator with a title for sections.
void print_separator(const std::string &title) {
    std::string line(50, '=');
    std::cout << COLOR_INFO << line << std::endl;
    std::cout << "[ " << title << " ]" << std::endl;
    std::cout << line << COLOR_RESET << std::endl;
}

// Detects if the current environment is Termux by checking the PREFIX environment variable.
bool is_termux() {
    const char* prefix = std::getenv("PREFIX");
    if (prefix != nullptr && std::string(prefix).find("com.termux") != std::string::npos) {
        return true;
    }
    return false;
}

// Detects if the Linux distribution is Ubuntu or Debian by reading /etc/os-release.
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
    bool termux = is_termux();
    bool is_linux = false;
    std::string distro;

#if defined(__linux__)
    is_linux = !termux;
#endif

    if (termux) {
        std::cout << COLOR_INFO << "[INFO] Detected system: Termux (Android)" << COLOR_RESET << std::endl;

        print_separator("UPDATE REAL");
        int ret = exec_cmd_realtime("pkg update -y 2>&1", "[UPDATE] ", COLOR_LOG);
        if (ret != 0) return 1;

        print_separator("UPGRADE REAL");
        ret = exec_cmd_realtime("pkg upgrade -y 2>&1", "[UPGRADE] ", COLOR_LOG);
        if (ret != 0) return 1;

        std::cout << COLOR_INFO << "[SUCCESS] Termux update completed." << COLOR_RESET << std::endl;

    } else if (is_linux) {
        distro = detect_linux_distro();
        if (distro != "ubuntu" && distro != "debian") {
            std::cerr << COLOR_ERROR << "[ERROR] Unsupported Linux distribution: only Ubuntu or Debian are supported." << COLOR_RESET << std::endl;
            return 1;
        }

        std::cout << COLOR_INFO << "[INFO] Detected system: Linux (" << distro << ")" << COLOR_RESET << std::endl;

        // Dry-run simulation of apt-get commands using -s flag
        std::cout << COLOR_INFO << "[INFO] Running dry-run simulation of update..." << COLOR_RESET << std::endl;
        int ret = exec_cmd_realtime("sudo apt-get update -s 2>&1", "[UPDATE] ", COLOR_LOG, true);
        if (ret != 0) {
            std::cerr << COLOR_ERROR << "[ERROR] Update simulation failed. Aborting." << COLOR_RESET << std::endl;
            return 1;
        }

        print_separator("UPGRADE");

        std::cout << COLOR_INFO << "[INFO] Running dry-run simulation of upgrade..." << COLOR_RESET << std::endl;
        ret = exec_cmd_realtime("sudo apt-get upgrade -y -s 2>&1", "[UPGRADE] ", COLOR_LOG, true);
        if (ret != 0) {
            std::cerr << COLOR_ERROR << "[ERROR] Upgrade simulation failed. Aborting." << COLOR_RESET << std::endl;
            return 1;
        }

        print_separator("UPDATE REAL");
        ret = exec_cmd_realtime("sudo apt-get update 2>&1", "[UPDATE] ", COLOR_LOG);
        if (ret != 0) return 1;

        print_separator("UPGRADE REAL");
        ret = exec_cmd_realtime("sudo apt-get upgrade -y 2>&1", "[UPGRADE] ", COLOR_LOG);
        if (ret != 0) return 1;

        std::cout << COLOR_INFO << "[SUCCESS] Ubuntu/Debian update completed." << COLOR_RESET << std::endl;

    } else {
        std::cerr << COLOR_ERROR << "[ERROR] Unsupported or undetected operating system." << COLOR_RESET << std::endl;
        return 1;
    }

    return 0;
}

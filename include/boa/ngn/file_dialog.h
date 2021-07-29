#ifndef BOA_NGN_FILE_DIALOG_H
#define BOA_NGN_FILE_DIALOG_H

#include <filesystem>
#include <string>
#include <vector>

namespace boa::ngn {

namespace fs = std::filesystem;

class FileDialog {
public:
    enum class Mode {
        Open,
        Save,
    };

    static FileDialog &get();

    void open(const std::string &window_name, const std::string &start_path);
    void open(const std::string &window_name);
    bool draw(const std::string &window_name, Mode mode = Mode::Open, const std::vector<std::string> &exts = {});

    std::string get_selected_path() const;

private:
    std::string m_current_open_window_name;
    bool m_open{ false };
    fs::path m_current_view_path;
    fs::path m_current_selected_path;
};

}

#endif

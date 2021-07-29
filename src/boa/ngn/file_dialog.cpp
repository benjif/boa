#include "imgui.h"
#include "boa/ngn/file_dialog.h"
#include <algorithm>
#include <optional>

namespace boa::ngn {

static FileDialog *file_dialog_instance = nullptr;

FileDialog &FileDialog::get() {
    if (!file_dialog_instance)
        file_dialog_instance = new FileDialog;
    return *file_dialog_instance;
}

void FileDialog::destroy() {
    delete file_dialog_instance;
}

void FileDialog::open(const std::string &window_name, const std::string &start_path) {
    m_current_open_window_name = window_name;
    m_current_view_path = start_path;
    m_open = true;
}

void FileDialog::open(const std::string &window_name) {
    m_current_open_window_name = window_name;
    m_current_view_path = fs::current_path();
    m_open = true;
}

bool FileDialog::skip_entry(const fs::directory_entry &entry, const std::vector<std::string> &exts) {
    fs::path cur_path = entry.path();
    std::string filename = cur_path.filename().string();

    bool wrong_ext = !entry.is_directory() && (exts.size() != 0 && !std::any_of(exts.cbegin(), exts.cend(), [&](auto &ext){ return cur_path.extension().string() == ext; }));
    return (filename.compare("..") == 0 || filename.compare(".") == 0 ||
        (filename.size() > 1 && filename[0] == '.') || wrong_ext);
}

bool FileDialog::draw(const std::string &window_name, Mode mode, const std::vector<std::string> &exts) {
    if (!m_open || window_name.compare(m_current_open_window_name) != 0)
        return false;

    auto directory_it = fs::directory_iterator(m_current_view_path);

    static size_t item_current_idx = 0;
    static auto found = std::find_if(fs::begin(directory_it), fs::end(directory_it),
                              [&](auto &p) { return !skip_entry(p, exts); });
    static std::optional<fs::directory_entry> current_selected_entry =
        (found == fs::end(directory_it)) ? std::nullopt : std::make_optional<fs::directory_entry>(*found);

    ImGui::OpenPopup(m_current_open_window_name.c_str());
    //ImGui::SetNextWindowSize(ImVec2(540, 320));

    if (ImGui::BeginPopupModal(m_current_open_window_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char edit_filename[64];

        if (ImGui::Button("Up")) {
            m_current_view_path = m_current_view_path.parent_path();
            edit_filename[0] = '\0';
            item_current_idx = 0;
            ImGui::EndPopup();
            return false;
        }

        auto handle_operation = [&](const fs::directory_entry &entry) {
            if (entry.is_directory()) {
                edit_filename[0] = '\0';
                m_current_view_path = entry.path();
                return false;
            }

            m_open = false;
            m_current_selected_path = entry.path();

            return true;
        };

        if (ImGui::BeginListBox("")) {
            size_t i = 0;
            for (auto &p : directory_it) {
                fs::path cur_path = p.path();
                std::string filename = cur_path.filename().string();

                if (skip_entry(p, exts))
                    continue;

                const bool is_selected = (item_current_idx == i);
                const std::string prepend_type = p.is_directory() ? "[D] " : "[F] ";
                if (ImGui::Selectable((prepend_type + filename).c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    current_selected_entry = p;

                    if (ImGui::IsMouseDoubleClicked(0)) {
                        ImGui::EndListBox();
                        ImGui::EndPopup();
                        item_current_idx = 0;
                        return handle_operation(p);
                    }

                    item_current_idx = i;
                    if (!p.is_directory())
                        strcpy(edit_filename, filename.c_str());
                }

                if (is_selected)
                    ImGui::SetItemDefaultFocus();

                i++;
            }

            ImGui::EndListBox();
        }

        ImGui::InputText("", edit_filename, 64);

        if (ImGui::Button(mode == Mode::Open ? "Open" : "Save")) {
            ImGui::EndPopup();

            item_current_idx = 0;
            if (strlen(edit_filename) != 0)
                return handle_operation(fs::directory_entry(m_current_view_path.append(edit_filename)));
            else if (current_selected_entry.has_value())
                return handle_operation(*current_selected_entry);
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel")) {
            m_open = false;
            ImGui::EndPopup();
            return false;
        }

        ImGui::EndPopup();
    }

    return false;
}

std::string FileDialog::get_selected_path() const {
    return m_current_selected_path.string();
}

}

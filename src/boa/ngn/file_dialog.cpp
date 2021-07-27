#include "imgui.h"
#include "boa/ngn/file_dialog.h"

namespace boa::ngn {

static FileDialog *file_dialog_instance = nullptr;

FileDialog &FileDialog::get() {
    if (!file_dialog_instance)
        file_dialog_instance = new FileDialog;
    return *file_dialog_instance;
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

bool FileDialog::draw(const std::string &window_name, Mode mode) {
    if (!m_open || window_name.compare(m_current_open_window_name) != 0)
        return false;

    auto directory_it = fs::directory_iterator(m_current_view_path);

    static size_t item_current_idx;
    fs::directory_entry current_selected_entry;

    ImGui::OpenPopup(m_current_open_window_name.c_str());
    //ImGui::SetNextWindowSize(ImVec2(540, 320));
    if (ImGui::BeginPopupModal(m_current_open_window_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::Button("Up")) {
            m_current_view_path = m_current_view_path.parent_path();
            ImGui::EndPopup();
            return false;
        }

        static std::string edit_filename;
        if (ImGui::BeginListBox("")) {
            size_t i = 0;
            for (auto &p : directory_it) {
                fs::path cur_path = p.path();
                std::string filename = cur_path.filename();
                if (filename == ".." || filename == "." ||
                        (filename.size() > 1 && filename[0] == '.')) {
                    continue;
                }

                const bool is_selected = (item_current_idx == i);
                const std::string prepend_type = p.is_directory() ? "[D] " : "[F] ";
                if (ImGui::Selectable((prepend_type + filename).c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        ImGui::EndListBox();
                        ImGui::EndPopup();

                        if (p.is_directory()) {
                            m_current_view_path = cur_path;
                            return false;
                        } else {
                            m_open = false;
                            m_current_selected_path = cur_path;;
                            return true;
                        }
                    }

                    edit_filename = filename;
                    current_selected_entry = p;
                    item_current_idx = i;
                }

                if (is_selected)
                    ImGui::SetItemDefaultFocus();

                i++;
            }

            ImGui::EndListBox();
        }

        bool text_change = false;
        if (ImGui::InputText("Filename", edit_filename.data(), edit_filename.size()))
            text_change = true;

        if (ImGui::Button(mode == Mode::Open ? "Open" : "Save")) {
            if (text_change) {
                m_current_selected_path = current_selected_entry.path();
                m_current_selected_path = m_current_selected_path.append(edit_filename);
                m_open = false;
                ImGui::EndPopup();
                return true;
            } else if (current_selected_entry.is_regular_file()) {
                m_current_selected_path = current_selected_entry.path();
                m_open = false;
                ImGui::EndPopup();
                return true;
            }

            m_current_view_path = current_selected_entry.path();
        }

        ImGui::EndPopup();
    }

    return false;
}

std::string FileDialog::get_selected_path() const {
    return m_current_selected_path;
}

}

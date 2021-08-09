#ifndef BOA_NGN_SCRIPTING_H
#define BOA_NGN_SCRIPTING_H

#include "lua.hpp"
#include <string>

namespace boa::ngn {

class ScriptController {
public:
    ScriptController();
    ~ScriptController();

    void run_from_file(const std::string &script_path) const;
    void run_from_buffer(const std::string &script_content) const;

private:
    static int exception_callback(lua_State *L, lua_CFunction f);

    lua_State *m_state;
};

}

#endif

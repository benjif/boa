#include "boa/ngn/scripting.h"
#include "boa/ngn/scripting_interface.h"
#include "boa/utl/macros.h"
#include <stdexcept>
#include <fstream>
#include <sstream>

namespace boa::ngn {

ScriptController::ScriptController() {
    m_state = luaL_newstate();
    if (!m_state)
        LOG_FAIL("(Scripting) Failed to create luajit state");

    luaL_openlibs(m_state);

    lua_pushlightuserdata(m_state, (void *)exception_callback);
    luaJIT_setmode(m_state, -1, LUAJIT_MODE_WRAPCFUNC | LUAJIT_MODE_ON);
    lua_pop(m_state, 1);
}

ScriptController::~ScriptController() {
    lua_close(m_state);
}

void ScriptController::run_from_file(const std::string &script_path) const {
    if (luaL_dofile(m_state, script_path.c_str()) != 0) {
        LOG_WARN("(Scripting) Script error '{}' => '{}'", script_path, lua_tostring(m_state, -1));
        lua_pop(m_state, 1);
    }
}

void ScriptController::run_from_buffer(const std::string &script_content) const {
    luaL_loadbuffer(m_state, script_content.c_str(), script_content.size(), "buffer script");
    if (lua_pcall(m_state, 0, 0, 0) != 0) {
        LOG_WARN("(Scripting) Script error '{}'", lua_tostring(m_state, -1));
        lua_pop(m_state, 1);
    }
}

int ScriptController::exception_callback(lua_State *L, lua_CFunction f) {
    try {
        return f(L);
    } catch (const char *s) {
        lua_pushstring(L, s);
    } catch (std::exception &e) {
        lua_pushstring(L, e.what());
    } catch (...) {
        lua_pushliteral(L, "caught (...)");
    }

    return lua_error(L);
}

}

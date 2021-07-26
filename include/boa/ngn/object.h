#ifndef BOA_NGN_OBJECT_H
#define BOA_NGN_OBJECT_H

#include <string>
#include <utility>

namespace boa::ngn {

/*struct Named {
    std::string name;
};*/

struct LoadedAsset {
    LoadedAsset(std::string &&r_path)
        : resource_path(std::move(r_path))
    {
    }
    std::string resource_path;
};

struct EngineConfigurable {
    EngineConfigurable()
        : selected(false)
    {
    }

    EngineConfigurable(bool s)
        : selected(s)
    {
    }

    bool selected;
};

}

#endif

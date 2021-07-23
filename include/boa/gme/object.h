#ifndef BOA_GME_OBJECT_H
#define BOA_GME_OBJECT_H

namespace boa::gme {

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

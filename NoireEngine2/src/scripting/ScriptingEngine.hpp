#pragma once

#include "core/layers/Layer.hpp"
#include "utils/Singleton.hpp"

#include <unordered_map>

class Behaviour;

class ScriptingEngine : public Layer, Singleton
{
private:
    static ScriptingEngine* s_Instance;

public:
    ScriptingEngine();

public:
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate() override;
    void OnEvent(Event& e) override;

public:
    static ScriptingEngine* Get() { return s_Instance; }

    void Add(Behaviour*);

    Behaviour* GetScript(const std::string&);

    bool Exists(const std::string&);

    void Remove(const std::string&);

private:
    std::unordered_map<std::string, Behaviour*> m_Scripts;
};

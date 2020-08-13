// Copyright (c) 2009 - Mozy, Inc.

#include "config.h"

#include <algorithm>
#include <regex>

extern char **environ;

namespace Mordor2 {

bool
isValidConfigVarName(const std::string &name, bool allowDot)
{
    static const std::regex regname("[a-z][a-z0-9]*");
    static const std::regex regnameDot("[a-z][a-z0-9]*(\\.[a-z0-9]+)*");
    if (allowDot)
        return std::regex_match(name, regnameDot);
    else
        return std::regex_match(name, regname);
}

bool Config::s_locked = false;

ConfigVarBase::ptr
Config::lookup(const std::string &name)
{
    ConfigVarSet::iterator it = vars().find(name);
    if (it != vars().end())
        return it->second;
    return ConfigVarBase::ptr();
}

void
Config::visit(std::function<void (ConfigVarBase::ptr)> dg)
{
    for (ConfigVarSet::const_iterator it = vars().begin();
        it != vars().end();
        ++it) {
        dg(it->second);
    }
}


HijackConfigVar::HijackConfigVar(const std::string &name, const std::string &value)
    : m_var(Config::lookup(name))
{
    m_oldValue = m_var->toString();
    // failed to set value
    if (!m_var->fromString(value))
        m_var.reset();
}

HijackConfigVar::~HijackConfigVar()
{
    reset();
}

void
HijackConfigVar::reset()
{
    if (m_var) {
        m_var->fromString(m_oldValue);
        m_var.reset();
    }
}

}

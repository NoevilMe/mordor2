#ifndef __MODOR_CONFIG_H__
#define __MODOR_CONFIG_H__
// Copyright (c) 2009 - Mozy, Inc.

#include "noncopyable.h"

#include <assert.h>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>

namespace Mordor {

/*
Configuration Variables (ConfigVars) are a mechanism for configuring the
behavior of Mordor at runtime (e.g. without requiring recompilation).
It is a generic and useful mechanim, and software that uses Mordor can
define and use its own set of ConfigVars.

ConfigVars are stored in a singleton key-value table.

Typical usages of Configuration Variables include:
-Variables to adjust the level of logging (e.g. "log.debugmask")
-Variables to control the frequency of periodic timers
-Variables to enable experimental features or debugging tools

The key name of a ConfigVar can only contain lower case letters and digits
and the "." separator.  When defined using an environmental variable
upper case characters are converted to lower case, and the "_" character
can be used in place of ".".

The value of a ConfigVar has a specific type, e.g. string, integer or
double.  To access the specific value it is necessary the use the correctly
templated ConfigVar object.  e.g. ConfigVar<std::string> to read a string.
For convenience the toString() and fromString() can be used to
access the value in a general way, for example when iterating through all the
configuration variables.

A ConfigVar can only be defined once (via the templated version of
Config::lookup()), and this typically happens at global scope in a source code
file, with the result assigning to a global variable.

for example:

static ConfigVar<std::string>::ptr g_servername =
    Config::lookup<std::string>("myapp.server",
                                std::string("http://test.com"),
                                "Main Server");

Outside the source code where the ConfigVar is defined the variables can
be read or written by performing a lookup using the non-templated version of
Config::lookup()

for example:

static ConfigVarBase::ptr servername = Config::lookup("myapp.server");
std::cout << servername.toString();

To access the real value of a ConfigVar you would typically use a cast operation
like this:

ConfigVar<bool>::ptr boolVar = boost::dynamic_pointer_cast<ConfigVar<bool>
>(Config::lookup('myapp.showui')) if (configVarPtr) { bool b = boolVar->val();
    ...
}

In this case the type specified must exactly match the type used when the
ConfigVar was defined.

In addition to programmatic access it is possible to override the default
value of a ConfigVar using built in support for reading environmental
variables (Config::loadFromEnvironment()), Windows registry settings
(Config::loadFromRegistry()) etc.  These mechanisms are optional and must
be explicitly called from the code that uses Mordor.  You could also easily
extend this concept with your own code to read ConfigVars from ini files,
Apple property lists, sql databases etc.

Like any other global variable, ConfigVars should be used with some degree of
constraint and common sense.  For example they make sense for things that
are primarily adjusted only during testing, for example to point a client
to a test server rather than a default server or to increase the frequency of
a periodic operation.  But they should not be used as a replacement for clean
APIs used during the regular software flow.
*/

/// check if the name is a valid ConfigVar name
/// @param name     ConfigVar name
/// @param allowDot Whether dot is allowed in the ConfigVar name
/// @note ConfigVar name rule when allowDot value is
///   - true:  [a-z][a-z0-9]*(\.[a-z0-9]+)*
///   - false: [a-z][a-z0-9]*
/// @return if @p name is a valid ConfigVar name
bool isValidConfigVarName(const std::string &name, bool allowDot = true);

class ConfigVarBase : public Noncopyable {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

public:
    ConfigVarBase(const std::string &name, const std::string &description = "",
                  bool lockable = false)
        : m_name(name), m_description(description), m_lockable(lockable) {}
    virtual ~ConfigVarBase() {}

    std::string name() const { return m_name; }
    std::string description() const { return m_description; }
    bool isLockable() const { return m_lockable; }

    void monitor(std::function<void()> dg) {
        m_cb = dg;
        m_cb();
    }

    virtual std::string toString() const = 0;
    /// @return If the new value was accepted
    virtual bool fromString(const std::string &str) = 0;

protected:
    std::function<void()> m_cb;
private:
    std::string m_name, m_description;
    bool m_lockable;
};

template <class T> bool isConfigNotLocked(const T &);

template <class T> class ConfigVar : public ConfigVarBase {
public:
    struct BreakOnFailureCombiner {
        typedef bool result_type;
        template <typename InputIterator>
        bool operator()(InputIterator first, InputIterator last) const {
            try {
                for (; first != last; ++first)
                    if (!*first)
                        return false;
            } catch (...) {
                return false;
            }
            return true;
        }
    };

    typedef std::shared_ptr<ConfigVar> ptr;

public:
    ConfigVar(const std::string &name, const T &defaultValue,
              const std::string &description = "", bool lockable = false)
        : ConfigVarBase(name, description, lockable), m_val(defaultValue) {
        // if Config is locked, should reject changes to lockable ConfigVars
    }

    std::string toString() const {
        std::stringstream ss;
        ss << m_val;
        return ss.str();
    }

    bool fromString(const std::string &str) {
        try {
            std::stringstream ss;
            ss << str;
            T v;
            ss >> v;
            return val(v);
        } catch (...) {
            return false;
        }
    }

    // TODO: atomicCompareExchange and/or mutex
    T val() const { return m_val; }
    bool val(const T &v) {
        T oldVal = m_val;
        if (oldVal != v) {
            m_val = v;

            if (m_cb)
                m_cb();
        }
        return true;
    }

private:
    T m_val;
};

class Config {
private:
    static std::string getName(const ConfigVarBase::ptr &var) {
        return var->name();
    }

    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarSet;

public:
    /// Declare a ConfigVar
    ///
    /// @note A ConfigVar can only be declared once.
    /// @throws std::invalid_argument With what() == the name of the ConfigVar
    ///         if the value is not valid.
    template <class T>
    static typename ConfigVar<T>::ptr
    lookup(const std::string &name, const T &defaultValue,
           const std::string &description = "", bool lockable = false) {
        if (!isValidConfigVarName(name))
            throw std::invalid_argument(name);

        assert(vars().find(name) == vars().end());
        typename ConfigVar<T>::ptr v(
            new ConfigVar<T>(name, defaultValue, description, lockable));
        vars().insert(std::make_pair(name, v));
        return v;
    }

    // This signature of Lookup is used to perform a lookup for a
    // previously declared ConfigVar
    static ConfigVarBase::ptr lookup(const std::string &name);

    // Use to iterate all the ConfigVars
    static void visit(std::function<void(ConfigVarBase::ptr)> dg);

    /// Set lock flag of the Config
    /// @param locked If set to true, it will lock those lockable Configvars
    /// from
    ///               updating their values.
    static void lock(bool locked) { s_locked = locked; }
    static bool isLocked() { return s_locked; }

private:
    static ConfigVarSet &vars() {
        static ConfigVarSet vars;
        return vars;
    }
    static bool s_locked;
};

template <class T> bool isConfigNotLocked(const T &) {
    return !Config::isLocked();
}

/// helper class to allow temporarily change the ConfigVar Value
///
/// When an instance is created, the specified ConfigVar is hijacked to the @c
/// tempValue, after the instance is @c reset() or destroyed, the value is
/// restored.
class HijackConfigVar {
public:
    HijackConfigVar(const std::string &name, const std::string &value);
    const std::string &originValue() const { return m_oldValue; }
    void reset();
    ~HijackConfigVar();

private:
    std::shared_ptr<Mordor::ConfigVarBase> m_var;
    std::string m_oldValue;
};

} // namespace Mordor

#endif

#include "GameConfig.hpp"
#include <cstdlib>
#include <cstring>
#include <rw/defines.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
namespace pt = boost::property_tree;

const std::string kConfigDirectoryName("OpenRW");

GameConfig::GameConfig(const std::string& configName,
                       const std::string& configPath)
    : m_configName(configName)
    , m_configPath(configPath)
    , m_valid(false)
    , m_inputInvertY(false) {
    if (m_configPath.empty()) {
        m_configPath = getDefaultConfigPath();
    }

    // Look up the path to use
    auto configFile = getConfigFile();

    std::string dummy;
    m_valid = parseConfig(ParseType::FILE, configFile, ParseType::CONFIG, dummy);
}

std::string GameConfig::getConfigFile() const {
    return m_configPath + "/" + m_configName;
}

bool GameConfig::isValid() const {
    return m_valid;
}

std::string GameConfig::getDefaultConfigPath() {
#if defined(RW_LINUX) || defined(RW_FREEBSD) || defined(RW_NETBSD) || \
    defined(RW_OPENBSD)
    char* config_home = getenv("XDG_CONFIG_HOME");
    if (config_home != nullptr) {
        return std::string(config_home) + "/" + kConfigDirectoryName;
    }
    char* home = getenv("HOME");
    if (home != nullptr) {
        return std::string(home) + "/.config/" + kConfigDirectoryName;
    }

#elif defined(RW_OSX)
    char* home = getenv("HOME");
    if (home)
        return std::string(home) + "/Library/Preferences/" +
               kConfigDirectoryName;

#else
    return ".";
#endif

    // Well now we're stuck.
    RW_ERROR("No default config path found.");
    return ".";
}

std::string stripComments(const std::string &str) {
    auto s = std::string(str, 0, str.find_first_of(";#"));
    return s.erase(s.find_last_not_of(" \n\r\t")+1);
}

struct StringTranslator {
    typedef std::string internal_type;
    typedef std::string external_type;
    boost::optional<external_type> get_value(const internal_type &str) {
        return stripComments(str);
    }
    boost::optional<internal_type> put_value(const external_type &str) {
        return str;
    }
};

struct BoolTranslator {
    typedef std::string internal_type;
    typedef bool        external_type;
    boost::optional<external_type> get_value(const internal_type &str) {
        boost::optional<external_type> res;
        try {
            res = std::stoi(stripComments(str)) != 0;
        } catch (std::invalid_argument &e) {
        }
        return res;
    }
    boost::optional<internal_type> put_value(const external_type &b) {
        return internal_type(b ? "1" : "0");
    }
};

struct IntTranslator {
    typedef std::string internal_type;
    typedef int         external_type;
    boost::optional<external_type> get_value(const internal_type &str) {
        boost::optional<external_type> res;
        try {
            res = std::stoi(stripComments(str));
        } catch (std::invalid_argument &e) {
        }
        return res;
    }
    boost::optional<internal_type> put_value(const external_type &i) {
        return boost::optional<internal_type>(std::to_string(i));
    }
};

bool GameConfig::saveConfig() {
    auto filename = getConfigFile();
    return parseConfig(ParseType::CONFIG, "",
        ParseType::FILE, filename);
}

std::string GameConfig::getDefaultINIString() {
    std::string result;
    parseConfig(ParseType::DEFAULT, "", ParseType::STRING, result);
    return result;
}

bool GameConfig::parseConfig(
    GameConfig::ParseType srcType, const std::string &source,
    ParseType destType, std::string &destination)
{
    pt::ptree srcTree;

    try {
        if (srcType == ParseType::STRING) {
            pt::read_ini(source, srcTree);
        } else if (srcType == ParseType::FILE) {
            std::ifstream ifs(source);
            pt::read_ini(ifs, srcTree);
        }
    } catch (pt::ini_parser_error &e) {
        // Catches illegal input files (nonsensical input, duplicate keys)
        RW_MESSAGE(e.what());
        return false;
    }

    if (destType == ParseType::DEFAULT) {
        RW_ERROR("Target cannot be DEFAULT.");
        return false;
    }
    
    bool success = true;
    
    auto read_config = [&](const std::string &key, auto &target,
                          const auto &defaultValue, auto &translator,
                          bool optional=true) {
        typedef typename std::remove_reference<decltype(target)>::type config_t;

        config_t sourceValue;

        switch (srcType) {
            case ParseType::DEFAULT:
                sourceValue = defaultValue;
                break;
            case ParseType::CONFIG:
                sourceValue = target;
                break;
            case ParseType::FILE:
            case ParseType::STRING:
                try {
                    sourceValue = srcTree.get<config_t>(key, translator);
                } catch (pt::ptree_bad_path &e) {
                    // Catches missing key-value pairs: fail when required
                    if (!optional) {
                      success = false;
                      RW_MESSAGE(e.what());
                      return;
                    }
                    sourceValue = defaultValue;
                } catch (pt::ptree_bad_data &e) {
                    // Catches illegal value data: always fail
                    success = false;
                    RW_MESSAGE("invalid data");
                    RW_MESSAGE(e.what());
                    return;
                }
                break;
        }
        srcTree.put(key, sourceValue, translator);

        switch (destType) {
            case ParseType::DEFAULT:
                // Target cannot be DEFAULT (case already handled)
                success = false;
                break;
            case ParseType::CONFIG:
                //Don't care if success == false
                target = sourceValue;
                break;
            case ParseType::FILE:
            case ParseType::STRING:
                break;
        }
    };

    auto deft = StringTranslator();
    auto boolt = BoolTranslator();

    // Add new configuration parameters here.
    // Additionally, add them to the unit test.

    // @todo Don't allow path separators and relative directories
    read_config("game.path", this->m_gamePath, "/opt/games/Grand Theft Auto 3", deft, false);
    read_config("game.language", this->m_gameLanguage, "american", deft);

    read_config("input.invert_y", this->m_inputInvertY, false, boolt);

    if (!success)
        return success;

    try {
        if (destType == ParseType::STRING) {
            std::ostringstream ostream;
            pt::write_ini(ostream, srcTree);
            destination = ostream.str();
        } else if (destType == ParseType::FILE) {
            pt::write_ini(destination, srcTree);
        }
    } catch (pt::ini_parser_error &e) {
        success = false;
        RW_MESSAGE(e.what());
    }

    return success;
}


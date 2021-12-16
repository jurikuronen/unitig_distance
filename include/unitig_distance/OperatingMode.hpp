#pragma once

#include <ostream>
#include <type_traits>

enum class OperatingMode {
    DEFAULT                         = 0x00,
    CDBG                            = 0x01,
    SGGS                            = 0x02,
    CDBG_AND_SGGS                   = 0x03,
    SGG_FILTER                      = 0x04, // Not implemented.

    CDBG_AND_SGGS_FILTERED          = 0x07, // Not implemented.
    GENERAL                         = 0x08,

    FILTER                          = 0x10,
    CDBG_FILTERED                   = 0x11,
    CDBG_FILTERED_AND_SGGS          = 0x13,

    CDBG_FILTERED_AND_SGGS_FILTERED = 0x17, // Not implemented.

    GENERAL_FILTERED                = 0x18

};

using underlying_type = std::underlying_type<OperatingMode>::type;

static OperatingMode operator|(const OperatingMode lhs, const OperatingMode rhs) {
    return static_cast<OperatingMode>(static_cast<underlying_type>(lhs) | static_cast<underlying_type>(rhs));
}

static OperatingMode operator|=(OperatingMode& lhs, const OperatingMode rhs) {
    return lhs = lhs | rhs;
}

static OperatingMode operator&(const OperatingMode lhs, const OperatingMode rhs) {
    return static_cast<OperatingMode>(static_cast<underlying_type>(lhs) & static_cast<underlying_type>(rhs));
}

static bool operating_mode_to_bool(const OperatingMode om) {
    return static_cast<bool>(underlying_type(om));
}

static std::ostream& operator<<(std::ostream& os, const OperatingMode om) {
    switch (om) {
        case OperatingMode::CDBG:                            os << "CDBG"; break;
        case OperatingMode::CDBG_AND_SGGS:                   os << "CDBG_AND_SGGS"; break;
        case OperatingMode::GENERAL:                         os << "GENERAL"; break;
        case OperatingMode::CDBG_FILTERED:                   os << "CDBG_FILTERED"; break;
        case OperatingMode::CDBG_FILTERED_AND_SGGS:          os << "CDBG_FILTERED_AND_SGGS"; break;
        case OperatingMode::CDBG_FILTERED_AND_SGGS_FILTERED: os << "CDBG_FILTERED_AND_SGGS_FILTERED"; break;
        case OperatingMode::GENERAL_FILTERED:                os << "GENERAL_FILTERED"; break;
        default:                                             os << "DEFAULT";
    }
    return os;
}




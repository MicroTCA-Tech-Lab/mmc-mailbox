/***************************************************************************
 *      ____  _____________  __    __  __ _           _____ ___   _        *
 *     / __ \/ ____/ ___/\ \/ /   |  \/  (_)__ _ _ __|_   _/ __| /_\  (R)  *
 *    / / / / __/  \__ \  \  /    | |\/| | / _| '_/ _ \| || (__ / _ \      *
 *   / /_/ / /___ ___/ /  / /     |_|  |_|_\__|_| \___/|_| \___/_/ \_\     *
 *  /_____/_____//____/  /_/      T  E  C  H  N  O  L  O  G  Y   L A B     *
 *                                                                         *
 *          Copyright 2022 Deutsches Elektronen-Synchrotron DESY.          *
 *                          All rights reserved.                           *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <cstring>
#include <ostream>
#include <string>

#include "mmcmb.h"

// Convenience functions for C++ users

enum class FruId { AMC = 0, RTM = 1, FMC1 = 2, FMC2 = 3 };

inline std::ostream& operator<<(std::ostream& os, FruId d)
{
    switch (d) {
        case FruId::AMC:
            os << "AMC";
            break;
        case FruId::RTM:
            os << "RTM";
            break;
        case FruId::FMC1:
            os << "FMC1";
            break;
        case FruId::FMC2:
            os << "FMC2";
            break;

        default:
            os.setstate(os.failbit);
            break;
    }
    return os;
}

// Convert a fixed-size C string (such as mb_fru_description_t::manufacturer) into a std::string
template <typename T>
std::string mb_to_str(const T& chararray)
{
    return {chararray, strnlen(chararray, sizeof(chararray))};
}

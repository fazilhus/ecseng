//
// Created by fazkhu-3-local on 03/13/26.
//

#pragma once

namespace Math {

    inline float deg_to_rad(const float deg) {
        return deg * static_cast<float>(0.01745329251994329576923690768489);
    }

    inline float rad_to_deg(const float rad) {
        return rad * static_cast<float>(57.295779513082320876798154814105);
    }

}
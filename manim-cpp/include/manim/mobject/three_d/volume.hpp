#pragma once

#include "manim/mobject/mobject.hpp"

namespace manim {

// Simplified volume placeholder: lightweight type derived from Mobject.
class Volume : public Mobject {
public:
    Volume() = default;
    ~Volume() override = default;
};

using GPUVolume = Volume;

}  // namespace manim

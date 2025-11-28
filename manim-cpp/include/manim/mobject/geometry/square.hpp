#pragma once

#include "manim/mobject/mobject.hpp"

namespace manim {

class Square : public Mobject {
public:
    Square(float size = 1.0f) : size_(size) {
        generate_points();
    }
    Square(const Square& other) : Mobject() {
        size_ = other.size_;
        points_cpu_ = other.points_cpu_;
        num_points_ = points_cpu_.size();
    }
    ~Square() override = default;

    void generate_points() override {
        // Minimal square centered at origin
        float h = 0.5f * size_;
        points_cpu_ = {
            {-h, -h, 0.0f},
            {h, -h, 0.0f},
            {h, h, 0.0f},
            {-h, h, 0.0f}
        };
        num_points_ = points_cpu_.size();
    }

    Ptr copy() const override {
        auto s = std::make_shared<Square>(size_);
        s->points_cpu_ = points_cpu_;
        s->num_points_ = num_points_;
        return s;
    }

private:
    float size_ = 1.0f;
};

}  // namespace manim

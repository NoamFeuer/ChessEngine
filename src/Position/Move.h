#pragma once
#include <cstdint>

namespace position {
    struct Move {
        std::uint32_t data;

        Move() = default;

        Move(int fromSquare, int toSquare, bool castle = false, bool enPassant = false, int promotion = 0) {
            data = static_cast<std::uint32_t>(fromSquare) |
                   (static_cast<std::uint32_t>(toSquare) << 6) |
                   (castle ? (1u << 13) : 0u) |
                   (enPassant ? (1u << 14) : 0u) |
                   (static_cast<std::uint32_t>(promotion) << 15);
        }

        int fromSquare() const { return static_cast<int>(data & 63u); }
        int toSquare()  const { return static_cast<int>((data >> 6) & 63u); }
        bool capture()  const { return (data >> 12) & 1u; }
        bool castle()   const { return (data >> 13) & 1u; }
        bool enPassant() const { return (data >> 14) & 1u; }
        int promotion() const { return static_cast<int>((data >> 15) & 7u); }

        void setCapture() { data |= 1u << 12; }
    };
}
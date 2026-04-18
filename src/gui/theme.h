#ifndef GUI_THEME_H
#define GUI_THEME_H

#include "raylib.h"

namespace gui {

inline constexpr Color kTextStrong  = {232, 240, 248, 255};
inline constexpr Color kText        = {220, 232, 242, 255};
inline constexpr Color kTextMuted   = {151, 171, 190, 255};
inline constexpr Color kBorder      = {71,  98,  122, 255};
inline constexpr Color kSurface     = {24,  52,  78,  255};
inline constexpr Color kSurfaceAlt  = {30,  61,  90,  255};
inline constexpr Color kHeader      = {19,  43,  68,  255};
inline constexpr Color kAccent      = {89,  153, 224, 255};
inline constexpr Color kAccentSoft  = {137, 178, 222, 255};
inline constexpr Color kDanger      = {238, 103, 112, 255};
inline constexpr Color kSuccess     = {74,  197, 139, 255};
inline constexpr Color kShadow      = {5,   15,  25,  95};
inline constexpr Color kOverlay     = {0,   0,   0,   160};
inline constexpr Color kSurfaceDark = {14,  34,  56,  255};

}  // namespace gui

#endif

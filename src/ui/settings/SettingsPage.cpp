// src/ui/settings/SettingsPage.cpp
#include "SettingsPage.h"

// SettingsPage 是纯抽象基类，所有虚函数在头文件中声明/内联。
// 本文件仅用于：
//   1. 让构建系统显式编译该翻译单元（qmake 对纯头文件 Q_OBJECT 处理更稳）
//   2. 未来放置公共辅助函数的占位
//
// 若确实不需要，可以从 CMake/qmake 中移除本文件，
// 保留 SettingsPage.h 即可（moc 会生成 moc_SettingsPage.cpp）。
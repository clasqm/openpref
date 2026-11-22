/*
 *      OpenPref - cross-platform Preferans game
 *
 *      This file is part of OpenPref; see the other files for copyright
 *      and licence information (GPLv3).
 */

#ifndef SCREENUTIL_H
#define SCREENUTIL_H

#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QWidget>

// Available geometry of the screen holding `w` (or of the primary screen when
// `w` is null). This replaces the Qt4-only QDesktopWidget/QApplication::
// desktop() pair and is the Qt6-compatible form as well.
inline QRect availableGeometryFor (const QWidget *w) {
  const QScreen *screen = w ? w->screen() : 0;
  if (!screen) screen = QGuiApplication::primaryScreen();
  if (!screen) return QRect(0, 0, 1024, 768);
  return screen->availableGeometry();
}

#endif // SCREENUTIL_H

# OpenPref — Qt 4 → Qt 5 conversion plan (Haiku)

Status: **plan only, no source changes made yet.**
Branch: `converttoqt5` (per `AGENTS.md`). Base: `originalqt4`.
Target: Qt 5.15.x on Haiku x86_64 (`qt5`, `qt5_devel`, `qt5_tools`).

---

## 1. What the program is

OpenPref is a Qt4 reimplementation of the card game Preferans: one human against
two AI players, drawn entirely onto an offscreen `QImage` that `Kpref`
(`QMainWindow`) blits in `paintEvent()`.

```
Kpref (QMainWindow, kpref.cpp/h)          — window, menus, options, input
 ├─ PrefDesktop (QObject, desk/desktop.*) — game loop, deals, scoring, save/load
 │   ├─ HumanPlayer / AIPlayer / AlphaBetaPlayer (logic/*)
 │   └─ ScoreBoard (score/plscore.*)
 ├─ DeskView (QObject, desk/deskview.*)   — all painting, card bitmaps,
 │   └─ SleepEventLoop/SleepEventFilter   — blocking "sleep with input" loops
 ├─ FormBid (forms/formbid.*)             — bidding dialog, built in code
 ├─ QBidButton (forms/qbidbutton.*)       — bid buttons, icon from resources
 ├─ NewGameDialog / OptDialog / HelpBrowser — 3 Qt Designer .ui files
 └─ Card / CardList / NCounter / serialization helpers
```

Roughly 5 700 lines of C++; all string literals are English/ASCII; the UI is
built partly from `.ui` files and partly in code; every bitmap is loaded from the
Qt resource system (`:/pics/...`).

GPLv3. File headers must be preserved.

---

## 2. Findings that shape the plan

| # | Finding | Impact |
|---|---------|--------|
| F1 | **There is no build file at all.** No `.pro`, no `CMakeLists.txt` anywhere. Only 7 orphan qmake `.pri` fragments and 6 orphan `*.cmake` fragments, none referenced by a root file. | The tree cannot be built today, on any OS. Build system work is mandatory, not cosmetic. |
| F2 | **There is no `.qrc`** (never existed in this repo's history). Yet `deskview.cpp`, `qbidbutton.cpp`, `formbid.cpp` and `kpref.cpp` load every image from `:/pics/…`. | Compiles fine, then runs with an **empty deck, no card faces, no menu icons**. The resource file has to be authored from `pics/` (111 PNGs). |
| F3 | 9 of 40 source files are **CP1251**, the rest ASCII/UTF-8. All non-ASCII bytes are inside comments (Russian notes) — no Cyrillic string literals. | Not fatal, but Qt5 always assumes UTF-8 for literals and `moc` chokes on some legacy encodings. Low-risk cleanup. |
| F4 | `AUTHORS` already lists "Michel Clasquin-Johnson, port to qt5" — a previous attempt stopped after the orphan `.cmake` fragments and the Qt4-only `FindLinguist.cmake`. | We restart the build system rather than patch the fragments. |
| F5 | Only Qt5 **Widgets** is needed: no QtScript, QtNetwork, QtXml, QtOpenGL, QtSql, QtPrintSupport, QtMultimedia usage anywhere. | One `find_package` and one link library. No extra packages to install. |
| F6 | Old message boxes use the Qt4 int-button API (`QMessageBox::Yes | QMessageBox::Default`). | Still present and **not** marked deprecated in 5.15 — compiles as is. Modernisation optional. |

### Qt 5 deprecation mechanics (why some lines break and others only warn)

`qglobal.h` defaults `QT_DISABLE_DEPRECATED_BEFORE` to **5.0.0**, and
`QT_DEPRECATED_SINCE(major,minor)` is true only for `major.minor > 5.0`.
So anything wrapped in `QT_DEPRECATED_SINCE(5, 0)` in the Qt 5.15 headers is
**compiled out** → hard error, while `QT_DEPRECATED_SINCE(5, 14)` / `(5, 15)`
members are still present → warning only. That is exactly the split below.

---

## 3. P0 — will not compile / will not work (must fix)

### 3.1 `qFindChildren` — removed in Qt5
`src/forms/formbid.cpp:94` and `:105`

```cpp
QList<QWidget *> wList = qFindChildren<QWidget *>(this);
```
`qFindChildren` is inside `QT_DEPRECATED_SINCE(5,0)` → *"‘qFindChildren’ was not
declared in this scope"*.
Replace with the member template:
```cpp
const QList<QWidget *> wList = findChildren<QWidget *>();
```
(`buttonList()`/`bidButtonList()`); the following `dynamic_cast` filtering stays.

### 3.2 `QString::toAscii()` — removed in Qt5
`src/desk/deskview.cpp:84`

```cpp
resname.sprintf("%i%i", face, suit);
fname.sprintf(QString(":/pics/cards/" + deckPath + "%i%i.png").toAscii(), face, suit);
```
`toAscii()` is inside `QT_DEPRECATED_SINCE(5,0)` → hard error. Do not merely swap
in `toLatin1()`; rewrite the whole block with `arg()` (also removes the
`QString::sprintf` dependency, see 4.2):
```cpp
resname = QString::number(face) + QString::number(suit);
fname   = QStringLiteral(":/pics/cards/%1%2%3.png").arg(deckPath).arg(face).arg(suit);
```

### 3.3 Missing resource file — run-time failure, not compile failure
`openpref.qrc` must be created in `src/` and compiled into the binary.
Every path in the code is `:/pics/<...>`, so the resource prefix must be `/`
with the files listed as `pics/...`:

```xml
<RCC>
  <qresource prefix="/">
    <file>pics/cloth.png</file>
    <file>pics/cards/classic/71.png</file>
    ... 111 entries, generated from `find pics -name '*.png'` ...
  </qresource>
</RCC>
```
Consumers, for the record: `deskview.cpp:84,94,95,100,176-181,413`,
`qbidbutton.cpp:28`, `formbid.cpp:180-208`, `kpref.cpp:130-151`.
`pics/*.xcf` and `pics/openpref.ico` stay out of the resource (source art /
Windows icon).

### 3.4 Includes that Qt4 headers provided transitively
Qt5 split `QtGui`/`QtWidgets` and trimmed header coupling, so a few translation
units now use symbols they never include. To be added (audit each during the
fix, they are cheap and harmless):

| File | Symbol(s) | Include to add |
|------|-----------|----------------|
| `main.cpp` | `strcmp`, `strcasecmp`, `getenv` (lines 56, 60, 66, 69) — only `<time.h>` is included | `<cstring>`, `<strings.h>`, `<cstdlib>` |
| `desk/desktop.cpp` | `QFile` at 412, 425, 608-632; `QMessageBox` at 254 | `<QFile>`, `<QMessageBox>` |
| `desk/deskview.cpp` | `QTextOption` at ~401, 460, 553; `QFont`/`QBrush` are fine via `QPainter` | `<QTextOption>` |
| `desk/deskview.h` | `QKeyEvent`, `QMouseEvent` in the `SleepEventLoop` declarations | `<QKeyEvent>`, `<QMouseEvent>` |
| `kpref.h` | `QKeyEvent` in `keyPressEvent()`, `QAction` members | `<QKeyEvent>`, `<QAction>` |

### 3.5 Dead Qt4 include
`main.cpp:26` — `#include <QTextCodec>` is unused (`QTextCodec` is a QtCore class
in Qt5 and is compiled out in `QT_NO_TEXTCODEC` builds). Delete the line.
Same audit for `deskview.cpp:32` (`QPixmap`), `kpref.cpp:29` (`QTextBrowser`) —
unused, can go, but harmless.

---

## 4. P1 — compiles with deprecation warnings (fix for a clean build)

### 4.1 `qsrand()` / `qrand()` — deprecated in 5.15
`main.cpp:69`, `desk/desktop.cpp:112`, `cardutil/cardlist.cpp:281`,
`logic/aiplayer.cpp:537, 1137, 1656`.

Present but `QT_DEPRECATED_VERSION_X_5_15`. Because `QT_DEPRECATED_WARNINGS_SINCE`
defaults to `QT_VERSION` (5.15.x), the compiler *will* warn on every use.

* `main.cpp`: drop the `qsrand((unsigned)time(0))` call.
* Game code: replace with `QRandomGenerator::global()->bounded(n)`, keeping the
  existing ranges exactly (`%4+1` → `bounded(4)+1`, `%3+1`, `% (f+1)`,
  `%4 == 0` → `bounded(4) == 0`).
* If reproducible games matter (the AI is a search engine with a timeout), keep
  a single seeded `QRandomGenerator` owned by the game instead of the global
  one; decision requested in §9.

### 4.2 `QString::sprintf()` — deprecated in 5.14
`deskview.cpp:83, 89, 100`, `qbidbutton.cpp:28, 29`.
Replace with `.arg()` (`:/pics/bids/s%1%2.png` style) or `QString::asprintf()`.
The pure C `sprintf(char*)` calls at `deskview.cpp:250, 267, 434` are fine.

### 4.3 `QDesktopWidget` / `QApplication::desktop()` — deprecated in 5.11
`kpref.cpp:24,71`, `forms/formbid.cpp:24,57`, `forms/optform.cpp:23,44`,
`forms/helpbrowser.cpp:23,44`, `forms/newgameform.cpp:23,44`.

All five sites do the same thing: centre a window on the available desktop.
Replace with a small shared helper (new `src/util/screenutil.h/.cpp`, or a
static in `baser.cpp`):

```cpp
static QRect availableGeometryFor(const QWidget *w) {
  const QScreen *s = w && w->screen() ? w->screen()
                                      : QGuiApplication::primaryScreen();
  return s ? s->availableGeometry() : QRect(0, 0, 1024, 768);
}
```
then `Kpref` uses `QRect dims(availableGeometryFor(this))`, the four dialogs the
same. Drop the `<QDesktopWidget>` includes. This is also the Qt6-ready form.

### 4.4 `Qt::CTRL + Qt::Key_N` shortcut arithmetic
`kpref.cpp:130-149` — compiles (int → `QKeySequence`), no warning. Optional
cleanup to `QKeySequence(Qt::CTRL | Qt::Key_N)`.

---

## 5. P2 — optional modernisation (behaviour-identical, not needed to build)

* **Menus**: `QMenu::addAction(icon, text, receiver, SLOT(...), shortcut)` is
  still declared in the 5.15 `QMenu` header (not deprecated there), so
  `kpref.cpp:130-151` builds as is. Converting to explicit `QAction` objects +
  `connect(act, &QAction::triggered, this, &Kpref::slot…)` would remove the
  remaining Qt4-flavoured code and prepare Qt6. Do it only if we want the tree
  to be Qt6-ready in one pass.
* **Message boxes**: `QMessageBox::question(this, title, text, Yes|Default, No|Escape)`
  (`kpref.cpp:320,437,465`, `formbid.cpp:66`) and
  `QMessageBox mb(caption, text, Information, Ok|Default, 0, 0)`
  (`deskview.cpp:483`) still exist in 5.15 (the "obsolete" block of
  `qmessagebox.h`). Behaviour on Haiku is fine. Leaving them is acceptable;
  modernising to `StandardButtons` + `defaultButton` is cosmetic.
* **Mouse events**: `event->x()/y()` (`kpref.cpp:264,275,276`,
  `deskview.cpp:158,159`) are **not** deprecated in 5.15. Leave.
* **Window icon**: `openpref.png` is never used as an icon. Add
  `setWindowIcon(QIcon(":/pics/openpref.png"))` in `Kpref::Kpref()` (also fixes
  the `.ui` files' dangling relative icon path `../../../../pics/tool.png`,
  which uic turns into a run-time file lookup that will never resolve —
  `optform.ui:31-33`).
* **`DeskView::GetImgByName`** (`deskview.cpp:113-116`) is missing its `return`
  in the not-found branch (`if (!cardI.contains(name)) cardI["empty"];`), so it
  returns `nullptr` for unknown names. `drawCard()` already recovers, so it is
  latent, not active. Worth a one-line fix while we are in the file.

---

## 6. Build system: replace fragments with one `CMakeLists.txt`

`AGENTS.md` requires `mkdir build && cd build && cmake .. && make`, `make clean`,
and CMake instead of qmake. There is no `.pro` to replace, so:

**Create `CMakeLists.txt` at the repo root** (so the three commands above work
verbatim from a fresh clone):

```cmake
cmake_minimum_required(VERSION 3.16)
project(openpref VERSION 0.1.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/src/ui)

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets)

add_executable(openpref
    src/src/main.cpp src/src/kpref.cpp src/src/prfconst.cpp
    src/src/util/*.cpp  src/src/cardutil/*.cpp  src/src/forms/*.cpp
    src/src/logic/*.cpp src/src/score/*.cpp      src/src/desk/*.cpp
    src/src/kpref.h src/src/main.h src/src/prfconst.h   # for AUTOMOC
    src/src/desk/deskview.h src/src/desk/desktop.h
    src/src/forms/*.h src/src/desk/*.h
    src/src/ui/optform.ui src/src/ui/newgameform.ui src/src/ui/helpbrowser.ui
    src/openpref.qrc
)
target_include_directories(openpref PRIVATE
    src/src src/src/util src/src/cardutil src/src/forms
    src/src/score src/src/logic src/src/desk)
target_link_libraries(openpref PRIVATE Qt5::Widgets)
```
(exact file list to be written out explicitly; `GLOB` avoided so new files are
deliberate). `main.h` contains only a commented-out `Q_OBJECT` class — moc emits
"No relevant classes" but that is harmless; it can be dropped from the list.

**Delete the legacy build files** once the root `CMakeLists.txt` builds:
`src/src/main.pri`, `src/src/*/*.pri` (6), `src/src/src.cmake`,
`src/src/*/*.cmake` (6), `src/cmake/FindLinguist.cmake` (contains
`QT4_WRAP_TS` + `lrelease-qt4`), `src/cmake/cmake_uninstall.cmake.in`.

**Translations**: no `.ts`/`.qm` files exist anywhere, so `QT4_WRAP_TS` has
nothing to do. Keep the `QTranslator` block in `main.cpp` (it fails silently and
is useful if translations are added later) but do not wire Linguist into CMake.

Optional and cheap: add a `.gitignore` with `build/` and `*.o`.

---

## 7. Haiku-specific notes

* Nothing in the code is Linux-specific: `unistd.h`, `localtime_r`, `getenv`,
  `fopen`/`sprintf` and `exit()` all exist on Haiku. No `/proc`, no `/usr`, no
  `/etc`, no `systemd`, no `libsocket`/`libnsl`.
* Qt5 lives under `/boot/system`; `find_package(Qt5 5.15)` resolves through
  `/boot/system/develop/lib/cmake/Qt5` once `qt5_devel` is installed. If the
  default prefix search misses it, add
  `-DCMAKE_PREFIX_PATH=/boot/system/develop`.
* `QSettings` will write to `~/config/settings/OpenPref/OpenPref.conf` — the
  Haiku-correct location, no change needed.
* `main.cpp:38-42` still computes an i18n directory as
  `applicationDirPath()/../share/openpref/i18n`. That is relative to the binary,
  not a hard-coded `/usr`, so it is harmless; leaving it keeps the code
  package-friendly once a `.hpkg` layout exists.
* If the first binary aborts with *"this application failed to start because no
  Qt platform plugin could be initialized"*, the Haiku QPA plugin is missing or
  not found — set `QT_QPA_PLATFORM_PLUGIN_PATH` or install the package that
  ships it. This is an environment issue, not a code issue.
* Application icon: Haiku reads the `BEOS:ICON` attribute rather than a window
  icon. Out of scope for the conversion; a `.rdef`/attribute step can follow if
  wanted.
* No extra packages are required beyond `qt5`, `qt5_devel`, `qt5_tools`. Qt
  Widgets only — no Qt Script despite the `qt5_script` packages being present.

---

## 8. Order of work (each step is a separate commit)

1. **T1 — plumbing**: `openpref.qrc` (111 PNGs) + root `CMakeLists.txt`. Compiles
   nothing new, but makes every later step testable. *(Also the step that makes
   the deck visible.)*
2. **T2 — P0 API**: `qFindChildren` → `findChildren` (formbid.cpp ×2);
   `toAscii`/`sprintf` rewrite in `loadCards()` (deskview.cpp); drop
   `<QTextCodec>` (main.cpp).
3. **T3 — includes**: add the missing standard/Qt includes from §3.4.
4. **T4 — P1 deprecations**: `qsrand`/`qrand` → `QRandomGenerator`;
   remaining `QString::sprintf` (qbidbutton.cpp); `QDesktopWidget` →
   `QScreen` helper in the 5 sites.
5. **T5 — encoding**: convert the 9 CP1251 files to UTF-8 (comments only; verify
   with `file` + a diff that is whitespace/encoding-only).
6. **T6 — cleanup**: delete `.pri` fragments, `*.cmake` fragments and
   `cmake/FindLinguist.cmake`; optional `.gitignore`.
7. **T7 — P2 polish** (only if wanted): menus → `QAction`,
   `setWindowIcon`, `GetImgByName` return bug, message boxes.
8. **T8 — docs**: short `README.md` build section
   (`mkdir build && cd build && cmake .. && make`).

---

## 9. Decisions needed before starting T4/T7

1. **Randomness**: `QRandomGenerator::global()` (simple, warning-free) or one
   seeded generator owned by the game (reproducible AI games, needs a little
   plumbing)? *Recommendation: global, matching current non-seeded-per-run
   behaviour.*
2. **Delete the legacy build files** (`.pri`, fragment `.cmake`,
   `cmake/FindLinguist.cmake`)? *Recommendation: yes — they are unreferenced and
   AGENTS.md asks for CMake instead of qmake.*
3. **Qt5-only or Qt6-ready code?** *Recommendation: Qt5-only compile target, but
   use the Qt6-compatible forms (`QScreen`, `arg()`, `QRandomGenerator`) so a
   later Qt6 pass is small.*
4. **P2 menu/message-box modernisation in this branch, or a later one?**
   *Recommendation: later — it is not needed to build or run.*

---

## 10. Verification (manual, on Haiku)

Per `AGENTS.md` the agent does not run builds; the following is the hand-over
checklist:

1. `mkdir build && cd build && cmake .. && make` → completes; expect no errors,
   at most ASCII-cast/deprecation warnings if any survive.
2. `make clean` works.
3. `./openpref` → window opens; **card faces, cloth, bid icons and menu icons
   are visible** (proves the `.qrc`); menu shortcuts Ctrl+N/O/S/P/R/Q work.
4. New game → bidding dialog → play a few tricks → score/first-score paper
   renders (rotated text is drawn with `QPainter::rotate`, unaffected by Qt5).
5. Options dialog: toggle "PrefClub Remix" (switches deck and card size) and
   "Show AI cards"; both must survive a restart (`QSettings`).
6. Save a game (`*.prf`) and re-open it (`PrefDesktop::saveGame/loadGame`,
   `QByteArray` serialization — unaffected by Qt5).
7. Restart and confirm geometry/centring still behaves (the `QScreen` change).

Any compiler error output should come back here for the next iteration.

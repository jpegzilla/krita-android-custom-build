#!/usr/bin/env python3
"""Experimental changes to KDE Krita v6.0.4 Android (Krita 5.3.4).
Provides assignable one-finger double-tap, Mirror View, narrow Layers
docker and a welcome-page .kra thumbnail gallery. Not compiled or tested.
"""
from pathlib import Path
import sys

from patch_paths import fragment

def main(root):
    if not (root / 'libs/ui/KisWelcomePageWidget.cpp').is_file():
        raise SystemExit('Pass the root of Krita tag v6.0.4')
    changed = {}
    def replace(path, old, new):
        data = changed.get(path)
        if data is None:
            data = (root / path).read_text(encoding='utf-8')
        count = data.count(old)
        if count != 1:
            raise RuntimeError(f'{path}: expected one match; found {count}. Use v6.0.4.')
        changed[path] = data.replace(old, new, 1)

    # Keep serialized IDs stable by appending the gesture to the enum.
    p = 'libs/ui/input/kis_shortcut_configuration.h'
    replace(p, '''        FiveFingerDrag,
        OneFingerHold,
#endif
        MaxGesture,''', '''        FiveFingerDrag,
        OneFingerHold,
        OneFingerDoubleTap,
#endif
        MaxGesture,''')
    p = 'libs/ui/input/kis_shortcut_configuration.cpp'
    replace(p, '''    case OneFingerHold:
        return i18n("One Finger Hold");
#endif''', '''    case OneFingerHold:
        return i18n("One Finger Hold");
    case OneFingerDoubleTap:
        return i18n("One Finger Double Tap");
#endif''')
    p = 'libs/ui/input/KisTouchGestureAction.h'
    replace(p, '''        ToggleEraserPreset,
    };''', '''        ToggleEraserPreset,
        MirrorView,
    };''')
    p = 'libs/ui/input/KisTouchGestureAction.cpp'
    replace(p, '''    shortcuts.insert(i18n("Activate Transform Tool"), KisToolTransform);
    setShortcutIndexes(shortcuts);''', '''    shortcuts.insert(i18n("Activate Transform Tool"), KisToolTransform);
    shortcuts.insert(i18n("Mirror View"), MirrorView);
    setShortcutIndexes(shortcuts);''')
    replace(p, '''    case ToggleEraserPreset:
        actionName = QStringLiteral("eraser_preset_action");
        break;
    }
''', '''    case ToggleEraserPreset:
        actionName = QStringLiteral("eraser_preset_action");
        break;
    case MirrorView:
        actionName = QStringLiteral("mirror_canvas");
        break;
    }
''')
    p = 'libs/ui/input/kis_input_manager_p.cpp'
    replace(p, '''    case KisShortcutConfiguration::OneFingerTap:
    case KisShortcutConfiguration::OneFingerDrag:
''', '''    case KisShortcutConfiguration::OneFingerTap:
    case KisShortcutConfiguration::OneFingerDoubleTap:
    case KisShortcutConfiguration::OneFingerDrag:
''')
    p = 'libs/ui/input/kis_touch_shortcut.h'
    replace(p, '''        bool isHoldType() const;
''', '''        bool isHoldType() const;
        bool isDoubleTapType() const;
''')
    replace(p, '''        bool matchTapType(QTouchEvent *event);
''', '''        bool matchTapType(QTouchEvent *event);
        bool matchDoubleTapType(QTouchEvent *event);
''')
    p = 'libs/ui/input/kis_touch_shortcut.cpp'
    replace(p, '''void KisTouchShortcut::setMinimumTouchPoints(int min)
''', '''bool KisTouchShortcut::isDoubleTapType() const
{
#ifdef Q_OS_MACOS
    return false;
#else
    return d->type == KisShortcutConfiguration::OneFingerDoubleTap;
#endif
}

void KisTouchShortcut::setMinimumTouchPoints(int min)
''')
    replace(p, '''bool KisTouchShortcut::matchDragType(QTouchEvent *event)
''', '''bool KisTouchShortcut::matchDoubleTapType(QTouchEvent *event)
{
    return isDoubleTapType() && matchTouchPoint(event);
}

bool KisTouchShortcut::matchDragType(QTouchEvent *event)
''')
    p = 'libs/ui/input/kis_shortcut_matcher.h'
    replace(p, '''    void fireReadyTouchShortcut(QTouchEvent *event);
''', '''    void fireReadyTouchShortcut(QTouchEvent *event, bool bypassDoubleTap = false);
''')
    p = 'libs/ui/input/kis_shortcut_matcher.cpp'
    replace(p, '''#include <QTabletEvent>
''', '''#include <QTabletEvent>
#include <QApplication>
#include <QElapsedTimer>
#include <QTimer>
''')
    replace(p, '''    QScopedPointer<QEvent> bestCandidateTouchEvent;
''', '''    QScopedPointer<QEvent> bestCandidateTouchEvent;

    // Delay a single tap only when a double-tap shortcut is assigned.
    QTimer pendingSingleTapTimer;
    QElapsedTimer lastOneFingerTapAge;
    QPointF lastOneFingerTapPosition;
    QScopedPointer<QEvent> pendingSingleTapEvent;
''')
    replace(p, '''KisShortcutMatcher::KisShortcutMatcher()
    : m_d(new Private)
{}
''', '''KisShortcutMatcher::KisShortcutMatcher()
    : m_d(new Private)
{
    m_d->pendingSingleTapTimer.setSingleShot(true);
    QObject::connect(&m_d->pendingSingleTapTimer, &QTimer::timeout, [this]() {
        if (m_d->pendingSingleTapEvent) {
            auto *queued = static_cast<QTouchEvent *>(m_d->pendingSingleTapEvent.data());
            fireReadyTouchShortcut(queued, true);
            m_d->pendingSingleTapEvent.reset();
        }
        m_d->lastOneFingerTapAge.invalidate();
    });
}
''')
    replace(p, '''void KisShortcutMatcher::fireReadyTouchShortcut(QTouchEvent *event)
{
    KisTouchShortcut *goodCandidate = matchTouchShortcut(event);
    if (goodCandidate) {
        DEBUG_TOUCH_ACTION("starting", event)
        goodCandidate->action()->activate(goodCandidate->shortcutIndex());
        goodCandidate->action()->begin(goodCandidate->shortcutIndex(), event);

        goodCandidate->action()->end(event);
        goodCandidate->action()->deactivate(goodCandidate->shortcutIndex());
    }
}
''', fragment("customizations/148-void-kisshortcutmatcher-fire.inc"))
    replace(p, '''void KisShortcutMatcher::clearShortcuts()
{
    reset("Clearing shortcuts");
''', '''void KisShortcutMatcher::clearShortcuts()
{
    m_d->pendingSingleTapTimer.stop();
    m_d->pendingSingleTapEvent.reset();
    m_d->lastOneFingerTapAge.invalidate();
    reset("Clearing shortcuts");
''')

    # Layer controls: keep the necessary commands, move some to an overflow menu.
    p = 'plugins/dockers/layerdocker/LayerBox.cpp'
    replace(p, '''    m_wdgLayerBox->setupUi(mainWidget);
''', '''    m_wdgLayerBox->setupUi(mainWidget);
#ifdef Q_OS_ANDROID
    auto *moreLayers = new QToolButton(mainWidget);
    moreLayers->setText(i18n("More"));
    moreLayers->setToolTip(i18n("Additional layer actions"));
    moreLayers->setPopupMode(QToolButton::InstantPopup);
    auto *moreMenu = new QMenu(moreLayers);
    auto overflow = [moreMenu](const QString &label, QAbstractButton *button) {
        QAction *action = moreMenu->addAction(label);
        QObject::connect(action, &QAction::triggered, button, &QAbstractButton::click);
        button->hide();
    };
    overflow(i18n("Duplicate Layer"), m_wdgLayerBox->bnDuplicate);
    overflow(i18n("Move Layer Down"), m_wdgLayerBox->bnLower);
    overflow(i18n("Move Layer Up"), m_wdgLayerBox->bnRaise);
    overflow(i18n("Layer Properties"), m_wdgLayerBox->bnProperties);
    moreLayers->setMenu(moreMenu);
    m_wdgLayerBox->hbox1->addWidget(moreLayers);
    m_wdgLayerBox->opacityLabel->hide();
    m_wdgLayerBox->cmbComposite->setMinimumWidth(0);
    m_wdgLayerBox->cmbComposite->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_wdgLayerBox->doubleOpacity->setMinimumWidth(0);
    m_wdgLayerBox->doubleOpacity->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_wdgLayerBox->listLayers->setMinimumWidth(0);
    m_wdgLayerBox->bnLayerFilters->setFixedSize(24, 24);
    m_wdgLayerBox->configureLayerDockerToolbar->setFixedSize(24, 24);
    mainWidget->setMinimumWidth(85);
#endif
''')

    # Non-destructive thumbnail gallery on Android Welcome page. SAF URI folders
    # cannot be recursively enumerated with QDir; recent file entries are fallback.
    p = 'libs/ui/KisWelcomePageWidget.cpp'
    replace(p, '''#include <QDir>
''', '''#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QPushButton>
#include <QSettings>
#include <QSet>
#include <algorithm>
''')
    replace(p, '''        connect(openFileLink, SIGNAL(clicked(bool)), this, SLOT(slotOpenFileClicked()));
''', fragment("customizations/276-connect-openfilelink-signal-.inc"))
    # KDE's older Exiv2 finder expects "(0)" while current headers use "(0U)".
    # The generated EXV_PACKAGE_VERSION string is stable across both formats.
    p = 'cmake/modules/FindLibExiv2.cmake'
    replace(p, '''include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibExiv2''', '''# Exiv2 0.28.x may generate "(0U)" version macros, which the older
# three separate numeric regexes above do not recognize (result: "..").
# Recover the actual installed version from its generated header or .pc file.
if(NOT LibExiv2_VERSION OR LibExiv2_VERSION STREQUAL "..")
    foreach(_exiv2_header "exv_conf.h" "version.hpp")
        set(_exiv2_header_path "${LibExiv2_INCLUDE_DIRS}/exiv2/${_exiv2_header}")
        if(EXISTS "${_exiv2_header_path}")
            file(STRINGS "${_exiv2_header_path}" _exiv2_package_version
                 REGEX "^#[ \t]*define[ \t]+EXV_PACKAGE_VERSION[ \t]+")
            if(_exiv2_package_version)
                string(REGEX MATCH "[0-9]+[.][0-9]+[.][0-9]+"
                       _exiv2_detected_version "${_exiv2_package_version}")
                if(_exiv2_detected_version)
                    set(LibExiv2_VERSION "${_exiv2_detected_version}")
                    break()
                endif()
            endif()
        endif()
    endforeach()
endif()
if(NOT LibExiv2_VERSION OR LibExiv2_VERSION STREQUAL "..")
    set(_exiv2_pc "${LibExiv2_INCLUDE_DIRS}/../lib/pkgconfig/exiv2.pc")
    if(EXISTS "${_exiv2_pc}")
        file(STRINGS "${_exiv2_pc}" _exiv2_pc_version REGEX "^Version:[ \t]*")
        string(REGEX MATCH "[0-9]+[.][0-9]+[.][0-9]+"
               _exiv2_detected_version "${_exiv2_pc_version}")
        if(_exiv2_detected_version)
            set(LibExiv2_VERSION "${_exiv2_detected_version}")
        endif()
    endif()
endif()
if(LibExiv2_VERSION AND NOT LibExiv2_VERSION STREQUAL "..")
    message(STATUS "Detected installed Exiv2 version: ${LibExiv2_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibExiv2''')

    for name, contents in changed.items():
        (root / name).write_text(contents, encoding='utf-8')
        print('Patched:', name)
    print('Patch completed. This does not build an APK.')

if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('Usage: python3 apply_customizations.py /path/to/Krita-v6.0.4')
    main(Path(sys.argv[1]).expanduser().resolve())

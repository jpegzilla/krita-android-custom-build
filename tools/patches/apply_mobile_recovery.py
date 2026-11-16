#!/usr/bin/env python3
"""Add a safe, on-demand autosave/backup recovery browser to Krita's mobile gallery."""
from pathlib import Path
import sys

from patch_paths import fragment, replace_once


def main(root):
    path=Path(root)/"libs/ui/KisWelcomePageWidget.cpp"
    s=path.read_text(encoding="utf-8")
    s=replace_once(s, '#include <QLineEdit>\n',
          '#include <QLineEdit>\n#include "KisAutoSaveRecoveryDialog.h"\n',
          "native recovery location header")
    s=replace_once(s,
'''        galleryControls->setColumnStretch(0,1);''',
'''        auto *recoverButton=new QPushButton(i18n("Recover"),galleryPage);
        recoverButton->setIcon(KisIconUtils::loadIcon("document-revert"));
        recoverButton->setMinimumHeight(43);
        recoverButton->setMinimumWidth(84);
        recoverButton->setToolTip(i18n("Find autosaves and backup projects"));
        galleryControls->addWidget(recoverButton,0,0);
        galleryControls->setColumnStretch(0,1);''',
          "recovery toolbar control")
    code=fragment("mobile_recovery/25-connect-recoverbutton-qpushb.inc")
    s=replace_once(s,'''        galleryControls->setColumnStretch(0,1);''',
          '''        galleryControls->setColumnStretch(0,1);'''+code,
          "recovery browser")
    path.write_text(s,encoding="utf-8")
    print("Recovery browser installed: on-demand scan, copy-only restore, non-destructive")

if __name__=="__main__":
    if len(sys.argv)!=2:
        raise SystemExit("Usage: apply_mobile_recovery.py KRITA_SOURCE")
    main(sys.argv[1])

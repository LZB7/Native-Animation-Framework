#!/usr/bin/env python3
from phase_d_audit_common import audit

raise SystemExit(audit("Creator", {
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/MainMenuHandler.h": ('{ "Creator", BindGoto(kCreator) }',),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/CreatorMenuHandler.h": (
        '"New Project"', '"Load Project"', '"Save Project"', '"[+] New Face Animation"',
        '"[+] New Body Animation"', '"[*] Setup Multi-Actor Studio Instance"',
        '"[->] Open in Animation Studio"', '"[->] Open in Face Animator"',
        'std::filesystem::create_directories(basePath, directoryError)'),
    "f4se-plugin/src/Menu/IStateManager.h": (
        'std::filesystem::create_directories("Data\\\\NAF", directoryError)',
        "return outDoc.save_file(xmlPath.c_str())"),
}))

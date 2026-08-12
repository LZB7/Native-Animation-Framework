#!/usr/bin/env python3
from phase_d_audit_common import audit

raise SystemExit(audit("Studio", {
    "f4se-plugin/src/Menu/Menu.h": ('RegisterMenu("NAFStudioMenu", Menu::NAFStudioMenu::Create)',),
    "f4se-plugin/src/Menu/NAFStudioMenu/NAFStudioMenu.h": (
        'LoadMovieEx(*this, "Interface/NAFStudioMenu.swf"', "SetupBake()", "CalculateInverseKinematics()",
        "SetUseExternalInput(true)", "SetUseExternalInput(false)", "ClearManagedRefs()",
        'GetScreenDimension("iSize W:Display")', 'GetScreenDimension("iSize H:Display")'),
}, ("REL::ID(1302430)", "REL::ID(644641)")))

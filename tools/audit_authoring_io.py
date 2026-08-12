#!/usr/bin/env python3
from phase_d_audit_common import audit

raise SystemExit(audit("authoring file IO", {
    "f4se-plugin/src/Menu/IStateManager.h": (
        'std::format("Data\\\\NAF\\\\{}_project", projectName)',
        'std::format("{}.xml", basePath)', 'std::format("{}.nanim", basePath)',
        "bodyAnims.SaveToFile(nanimPath)", "return outDoc.save_file(xmlPath.c_str())"),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/CreatorMenuHandler.h": (
        '"Data\\\\NAF\\\\pose_snapshot.nanim"', '"_project.xml"', "doc.load_file(path.c_str())"),
    "ck/project_files/NAF/NAF_graphInfo.xml": ('basePoseFile="Animations/Human/BasePose.nanim"',),
}))

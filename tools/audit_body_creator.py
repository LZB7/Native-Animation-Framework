#!/usr/bin/env python3
from phase_d_audit_common import audit

raise SystemExit(audit("Body Creator", {
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/BodyCreatorHandler.h": (
        "GetTargetNodes", "GetTargetChains", "ToggleChainEnabled", "ResetTargetChain",
        "BakeAnimation", "PackageAnim", "SaveChanges"),
    "f4se-plugin/src/Menu/NAFStudioMenu/NAFStudioMenu.h": (
        "SaveToNANIM", "SetupBake", "GenerateAtTime", "CalculateInverseKinematics", "SampleAtTime"),
}))

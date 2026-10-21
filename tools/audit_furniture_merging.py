"""Audit NAF Extended's furniture-only deterministic aggregation contract."""

from itertools import permutations
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
GLOBAL = (ROOT / "f4se-plugin/src/Data/Global.h").read_text(encoding="utf-8")
FURNITURE = (ROOT / "f4se-plugin/src/Data/User/Furniture.h").read_text(encoding="utf-8")
FORMS = (ROOT / "f4se-plugin/src/Data/User/IdentifiableObject.h").read_text(encoding="utf-8")
CACHE = (ROOT / "f4se-plugin/src/Data/Cache/XMLCache.h").read_text(encoding="utf-8")
FIXTURES = ROOT / "tests/fixtures/furniture"


def require(text: str, token: str, label: str) -> None:
    if token not in text:
        raise SystemExit(f"FAIL: missing {label}: {token}")


for token in (
    "class FurnitureMap : public IDMap<Furniture>",
    "void merge_insert(std::shared_ptr<Furniture> contribution)",
    "target.second->MergeFrom(std::move(*contribution))",
    "Furnitures.merge_insert(std::move(obj))",
    "entry.second.second->FinalizeEntries()",
    "f.second.second->LinkAndDedupe(verbose)",
    "Merged furniture group",
):
    require(GLOBAL, token, "furniture-specific registration")

require(GLOBAL, "void priority_insert(std::shared_ptr<T> ele)", "unchanged generic priority insertion")
if "map.merge_insert(obj)" in GLOBAL:
    raise SystemExit("FAIL: furniture merge policy leaked into generic registries")

for token in (
    "sourceFile = std::filesystem::path(m.GetFileName()).lexically_normal().generic_string()",
    "void MergeFrom(Furniture&& other)",
    "other.loadPriority > loadPriority",
    "other.loadPriority == loadPriority && other.sourceFile < sourceFile",
    "void FinalizeEntries()",
    "void LinkAndDedupe(bool verbose)",
    "seenForms.insert(resolved).second",
    "seenKeywords.insert(resolved).second",
):
    require(FURNITURE, token, "merge/deduplication behavior")

require(FORMS, "std::string RawIdentity() const", "canonical pre-link form identity")
require(FORMS, 'std::format("{}:{:08X}", source, formID)', "plugin-qualified numeric form identity")
for token in ("currentFiles.size() != primaryCache.files.size()", "primaryCache.files", "cached.filename"):
    require(CACHE, token, "source-set-aware XML cache")
for token in ("Furnitures.clear()", "XMLCache::Delete()", "Init(false)", "LinkDataReferences(false)"):
    require(GLOBAL, token, "reload reconstruction")


def parse(path: Path):
    root = ET.parse(path).getroot()
    group = root.find("group")
    if group is None:
        raise ValueError("missing group")
    forms = []
    keywords = []
    for item in group.findall("furniture"):
        source, form, keyword = item.get("source", ""), item.get("form", ""), item.get("keyword", "")
        if source or form:
            forms.append((source.casefold(), int(form, 16) if form else 0))
        if keyword:
            keywords.append(keyword)
    return {
        "id": group.get("id"),
        "priority": int(group.get("loadPriority", "0")),
        "source": path.as_posix(),
        "start": group.get("startAnimation"),
        "stop": group.get("stopAnimation"),
        "forms": forms,
        "keywords": keywords,
    }


def merge(definitions):
    result = None
    for item in definitions:
        if result is None:
            result = {**item, "forms": list(item["forms"]), "keywords": list(item["keywords"]), "contributors": 1}
            continue
        result["forms"].extend(item["forms"])
        result["keywords"].extend(item["keywords"])
        result["contributors"] += 1
        if item["priority"] > result["priority"] or (
            item["priority"] == result["priority"] and item["source"] < result["source"]
        ):
            for key in ("priority", "source", "start", "stop"):
                result[key] = item[key]
    result["forms"] = sorted(set(result["forms"]))
    result["keywords"] = sorted(set(result["keywords"]))
    return result


definitions = [parse(path) for path in sorted(FIXTURES.glob("shared-furniture-*.xml"))]
if len(definitions) != 3 or {item["id"] for item in definitions} != {"Phase4SharedFurniture"}:
    raise SystemExit("FAIL: fixtures must provide three definitions of Phase4SharedFurniture")

expected = merge(definitions)
assert expected["contributors"] == 3
assert expected["forms"] == [
    ("fallout4.esm", 0x0006F9DC),
    ("fallout4.esm", 0x000AB568),
    ("fallout4.esm", 0x001340D3),
    ("fallout4.esm", 0x0023C9C4),
]
assert expected["keywords"] == ["AnimFurnChairSitAnims", "AnimFurnChairWithRadio", "IsSleepFurniture"]
assert expected["source"].endswith("shared-furniture-b.xml")
assert expected["start"] is None and expected["stop"] == "ChairSitExit"

normalized = (expected["forms"], expected["keywords"], expected["source"], expected["start"], expected["stop"])
for ordering in permutations(definitions):
    candidate = merge(ordering)
    actual = (candidate["forms"], candidate["keywords"], candidate["source"], candidate["start"], candidate["stop"])
    if actual != normalized:
        raise SystemExit("FAIL: furniture merge depends on parse completion order")

# Reload is reconstruction, not accumulation: identical source input yields identical output.
assert merge(definitions) == merge(definitions)
assert merge(definitions[:2])["forms"] == [
    ("fallout4.esm", 0x0006F9DC),
    ("fallout4.esm", 0x001340D3),
    ("fallout4.esm", 0x0023C9C4),
]
unique = parse(FIXTURES / "unique-furniture.xml")
assert unique["id"] == "Phase4UniqueFurniture"
assert merge([unique])["forms"] == [("fallout4.esm", 0x0023C9C5)]
position_root = ET.parse(FIXTURES / "phase4-position.xml").getroot()
position = position_root.find("position")
assert position is not None
assert position.get("id") == "Phase4 Furniture Merge Test"
assert position.get("animation") == "RBBelly1"
assert position.get("location") == "Phase4SharedFurniture"

try:
    ET.fromstring("<furnitureData><group")
except ET.ParseError:
    pass
else:
    raise SystemExit("FAIL: malformed contributor was accepted")

print("PASS: furniture groups merge only in the furniture registry; three contributors normalize to four forms and three keywords with duplicate removal, deterministic priority/path metadata selection, source-set cache invalidation, and clean reload reconstruction")


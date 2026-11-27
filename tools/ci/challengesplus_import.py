#!/usr/bin/env python3
"""Regenerate the Challenges+ C datasets from ImperfectDark-Docs/challenges+/.

Each challenge-NN.md is the source of truth for one Plus challenge config,
its description/bot names, its medal bonus requirement and any sim slots
beyond the ROM's eight. Synopsis:

    python3 tools/ci/challengesplus_import.py [--check] [<repo-root>]

Without --check the generated C files are written in place. With --check the
files are regenerated in memory and diffed against what is on disk, so CI can
catch drift.
"""
import difflib
import json
import os
import re
import sys

CHECK = "--check" in sys.argv
args = [a for a in sys.argv[1:] if a != "--check"]
root = args[0] if args else os.path.dirname(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
docs = os.path.join(root, "ImperfectDark-Docs", "challenges+")
srcdir = os.path.join(root, "src", "game")
configs_path = os.path.join(srcdir, "mpconfigsplus.c")
strings_path = os.path.join(srcdir, "mpstringsplus.c")

DIFFICULTIES = {
    "Meat": "BOTDIFF_MEAT",
    "Easy": "BOTDIFF_EASY",
    "Normal": "BOTDIFF_NORMAL",
    "Hard": "BOTDIFF_HARD",
    "Perfect": "BOTDIFF_PERFECT",
    "Dark": "BOTDIFF_DARK",
    "Off": "BOTDIFF_DISABLED",
}

BONUS_IDS = {
    "NONE": "CHALBONUS_NONE",
    "HEADSHOTS": "CHALBONUS_HEADSHOTS",
    "ACCURACY": "CHALBONUS_ACCURACY",
    "KILLS": "CHALBONUS_KILLS",
    "DEATHS": "CHALBONUS_DEATHS",
    "KD_RATIO": "CHALBONUS_KD_RATIO",
    "FASTEST_2KILLS": "CHALBONUS_FASTEST_2KILLS",
    "LONGEST_LIFE": "CHALBONUS_LONGEST_LIFE",
    "MAX_SIMUL_KILLS": "CHALBONUS_MAX_SIMUL_KILLS",
    "MULTIKILLS": "CHALBONUS_MULTIKILLS",
    "LONGEST_SPREE": "CHALBONUS_LONGEST_SPREE",
    "TOKEN_HELD_TIME": "CHALBONUS_TOKEN_HELD_TIME",
    "BACKSHOTS": "CHALBONUS_BACKSHOTS",
    "DISTANCE": "CHALBONUS_DISTANCE",
    "DAM_RECEIVED": "CHALBONUS_DAM_RECEIVED",
    "ITEM_USE": "CHALBONUS_ITEM_USE",
    "FINISH_TIME": "CHALBONUS_FINISH_TIME",
}

START_ARMED = {
    "none": "SPAWNWEAPON_OFF",
    "off": "SPAWNWEAPON_OFF",
    "first": "SPAWNWEAPON_FIRST",
    "fiesta": "SPAWNWEAPON_RANDOM",
    "random": "SPAWNWEAPON_RANDOM",
    "loadout": "SPAWNWEAPON_LOADOUT",
}

DISABLED_SIM = ("BOTTYPE_GENERAL", "MPHEAD_DARK_COMBAT", "MPBODY_DARK_COMBAT",
                "MPTEAM_1", ["BOTDIFF_DISABLED"] * 4)

# Human-readable names for the description panel, in the same order as the
# registry. NONE is omitted from the panel entirely.
BONUS_LABELS = {
    "NONE": None,
    "HEADSHOTS": "Headshots",
    "ACCURACY": "Accuracy",
    "KILLS": "Kills",
    "DEATHS": "Deaths",
    "KD_RATIO": "K/D Ratio",
    "FASTEST_2KILLS": "Fastest Two Kills",
    "LONGEST_LIFE": "Longest Life",
    "MAX_SIMUL_KILLS": "Max Simultaneous Kills",
    "MULTIKILLS": "Multi-Kills",
    "LONGEST_SPREE": "Longest Spree",
    "TOKEN_HELD_TIME": "Objective Time",
    "BACKSHOTS": "Back Shots",
    "DISTANCE": "Distance",
    "DAM_RECEIVED": "Damage Received",
    "ITEM_USE": "Item Use",
    "FINISH_TIME": "Finish Time",
}

# These are written in seconds in the template and converted to ticks for the
# grader; the panel shows the author's seconds with an s suffix.
BONUS_TIME_TAGS = {"FINISH_TIME", "TOKEN_HELD_TIME", "LONGEST_LIFE", "FASTEST_2KILLS"}
BONUS_PERCENT_TAGS = {"ACCURACY", "KD_RATIO"}


def tier_description(data, tier):
    label = BONUS_LABELS[data["bonus_req"]]
    if label is None:
        return data["description"]

    def fmt(value):
        suffix = "s" if data["bonus_req"] in BONUS_TIME_TAGS else (
            "%" if data["bonus_req"] in BONUS_PERCENT_TAGS else "")
        return f"{value}{suffix}"

    return "\n".join([
        data["description"],
        "",
        f"Bonus - {label}",
        "Bronze - Win",
        f"Silver - {fmt(data['silver'][tier])}",
        f"Gold - {fmt(data['gold'][tier])}",
        f"Perfect - {fmt(data['perfect'][tier])}",
    ])

# Every bare identifier written into the generated C must be a real #define.
DEFINES = set()
with open(os.path.join(root, "src", "include", "constants.h")) as fh:
    for line in fh:
        m = re.match(r"\s*#define\s+(\w+)", line)
        if m:
            DEFINES.add(m.group(1))


def check_ident(value, path, what):
    for token in re.findall(r"[A-Za-z_]\w*", value):
        if token.startswith("MP") and token not in DEFINES:
            raise ValueError(f"{path}: unknown {what} identifier {token}")


def ident(text, pattern, path, what):
    m = re.search(pattern, text, re.M)
    if not m:
        raise ValueError(f"{path}: missing {what}")
    return m.group(1)


def table_cell(text, field, path):
    m = re.search(r"^\|\s*" + re.escape(field) + r"\s*\|\s*`?([^|`]+?)`?\s*\|", text, re.M)
    if not m:
        raise ValueError(f"{path}: missing table field {field}")
    return m.group(1).strip()


def parse_tier_values(value, field, path):
    parts = [p.strip() for p in value.split(",")]
    if len(parts) == 1:
        parts = parts * 4
    if len(parts) != 4 or any(not re.fullmatch(r"-?\d+", p) for p in parts):
        raise ValueError(f"{path}: {field} must be one integer or four comma-separated integers: {value}")
    nums = [int(p) for p in parts]
    for x in nums:
        if x < 0 or x > 400:
            raise ValueError(f"{path}: {field} value {x} outside 0-400")
    return nums


def parse_challenge(n):
    # A template renamed to challenge+-NN.md (or challenge+NN.md) is the
    # marker that the author has finished converting that challenge. The
    # in-game Plus list shows only converted entries.
    for name in (f"challenge+-{n:02d}.md", f"challenge+{n:02d}.md",
                 f"challenge-{n:02d}.md"):
        candidate = os.path.join(docs, name)
        if os.path.exists(candidate):
            path = candidate
            break
    else:
        raise ValueError(f"missing template for challenge {n:02d}")
    converted = os.path.basename(path).startswith("challenge+")
    text = open(path).read()

    m = re.search(r"^- \*\*Description\*\*:\n((?:    .*\n)+)", text, re.M)
    if not m:
        raise ValueError(f"{path}: missing description")
    description = "\n".join(line.strip() for line in m.group(1).strip().splitlines())

    data = {
        "converted": converted,
        "description": description,
        "scenario": ident(text, r"- \*\*Scenario\*\*:\s*`([^`]+)`", path, "scenario"),
        "stage": ident(text, r"- \*\*Arena\*\*:\s*`([^`]+)`", path, "arena"),
        "timelimit": table_cell(text, "Time limit", path),
        "options": table_cell(text, "Options", path),
        "startarmed": table_cell(text, "Start armed", path).lower(),
        "chrslots": table_cell(text, "chrslots (sim bits)", path),
        "bonus_req": table_cell(text, "bonus_requirement", path),
        "weapons": [],
        "sims": [],
    }
    data["scorelimits"] = parse_tier_values(
        table_cell(text, "Score limit (1p/2p/3p/4p)", path), "score limit", path)
    data["silver"] = parse_tier_values(
        table_cell(text, "silver (1p/2p/3p/4p)", path), "silver", path)
    data["gold"] = parse_tier_values(
        table_cell(text, "gold (1p/2p/3p/4p)", path), "gold", path)
    data["perfect"] = parse_tier_values(
        table_cell(text, "perfect (1p/2p/3p/4p)", path), "perfect", path)
    if data["bonus_req"] not in BONUS_IDS:
        raise ValueError(f"{path}: unknown bonus_requirement {data['bonus_req']}")

    # The examples list must show every valid tag so authors cannot miss one.
    tagblock = re.search(r"Valid `bonus_requirement` tags.*?Each tag maps", text, re.S)
    if not tagblock:
        raise ValueError(f"{path}: missing bonus_requirement examples list")
    for tag in BONUS_IDS:
        if f"`{tag}`" not in tagblock.group(0):
            raise ValueError(f"{path}: examples list missing tag {tag}")
    if not re.fullmatch(r"-?\d+", data["timelimit"]):
        raise ValueError(f"{path}: timelimit is not an integer: {data['timelimit']}")
    if data["startarmed"] not in START_ARMED:
        raise ValueError(f"{path}: Start armed must be none/first/fiesta/loadout, got {data['startarmed']}")

    for slot in range(1, 7):
        data["weapons"].append(ident(
            text, rf"^\|\s*{slot}\s*\|\s*`([^`]+)`", path, f"weapon {slot}"))

    row_re = re.compile(
        r"^\|\s*(\d+)\s*\|\s*`([^`]+)`\s*\|\s*`([^`]+)`\s*\|\s*`([^`]+)`\s*\|"
        r"\s*`([^`]+)`\s*\|\s*`([^`]+)`\s*\|\s*([^|]*?)\s*\|$", re.M)
    rows = []
    for m in row_re.finditer(text):
        num = int(m.group(1))
        diffs = [d.strip() for d in m.group(6).split(",")]
        if len(diffs) != 4 or any(d not in DIFFICULTIES for d in diffs):
            raise ValueError(f"{path}: sim {num} has bad difficulties {diffs}")
        rows.append({
            "num": num,
            "type": m.group(2),
            "head": m.group(3),
            "body": m.group(4),
            "team": m.group(5),
            "diffs": [DIFFICULTIES[d] for d in diffs],
            "name": m.group(7).strip(),
        })
    if len(rows) < 8 or len(rows) > 16:
        raise ValueError(f"{path}: expected 8-16 sim rows, got {len(rows)}")
    if [r["num"] for r in rows] != list(range(1, len(rows) + 1)):
        raise ValueError(f"{path}: sim row numbers must be 1..N in order")
    data["sims"] = rows

    check_ident(data["scenario"], path, "scenario")
    check_ident(data["stage"], path, "arena")
    check_ident(data["options"], path, "options")
    for w in data["weapons"]:
        check_ident(w, path, "weapon")
    for sim in rows:
        for key in ("type", "head", "body", "team"):
            check_ident(sim[key], path, f"sim {key}")
    return data


def cstr(s):
    return json.dumps(s)


def sim_line(sim, indent="\t\t\t"):
    d = ", ".join(f"{x:<18}" for x in sim["diffs"])
    return (f'{indent}{{ {sim["type"]}, {sim["head"]}, {sim["body"]}, '
            f'{sim["team"]}, {{ {d} }} }},')


def config_entry(n, d):
    lines = [
        f"\t// MPCONFIG_CHALLENGE{n:02d}",
        "\t{",
        "\t\t{",
        '\t\t\t"",',
        f'\t\t\t{d["options"]},',
        f'\t\t\t{d["scenario"]},',
        f'\t\t\t{d["stage"]},',
        # One score row feeds the team limit; the player score limit is off.
        f'\t\t\t{d["timelimit"]}, 100, {d["scorelimits"][0]},',
        f'\t\t\t{d["chrslots"]},',
        "\t\t\t{",
    ]
    for w in d["weapons"]:
        lines.append(f"\t\t\t\t{w},")
    lines += ["\t\t\t},", "\t\t}, {"]
    for sim in d["sims"][:8]:
        lines.append(sim_line(sim))
    lines += ["\t\t},", "\t},"]
    return "\n".join(lines)


def strings_entry(n, d):
    if len(d["description"]) >= 200:
        raise ValueError(f"challenge-{n:02d}: description is {len(d['description'])} chars, limit is 199")
    lines = [
        f"\t// MPCONFIG_CHALLENGE{n:02d}",
        "\t{",
        f'\t\t{cstr(d["description"])},',
        "\t\t{",
    ]
    for sim in d["sims"][:8]:
        lines.append(f'\t\t\t{cstr(sim["name"] + chr(10))},')
    lines += ["\t\t},", "\t},"]
    return "\n".join(lines)


def regenerate(path, entries, array_decl):
    text = open(path).read()
    start = text.index("\t// MPCONFIG_CHALLENGE01")
    end = text.rindex("};")
    prefix = text[:start]
    suffix = text[end:]
    extra = "\n\n" + array_decl if array_decl else ""
    return prefix + "\n".join(entries) + "\n" + suffix.rstrip("\n") + extra + "\n"


def build_arrays(all_data):
    converted = ["u8 g_MpChallengesPlusConverted[30] = {"]
    startarmed = ["u8 g_MpPlusStartArmed[30] = {"]
    scorelimits = ["u16 g_MpPlusScoreLimits[30][4] = {"]
    bonus = ["struct challengebonus g_MpChallengeBonusPlus[30] = {"]
    extra_sims = ["struct mpconfigsim g_MpPlusExtraSims[30][8] = {"]
    extra_names = ["char g_MpPlusExtraBotNames[30][8][15] = {"]
    tier_desc = ['char g_MpPlusTierDescriptions[30][4][320] = {']

    for n in range(1, 31):
        d = all_data[n]
        converted.append(f"\t{1 if d['converted'] else 0},")
        startarmed.append(f"\t{START_ARMED[d['startarmed']]},")
        scorelimits.append("\t{ " + ", ".join(str(v) for v in d["scorelimits"]) + " },")
        bid = BONUS_IDS[d["bonus_req"]]
        # Times are written in seconds in the docs and stored as 60 Hz ticks.
        def value(v):
            x = int(v)
            if d["bonus_req"] in ("FINISH_TIME", "TOKEN_HELD_TIME",
                                  "LONGEST_LIFE", "FASTEST_2KILLS"):
                return x * 60  # SECSTOTIME60
            return x
        silver = ", ".join(str(value(v)) for v in d["silver"])
        gold = ", ".join(str(value(v)) for v in d["gold"])
        perfect = ", ".join(str(value(v)) for v in d["perfect"])
        bonus.append(f"\t{{ {bid}, {{ {silver} }}, {{ {gold} }}, {{ {perfect} }} }},")

        extra = d["sims"][8:]
        extra_sims.append("\t{")
        extra_names.append("\t{")
        for _ in range(8):
            if extra:
                sim = extra.pop(0)
                extra_sims.append("\t\t" + sim_line(sim, "").strip())
                extra_names.append(f'\t\t{cstr(sim["name"] + chr(10))},')
            else:
                extra_sims.append("\t\t" + sim_line(
                    dict(zip(("type", "head", "body", "team", "diffs"),
                             DISABLED_SIM)), "").strip())
                extra_names.append('\t\t"\\n",')
        extra_sims.append("\t},")
        extra_names.append("\t},")

        tier_desc.append("\t{")
        for tier in range(4):
            text = tier_description(d, tier)
            if len(text) >= 320:
                raise ValueError(f"challenge-{n:02d}: tier {tier} description is {len(text)} chars, limit is 319")
            tier_desc.append(f"\t\t{cstr(text)},")
        tier_desc.append("\t},")

    converted.append("};")
    startarmed.append("};")
    scorelimits.append("};")
    bonus.append("};")
    extra_sims.append("};")
    extra_names.append("};")
    tier_desc.append("};")
    return ("\n".join(converted), "\n".join(startarmed), "\n".join(scorelimits),
            "\n".join(bonus), "\n".join(extra_sims), "\n".join(extra_names),
            "\n".join(tier_desc))


all_data = {n: parse_challenge(n) for n in range(1, 31)}
cv, sa, sl, b, es, en, td = build_arrays(all_data)
config_text = regenerate(configs_path,
                         [config_entry(n, all_data[n]) for n in range(1, 31)],
                         cv + "\n\n" + sa + "\n\n" + sl + "\n\n" + b + "\n\n" + es + "\n\n" + td)
strings_text = regenerate(strings_path,
                          [strings_entry(n, all_data[n]) for n in range(1, 31)],
                          en)

if CHECK:
    problems = []
    for path, generated in ((configs_path, config_text), (strings_path, strings_text)):
        current = open(path).read()
        if generated != current:
            diff = "".join(difflib.unified_diff(
                current.splitlines(True), generated.splitlines(True),
                fromfile=path, tofile=path + " (generated)"))
            problems.append(diff)
    if problems:
        print("Challenges+ C datasets are out of sync with the docs:\n")
        print("\n".join(problems))
        sys.exit(1)
    print("Challenges+ C datasets match all 30 docs")
else:
    open(configs_path, "w").write(config_text)
    open(strings_path, "w").write(strings_text)
    print("Regenerated mpconfigsplus.c and mpstringsplus.c from 30 docs")

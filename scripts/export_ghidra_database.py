#!/usr/bin/env nix-shell
#!nix-shell -p python311 -i python3

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import tomllib
from typing import Optional
import shutil

import ghidra_helpers

SCRIPT_PATH = Path(os.path.realpath(__file__)).parent


def fetchVersions(args):
    """
    Fetches all the versions of the program being exported.
    """
    with tempfile.NamedTemporaryFile(prefix="versions") as f:
        ghidra_helpers.runAnalyze(
            args.GHIDRA_REPO_NAME,
            project_name=args.project_name,
            process=args.program,
            username=args.username,
            ssh_key=args.ssh_key,
            pre_scripts=[["ExportFileVersions.java", f.name]],
        )
        versions = json.loads(f.read())
    versions.sort(key=lambda x: x["version"])
    return versions


XML = "xml"
DECOMP = "decomp"


def export(args, version: dict):
    verFileInRepo = args.program + ".version"
    outVer = args.GIT_REPO_PATH / str(verFileInRepo)

    if args.EXPORT_TYPE == XML:
        srcInRepo = args.program + ".xml"
        out = args.GIT_REPO_PATH / str(srcInRepo)
        if not out.exists():
            out.touch()
        script = "ExportToXML.java"
    else:
        srcInRepo = "src/" + args.program
        out = args.GIT_REPO_PATH / str(srcInRepo)
        if out.exists():
            shutil.rmtree(out)
        out.mkdir(parents=True, exist_ok=True)
        script = "ExportDecomp.java"

    ghidra_helpers.runAnalyze(
        args.GHIDRA_REPO_NAME,
        project_name=args.project_name,
        process=args.program,
        username=args.username,
        ssh_key=args.ssh_key,
        pre_scripts=[[script, str(out), str(version["version"])]],
    )

    if args.EXPORT_TYPE == XML:
        # The XML contains the timestamp of when the export was done. This is kinda
        # annoying as it introduces some noise in the diff. Let's patch it out.
        text = out.read_text()
        text = re.sub(
            '<INFO_SOURCE (.*) TIMESTAMP=".*" (.*)/>', r"<INFO_SOURCE \1 \2/>", text
        )
        out.write_text(text)

    # Add a file, verFileInRepo, which contains the current version number. This
    # is used so the next run of export_ghidra_database.py knows from which
    # version it needs to start to read again.
    outVer.write_text(str(version["version"]))

    # Add the files to the git repo.
    subprocess.run(
        ["git", "-C", args.GIT_REPO_PATH, "add", srcInRepo, verFileInRepo],
        check=True,
    )

    # Ghidra Create Time is stored as num of milliseconds since UNIX epoch.
    # Python expects number of seconds, so fix it.
    # We also set the timezone to UTC to ensure our commits are reproducible.
    # Turns out the timezone leaks into the commit data, so having a different
    # timezone may end up creating different commit hash.
    createTime = datetime.fromtimestamp(version["createTime"] / 1000.0, tz=timezone.utc)

    # Git needs both a name and an email to make a commit. To satisfy it, we
    # first try to find the user in the `user_mapping` file, which maps ghidra
    # username to a string in the form 'user <email>', and extract both values
    # from there. If we don't find the user there, we use the ghidra username
    # as-is, and `unknown` as the email.
    if version["user"] in args.user_mappings:
        user = args.user_mappings[version["user"]]
        user, email = user.rsplit("<", 1)
        email = email.removesuffix(">").strip()
        user = user.strip()
    else:
        user = version["user"]
        email = "unknown"

    # We use the comment as a git message. In ghidra, the comment may be empty,
    # but git disallows this. In this case, we leave a default commit message.
    commitMsg = version["comment"]
    if commitMsg == "":
        commitMsg = "Generic reverse engineering progress"

    # Let's commit now.
    gitEnv = {
        "GIT_COMMITTER_NAME": user,
        "GIT_COMMITTER_EMAIL": email,
        "GIT_COMMITTER_DATE": createTime.isoformat(),
        "GIT_AUTHOR_NAME": user,
        "GIT_AUTHOR_EMAIL": email,
        "GIT_AUTHOR_DATE": createTime.isoformat(),
    }
    subprocess.run(
        ["git", "-C", args.GIT_REPO_PATH, "commit", "-m", commitMsg],
        env=gitEnv,
        check=True,
    )


def parseUserMappings(path: str):
    print(path)
    try:
        res = tomllib.loads(Path(path).read_text())
    except Exception as e:
        print(e)
        raise
    return res


def getLatestVersionInRepo(git_repo_path: Path, program: str) -> Optional[int]:
    verFileInRepo = program + ".version"
    verFile = git_repo_path / verFileInRepo
    if verFile.exists():
        return int(verFile.read_text())
    return None


def main():
    parser = argparse.ArgumentParser(
        description="Export a ghidra database history to git",
    )
    parser.add_argument("GHIDRA_REPO_NAME")
    parser.add_argument("--project-name")
    parser.add_argument("GIT_REPO_PATH", type=Path)
    parser.add_argument("EXPORT_TYPE", choices=[XML, DECOMP])
    parser.add_argument(
        "--username", help="Username to use when connecting to the ghidra server."
    )
    parser.add_argument(
        "--ssh-key",
        help="""SSH key to use to authenticate to a ghidra server.
                        Note that the ghidra server must have SSH authentication enabled for this to work.
                        To enable SSH auth, add -ssh in the wrapper.parameters of the Ghidra Server's server.conf""",
    )
    parser.add_argument(
        "--user-mappings",
        type=parseUserMappings,
        default={},
        help='JSON mapping of ghidra username to git "user <email>" format',
    )
    parser.add_argument("--program", help="Program to export")
    args = parser.parse_args()

    versions = fetchVersions(args)

    versionInRepo = getLatestVersionInRepo(args.GIT_REPO_PATH, args.program)

    for version in versions:
        if versionInRepo is not None and version["version"] <= versionInRepo:
            continue
        export(args, version)


if __name__ == "__main__":
    main()

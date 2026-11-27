import subprocess
Import("env")

try:
    commit = subprocess.check_output(
        ["git", "rev-parse", "--short", "HEAD"],
        cwd=env.subst("$PROJECT_DIR"),
        stderr=subprocess.DEVNULL,
        text=True,
    ).strip() or "unknown"
except Exception:
    commit = "unknown"

env.Append(CPPDEFINES=[("GIT_COMMIT", env.StringifyMacro(commit))])

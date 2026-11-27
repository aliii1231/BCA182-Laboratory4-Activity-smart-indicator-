Import("env")

import subprocess

project_dir = env.subst("$PROJECT_DIR")
try:
    commit = subprocess.check_output(
        ["git", "rev-parse", "--short", "HEAD"],
        cwd=project_dir,
        stderr=subprocess.DEVNULL,
        text=True,
    ).strip()
    if not commit:
        commit = "unknown"
except (OSError, subprocess.CalledProcessError):
    commit = "unknown"

commit = commit.replace("\\", "\\\\").replace('"', '\\"')
env.Append(CPPDEFINES=[("GIT_COMMIT", '"%s"' % commit)])

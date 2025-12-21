from pathlib import Path
import json
from datetime import datetime

from SCons.Script import Import

Import("env")

project_dir = Path(env["PROJECT_DIR"])
state_path = project_dir / ".version_state.json"
header_path = project_dir / "src" / "version_auto.h"

version_state = {"major": 1, "minor": 0, "build": 0}
if state_path.exists():
    try:
        version_state.update(json.loads(state_path.read_text()))
    except Exception:
        pass

version_state["build"] = int(version_state.get("build", 0)) + 1
version_string = f"{version_state['major']}.{version_state['minor']}.{version_state['build']}"

state_path.write_text(json.dumps(version_state, indent=2) + "\n")

generated = (
    "#pragma once\n"
    f"// Auto-generated on {datetime.utcnow().isoformat()}Z\n"
    f"constexpr const char* APP_VERSION = \"{version_string}\";\n"
)
header_path.write_text(generated)

env.Append(CPPDEFINES=[( "APP_VERSION_STR", f'"{version_string}"' )])

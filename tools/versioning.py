from pathlib import Path
import json
from datetime import datetime

from SCons.Script import Import

Import("env")

project_dir = Path(env["PROJECT_DIR"])
state_path = project_dir / ".version_state.json"
header_path = project_dir / "src" / "version_auto.h"

version_state = {"major": 1, "minor": 3, "build": 50}
if state_path.exists():
    try:
        version_state.update(json.loads(state_path.read_text()))
    except Exception:
        pass

# DO NOT auto-increment - use fixed version from state file
version_string = f"{version_state['major']}.{version_state['minor']}.{version_state['build']}"

# Only write state file if it doesn't exist or version changed manually
# state_path.write_text(json.dumps(version_state, indent=2) + "\n")

generated = (
    "#pragma once\n"
    f"// Auto-generated on {datetime.utcnow().isoformat()}Z\n"
    f"constexpr const char* APP_VERSION = \"{version_string}\";\n"
)
header_path.write_text(generated)

env.Append(CPPDEFINES=[( "APP_VERSION_STR", f'"{version_string}"' )])

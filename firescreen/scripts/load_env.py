"""PlatformIO pre-script: inject key=value pairs from the gitignored `.env`
as compile-time string defines (see .env.example for the recognized keys).

Values become C string literals via StringifyMacro, so no shell-escaping
issues with spaces or special characters in WiFi passwords.
"""

import os

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

RECOGNIZED = [
    "WIFI_SSID",
    "WIFI_PASSWORD",
    "AIRNOW_TOKEN",
    "HOMELAB_HOST",
    "ZIP",
    "LAT",
    "LON",
]

env_path = os.path.join(env["PROJECT_DIR"], ".env")
found = {}

if os.path.isfile(env_path):
    with open(env_path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, _, value = line.partition("=")
            key, value = key.strip(), value.strip().strip('"').strip("'")
            if key in RECOGNIZED and value:
                found[key] = value

env.Append(CPPDEFINES=[(k, env.StringifyMacro(v)) for k, v in found.items()])

if "WIFI_SSID" not in found or "WIFI_PASSWORD" not in found:
    print(
        "\033[93mload_env.py: WIFI_SSID/WIFI_PASSWORD not set in .env — "
        "firmware will only work in USB test mode.\033[0m"
    )
if "AIRNOW_TOKEN" not in found:
    print("load_env.py: AIRNOW_TOKEN not set — the AirNow row will show a dash.")

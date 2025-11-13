Import("env")  # type: ignore

"""Minimal stub of Arduino's pioarduino-build helper.

The custom framework package distributed with Ghost Toolbox omits the
pioarduino-build.py script that PlatformIO expects to import.  The stock
Espressif version configures include paths for the selected board variant
and then returns control to the main builder.  We only need enough logic
here to keep PlatformIO from aborting, so the stub ensures the variant
include directory is present and otherwise stays out of the way.
"""

from os.path import exists, join

platform = env.PioPlatform()
board = env.BoardConfig()
framework_dir = platform.get_package_dir("framework-arduinoespressif32")
variant_name = board.get("build.variant")

if framework_dir and variant_name:
    variant_dir = join(framework_dir, "variants", variant_name)
    if exists(variant_dir):
        env.AppendUnique(CPPPATH=[variant_dir])

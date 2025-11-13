import hashlib
from typing import TYPE_CHECKING, Any
import requests

if TYPE_CHECKING:
    Import: Any = None
    env: Any = {}

import glob
import gzip
from os import makedirs, remove, rename
from os.path import basename, dirname, exists, isfile, join
from shutil import copyfile

Import("env")  # type: ignore

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-arduinoespressif32")


def _subst_path(name: str) -> str:
    value = env.subst(name)
    if not value or "$" in value:
        return ""
    return value


def _ensure_pioarduino_build() -> None:
    project_dir = env.get("PROJECT_DIR")
    if not project_dir:
        raise RuntimeError("PROJECT_DIR is undefined in PlatformIO environment")

    source_script = join(project_dir, "tools", "pioarduino-build.py")
    if not exists(source_script):
        raise RuntimeError(
            "tools/pioarduino-build.py is missing from the repository; "
            "please update your checkout."
        )

    tool_dirs = set()
    if FRAMEWORK_DIR:
        tool_dirs.add(join(FRAMEWORK_DIR, "tools"))

    project_core_dir = _subst_path("$PROJECT_CORE_DIR")
    if project_core_dir:
        tool_dirs.add(join(project_core_dir, "packages", "framework-arduinoespressif32", "tools"))

    project_packages_dir = _subst_path("$PROJECT_PACKAGES_DIR")
    if project_packages_dir:
        tool_dirs.add(join(project_packages_dir, "framework-arduinoespressif32", "tools"))

    placed = False
    for tool_dir in tool_dirs:
        target = join(tool_dir, "pioarduino-build.py")
        if exists(target):
            placed = True
            continue

        makedirs(tool_dir, exist_ok=True)
        copyfile(source_script, target)
        placed = True

    if not placed:
        raise RuntimeError(
            "Unable to place pioarduino-build.py in any known framework directory."
        )


_ensure_pioarduino_build()

board_mcu = env.BoardConfig()
mcu = board_mcu.get("build.mcu", "")
patchflag_path = join(FRAMEWORK_DIR, "tools", "sdk", mcu, "lib", ".patched")

# patch file only if we didn't do it befored
if not isfile(patchflag_path):
    original_file = join(FRAMEWORK_DIR, "tools", "sdk", mcu, "lib", "libnet80211.a")
    patched_file = join(
        FRAMEWORK_DIR, "tools", "sdk", mcu, "lib", "libnet80211.a.patched"
    )

    env.Execute(
        "pio pkg exec -p toolchain-xtensa-%s -- xtensa-%s-elf-objcopy  --weaken-symbol=s %s %s"
        % (mcu, mcu, original_file, patched_file)
    )
    if isfile("%s.old" % (original_file)):
        remove("%s.old" % (original_file))
    rename(original_file, "%s.old" % (original_file))
    env.Execute(
        "pio pkg exec -p toolchain-xtensa-%s -- xtensa-%s-elf-objcopy  --weaken-symbol=ieee80211_raw_frame_sanity_check %s %s"
        % (mcu, mcu, patched_file, original_file)
    )

    def _touch(path):
        with open(path, "w") as fp:
            fp.write("")

    env.Execute(lambda *args, **kwargs: _touch(patchflag_path))


def hash_file(file_path):
    """Generate SHA-256 hash for a single file."""
    hasher = hashlib.sha256()
    with open(file_path, "rb") as f:
        # Read the file in chunks to avoid memory issues
        for chunk in iter(lambda: f.read(4096), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def hash_files(file_paths):
    """Generate a combined hash for multiple files."""
    combined_hash = hashlib.sha256()

    for file_path in file_paths:
        file_hash = hash_file(file_path)
        combined_hash.update(file_hash.encode("utf-8"))  # Update with the file's hash

    return combined_hash.hexdigest()


def save_checksum_file(hash_value, output_file):
    """Save the hash value to a specified output file."""
    with open(output_file, "w") as f:
        f.write(hash_value)


def load_checksum_file(input_file):
    """Load the hash value from a specified input file."""
    with open(input_file, "r") as f:
        return f.readline().strip()

def minify_css(c):
    minify_req = requests.post("https://www.toptal.com/developers/cssminifier/api/raw", {"input": c.read().decode('utf-8')})
    return c if minify_req is False else minify_req.text.encode('utf-8')

def minify_js(js):
    minify_req = requests.post('https://www.toptal.com/developers/javascript-minifier/api/raw', {'input': js.read().decode('utf-8')})
    return js if minify_req is False else minify_req.text.encode('utf-8')

def minify_html(html):
    minify_req = requests.post('https://www.toptal.com/developers/html-minifier/api/raw', {'input': html.read().decode('utf-8')})
    return html if minify_req is False else minify_req.text.encode('utf-8')

# gzip web files
def prepare_www_files():
    HEADER_FILE = join(env.get("PROJECT_DIR"), "include", "webFiles.h")
    filetypes_to_gzip = ["html", "css", "js"]
    data_src_dir = join(env.get("PROJECT_DIR"), "embedded_resources/web_interface")
    checksum_file = join(data_src_dir, "checksum.sha256")
    checksum = ""

    if not exists(data_src_dir):
        print(f'Error: Source directory "{data_src_dir}" does not exist!')
        return

    if exists(checksum_file):
        checksum = load_checksum_file(checksum_file)

    files_to_gzip = []
    for extension in filetypes_to_gzip:
        files_to_gzip.extend(glob.glob(join(data_src_dir, "*." + extension)))

    files_checksum = hash_files(files_to_gzip)
    if files_checksum == checksum:
        print("[GZIP & EMBED INTO HEADER] - Nothing to process.")
        return

    print(f"[GZIP & EMBED INTO HEADER] - Processing {len(files_to_gzip)} files.")

    makedirs(dirname(HEADER_FILE), exist_ok=True)

    with open(HEADER_FILE, "w") as header:
        header.write(
            "#ifndef WEB_FILES_H\n#define WEB_FILES_H\n\n#include <Arduino.h>\n\n"
        )
        header.write(
            "// THIS FILE IS AUTOGENERATED DO NOT MODIFY IT. MODIFY FILES IN /embedded_resources/web_interface\n\n"
        )

        for file in files_to_gzip:
            gz_file = file + ".gz"
            with open(file, "rb") as src, gzip.open(gz_file, "wb") as dst:
                ext = basename(file).rsplit(".", 1)[-1].lower()
                if ext == 'html':
                    minified = minify_html(src)
                elif ext == 'css':
                    minified = minify_css(src)
                elif ext == 'js':
                    minified = minify_js(src)
                else:
                    raise ValueError(f"Unsupported file type: {ext}")

                dst.write(minified)

            with open(gz_file, "rb") as gz:
                compressed_data = gz.read()
                var_name = basename(file).replace(".", "_")

                header.write(f"const uint8_t {var_name}[] PROGMEM = {{\n")

                # Write hex values, inserting a newline every 15 bytes
                for i in range(0, len(compressed_data), 15):
                    hex_chunk = ", ".join(
                        f"0x{byte:02X}" for byte in compressed_data[i : i + 15]
                    )
                    header.write(f"  {hex_chunk},\n")

                header.write("};\n\n")
                header.write(
                    f"const uint32_t {var_name}_size = {len(compressed_data)};\n\n"
                )

            remove(gz_file)  # Clean up temporary gzip file

        header.write("#endif // WEB_FILES_H\n")

    save_checksum_file(files_checksum, checksum_file)

    print(f"[DONE] Gzipped files embedded into {HEADER_FILE}")


prepare_www_files()

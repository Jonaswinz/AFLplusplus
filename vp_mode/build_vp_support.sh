#!/bin/bash

VP_VERSION_FILE="./VP_VERSION"
VP_FOLDER="VPs"

echo "================================================="
echo "VP-Mode Build Script"
echo "================================================="
echo

echo "[*] Performing basic sanity checks..."

PLT=$(uname -s)

if [ "$PLT" != "Linux" ]; then
  echo "[-] Error: VP instrumentation is unsupported on $PLT."
  exit 1
fi

mkdir -p "$VP_FOLDER"

while IFS= read -r line || [ -n "$line" ]; do
    # Skip empty or comment lines
    [[ -z "$line" || "$line" == \#* ]] && continue

    # Sanitize line and parse values
    line=$(echo "$line" | tr -d '\r' | xargs)
    read -r repo ref name <<< "$line"

    if [[ -z "$repo" || -z "$ref" || -z "$name" ]]; then
        echo "[!] Skipping malformed line: $line"
        continue
    fi

    TARGET_DIR="$VP_FOLDER/$name"

    echo
    echo "[*] Processing version: $name"
    echo "    Repo: $repo"
    echo "    Ref : $ref"
    echo "    Path: $TARGET_DIR"

    if [ -d "$TARGET_DIR/.git" ]; then
        echo "[*] Repository already exists. Skipping clone and checkout."
    else
        echo "[*] Cloning repository into $TARGET_DIR ..."
        git clone --recursive "$repo" "$TARGET_DIR" || { echo "[-] Clone failed!"; continue; }

        cd "$TARGET_DIR" || { echo "[-] Failed to enter $TARGET_DIR"; continue; }

        echo "[*] Checking out to $ref ..."
        git checkout "$ref" || echo "[!] Warning: could not check out to $ref"

        cd - >/dev/null
    fi

    echo "[+] Done setting up $name"
done < "$VP_VERSION_FILE"

echo
echo "[*] Building all VPs ..."
if ! make -j$(nproc); then
  echo "[-] Build failed during make process."
  exit 1
fi

echo "[✓] All builds completed successfully."

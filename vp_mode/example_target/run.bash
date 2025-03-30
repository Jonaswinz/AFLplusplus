#!/bin/bash

CURRENT_FILE=$(realpath "$0")
SCRIPT_DIR=$(dirname "$CURRENT_FILE")

echo "Using settings.bash from: $SCRIPT_DIR"

. "$SCRIPT_DIR/settings.bash"

mkdir -p $SCRIPT_DIR/out

./afl-fuzz -i $SCRIPT_DIR/seeds -o $SCRIPT_DIR/out -m none -v -- $SCRIPT_DIR/arch_pro_minimal.cfg

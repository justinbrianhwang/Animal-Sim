#!/bin/bash
export PATH="/home/$USER/.local/bin:/usr/local/bin:/usr/bin:/bin"
cd "$(dirname "$0")"
mkdir -p external/glad
python3 -m glad --api gl:core=3.3 --out-path external/glad c --header-only
echo "glad files generated:"
find external/glad -type f

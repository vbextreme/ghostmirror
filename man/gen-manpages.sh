#!/bin/bash

CWD="$(dirname "$0")"
OUTPUT_FILE="${CWD}/ghostmirror.1"

# read from environment if set otherwise use a default value
CURRENT_VERSION="${CURRENT_VERSION:='0.9.19'}"
CURRENT_DATE="$(LC_ALL='C' date '+%d %b %Y')"

# escape dots '.' became '\&.'
CURRENT_VERSION="${CURRENT_VERSION//'.'/'\\\&.'}"

# read .skel content
CONTENT=$(< "${OUTPUT_FILE}.skel")

# replace placeholders in ghostmirror.1.skel file
sed -s 's/:CURRENT_DATE:/'"${CURRENT_DATE}"'/g;s/:CURRENT_VERSION:/'"${CURRENT_VERSION}"'/g' <<< "${CONTENT}" > "${OUTPUT_FILE}"

#!/usr/bin/env bash

if [ $# -ne 1 ]
then
	echo "Usage: $(basename \"$0\") <test.sh> <data folder>" >&2
	exit 1
fi

TMP_COMPILER=$(mktemp)
go build -o "$TMP_COMPILER" ./cmd/compiler

DATA_FOLDER=$1
TMP_FILE=$(mktemp)
EXIT_CODE=0

# test scan files
for file in "$DATA_FOLDER"/scan/*.minijava
do
	echo "Testing \"$file\"..."

	if ! "$TMP_COMPILER" scan "$file" > "$TMP_FILE"
	then
		EXIT_CODE=1
		break
	fi

	if ! git --no-pager diff --color=always --no-index "$file.out" "$TMP_FILE"
	then
		EXIT_CODE=1
		break
	fi
done
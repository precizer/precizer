#!/bin/bash

VER=3520000

echo Downloading...
wget https://sqlite.org/2026/sqlite-amalgamation-${VER}.zip -O sqlite-amalgamation-${VER}.zip

echo Extracting...
# Extract without subdir and force file overwrites
unzip -joqq sqlite-amalgamation-${VER}.zip

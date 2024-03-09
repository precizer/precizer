#!/bin/bash

VER=3450100

echo Creating temp dir...
TMPDIR=$(mktemp -d)
cd $TMPDIR

echo Downloading...
wget https://sqlite.org/2024/sqlite-amalgamation-${VER}.zip -O sqlite-amalgamation-${VER}.zip

echo Extracting...
# Extract without subdir and force file overwrites
unzip -joqq sqlite-amalgamation-${VER}.zip

echo Compiling...
cc -c -pipe -std=c11 -fbuiltin -mrdrnd -finline-functions -flto \
 -O3 -funroll-loops -DNDEBUG -march=native \
 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion \
 -Winit-self -Wunreachable-code -Wformat-y2k -Wformat-nonliteral \
 -Wformat-security -Wmissing-include-dirs -Wswitch-default -Wtrigraphs \
 -Wstrict-overflow=5 -Wfloat-equal -Wundef -Wshadow -Wbad-function-cast \
 -Wcast-qual -Wcast-align -Wwrite-strings -Winline -Wlogical-op \
 -Wsuggest-attribute=const -Wsuggest-attribute=pure \
 -Wsuggest-attribute=noreturn -Wsuggest-attribute=format \
 -Wmissing-format-attribute \
 -o sqlite3.o sqlite3.c 2>./sqlite_output.log

rm shell.c sqlite3.c sqlite3.h sqlite3ext.h sqlite3.o sqlite-amalgamation-${VER}.zip
echo
echo "Count of warnings (the left side) and explanations from the compiler:"
echo
cat ./sqlite_output.log | \
grep "sqlite3.c:" | \
grep -v "sqlite3.c: In" | \
grep -v "sqlite3.c: At" | \
grep -v ": note:" | \
grep -v "In file included from" | \
sed 's/sqlite3.c\:[0-9]*\:[0-9]*\: warning\: \(.*\)$/\1/' | \
sed 's/.*discards ‘const’ qualifier from pointer target type/... discards ‘const’ qualifier from pointer target type/' | \
sed 's/conversion to ‘.*’ from ‘.*’ may change the sign of the result/conversion to ‘.....’ from ‘.....’ may change the sign of the result/' | \
sed 's/conversion to ‘.*’ from ‘.*’ may alter its value/conversion to ‘.....’ from ‘.....’ may alter its value/' | \
sed 's/ if it is known to return normally//' | \
sed 's/.* is not defined.*/........... is not defined/' | \
sed 's/\(.*\) \[.*\]$/\1/' | \
sort | \
uniq --count | \
sort -nr
echo
rm sqlite_output.log

echo Software versions
echo Uname:
lsb_release -a
echo
echo Compiler:
cc -v
echo
echo Libc:
ldd --version

cd /tmp
rm -rf $TMPDIR

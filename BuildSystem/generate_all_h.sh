#!/bin/sh

echo "#ifndef ${PREFIX}_all_h"
echo "#define ${PREFIX}_all_h"
echo ""
echo "/* This file is auto-generated and should not be edited. */"
echo ""
for fn in `ls *.h* | grep -E '.*\.(h|hpp)$'`; do
	echo $fn | egrep '(^_.*|^all\.hp?p?$)' > /dev/null
	if [ $? -eq 0 ]; then
		continue
	fi
	echo "#include \"$fn\""
done
echo ""
echo "#endif"
echo ""

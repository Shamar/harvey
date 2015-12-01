#!/bin/bash
set -e

if [ "${COVERITY_SCAN_BRANCH}" != 1 ]; then
	git clean -x -d -f
	(cd $TRAVIS_BUILD_DIR && ./bootstrap.sh)

	export ARCH=amd64 && export PATH="$TRAVIS_BUILD_DIR/util:$PATH"
	echo
	echo "Vendorized code verification..."
	echo
	for v in `find|grep vendor.json`; do
		echo "cd `dirname $v`"
		(cd `dirname $v`; vendor -check)
	done

	build all
fi

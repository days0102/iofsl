#!/bin/sh

#
# Usage: create-nightly.sh <destdir>
#

if test -z "${IOFWD_SRCDIR}" ; then
   echo "Need IOFWD_SRCDIR !"
   exit 1
fi

DESTDIR=$1
if test -z "${DESTDIR}"; then
   echo "Need argument: destination directory"
   exit 1
fi

#
# First get the lastest master revision
#
git fetch || exit 1
git checkout origin/master || exit 1

GITVERSION=$(git log -n1 --pretty=format:%h)
GITDATE=$(git log -n1 --pretty=format:%ci)
${IOFWD_SRCDIR}/prepare || exit 1
${IOFWD_SRCDIR}/scripts/doeconfig.sh || exit 1

DISTVERSION=$(make distversion)

cd ${IOFWD_SRCDIR}
make dist || exit 1

DESTNAME=${DESTDIR}/iofwd-$(date -d "${GITDATE}" +%Y%m%d).tar.gz

cp iofwd-${DISTVERSION}.tar.gz ${DESTNAME}
chmod 644 ${DESTNAME}


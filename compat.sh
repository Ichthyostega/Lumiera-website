#!/usr/bin/env bash
#
# compat.sh -- maintain a compatibility branch in Git
#
# - invoke to rebase the compat branch on top of current head
# - invoke with -r to back out and bring all pending changes down
#
set -e

function fail() {
	echo -e "\nFATAL: $1\n\n"
	exit -1
}

function getBranch() {
    git show -s --format=%h $1 2>/dev/null
}

COMPAT_BRANCH="compat/$(whoami)"
HEAD=$(getBranch HEAD)                         || fail "unable to establish git HEAD"
CURRENT=$(git branch --show-current)           || fail "unable to determine current branch"

getBranch $COMPAT_BRANCH >/dev/null            || fail "Need Git branch $COMPAT_BRANCH"


# do we have pending changes?
PENDING="$(git status --porcelain| wc -l)"



function switchBranch() {
    local TARGET=$1
    local ACTION=$2

    [[ "$HEAD" != $(getBranch $TARGET) ]]      || fail "already at $TARGET -- nothing to do"
    if (( $PENDING ))
        then
        echo -e "\nATTENTION!!! pending chagnges will be stashed and restored...\n"
        git stash save "move-to-branch-$TARGET"
    fi

    $ACTION                                    || fail "unable to return to branch $TARGET"

    if (( $PENDING ))
        then
        git stash pop --index                  || fail "could not restore stashed changes -- they are now on the stash"
        #
        echo -e "\n\nCOMPAT: After switch to branch \"$TARGET\": pending chagnges were restored...\n"
    else
        echo -e "\n\nCOMPAT: successfully switched to \"$TARGET\"."
    fi
}


# Determine action...
#
if [ "$1" == "-r" ]
    then
    echo -e "\nCOMPAT: Drop back to the preceding base branch..."
    BASE_BRANCH=$(git show-branch -a 2>/dev/null   \
                 | grep '\*'                       \
                 | grep -v "$COMPAT_BRANCH"        \
                 | head -n1                        \
                 | sed 's/.*\[\(.*\)\].*/\1/'      \
                 | sed 's/[\^~].*//'               \
                 )
    echo "Base-Branch: $BASE_BRANCH"
    #
    switchBranch $BASE_BRANCH "git checkout $BASE_BRANCH"


else
    echo -e "\nCOMPAT: will rebase $COMPAT_BRANCH...."
    #
    switchBranch $COMPAT_BRANCH "git rebase $CURRENT $COMPAT_BRANCH"
fi


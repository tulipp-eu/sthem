#!/bin/bash

TARGET="root@jetsontx2"
ELFFILE=%name%
ARGUMENTS=""

# Milliseconds
DELAY=4000

SLEEP=0.1
TIMEOUT=100

PASS=%pass%

TARGET_DIR="/tmp/sthem"

WRAPPER_ARGS="--offset=$TARGET_DIR/offset --vmap=$TARGET_DIR/vmmap --pid=$TARGET_DIR/pidcollision --time=$DELAY"

if [ $PASS -eq 0 ]; then

    [ ! -f $ELFFILE ]  && {
        echo "'$ELFFILE' not found" >&2
        exit 1
    }

    readelf -h $ELFFILE | grep -q "DYN" && {
        WRAPPER_ARGS=$WRAPPER_ARGS" --collision"
        DYN=1
    } || {
        echo "WARNING: static binary detected, samples may collide with other processes!"
        DYN=0
    }


    WRAPPER="pmapelf $WRAPPER_ARGS -- $TARGET_DIR/$ELFFILE $ARGUMENTS"


    ssh $TARGET "
mkdir -p $TARGET_DIR && \
rm -Rf $TARGET_DIR/offset $TARGET_DIR/vmmap $TARGET_DIR/pidcollision $TARGET_DIR/stdout $TARGET_DIR/stderr 2>/dev/null
" || exit 1

    scp $ELFFILE $TARGET":"$TARGET_DIR"/" || exit 1

    ssh $TARGET "
ldd $TARGET_DIR/$ELFFILE >/dev/null;
command -v pmapelf >/dev/null || echo 'pmapelf not found!'
" || exit 1

    ssh $TARGET "
(
    RET=1; EXIT=0; trap 'EXIT=1' INT; 
    while [ \$EXIT -eq 0 ] && [ \$RET -eq 1 ]; do
       $WRAPPER >$TARGET_DIR/stdout 2>$TARGET_DIR/stderr; RET=\$?
    done
) >$TARGET_DIR/bgout 2>&1 &

PID=\$!; T=0
echo \"Background process started [\$PID]\"
echo -n \"Waiting for $TARGET_DIR/offset ... \"
while [ ! -f $TARGET_DIR/offset ] && [ \$T -lt $TIMEOUT ]; do
    sleep $SLEEP; T=\$(( \$T + 1 ))
done
[ \$T -ge $TIMEOUT ] && { 
  echo timeout!
  kill -s SIGINT \$PID; wait \$PID
  [ -f $TARGET_DIR/stderr ] && cat $TARGET_DIR/stderr;
  exit 1
}
echo ready!
disown -a -h" || exit 1

    scp $TARGET":"$TARGET_DIR"/offset" ./offset || exit 1

elif [ $PASS -eq 1 ]; then
    ssh $TARGET "[ -e $TARGET_DIR/pidcollision ] && {
    [ -f $TARGET_DIR/stderr ] && cat $TARGET_DIR/stderr
    exit 1
}
T=0
echo -n \"Waiting for $TARGET_DIR/vmmap ... \"
while [ ! -f $TARGET_DIR/vmmap ] && [ \$T -lt $TIMEOUT ]; do
    sleep $SLEEP; T=\$(( \$T + 1 ))
done
[ \$T -ge $TIMEOUT ] && {
  echo timeout!
  [ -f $TARGET_DIR/stderr ] && cat $TARGET_DIR/stderr;
  exit 1;
}
echo ready!
" || exit 1
    scp $TARGET":"$TARGET_DIR"/vmmap" ./vmmap || exit 1

else
    echo "Unknown pass argument!" >&2
    exit 1
fi

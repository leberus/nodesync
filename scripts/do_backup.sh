#!/bin/bash

DATE=$(date +%Y%m%d-%k%M%S)
DATE=`echo $DATE | sed 's/ //'`
DIR="/home/os/nodesync"
FILES="*.[ch]"
BACKUP_DIR="$DIR/backup/backup_$DATE"


	mkdir -p $BACKUP_DIR
	cd ..
	cp $FILES $BACKUP_DIR
	exit 0


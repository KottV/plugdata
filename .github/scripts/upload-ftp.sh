#!/bin/sh


HOST='glyphpress.com'
USER=${FTP_USERNAME}
PASSWD=${FTP_PASSWORD}
FILE=$1


if [ -d ${FILE} ];
then
tar -czvf "${FILE}.tar.gz" ${FILE}
FILE="${FILE}.tar.gz"
fi

ftp -inv $HOST <<END_SCRIPT
quote USER $USER
quote PASS $PASSWD
quote PASV
binary
delete $FILE
quit
END_SCRIPT

# Wait 2 minutes to ensure the file is gone
sleep 120

ftp -inv $HOST <<END_SCRIPT
quote USER $USER
quote PASS $PASSWD
quote PASV
binary
delete $FILE
put $FILE
quit
END_SCRIPT

exit 0

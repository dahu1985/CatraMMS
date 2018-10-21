#!/bin/bash

if [ $# -ne 1 ]
then
	echo "Usage $0 start | stop | status"

	exit
fi

command=$1

if [ "$command" != "start" -a "$command" != "stop" -a "$command" != "status" ]
then
	echo "Usage $0 start | stop  | status[nodaemon]"

	exit
fi

if [ "$command" == "start" ]
then
	sudo /opt/catramms/nginx/sbin/nginx 
elif [ "$command" == "status" ]
then
	ps -ef | grep nginx | grep -v grep | grep -v status
elif [ "$command" == "stop" ]
then
	PID=$(cat /opt/catramms/nginx/conf/nginx.conf | grep -Ev '^\s*#' | awk 'BEGIN { RS="[;{}]" } { if ($1 == "pid") print $2 }' | head -n1)
	#echo $PID
	sudo start-stop-daemon --stop --quiet  --retry=TERM/30/KILL/5 --pidfile $PID --name nginx
fi

#! /bin/sh

case "$1" in
   start)
      echo "Starting aesdsocket server"
      /usr/bin/aesdsocket -d 
      ;;
   stop)
      echo "Stopping aesdsocket server" 
      pid="$(pidof aesdsocket)"
      kill -9 $pid
      ;;
     *)
      echo "Usage: aesdsocket-start-stop.sh {start|stop}"
      exit 1
esac

exit 0

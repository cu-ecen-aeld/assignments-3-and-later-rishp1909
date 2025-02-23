#!/bin/sh

#if [ $# != 3 ]
#then
#   echo "Less no of arguments provided"
#   exit 1
#fi

if [ -z "$1" ] || [ -z "$2" ]
then
   echo "No empty arguments please"
   exit 1
fi


if [ ! -d "$1" ]
then 
   echo "File is not directory"
   exit 1
fi
 
filesdir=$1
searchstr=$2
 
nooffiles=$( ls $1 | wc -l )
matchstrings=$( grep -ri $2 $1 | wc -l)
 
echo "The number of files are $nooffiles and the number of matching lines are $matchstrings"

exit 0
    

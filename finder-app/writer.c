#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <unistd.h>
#include <string.h>

int fd;

int main(int argc, char *argv[])
{
  openlog ("writer", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  if(argc < 3) {
    syslog(LOG_ERR,"Less no of arguments provided");
    return 1;
  }  
  
  if(argv[1] == NULL)
     syslog(LOG_ERR,"File name can not be NULL");
     
  fd = open (argv[1], O_WRONLY | O_CREAT | O_TRUNC,
                S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
  if (fd == -1)
  {
    syslog(LOG_ERR,"File couldn't be opened %s",argv[1]);
  }
  syslog(LOG_USER,"Writing %s to %s",argv[2],argv[1]);
  
  write(fd,argv[2],strlen(argv[2]));
  close(fd);
  closelog();

 return 0;  
}

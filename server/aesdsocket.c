#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> // read(), write(), close()
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#define MAX 20*1024
#define PORT 9000 
#define SA struct sockaddr 


struct thread_data{
   pthread_mutex_t *mutex;
   int connfd;
};

#define UNUSED(x) (void)(x)

static void timeHandler(int sig, siginfo_t *si, void *uc);
pid_t gettid(void);

struct t_eventData{
    int myData;
};
pthread_t thread_pool[100];
int noOfThreads= 0;
pthread_mutex_t mutex;

void* threadfunc(void* thread_param)
{
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    char buff[MAX]; 
    char recBuf[24 *1024];
    int n; 
    size_t total_bytes_received = 0;
    int connfd = thread_func_args->connfd;

    for (;;) { 
             bzero(buff, MAX); 
	     int err;
	     while (1) {
	      int total_bytes_received = recv(connfd, buff, MAX, 0);
              int recIndex = 0;
	      if (total_bytes_received < 0) 
	      {
		syslog(LOG_ERR,"Closed connection from XXX\n");
		break; // done reading
	      }

	      if(total_bytes_received)
	      {
	        // Lock mutex to update /var/tmp/aesdsocketdata
                int rc = pthread_mutex_lock(thread_func_args->mutex);
                FILE *fp = NULL;
                char *line;
                ssize_t rd;
                size_t len;
                
		if ( rc != 0 ) {
		  syslog(LOG_ERR,"Attempt to obtain mutex failed with %d\n",rc);
		}
	        
	        // Open file 
	        fp = fopen("/var/tmp/aesdsocketdata","a+");
	        if(fp == NULL)
	        {
                    syslog(LOG_ERR," /var/tmp/aesdsocketdata file opening failed \n");
                }
                fseek(fp, 0L, SEEK_END);

                recIndex = ftell(fp);
                fseek(fp, 0L, SEEK_SET);
                // Read complete file
                fread(recBuf, sizeof(char), recIndex, fp);
                // Append Data
	        fputs(buff,fp);
	        fflush(fp);
	        fclose(fp);
                
                // add received data to buffer
                memcpy(&recBuf[recIndex],buff,total_bytes_received );
                recIndex = recIndex + total_bytes_received;
		total_bytes_received = 0;
		// Send data
		err = send(connfd, recBuf, recIndex, 0);
		if (err < 0) 
		{
		   syslog(LOG_USER,"Client write failed\n");
		}
		rc = pthread_mutex_unlock(thread_func_args->mutex);
		if ( rc != 0 ) {
	            syslog(LOG_ERR,"Attempt to unlock mutex failed with %d\n",rc);
		}
	      }

	    }

    } 	
    return thread_param;
}
void signal_handler(int signal_number) {
    if(signal_number == SIGINT)
    {
        syslog(LOG_USER,"Caught signal, exiting: SIGINT");
    }
    if(signal_number == SIGTERM)
    {
        syslog(LOG_USER,"SIGTERM received");
    }
    syslog(LOG_USER,"Caught signal, exiting");
    exit(0);
}
void setup10SecTimer();
void startProcess()
{
    int rc;
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr = {0};
     struct sockaddr_in  cli = {0};
    char buffer[INET_ADDRSTRLEN];
    int fd;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    char *filename = "/var/tmp/aesdsocketdata";
    
    syslog(LOG_USER,"In startPRocess");
    
    
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    ///var/tmp/aesdsocketdata
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
    close(fd);
    syslog(LOG_USER,"/var/tmp/aesdsocketdata created");
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        syslog(LOG_ERR,"socket creation failed...\n"); 
        exit(-1); 
    }
    else
        syslog(LOG_USER,"Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
    
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        syslog(LOG_ERR,"socket bind failed...\n"); 
        exit(-1); 
    } 
    else
        syslog(LOG_USER,"Socket successfully binded..\n"); 
        

    rc = pthread_mutex_init(&mutex, NULL);
    
    if ( rc != 0 ) {
          syslog(LOG_ERR,"Attempt to obtain mutex failed with %d\n",rc);
    }
    
    setup10SecTimer();
 
    while(1)
    {
            // Now server is ready to listen and verification 
	    if ((listen(sockfd, 5)) != 0) { 
		syslog(LOG_ERR,"Listen failed...\n"); 
		exit(-1); 
	    } 
	    else
		syslog(LOG_USER,"Server listening..\n"); 
	    len = sizeof(cli); 
	  
	    // Accept the data packet from client and verification 
	    connfd = accept(sockfd, (SA*)&cli, &len); 
	    if (connfd < 0) { 
		syslog(LOG_ERR,"server accept failed...\n"); 
		exit(-1); 
	    } 
	    else
	    {   
		inet_ntop(AF_INET, &cli.sin_addr, buffer, sizeof(buffer));
		syslog(LOG_USER,"Accepted connection %d from %s\n",noOfThreads + 1,buffer);
		
		struct thread_data* tData = malloc(sizeof(struct thread_data));
		tData->connfd = connfd;
		tData->mutex = &mutex;
		int rc = pthread_create(&thread_pool[noOfThreads],
	                            NULL, // Use default attributes
	                            threadfunc,
	                            tData);
	        
		 if ( rc != 0 ) {
		     syslog(LOG_ERR,"pthread_create failed with error %d creating thread\n",rc);
		 }
		 
	    }
         
    }
    // After chatting close the socket 
    close(sockfd); 
}


void daemonize( char *cmd ){
          pid_t pid;
 
         /* Clear file creation mask */
         umask( 0 );
 
        /* Spawn a new process and exit */
        if( (pid = fork()) < 0 ){ /* Fork error */
                 fprintf( stderr, "%s: Fork error\n", cmd );
                 exit( errno );
         }
         else if( pid > 0 ){ /* Parent process */
                 exit( 0 );
         }
         /* Child process */
         setsid();
 
         /* Change working directory to root directory */
         if( chdir( "/" ) < 0 ){
                 fprintf( stderr, "%s: Error changing directory\n", cmd );
                 exit( errno );
         }
 
         /* Close all open file descriptors */
         for( int i = 0; i < 1024; i++ ){
                 close( i );
         }
 
         /* Reassociate file descriptors 0, 1, and 2 with /dev/null */
         int fd0 = open( "/dev/null", O_RDWR );
         int fd1 = dup( 0 );
         int fd2 = dup( 0 );
 
         /* Open the log file */
         openlog( cmd, LOG_CONS, LOG_DAEMON );
         if( fd0 != 0 || fd1 != 1 || fd2 != 2 ){
                 syslog( LOG_ERR, "Unexpected file descriptors %d, %d, %d\n",
                         fd0, fd1, fd2 );
                 exit( errno );
         }
         //setup10SecTimer();

 }
 
void expired(union sigval timer_data){
   char outstr[200];
   char buff[300];
   time_t t;
   struct tm *tmp;
   int rc;
   FILE *fp = NULL;
   t = time(NULL);
   tmp = localtime(&t);
   if (tmp == NULL) {
       perror("localtime");
       exit(EXIT_FAILURE);
   }

   if (strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", tmp) == 0) {
       syslog(LOG_ERR,"strftime returned 0");
       exit(EXIT_FAILURE);
   }
   sprintf(buff,"timestamp:%s\n",outstr);

   // Lock mutex to update /var/tmp/aesdsocketdata
   rc = pthread_mutex_lock(&mutex);

   // Open file 
   fp = fopen("/var/tmp/aesdsocketdata","a+");
   if(fp == NULL)
   {
      syslog(LOG_ERR," /var/tmp/aesdsocketdata file opening failed \n");
   }

   // Append Data
   fputs(buff,fp);
   fclose(fp);

   rc = pthread_mutex_unlock(&mutex);
   if ( rc != 0 ) {
     printf("Unlock error\n");
     syslog(LOG_ERR,"Attempt to unlock mutex failed with %d\n",rc);
  }
}

void setup10SecTimer()
{
    int res = 0;
    timer_t timerId = 0;

    struct t_eventData eventData = { .myData = 0 };


    /*  sigevent specifies behaviour on expiration  */
    struct sigevent sev = { 0 };

    /* specify start delay and interval
     * it_value and it_interval must not be zero */

    struct itimerspec its = {   .it_value.tv_sec  = 10,
                                .it_value.tv_nsec = 0,
                                .it_interval.tv_sec  = 10,
                                .it_interval.tv_nsec = 0
                            };

    printf("Simple Threading Timer - thread-id: %d\n", gettid());

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = &expired;
    sev.sigev_value.sival_ptr = &eventData;


    /* create timer */
    res = timer_create(CLOCK_REALTIME, &sev, &timerId);


    if (res != 0){
        syslog(LOG_ERR, "Error timer_create: %s\n", strerror(errno));
        exit(-1);
    }

    /* start timer */
    res = timer_settime(timerId, 0, &its, NULL);

    if (res != 0){
        syslog(LOG_ERR, "Error timer_settime: %s\n", strerror(errno));
        exit(-1);
    }

}

int main(int argc, char **argv) 
{ 
    if(argc >= 2 )
    {
       daemonize("aesd");
    }
    else
    {
       //Install Sighandlers
       if (signal(SIGTERM, signal_handler) == SIG_ERR) {
          perror("signal");
          exit(1);
        }
       if (signal(SIGINT, signal_handler) == SIG_ERR) {
          perror("signal");
          exit(1);
        }
        openlog ("aeldsocket", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    }
    syslog(LOG_USER,"Main thread started");

    startProcess();
} 



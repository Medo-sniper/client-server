/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#define BUFSIZE 1024

typedef struct {   
  int ID;   
  time_t CE_TIME;  
  char HOST[256];  
  char IP[40];  
} LOCAL_CACHE;  

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

char *check_cache(LOCAL_CACHE *cache, char *host);
int update_time(LOCAL_CACHE *cache, char *host, char *ip);

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */


  char ip_addr[40];   
  struct hostent *fresh_ent;   // new host we do not have yet   
  int i;   
  char *mapped_ip = NULL;    
  
  LOCAL_CACHE l_cache[10];   
    
    //new define
  struct hostent *myent;
  int len = 0;
  char *buf2;
  struct in_addr  myen;
  long int *add; 

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);
  printf("Server port number: %hu (%d)\n",(unsigned short)serveraddr.sin_port,portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  bzero(l_cache, sizeof(LOCAL_CACHE) * 10);

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {
    bzero(ip_addr, 40);

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */

    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    

    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);
    printf("client port number: %d\n",clientaddr.sin_port);
    printf("server received %d/%d bytes: %s\n", (int)strlen(buf), n, buf);


    // check if we have the desired host in saved list   
    mapped_ip = check_cache(l_cache, buf);   
  
    if(mapped_ip != NULL)   
    {      // if we have it    
        printf("Server has %s host in cache\n", buf);  // echo we have it in saved list   
        strcpy(buf, mapped_ip);   // copy it    
    }    
    else   
    {   
        fresh_ent = gethostbyname(buf);    // get ip address for new entry    
        if(fresh_ent)    
        {        // if it is new host    
            for(i = 0; fresh_ent->h_addr_list[i]; ++i) 
            {   
                strcat(ip_addr, inet_ntoa(*(struct in_addr*)fresh_ent->h_addr_list[i]));   
            }   
        }  
        else 
        {   
            printf("Get Host Error\n");    // echo host error    
        }   

        if(update_time(l_cache, buf, ip_addr))  
        {     
            printf("Server has no %s host in cache\n", buf);     //  echo we do not have host in cache   
            printf("Server is calling DNS server\n");       // echo getting ip address from DNS server   
            strcpy(buf, ip_addr);    // copy ip address to send it to client    
        } 
        else 
        {   
            printf("CACHE FULL\n");    // echo cach full   
        }   
    }  


   
    /* 
     * sendto: echo the input back to the client 
     */
    n = sendto(sockfd, buf, strlen(buf), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      error("ERROR in sendto");
  }
}

/* check cache for host and ip address */   
char *check_cache(LOCAL_CACHE *cache, char *host)   
{       
    int a;   
    char *ip = (char *) malloc(sizeof(char) * 40);   

    bzero(ip, 40);  
    for(a=0; a <10; a++)  
    {   
        if(strcmp(host, cache[a].HOST) != 0)  
        {   
            return NULL;  
        }  
        else   
        {   
            if((time(NULL) - cache[a].CE_TIME) <= 10)   
            {   
                strcpy(ip, cache[a].IP);   
                return ip;   
            }   
        }   
    }   
    return NULL;   
}   

/* add new entry to cache and return ip address */   
int update_time(LOCAL_CACHE *cache, char *host, char *ip)   
{   
    int a;   
    
    for(a=0; a<10; a++) 
    {   
        if(strcmp(host, cache[a].HOST) == 0)  
        {   
            strcpy(cache[a].IP, ip);   
            cache[a].CE_TIME = time(NULL);   
            return 1;  
        }  
    }  

    for(a=0; a<10; a++)  
    {   
        if(cache[a].ID == 0) 
        {   
            strcpy(cache[a].HOST, host);  
            strcpy(cache[a].IP, ip);   
            cache[a].CE_TIME = time(NULL);   
            return 1;   
        }  
    }  

    return 0;
}

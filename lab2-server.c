#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define MAX_MESSAGE_LENGTH 100
//Prototyping
void *handle_client(void *arg);
int check_open();
char *readline(int fd);
int checkNumOnline();

struct connected_clients
{
        char username [11];
        int *connfd_ptr;
};
 //global variables
pthread_mutex_t lock;
const char * const chat_server1 = "<cs407chat>\n";
const char * const chat_server2 = "<ok>\n";


//struct of connected clients with fd and username
struct connected_clients clients[MAX_CLIENTS];




int main(int argc, char *argv[]){
    //set initial fd to 0
    clients[0].connfd_ptr=malloc(sizeof(int));
    clients[1].connfd_ptr=malloc(sizeof(int));
    clients[2].connfd_ptr=malloc(sizeof(int));
    clients[3].connfd_ptr=malloc(sizeof(int));
    clients[4].connfd_ptr=malloc(sizeof(int));
    
    
    
    // define thread
    pthread_t thread_id;
    int client_socket;

    //create server socket
    int server_socket;
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) ==0)
    {
            perror("socket fialed");
            exit(EXIT_FAILURE);
    }
    
    
    //define server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(4070);
    server_address.sin_addr.s_addr = INADDR_ANY;
    
    int i=1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
    
      // bind the socket to our specified IP and port
   if( bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address))<0)
   {
       perror("bind failed");
       exit(EXIT_FAILURE);
   }
    
    printf("running.......\n");
     //listen for connections
    listen(server_socket,5);
    //accept loop
    while(1){

        
        client_socket = accept(server_socket, NULL, NULL);
        int *connfd_ptr = malloc(sizeof(int));
        *connfd_ptr = client_socket;
        
        printf("New client accepted\n");
      //  send(client_socket, server_message, sizeof(server_message), 0);
        pthread_create(&thread_id, NULL, handle_client,  connfd_ptr);
    }
    return 0;
}

void *handle_client(void *arg){
    int n_read;
    int *clnt_id = (int *)arg;
    char usrMess [114] ={0};
    int test = check_open();
    char online[30];
    //check to see if open spot, close connection if not
    if(test==0){
     close(*clnt_id);   
     return NULL;
    }

    
    char hasJoined [30];
    char server_message [12]= "<cs407chat>\n";
    char client_message [12] ;
    char buffer [102] = {0};
    if(send(*clnt_id, server_message, sizeof(server_message), 0)==-1){
            fprintf(stderr,"error");
            close(*clnt_id);
            return NULL;
    }
    printf("protocol exchange sent.......\n");
    
    if((n_read=read( *clnt_id, buffer,101))==0){
            close(*clnt_id);
            return NULL;
    }
    printf("%s\n",buffer);
    for(int i =0; i<12; i++){
            client_message[i]=buffer[i];
    }
    int i=0;
    //check for proper protocol
    while(i <11)
    {
        if(client_message[i]==server_message[i]){
         i++;
        }
        else{
            //incorrect response close connection
            close(*clnt_id);
            break;
        }
    }
    memset(buffer,0,101);
    
    
    //server sends ok\n client responds with username
    send(*clnt_id, chat_server2, sizeof(chat_server2), 0);
    int nread = read(*clnt_id, buffer, 101);
    char usr [11]={0};

    //try strncat 
    strncat(usr, buffer, 10);
    int length = strlen(usr);
    usr[length] = '\0';
    //usr[length-] = '\0';
   
  
    
    int val =0; 
    // check existing usernames, if already exists close connection then 
    // terminate thread by returning null
    for(int i=0; i<5; i++){
            if(strncmp(buffer,clients[i].username,10)==0){
                    printf("identical username closing connection\n");
                    val =1;
                    close(*clnt_id);
                    break;
            }
    }
    if(val==1){
        return NULL;
    }
    else{
        
        printf("unique username, being added to clients.....\n");
        for(int i=0; i<5;i++){
            if(*clients[i].connfd_ptr==0){
                pthread_mutex_lock(&lock);
                *clients[i].connfd_ptr = *clnt_id;
                strcpy(clients[i].username, usr);
                pthread_mutex_unlock(&lock);
                printf("%s \n", usr);
                printf("%s \n", clients[i].username);
                printf("%s added with %d fd\n", clients[i].username, *clients[i].connfd_ptr);
                break;
            }
        }
    }
     
 
    memset(hasJoined, 0,29);
    snprintf(hasJoined, sizeof(hasJoined), "%s has joined...\n", usr);
    int numOnline = checkNumOnline()-1;
    memset(online, 0, 29);
    //format message to be sent to user about # of people online
    snprintf(online, sizeof(online), "There are %d others users\n", numOnline);
    printf("%s", online);
    send(*clnt_id, online, sizeof(online), 0);
    
    
    //alert others user "x" has joined
    for(int i=0; i<5; i++){
        //get file descriptor from struct
           int *cycle_fd = clients[i].connfd_ptr;
            if(*cycle_fd == 0){
             printf(".....\n");   
            }
        //send to all but users file descriptor
            else if( *cycle_fd != *clnt_id && *cycle_fd != 0){
                send(*cycle_fd, hasJoined, sizeof(hasJoined), 0);
            }
        }
    
    //testing to print out connected client file descriptors
    for(int i=0; i< 5; i++){
            if(clients[i].connfd_ptr==0){
                    break;
            }
            printf("connected client fd: %d\n", *clients[i].connfd_ptr);
            
    }
    printf("%s \n",usr);    
    //infinite loop to send messages to other clients
     while(1){
      
        memset(buffer, 0, 101);
        memset(usrMess, 0,110);
        int readval = read(*clnt_id, buffer, 101);
        if(readval ==0 || readval ==-1){
            break;
            
        }
       else{
            
            
            snprintf(usrMess, sizeof(usrMess),"%s: %s", usr, buffer);
            if(readval>1){
            for(int i=0; i<5; i++){
            //read then send buffer to list of connected clients
            int *cycle_fd = clients[i].connfd_ptr;
             if( *cycle_fd != *clnt_id && *cycle_fd != 0){
                   // printf("testing\n");
                 //   printf("%s\n", usrMess);
                    send(*cycle_fd, usrMess, sizeof(usrMess), 0);
                }
            }
            }
       }
    }
    for(int i=0; i<5; i++){
                    if(*clients[i].connfd_ptr == *clnt_id){
                            pthread_mutex_lock(&lock);
                            *clients[i].connfd_ptr = 0;
                            memset(clients[i].username, 0, 10);
                            pthread_mutex_unlock(&lock);
                            break;
                    }
    }       
        
    pthread_detach(pthread_self());
    close(*clnt_id);

   return NULL;
}


int checkNumOnline(){
   int online=0;
    for(int i=0; i<5; i++){
        if(*clients[i].connfd_ptr !=0){
            online++;
        }
        
    }
    return online;
    
    
}
//check stored FD for open spot
//return 1 if no open spots
//return 0 if open spot
int check_open(){
    pthread_mutex_lock(&lock);
     int check = 0;
     for(int i=0; i<5; i++){
         
           if(*clients[i].connfd_ptr==0){
                check++;
           }
     }
     pthread_mutex_unlock(&lock);
     if(check==0){//zero open spots
        //close socket
         printf("closing socket....\n");
         return 0;
     }
     else{
         // continue adding user to system
        
        return 1;
     }
}
char *readline(int fd)
{
  static char line[MAX_MESSAGE_LENGTH + 1];
  char next;
  int pos = 0, nread;

  errno = 0;
  while ((nread = read(fd,&next,1)) > 0 && next != '\n')
    line[pos++] = next;

  line[pos] = '\0';

  if (errno || (nread == 0 && pos == 0))
    return NULL;

  return line;
}






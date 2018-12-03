/// interface.c
/// interface pure = client udp en C

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <signal.h>

#include "interface.h"

#define TIMEOUT_MS 1
#define MAXSIZE 1024

// Global variable
int pure_socket_fd;
struct sockaddr_in pure_server_addr;

// last received message
uint8_t pure_request_id;
int     pure_recv_message_len;
int     pure_recv_responce_flag;
pure_recv_message_t pure_recv_message;

// thread activity
int     pure_recv_thread_active=0;
pthread_t pure_recv_thread;

// System service data
pure_services_t      pure_services;
pure_notifications_t pure_notifications;
pure_diagnostics_t   pure_diagnostics;


// notification callback = function pointer
void (*pure_notification_cb)(int source, float timestamp, void *data);

// private functions
void pure_recv_error(int result);




//=============================================================================
//  Pure basic communication functions
//=============================================================================

//-----------------------------------------------------------------------------
void pure_recv_callback(void *none)
/// callback function. 
/// Manages the message received from pure socket.
/// Check message type (responce or outbound)
/// In case of outbound -> call pure_process_notification
/// Message is kept in global variable until :
///   - end of responce processing
///   - or next reception of outbound 
{
  struct sockaddr_in src_addr;
  socklen_t src_addr_len;
  pure_recv_responce_flag = 0;

  while(pure_recv_thread_active) {    
    if(pure_recv_responce_flag==0) {
      src_addr_len = sizeof(src_addr);
      pure_recv_message_len = 
	recvfrom(pure_socket_fd, 
		 (void *)pure_recv_message.buffer, MAXSIZE, 0,
		 (struct sockaddr *) &src_addr, &src_addr_len);
      
      /* Todo DATA log
	for(size_t i=0;i<pure_recv_message_len; i++)
	printf("%02X ", pure_recv_message.buffer[i]);
	printf("\n");
      */
      if(pure_recv_message.responce.identifier!=0xFF)
	pure_recv_responce_flag = 1;
      else
	pure_notification_cb(pure_recv_message.notification.source,
			     //(float)pure_recv_message.notification.timestamp_hi/100*
			     +(float)pure_recv_message.notification.timestamp_lo/100,
			     pure_recv_message.notification.data);
    };
  };
  pthread_exit(0);
}

//-----------------------------------------------------------------------------
int pure_connection_start(const char *ip, int port, 
			  void (*_notification_cb)(int, float, void*))

{
  // create socket
  int sockfd;
  
  pure_socket_fd = -1;
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("pure UDP client (create_socket):");
    return 1;
  }
  
  // Bind
  struct sockaddr_in this;
  bzero(&this,sizeof(this));
  this.sin_family = AF_INET;
  this.sin_port = htons(0);
  this.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sockfd, (const struct sockaddr*)&this, sizeof(this)) < 0) {
    perror("pure UDP client (create_socket):");
    return 1;
  };

  // Everything is ok then set pure_socket_fd
  pure_socket_fd = sockfd;
  
  // Create server address structure
  bzero(&pure_server_addr, sizeof(pure_server_addr));
  pure_server_addr.sin_family = AF_INET;
  pure_server_addr.sin_port = htons(port);
  pure_server_addr.sin_addr.s_addr = inet_addr(ip);

  // Notification callback
  pure_notification_cb = _notification_cb;
  pure_recv_thread_active = 1;
  if(pthread_create (&pure_recv_thread, NULL, (void *)pure_recv_callback, (void *)"")<0) {
    perror("pthread_create error\n");
    return 1;
  }

  pure_request_id=0x10;

  // services
  pure_services.number = 0;
  pure_services.list = NULL;

  // notifications
  pure_notifications.number = 0;
  pure_notifications.list = NULL;

  // diagnostic
  pure_diagnostics.number = 0;
  pure_diagnostics.list = NULL;

  return 0;
}

//-----------------------------------------------------------------------------
void pure_connection_stop()
{
  if(pure_socket_fd>0) {
    pure_recv_thread_active = 0;  //stop recv thread
    usleep(200e3);                //wait 200ms
    close(pure_socket_fd);
    pure_socket_fd = -1;
  }

  if(pure_services.list != NULL)
    free(pure_services.list);
  
  if(pure_notifications.list!=NULL)
    free(pure_notifications.list);

  if(pure_diagnostics.list!=NULL)
    free(pure_diagnostics.list);
}


//-----------------------------------------------------------------------------
int pure_send_message(uint8_t* buffer, int len)
{
  /* TODO log data
  printf("send: ");
  for(size_t i=0;i<tx_len+4;i++) printf("%02X ", send_request.buffer[i]);
  printf("\n");
  */
  int n=sendto(pure_socket_fd,(const void *)buffer, len, 0,
	       (const struct sockaddr *)&pure_server_addr,sizeof(pure_server_addr));
  if(n< 0) {
    perror("UDP client (sendto error)");
    return n;
  }
  else return 0;
}


//-----------------------------------------------------------------------------
int pure_send_request(int service_instance, enum pure_action_code_t action, 
		      void *tx_data, int tx_len,
		      void *rx_data, int *rx_len)
{
  pure_send_message_t send_request;
  int count = 0;

  // Send request order
  send_request.request.identifier = pure_request_id;
  send_request.request.action = action;
  send_request.request.target = service_instance; 
  memcpy(send_request.request.data, tx_data, tx_len);

  // Send request
  pure_send_message(send_request.buffer, tx_len+4);
  
  // Wait request responce 
  // timeout = 100ms
  while((!pure_recv_responce_flag)&&(count<100)) {
    usleep(1e3); // 1ms
    count++;
  }
  if(count>=100) {
    printf("(pure_send_request): timeout error\n");
    return 1;
  }
    
  // Check request responce
  if(pure_recv_message.responce.identifier!=pure_request_id) {
    printf("Error while receiving request responce: bad identifier\n");
    return 2;
  }

  
  if(pure_recv_message.responce.result!=0x00) {
    // for information only
    pure_recv_error(pure_recv_message.responce.result);
  }
  
  // copy responce data
  *rx_len = pure_recv_message_len - 5;
  memcpy(rx_data, pure_recv_message.responce.data, *rx_len); 
  
  // Release request receive responce flag
  pure_recv_responce_flag=0;
    
  // Increment request identifier
  pure_request_id++;
  if(pure_request_id>0xF0)
    pure_request_id = 0x10;
  return 0;
}  

//-----------------------------------------------------------------------------
void pure_recv_error(int result)
{
  switch(result) {
  case 0x01:
    printf("pure_recv_message error: unknown target\n");
    break;
  case 0x02:
    printf("pure_recv_message error: not supported action\n");
    break;
  case 0x03:
    printf("pure_recv_message error: unknown action\n");
    break;
  case 0x04:
    printf("pure_recv_message error: invalid length\n");
    break;
  case 0x05:
    printf("pure_recv_message error: invalid data\n");
    break;
  default:
    printf("pure_recv_message error: unknown error code = 0x%02x\n", result);
    break;
  };pure_send_request;
};    



//=============================================================================
//  Directory functions
//=============================================================================

//-----------------------------------------------------------------------------
int pure_directory_init() 
{ 
  uint8_t buffer[1024];
  int len;
  int ret;

  // Get list of services
  ret=pure_send_request(0x0000, pure_action_GET, NULL, 0, buffer, &len);
  if(ret) 
    return ret;
  
  pure_services.number = len/4;
  pure_services.list = 
    malloc(pure_services.number*sizeof(pure_service_entry_t));
  
  for(size_t i=0; i<pure_services.number; i++) {
    pure_services.list[i].type = (int)buffer[i*4]+((int)buffer[i*4+1]<<8);
    pure_services.list[i].instance = (int)buffer[i*4+2]+((int)buffer[i*4+3]<<8);
    // to modify the type for 400x
    if(pure_services.list[i].type==0X0000)
      strcpy(pure_services.list[i].name, "Directory service");
    else if (pure_services.list[i].type==0X0001)
      strcpy(pure_services.list[i].name, "Notification service");
    else if (pure_services.list[i].type==0X0002)
      strcpy(pure_services.list[i].name, "Diagnostic service");
    else if (pure_services.list[i].type==0X4001)
      strcpy(pure_services.list[i].name, "I0O service");
    else if (pure_services.list[i].type==0X4003)
      strcpy(pure_services.list[i].name, "Car service");
    else if (pure_services.list[i].type==0X4004)
      strcpy(pure_services.list[i].name, "Laser service");
    else if (pure_services.list[i].type==0X4009)
      strcpy(pure_services.list[i].name, "Drive service");
    else if (pure_services.list[i].type==0X400A)
      strcpy(pure_services.list[i].name, "Encoder service");
    else
      strcpy(pure_services.list[i].name, "");
  }

  // For each service get name
  /*
  for(size_t i=0; i<pure_services.number; i++) {
    if(!pure_send_request(0x0000, pure_action_QUERY, 
			  (void*)&pure_services.list[i].instance, 2, 
			  buffer, &len))
      return 0;
    
    if(len>0) {
      buffer[len] = 0;
      strcpy(pure_services.list[i].name, "blabla");//(const char*) buffer);
    }
  }
  */
  
  return 0;
}

//-----------------------------------------------------------------------------
pure_services_t *pure_directory_list()
{
  return &pure_services;
}


//-----------------------------------------------------------------------------
void pure_directory_print()
{
  // Print result
  printf("\nNumber of active services = %i\n", pure_services.number);
  for(size_t i=0; i<pure_services.number; i++)
    printf("%04X - %04X : %s\n", 
	   pure_services.list[i].instance,
	   pure_services.list[i].type,
	   pure_services.list[i].name);  
}



//=============================================================================
//  Notification functions
//=============================================================================


//-----------------------------------------------------------------------------
int pure_notification_insert(uint16_t service, uint8_t mode)
{
  int buffer[256];
  int buffer_len;
  pure_notification_entry_t notification_entry;

  notification_entry.instance = service;
  notification_entry.mode = mode;
 
 // service 0x0001 : NotificationManager  
  return pure_send_request(0x0001, pure_action_INSERT, 
			   (void *)&notification_entry, sizeof(notification_entry), 
			   buffer, &buffer_len);
}

//-----------------------------------------------------------------------------
int pure_notification_delete(uint16_t service)
{
  int buffer[256];
  int buffer_len;
  pure_notification_entry_t notification_entry;

  notification_entry.instance = service;

 // service 0x0001 : NotificationManager  
  return pure_send_request(0x0001, pure_action_DELETE, 
			   (void *)&notification_entry, 2,
			   buffer, &buffer_len);
}

//-----------------------------------------------------------------------------
int pure_notifications_init()
{
  void *buffer;
  int buffer_len;

  if(!pure_send_request(0x0001, pure_action_GET, 
			NULL, 0,
			buffer, &buffer_len))
    return 0;

  pure_notifications.number = buffer_len/3;
  pure_notifications.list = 
    malloc(pure_notifications.number*sizeof(pure_notification_entry_t));
  memcpy((void *)pure_notifications.list, buffer, buffer_len);
  return 1;
}

//-----------------------------------------------------------------------------
pure_notifications_t *pure_notifications_list()
{
  return &pure_notifications;
}

//-----------------------------------------------------------------------------
void pure_notifications_print()
{  
  printf("Notifications list (%i):\n", pure_notifications.number);
  for(size_t i=0;i<pure_notifications.number; i++) 
    printf("service: 0x%04X - mode: %i\n", 
	   pure_notifications.list[i].instance,
	   pure_notifications.list[i].mode);
}

//-----------------------------------------------------------------------------
int pure_notifications_clear_all()
{
  printf("Clear notifications (%i):\n", pure_notifications.number);
  for(size_t i=0;i<pure_notifications.number; i++) 
    if(pure_notification_delete(pure_notifications.list[i].instance))
      return 1;
}



//=============================================================================
//  Diagnostic functions
//=============================================================================


//-----------------------------------------------------------------------------
int pure_diagnostic_init(int diagnostic_instance)
{
  uint32_t buffer[64];
  int len;
  
  // Get status value of all devices (--> devices number)
  if(pure_send_request( diagnostic_instance, pure_action_GET,
			 NULL, 0,
			 buffer, &len))
    return 1;

  pure_diagnostics.instance = diagnostic_instance;
  pure_diagnostics.number = len/4;
  pure_diagnostics.list = 
                malloc(pure_diagnostics.number*sizeof(pure_diagnostic_entry_t));

  // populate
  for(size_t i=0; i<pure_diagnostics.number; i++)
    pure_diagnostics.list[i].status = buffer[i];

  // Get name of each devices
  char name[256];
  uint32_t id;
  for(id=0; id < pure_diagnostics.number; id++) {
    if(pure_send_request( pure_diagnostics.instance, pure_action_QUERY,
			   &id, 4,
			   name, &len))
      return 1;
    name[len]=0; // string termination
    strcpy(pure_diagnostics.list[id].name, name);
  }
  return 0;
}

//-----------------------------------------------------------------------------
int pure_diagnostic_refresh()
{
  uint32_t buffer[64];
  int len;
  // Get status value of all devices
  if(pure_send_request( pure_diagnostics.instance, pure_action_GET,
			 NULL, 0,
			 buffer, &len))
    return 1;
  for(size_t i=0; i<pure_diagnostics.number; i++)
    pure_diagnostics.list[i].status = buffer[i];
  return 0;
}

//-----------------------------------------------------------------------------
void pure_diagnostic_print()
{
  printf("\nDevices number:%i\n", pure_diagnostics.number);
  for(size_t i=0; i<pure_diagnostics.number; i++) {
    printf("%s : %04x\n",
	   pure_diagnostics.list[i].name,
	   pure_diagnostics.list[i].status);
  }
}



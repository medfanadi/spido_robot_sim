//---------------------------------spido_pure_interface------------------------------------
/* This node is a simple control interface for the fast-b robot from Robosoft.
 * The node subscribes to the "/cmd_car_safe" topic with a message of type "cmd_car"
 * --------------------------
 * cmd_car message structure:
 * float32 linear_speed
 * float32 steering_angle
 * --------------------------
 * The callback function carCallback will transmit the target linear_speed and
 * steering_angle to the robot using the pure interface (interface.c)
 * -----------------------------------------------------------------------------
 * -----------------------------NEXT STEPS--------------------------------------
 * This is the only working feature for now, but there is a TODO list:
 * 
 * *Run some security tests
 * 
 * *Subscribe to another topic (cmd_drive for instance) to control the robot
 * giving two steering angles and each wheel speed.
 * 
 * *Same thing to control the two roll bars
 * 
 * *Publish diagnostic
 * 
 * *Publish each available data from the sensors
 * 
 * *Publish the laser data into a standard ROS laserscan message
 * 
 * *Anything else?
 * ----------------------------------END----------------------------------------
*/




#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/Float32.h"
#include "spido_pure_interface/cmd_car.h"


#include <sstream>

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

//#include <cmath.h>
#include <math.h>

extern "C"{
#include "interface.h"                     //Pure communication library (C code)
}                  

void process_notification(int service, float timestamp, void *data)
{
  printf("Notification callback from %i\n", service);
}

void carCallback(const spido_pure_interface::cmd_car::ConstPtr& msg)
{
  //Test cmd line: # rostopic pub  /cmd_car spido_pure_interface/cmd_car -- 1 1
  ROS_INFO("Received cmd_car: linear_speed: %f | steering_angle_front %f| steering_angle_rear %f",
	   msg->linear_speed, msg->steering_angle_front, msg->steering_angle_rear);
  struct pure_car_service_target_t car_target;
  car_target.enable = 1;
  car_target.speed = msg->linear_speed; 	           //target linear speed (m/s)
  car_target.steering_front = msg->steering_angle_front;             //target steering (rad)
  car_target.steering_rear = msg->steering_angle_rear;             //target steering (rad)
  
  if (car_target.speed> 0.5)
  {
      car_target.speed = 0.5;
  }
  if (std::fabs(car_target.steering_front) > 0.3)
  {
      car_target.steering_front = copysign(0.3, car_target.steering_front);
  }
  
if (std::fabs(car_target.steering_rear) > 0.3)
  {
      car_target.steering_rear = copysign(0.3, car_target.steering_rear);
  }
    
  pure_send_message_t message;
  message.notification.identifier = 0xFF;
  message.notification.target = 0x003;
  memcpy(message.notification.data, (void *)&car_target, sizeof(car_target));
  pure_send_message(message.buffer, sizeof(car_target)+3);
  //TODO: security test: what happends if the connection is lost while linear_speed <> 0 ?
  
  /*
  spido_pure_interface::cmd_car msg_feedback;
  msg_feedback.linear_speed = msg->linear_speed;
  msg_feedback.steering_angle = msg->steering_angle;
  cmd_car_feedback.publish(msg_feedback);
  */ //TODO: implement cmd_car_feedback (or juste remove this feature, need to discuss it)
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "spido_pure_interface");
  ros::NodeHandle n;
  //TODO: subscibe and publish on the good topics with appropriate message type
  //ros::Publisher cmd_car_feedback = //TODO: remove or implement this
  //n.advertise<spido_pure_interface::cmd_car>("cmd_car_feedback", 1000);
  ros::Subscriber sub = n.subscribe("cmd_car_safe", 1000, carCallback);
  ros::Rate loop_rate(10); 		                //Loop rate for the main loop (Hz)
  
//--------------------------Pure communication init-----------------------------
  uint16_t diagnostic_instance = 0;
  pure_services_t *services;
  
  pure_connection_start("192.168.1.2", 60000, process_notification);
  pure_directory_init();
  pure_directory_print();
  services = pure_directory_list();
    
//--------------------Search diagnostic service instance------------------------
  for(size_t i=0; i < services->number; i++)
    if(services->list[i].type == 0x0002)
    {
      diagnostic_instance = services->list[i].instance;
    }

  if(diagnostic_instance != 0) {
    pure_diagnostic_init(diagnostic_instance);
    pure_diagnostic_refresh();
    pure_diagnostic_print();
  };
 
  
//----------------------------Laser service-------------------------------------
/* ****************************** Laser broken ****************************** */
/*
  struct pure_laser_config_t laser_config;
  int len;
  pure_send_request(0x0006, pure_action_GET,
		    NULL, 0,(void *)&laser_config,&len);

  printf("\nLaser service data:\n");
  printf("x_pos= %.3f\n", laser_config.x_pos);
  printf("y_pos= %.3f\n", laser_config.y_pos);
  printf("heading= %.3f\n", laser_config.heading);
  printf("nb_echos= %i\n", laser_config.nb_echos);
*/
//-------------------------Laser service END------------------------------------

//-----------------------------Car service--------------------------------------
  struct pure_car_service_data_t car_config;
  int len;
  pure_send_request(0x0003, pure_action_GET,
		    NULL, 0,(void *)&car_config,&len);

  printf("\ncar service data:\n");
  printf("max_speed= %.3f\n", car_config.max_speed);
  printf("min_speed= %.3f\n", car_config.min_speed);
  printf("max_steering= %.3f\n", car_config.max_steering);
  printf("min_steering= %.3f\n", car_config.min_steering);
  printf("max_acceleration= %.3f\n", car_config.max_acceleration);
  printf("max_deceleration= %.3f\n", car_config.min_acceleration);
  printf("distance_between_axels= %f\n", car_config.wheelbase);
//--------------------------Car service END-------------------------------------
  struct pure_car_service_target_t car_target;
    car_target.enable = 1;
    car_target.speed = 0; 
    car_target.steering_front = 0;
    car_target.steering_rear = 0;
    pure_send_message_t message;
    message.notification.identifier = 0xFF;
    message.notification.target = 0x0003;
    memcpy(message.notification.data, (void *)&car_target, sizeof(car_target));
    pure_send_message(message.buffer, sizeof(car_target)+3);

  while (ros::ok())
  {

//---------------------------------TEST Section---------------------------------
    /*std_msgs::String msg;

    std::stringstream ss;
    ss << "hello world ";
    msg.data = ss.str();*/
    
    
    //spido_pure_interface::cmd_car msg;
    //cmd_car_feedback.publish(msg);
    
    
    /*struct pure_car_service_target_t car_target;
    car_target.enable = 1;
    car_target.speed = 0.0; 			     //target linear speed (m/s)
    car_target.steering = 0.3;			         //target steering (rad)
    
    pure_send_message_t message;
    message.notification.identifier = 0xFF;
    message.notification.target = 0x003;
    memcpy(message.notification.data, (void *)&car_target, sizeof(car_target));
    pure_send_message(message.buffer, sizeof(car_target)+3);
    
    sleep(1);
    
    car_target.steering = -0.3;
    message.notification.identifier = 0xFF;
    message.notification.target = 0x003;
    memcpy(message.notification.data, (void *)&car_target, sizeof(car_target));
    pure_send_message(message.buffer, sizeof(car_target)+3);

    sleep(1);*/
//-------------------------------END TEST Section-------------------------------
/*
    car_target.speed = 0.5;
    car_target.steering = 0.0;
    pure_send_message_t message;
    message.notification.identifier = 0xFF;
    message.notification.target = 0x003;
    memcpy(message.notification.data, (void *)&car_target, sizeof(car_target));
    pure_send_message(message.buffer, sizeof(car_target)+3);
*/
    ros::spinOnce();

    //loop_rate.sleep();
  }
  /*
  car_target.enable = 1;
  car_target.speed = 0;
  car_target.steering = 0;
  message.notification.identifier = 0xFF;
  message.notification.target = 0x0003;
  memcpy(message.notification.data, (void *)&car_target, sizeof(car_target));
  pure_send_message(message.buffer, sizeof(car_target)+3);
  */
  pure_connection_stop();
  printf("Connextion stopped\n");
  return 0;
}

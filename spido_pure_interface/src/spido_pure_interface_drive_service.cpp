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
#include "spido_pure_interface/cmd_drive.h"


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
		    #include <iostream>
//#include <cmath.h>
#include <math.h>

extern "C"{
#include "interface.h"                     //Pure communication library (C code)
}                  
using namespace std;
void process_notification(int service, float timestamp, void *data)
{
  printf("Notification callback from %i\n", service);
}

void driveCallback(const spido_pure_interface::cmd_drive::ConstPtr& msg)
{
  //Test cmd line: # rostopic pub  /cmd_car spido_pure_interface/cmd_car -- 1 1
  ROS_INFO("Received cmd_car: linear_speed: %f | steering_angle_front %f| steering_angle_rear %f",
	   msg->linear_speed, msg->steering_angle_front, msg->steering_angle_rear);

double wfr,wfl,wrr,wrl,psip=0,lf=0.85,lr=0.85,tr=0.5,V,rw=0.4,bf,br,vfrx,vfry,vflx,vfly,vrrx,vrry,vrlx,vrly;
V=msg->linear_speed;
bf=msg->steering_angle_front;
br=msg->steering_angle_rear;
vfrx=V+psip*tr;
vfry=psip*lf;
vflx=V-psip*tr;
vfly=psip*lf;
vrrx=V+psip*tr;
vrry=-psip*lr;
vrlx=V-psip*tr;
vrly=-psip*lr;

wfr=(vfrx*cos(bf)+vfry*sin(bf))/rw;
wfl=(vflx*cos(bf)+vfly*sin(bf))/rw;
wrr=(vrrx*cos(br)+vrry*sin(br))/rw;
wrl=(vrlx*cos(br)+vrly*sin(br))/rw;

/*cout<<wfr<<wfl<<wrr<<wrl<<endl;
cout<<bf<<br<<endl;
pure_drive_control_t drive_target[6];
for(size_t i=0;i<5;i++) drive_target[i].enable = 1;
for(size_t i=0;i<1;i++){
    drive_target[i].mode= 1;
    drive_target[i+3].mode = 1; 
}
    drive_target[2].mode= 0;
    drive_target[5].mode= 0;

    drive_target[0].target = wfr;//rad/s //s'assurer si drive 1 est la droite ou la gauche
    drive_target[1].target = wfl;//rad/s 
    drive_target[2].target = bf;//rad/s 
    drive_target[3].target = wrr;//rad/s //s'assurer si drive 4 est la droite ou la gauche
    drive_target[4].target = wrl;//rad/s 
    drive_target[5].target = br;//rad/s 


  if (drive_target[0].target> 1.25) //check this value
  {
      drive_target[0].target = 1.25;
  }
  if (drive_target[1].target> 1.25) //check this value
  {
      drive_target[1].target = 1.25;
  }
  if (drive_target[3].target> 1.25) //check this value
  {
      drive_target[3].target = 1.25;
  }
  if (drive_target[4].target> 1.25) //check this value
  {
      drive_target[4].target = 1.25;
  }
  if (drive_target[2].target> 0.3) //check this value
  {
      drive_target[2].target = 0.3;
  }
  if (drive_target[5].target> 0.3) //check this value
  {
      drive_target[5].target = 0.3;
  }
  */
struct pure_all_drives_control_t drive_target;
    drive_target.enable1 = 1;
    drive_target.mode1 = 1; 
    drive_target.target1 = wfl;

 drive_target.enable2 = 1;
    drive_target.mode2 = 1; 
    drive_target.target2 = wfr;


 drive_target.enable3 = 1;
    drive_target.mode3 = 0; 
    drive_target.target3 = bf;

 drive_target.enable4 = 1;
    drive_target.mode4 = 1; 
    drive_target.target4 = wrl;

 drive_target.enable5 = 1;
    drive_target.mode5 = 1; 
    drive_target.target5 = wrr;

 drive_target.enable6 = 1;
    drive_target.mode6 = 0; 
    drive_target.target6 = br;
  //cout<<sizeof(drive_target); 
  pure_send_message_t message;
  message.notification.identifier = 0xFF;
  message.notification.target = 0x002;
  memcpy(message.notification.data, (void *)&drive_target, sizeof(drive_target));
  pure_send_message(message.buffer, sizeof(drive_target)+3);
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
  ros::Subscriber sub = n.subscribe("cmd_drive_safe", 1000, driveCallback);
 // ros::Subscriber sub = n.subscribe("/odom", 1000, IMUCallback);
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

//-----------------------------Car service--------------------------------------
 /* struct pure_car_service_data_t drive_config;
  int len;
  pure_send_request(0x0002, pure_action_GET,
		    NULL, 0,(void *)&drive_config,&len);

  printf("\ncar service data:\n");
  printf("max_speed= %.3f\n", car_config.max_speed);
  printf("min_speed= %.3f\n", car_config.min_speed);
  printf("max_steering= %.3f\n", car_config.max_steering);
  printf("min_steering= %.3f\n", car_config.min_steering);
  printf("max_acceleration= %.3f\n", car_config.max_acceleration);
  printf("max_deceleration= %.3f\n", car_config.min_acceleration);
  printf("distance_between_axels= %f\n", car_config.wheelbase);*/
//--------------------------Car service END-------------------------------------
/*pure_drive_control_t drive_target[6];
for(size_t i=0;i<5;i++) drive_target[i].enable = 1;
for(size_t i=0;i<=1;i++){
    drive_target[i].mode= 1;
    drive_target[i+3].mode = 1; 
}
    drive_target[2].mode= 0;
    drive_target[5].mode= 0;

    drive_target[0].target = 0;//rad/s //s'assurer si drive 1 est la droite ou la gauche
    drive_target[1].target = 0;//rad/s 
    drive_target[2].target = 0;//rad/s 
    drive_target[3].target = 0;//rad/s //s'assurer si drive 4 est la droite ou la gauche
    drive_target[4].target = 0;//rad/s 
    drive_target[5].target = 0;//rad/s 
*/
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

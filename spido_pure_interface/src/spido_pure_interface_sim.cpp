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

class spidoPureInterface
{
public:
  spidoPureInterface()
  {
    //Topic you want to publish
    pub_ = n_.advertise<spido_pure_interface::cmd_car>("/real_command", 1);

    //Topic you want to subscribe
    sub_ = n_.subscribe("/cmd_car_safe", 1, &spidoPureInterface::callback, this);
  }

  void callback(const spido_pure_interface::cmd_car::ConstPtr& msg)
  {
    spido_pure_interface::cmd_car output;
    
    struct pure_car_service_target_t car_target;
    car_target.enable = 1;
    car_target.speed = msg->linear_speed; 	           //target linear speed (m/s)
    car_target.steering_front = msg->steering_angle_front;             //target steering (rad)
    car_target.steering_rear = msg->steering_angle_rear;             //target steering (rad)
  
    if (car_target.speed > 0.5)
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
    output.linear_speed = car_target.speed;
    output.steering_angle_front = car_target.steering_front;
    output.steering_angle_rear = car_target.steering_rear;
    
    pub_.publish(output);
  }

private:
  ros::NodeHandle n_; 
  ros::Publisher pub_;
  ros::Subscriber sub_;

};//End of class spidoPureInterface

void process_notification(int service, float timestamp, void *data)
{
  printf("Notification callback from %i\n", service);
}

int main(int argc, char **argv)
{
  //Initiate ROS
  ros::init(argc, argv, "spido_pure_interface");
  struct pure_car_service_target_t car_target;
    car_target.enable = 1;
    car_target.speed = 0; 
    car_target.steering_front = 0;
    car_target.steering_rear = 0;

  //Create an object of class SubscribeAndPublish that will take care of everything
  spidoPureInterface SPIObject;
  
  while (ros::ok())
  {
    ros::spin();
  }
  
  car_target.enable = 1;
  car_target.speed = 0;
  car_target.steering_front = 0;
  car_target.steering_rear = 0;
  
  pure_connection_stop();

  return 0;
}

#include "ros/ros.h"
#include "std_msgs/Float64.h"

#include <sstream>

int main(int argc, char **argv)
{

  ros::init(argc, argv, "spido_gazebo_driver");
  ros::NodeHandle n;
  ros::Publisher front_left_wheel_pub = n.advertise<std_msgs::Float64>("/spido/front_left_wheel/command", 1000);
  ros::Publisher front_right_wheel_pub = n.advertise<std_msgs::Float64>("/spido/front_right_wheel/command", 1000);
  ros::Publisher rear_left_wheel_pub = n.advertise<std_msgs::Float64>("/spido/rear_left_wheel/command", 1000);
  ros::Publisher rear_right_wheel_pub = n.advertise<std_msgs::Float64>("/spido/rear_right_wheel/command", 1000);
  
  ros::Publisher front_left_steering_pub = n.advertise<std_msgs::Float64>("/spido/front_left_steering/command", 1000);
  ros::Publisher front_right_steering_pub = n.advertise<std_msgs::Float64>("/spido/front_right_steering/command", 1000);
  ros::Publisher rear_left_steering_pub = n.advertise<std_msgs::Float64>("/spido/rear_left_steering/command", 1000);
  ros::Publisher rear_right_steering_pub = n.advertise<std_msgs::Float64>("/spido/rear_right_steering/command", 1000);
 
  ros::Rate loop_rate(100);

  int i = 0;
  float max = 1;
  std_msgs::Float64 front_left_steering_cmd;
  std_msgs::Float64 front_left_wheel_cmd;
  std_msgs::Float64 front_right_wheel_cmd;
  front_left_steering_cmd.data = 0;
  front_right_wheel_cmd.data = 0;
  while (ros::ok())
  {
    /*
    if (i<=400){
      front_left_wheel_cmd.data = (float)2*max*i/400 - max;
      front_right_wheel_cmd.data =(float)-2*max*i/400 + max;
      i++;
    }
    else {
      front_left_wheel_cmd.data = (float)-2*max*i/400 + max +2*max;
      front_right_wheel_cmd.data =(float) 2*max*i/400 - max +2*max;
      i++;
    }
    if (i>=800)
      i=0;
    */
    front_left_wheel_cmd.data = (float)8;
    front_right_wheel_cmd.data =(float)8;
    //ROS_INFO("i:""%i",i);
    //ROS_INFO("cmd_wheel:""%f",front_left_wheel_cmd.data);
    front_left_wheel_pub.publish(front_left_wheel_cmd);
    front_right_wheel_pub.publish(front_right_wheel_cmd);
    rear_left_wheel_pub.publish(front_left_wheel_cmd);
    rear_right_wheel_pub.publish(front_right_wheel_cmd);
    
    front_left_steering_pub.publish(front_left_steering_cmd);
    front_right_steering_pub.publish(front_left_steering_cmd);
    rear_left_steering_pub.publish(front_left_steering_cmd);
    rear_right_steering_pub.publish(front_left_steering_cmd);
    ros::spinOnce();

    loop_rate.sleep();
  }


  return 0;
}

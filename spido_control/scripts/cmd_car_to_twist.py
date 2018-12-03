#!/usr/bin/env python2
import rospy
from math import copysign, tan
from spido_pure_interface.msg import cmd_car
from std_msgs.msg import Float32
from geometry_msgs.msg import Twist

new_twist = False


def cmdcallback(data):
    global twist
    global new_twist
    l = 0.2                             #axial wheel to wheel length (real ~0.8)
    twist = Twist()
    twist.linear.x = data.linear_speed
    twist.angular.z = 2 * data.linear_speed * tan(data.steering_angle) / l
    new_twist = True
    

def cmd_car_to_twist():
    global new_twist
    global twist
    rospy.init_node('cmd_car_to_twist')
    rospy.Subscriber("cmd_car_safe", cmd_car, cmdcallback)
    cmd_vel_publisher = rospy.Publisher('cmd_vel',Twist,queue_size=10)
    r = rospy.Rate(10) # 50hz
    while not rospy.is_shutdown():
        if new_twist == True:
            cmd_vel_publisher.publish(twist)
            new_twist = False
        r.sleep()
        
if __name__ == '__main__':
    cmd_car_to_twist()

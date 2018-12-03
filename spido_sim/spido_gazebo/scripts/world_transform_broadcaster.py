#!/usr/bin/env python

import roslib
#roslib.load_manifest('cob_fit')
import rospy
import tf
import tf.msg
import geometry_msgs.msg
import gazebo_msgs.srv

class FixedTFBroadcaster:

  def __init__(self):
    self.pub_tf = rospy.Publisher("/tf", tf.msg.tfMessage)
    
    t2 = geometry_msgs.msg.TransformStamped()
    t2.header.frame_id = "world"
    t2.child_frame_id = "/spido_body"
    
    r = rospy.Rate(150)
    
    while not rospy.is_shutdown():     
      try:      
        #service call to obtain model prosition from gazebo
        response=model_state('spido','world')
        rospy.loginfo("model_state('spido','ground_plane')") 
      except rospy.ServiceException, e:
        rospy.loginfo("Service call failed: %s", e)
            
      if response.success:
        
        rospy.loginfo("Service call returned success")    
        # transform from world to cabinet

        t2.header.stamp = rospy.Time.now()
        t2.transform.translation = response.pose.position
        t2.transform.rotation = response.pose.orientation
        
        tfm2 = tf.msg.tfMessage([t2])
        self.pub_tf.publish(tfm2)
      if not response.success:
        rospy.loginfo("Service call returned error: %s",response.status_message)
      
      # sleep for 0.1s
      r.sleep()


if __name__ == '__main__':

    
    rospy.init_node('my_tf_broadcaster')
    rospy.loginfo("Starting my_tf_broadcaster...")
    
    rospy.sleep(5)
    
    rospy.loginfo("Waiting for service gazebo/get_model_state...")
    rospy.wait_for_service('gazebo/get_model_state')
    
    # maybe this should be persistent connection....
    model_state = rospy.ServiceProxy('gazebo/get_model_state', gazebo_msgs.srv.GetModelState)
    rospy.loginfo("gazebo/get_model_state called...")
    tfb = FixedTFBroadcaster()
    rospy.spin()
    rospy.loginfo("my_tf_broadcaster died...")

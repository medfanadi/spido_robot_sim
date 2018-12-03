# spido_simulation
RobuFast Robot folders (description, urdf, plugins, gazebo)

Spido robot is a fast all-terrain mobile robot with four-wheel drive and steering (4WDS). It can reach 12 m/s speed. 
It weighs about 700 kg. It is equipped with 4 dual wishbone independent suspensions and 2 active anti-roll bars. 
A Real Time Kinematic GPS (RTK-GPS) provides an accurate localization in real-time of the platform with respect 
to a reference station, and an IMU (Inertial Measurement Unit) gives orientation angles of the platform. 
A 3D LIDAR allows to perceive the environment and to identify dynamic or static obstacles.

http://www.isir.upmc.fr/index.php?op=view_equipe&lang=fr&pageid=1506&id=4

Spido_sim
=============

Packages for the simulation of the Spido

<h2>spido_control</h2>

<p>This package contains the launch and configuration files to spawn the joint controllers with the ROS controller_manager. 
It allows to launch the joint controllers for the Spido(4WDS).

The Summit XL simulation stack follows the gazebo_ros controller manager scheme described in
http://gazebosim.org/wiki/Tutorials/1.9/ROS_Control_with_Gazebo</p>

<h2>spido_gazebo</h2>

launch files and world files to start the models in gazebo

<h2>spido_description</h2>

<p>control the robot joints in all kinematic configurations, publishes odom topic and, if configured, 
also tf odom to base_link. 

When used as main controller of the simulated robot, this node also computes the odometry of the robot using the joint movements and a IMU and publish this odometry to /odom. The node has a flag in the yaml files that forces the publication or not of the odom->base_footprint frames, needed by the localization and mapping algorithms.
</p>

<h2>spido_pure_interface</h2>
<p> Control low-level (Front and rear steering angles)<p>

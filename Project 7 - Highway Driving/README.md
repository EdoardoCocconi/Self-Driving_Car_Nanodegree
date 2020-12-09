# CarND-Path-Planning-Project
[![Udacity - Self-Driving Car NanoDegree](https://s3.amazonaws.com/udacity-sdc/github/shield-carnd.svg)](http://www.udacity.com/drive)

<br>

<div align="center">
  <img width="640" img src="Assets/HighwayDriving.gif" alt="Highway driving simulation video">
</div>

<br>

<p align="center">
  <b><i>The highway autopilot plans the car's route and speed through the traffic in complete autonomy.</i></b>
</p>

<br>

## Project Introduction
The goal of this project is to safely navigate around a virtual highway with other traffic that is driving +-10 MPH of the 50 MPH speed limit. The simulation provides the car's location and sensor fusion data to determine position and velocity of the other vehicles. There is also a sparse map list of waypoints around the highway so the motion planner can take the shape of the road into account. The car should try to travel as close as possible to the 50 MPH speed limit, which means passing slower traffic when possible, note that other cars will try to change lanes too. The car should avoid hitting other cars at all cost as well as driving inside of the marked road lanes at all times, unless going from one lane to another. The car should be able to make one complete loop around the 4.32 miles (6946 meters) highway. Since the car is trying to keep a velocity of 50 MPH, it should take a little over 5 minutes to complete 1 loop. Also the car should not experience total acceleration over 10 m/s^2 and jerk that is greater than 10 m/s^3 for the safety and comfort of its passengers.

## Path Generation
In this project trajectories and speed are determined by a state machine. (lines 131-295 in main.cpp) The output of the trajectory generator is a series of points that represent the future positions and orientations of the car at subsequent cycles (lines 300-410 in main.cpp). The speed of the car can therefore be regulated by choosing the distance between these points (ref_vel at line 388 in main.cpp). The shape of the path is obtained by using the spline class and feeding it the target lane and velocity obtained with the state machine (see target_lane at lines 341-343 in main.cpp). The spline header has been downloaded from [this link](http://kluge.in-chemnitz.de/opensource/spline/) and is already included in the src folder. The points are not recalculated from scratch at every cycle but new points are appended to the end of the path after every movement of the car (lines 387-407 in main.cpp). Therefore the car's reactions are delayed and this has to be taken into account in the calculations (notice how end_path_s is used in lines 151-153, 159, 169, and 182 in main.cpp). It is assumed that the car manages to perfectly follow the chosen trajectory so there is no need for a controller.

## State Machine

#### GO:
This is the state that governs the gas pedal (lines 231-243 in main.cpp). The car's velocity increases with constant acceleration until the speed limit is reached (lines 238-241 in main.cpp). If the change lane state previously decided to switch lane, the GO state will accelerate towards the goal lane. So it is possible to change lane while accelerating. This state can lead to the BRAKE state if there is an obstacle (lines 187-190 in main.cpp) or to the CHANGE LANE state if the current lane is not optimal (lines 196-204 and 234-237 in main.cpp). 

#### BRAKE:
This is the state that governs the brake (lines 207-230 in main.cpp). For this project the brake has been chosen to generate a constant deceleration of 4m/s^2 (DECELERATION at line 229 in main.cpp). Knowing the deceleration, the speed of the vehicle, the distance to the obstacle, and the speed of the obstacle it is possible to calculate at what distance the car should start braking (lines 181-182 in main.cpp). The reaction time of the vehicle is properly taken into account by forecasting the future position of the other vehicles (lines 151 in main.cpp). These calculations allow the car to slow down to the same velocity of the vehicle in front leaving the exact safety distance necessary. if the vehicle in front was to stop, the braking time of our vehicle depends on the square of its speed, so the safety distance is proportional to this quantity (lines 156 in main.cpp). Similarly to the GO state, the BRAKE state can decelerate while changing lane (if the CHANGE LANE state decided to). This is particularly important to dodge cars that change lane without looking if our car is coming from behind. This state can lead to the FOLLOW state if after decelerating our vehicle travels at the same speed of the vehicle in front (lines 210-213 in main.cpp). It can also lead to the CHANGE LANE state if the current lane is not the optimal lane (lines 196-204 and 214-217 in main.cpp).

#### FOLLOW:
If all the points composing the trajectory were recalculated at every cycle, there would be no need for this state. This state is responsible for maintaining the safety distance while keeping the same speed of the vehicle in front (lines 245-276 in main.cpp). It is theoretically possible to achieve this wit just a GO and a BRAKE state, but the delay in the car reaction causes oscillations around the safety distance. This state takes therefore a different approach and copies the vehicle in front (line 251 in main.cpp). This state ends when the car in front of us changes lane and lets us proceed (lines 261-266 in main.cpp) or when our car changes lane (lines 271-274 in main.cpp).

#### CHANGE LANE:
This is the state that governs the steering wheel (lines 278-295 in main.cpp). The steering wheel is not connected to the gas or brake and this could be a state machine on its own. However, a change in lane could be indicative of the fact that the car could get out of its FOLLOW or BRAKE state and get back to the desired speed with the GO state. The CHANGE LANE state works like this:
* **Calculate the optimal lane:** The optimal lane is always the lane in which the first car encountered by the vehicle would be the farthest (lines 169-177 and 196-204 in main.cpp), taking into account the speed of the vehicles as well (s_predicted at lines 170, 172, and 174 in main.cpp). There are few situations where this is not the optimal solution. For example it doesn't take into account if a car will loose ground due to encountering a slow car on its path. However it is almost always the best choice and greatly simplifies calculations.
* **Check for obstacles:** The car looks for obstacles in the other lanes (lines 284 and 289 in main.cpp), specifically cars approaching from behind at high speed or cars in front of us that would be within the safety distance (lines 159-167 in main.cpp).
* **Change target lane:** If there are no obstacles in the way the target lane is moved of one lane towards the optimal lane (lines 285 and 290 in main.cpp). If there are no obstacles and this operation takes place the next state will be the GO state (lines 286 and 291 in main.cpp), otherwise the state machine will go back to whatever was the previous state (line 282 in main.cpp).
The CHANGE LANE state only lasts for one cycle. The vehicle has to wait 4 seconds to change lane again while accelerating or following (lines 234 and 271 in main.cpp), or 1 second while breaking for safety reasons (BRAKE_REACTION_TIME at line 214 in main.cpp). This allows the car to avoid rapid lane fluctuations.

## RESULTS & FUTURE WORK
As a result of this process the highway autopilot can safely and comfortably navigate through the traffic. As shown in the video above and in the red square of the image below, the car can complete more than 6 consecutive laps without braking any of the rules imposed for the success of the project.

<br>

<div align="center">
  <img width="640" img src="Assets/28Miles.PNG" alt="28 miles driven image">
</div>

<br>

The success of this autopilot come from the fact that only one constant has been found through trial and error (3.5 at lines 156, 250, and 252 in main.cpp). All the other numbers involved in the calculations are derived from physics formulae. This allows the car to be able to drive safely at every chosen speed. You can check this by uncommenting

```shell
sport = true;
```

at line 20 of main.cpp. The rare mistakes made at higher speeds by the autopilot are probably a result of the above mentioned constant. For future work every 3.5 in the main.cpp file should be substituted with a physical formula.
Another improvement that has to be made is the implementation of a safety break. In the current implementation the autopilot calculates at what distance the car has to break to reach the desired state with a gentle deceleration (line 182 in main.cpp). Some times, due to cars changing lane and sudden appearance of obstacles, it is instead necessary to calculate how hard the car has to break to avoid a collision. 
The autopilot is not ready for driving a real car. It is much better than human drivers in most situations but it makes mistakes in rare and unpredictable circumstances. On the other hand, humans are much better at improvising. For future work, changes would have to be made so these unpredictable situations can be replicated once they are individuated. Spotting mistakes without this change would be a very long and tedious job.

###Simulator
The simulator used for this project can be downloaded from the [releases tab](https://github.com/udacity/self-driving-car-sim/releases/tag/T3_v1.2).  

To run the simulator on Mac/Linux, first make the binary file executable with the following command:
```shell
sudo chmod u+x {simulator_file_name}
```

#### The map of the highway is in data/highway_map.txt
Each waypoint in the list contains  [x,y,s,dx,dy] values. x and y are the waypoint's map coordinate position, the s value is the distance along the road to get to that waypoint in meters, the dx and dy values define the unit normal vector pointing outward of the highway loop.

The highway's waypoints loop around so the frenet s value, distance along the road, goes from 0 to 6945.554.

## Basic Build Instructions

1. Clone this repo.
2. Make a build directory: `mkdir build && cd build`
3. Compile: `cmake .. && make`
4. Run it: `./path_planning`.

Here is the data provided from the Simulator to the C++ Program

#### Main car's localization Data (No Noise)

["x"] The car's x position in map coordinates

["y"] The car's y position in map coordinates

["s"] The car's s position in frenet coordinates

["d"] The car's d position in frenet coordinates

["yaw"] The car's yaw angle in the map

["speed"] The car's speed in MPH

#### Previous path data given to the Planner

//Note: Return the previous list but with processed points removed, can be a nice tool to show how far along
the path has processed since last time. 

["previous_path_x"] The previous list of x points previously given to the simulator

["previous_path_y"] The previous list of y points previously given to the simulator

#### Previous path's end s and d values 

["end_path_s"] The previous list's last point's frenet s value

["end_path_d"] The previous list's last point's frenet d value

#### Sensor Fusion Data, a list of all other car's attributes on the same side of the road. (No Noise)

["sensor_fusion"] A 2d vector of cars and then that car's [car's unique ID, car's x position in map coordinates, car's y position in map coordinates, car's x velocity in m/s, car's y velocity in m/s, car's s position in frenet coordinates, car's d position in frenet coordinates. 

## Details

1. The car uses a perfect controller and will visit every (x,y) point it recieves in the list every .02 seconds. The units for the (x,y) points are in meters and the spacing of the points determines the speed of the car. The vector going from a point to the next point in the list dictates the angle of the car. Acceleration both in the tangential and normal directions is measured along with the jerk, the rate of change of total Acceleration. The (x,y) point paths that the planner recieves should not have a total acceleration that goes over 10 m/s^2, also the jerk should not go over 50 m/s^3. (NOTE: As this is BETA, these requirements might change. Also currently jerk is over a .02 second interval, it would probably be better to average total acceleration over 1 second and measure jerk from that.

2. There will be some latency between the simulator running and the path planner returning a path, with optimized code usually its not very long maybe just 1-3 time steps. During this delay the simulator will continue using points that it was last given, because of this its a good idea to store the last points you have used so you can have a smooth transition. previous_path_x, and previous_path_y can be helpful for this transition since they show the last points given to the simulator controller with the processed points already removed. You would either return a path that extends this previous path or make sure to create a new path that has a smooth transition with this last path.

## Tips

A really helpful resource for doing this project and creating smooth trajectories was using http://kluge.in-chemnitz.de/opensource/spline/, the spline function is in a single hearder file is really easy to use.

---

## Dependencies

* cmake >= 3.5
  * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools](https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)
* [uWebSockets](https://github.com/uWebSockets/uWebSockets)
  * Run either `install-mac.sh` or `install-ubuntu.sh`.
  * If you install from source, checkout to commit `e94b6e1`, i.e.
    ```
    git clone https://github.com/uWebSockets/uWebSockets 
    cd uWebSockets
    git checkout e94b6e1
    ```
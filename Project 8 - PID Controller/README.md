# **PID Controller** 
[![Udacity - Self-Driving Car NanoDegree](https://s3.amazonaws.com/udacity-sdc/github/shield-carnd.svg)](http://www.udacity.com/drive)

<br>

<div align="center">
  <img width="640" img src="Assets/PIDController.gif" alt="Highway driving simulation video">
</div>

<br>

<p align="center">
  <b><i>The PID controller steers the wheel to follow a previously calculated path.</i></b>
</p>

<br/>

## Project Introduction
The goal of this project is to implement a PID controller so the car can safely complete a lap around a virtual track. The simulation provides the path that the car has to follow, as if the path finding process has already taken place. The path is invisible in the simulation, but as a rule of thumb the car should try to travel as close as possible to the center of the lane. The PID controller is the piece of software in charge of steering the wheel so that the car follows the pre-determined path.

## PID Controller
The PID controller has 3 components (line 25 in PID.cpp).

#### P (Proportional Controller):
The proportional controller produces a control effort proportional to the cross track error, which is equal to the distance of the car from the path (lines 19 and 25 in PID.cpp). This means that the car will steer towards the center of the lane with a steering angle proportional to how far the vehicle is from the path. If this component is too large, the trajectory of the car will overshoot and oscillate around the path. If this component is too small, the car will not be able to turn sharply enough to follow the path around corners.

#### D (Derivative Controller):
The derivative controller produces a control effort proportional to the change in cross track error (lines 18 and 25 in PID.cpp). This means that if the error is getting larger the car will steer more strongly towards the center of the lane, if the error is getting smaller the car will steer less and less towards the center of the lane. This dampens the oscillations produced by the proportional controller, allowing for an effective behavior in both turns and straightways.

#### I (Integral Controller):
The integral controller produces a control effort proportional to the integral of the cross track error over time (lines 20 and 25 in PID.cpp). This controller is added to compensate the mechanical defects of a system and minimise the steady state error. Having steady state error is like perfectly following the shape of a path but being always 1 meter to its right. This could be due to misalignment in the wheels. If this error keeps up, the integral component will grow with the passage of time and will eventually bring the car back to the path. 

## Tuning the PID Controller
All 3 components of the PID controller are multiplied by constants (kp, ki, and kd) to give more or less importance to the corresponding component (line 25 in PID.cpp). These constants can be found by hand by following this procedure:

* Set ki and kd to zero.
* Increase kp until the output oscillates.
* Set kp to about half the critical oscillating value.
* Increase ki until the steady state error is sufficiently reduced.
* Increase kd until overshoot and oscillations are eliminated. 

To avoid rebuilding every time a parameter is changed, the parameters can be passed to main.cpp. To do this, uncomment the input of main.cpp at line 39 together with lines 44, 46, and 48. Comment out lines 45, 47, and 49 and build. To run the code change directory to your build folder and run the command:

```shell
./pid 0.11 0.0 3.0
```

Where the 3 numbers are kp, ki, and kd respectively and can be changed to whatever you are experimenting.

The tuning process for this application followed this procedure, except ki has been left at zero because no mechanical system is involved and there is no steady state error to minimise.

the final values are:

```shell
kp = 0.11;
ki = 0.0;
kd = 3.0;
```

(lines 44-50 in main.cpp)

## Applying a Filter
The parameter kd found by the process described above is quite big (line 49 in main.cpp) and can some times generate big errors in the controller output. Therefore a filter has been applied to the control algorithm (lines 71-79 in main.cpp). If the steering angle undergoes a rapid change, the algorithm will wait 1 more cycle to check whether it is due to a glitch or to an actual turn in the path.

The need for a filter might underline a bigger bug in the program or in the simulation. Understanding or fixing the code of the simulation goes well beyond the purpose of this project.  


## Dependencies

* cmake >= 3.5
 * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1(mac, linux), 3.81(Windows)
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools]((https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)
* [uWebSockets](https://github.com/uWebSockets/uWebSockets)
  * Run either `./install-mac.sh` or `./install-ubuntu.sh`.
  * If you install from source, checkout to commit `e94b6e1`, i.e.
    ```
    git clone https://github.com/uWebSockets/uWebSockets 
    cd uWebSockets
    git checkout e94b6e1
    ```
    Some function signatures have changed in v0.14.x. See [this PR](https://github.com/udacity/CarND-MPC-Project/pull/3) for more details.
* Simulator. You can download these from the [project intro page](https://github.com/udacity/self-driving-car-sim/releases) in the classroom.

Fellow students have put together a guide to Windows set-up for the project [here](https://s3-us-west-1.amazonaws.com/udacity-selfdrivingcar/files/Kidnapped_Vehicle_Windows_Setup.pdf) if the environment you have set up for the Sensor Fusion projects does not work for this project. There's also an experimental patch for windows in this [PR](https://github.com/udacity/CarND-PID-Control-Project/pull/3).

## Basic Build Instructions

1. Clone this repo.
2. Make a build directory: `mkdir build && cd build`
3. Compile: `cmake .. && make`
4. Run it: `./pid`. 

Tips for setting up your environment can be found [here](https://classroom.udacity.com/nanodegrees/nd013/parts/40f38239-66b6-46ec-ae68-03afd8a601c8/modules/0949fca6-b379-42af-a919-ee50aa304e6a/lessons/f758c44c-5e40-4e01-93b5-1a82aa4e044f/concepts/23d376c7-0195-4276-bdf0-e02f1f3c665d)

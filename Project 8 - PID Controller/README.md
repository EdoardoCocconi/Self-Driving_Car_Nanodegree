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
The PID controller has 3 components.

#### P (Proportional Controller):
The proportional controller produces a control effort proportional to the cross track error (distance of the car from the path). This means that the car will steer towards the center of the lane with a steering angle proportional to how far the vehicle is from the path. If this component is too large, the trajectory of the car will overshoot and oscillate around the path. If this component is too small, the car will not be able to turn sharply enough to follow the path around corners.

#### D (Derivative Controller):
The derivative controller produces a control effort proportional to the change in cross track error. This means that if the error is getting larger the car will steer more strongly towards the center of the lane, if the error is getting smaller the car will steer less and less towards the center of the lane. This dampens the oscillations produced by the proportional controller, allowing for an effective behavior in both turns and straightways.

#### I (Integral Controller):
The integral controller produces a control effort proportional to the integral of the cross track error over time. This controller is added to compensate the mechanical defects of a system and minimise the steady state error. Having steady state error is like perfectly following the shape of a path but being always 1 meter to its right. This could be due to misalignment in the wheels. If this error keeps up, the integral component will grow with the passage of time and will eventually bring the car back to the path. 

## Tuning the PID Controller
All 3 components of the PID controller are multiplied by constants (kp, ki, and kd) to give more or less importance to the corresponding component. These constants can be found by hand by following this procedure:

* Set ki and kd to zero.
* Increase kp until the output oscillates.
* Set kp to about half the critical oscillating value.
* Increase ki until the steady state error is sufficiently reduced.
* Increase kd until overshoot and oscillations are eliminated. 

The tuning process for this application followed this procedure, except ki has been left at zero because no mechanical system is involved and there is no steady state error to minimise.

the final values are:

```shell
kp = 0.11;
ki = 0.0;
kd = 3.0;
```

## Applying a Filter
The parameter kd found by the process described above is quite big and can some times generate big errors in the controller output. Therefore a filter has been applied to the control algorithm. If the steering angle undergoes a rapid change, the algorithm will wait 1 more cycle to check whether it is due to a glitch or to an actual turn in the path.

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

## Editor Settings

We've purposefully kept editor configuration files out of this repo in order to
keep it as simple and environment agnostic as possible. However, we recommend
using the following settings:

* indent using spaces
* set tab width to 2 spaces (keeps the matrices in source code aligned)

## Project Instructions and Rubric

Note: regardless of the changes you make, your project must be buildable using
cmake and make!

More information is only accessible by people who are already enrolled in Term 2
of CarND. If you are enrolled, see [the project page](https://classroom.udacity.com/nanodegrees/nd013/parts/40f38239-66b6-46ec-ae68-03afd8a601c8/modules/f1820894-8322-4bb3-81aa-b26b3c6dcbaf/lessons/e8235395-22dd-4b87-88e0-d108c5e5bbf4/concepts/6a4d8d42-6a04-4aa6-b284-1697c0fd6562)
for instructions and the project rubric.

## Call for IDE Profiles Pull Requests

Help your fellow students!

We decided to create Makefiles with cmake to keep this project as platform
agnostic as possible. Similarly, we omitted IDE profiles in order to we ensure
that students don't feel pressured to use one IDE or another.

However! I'd love to help people get up and running with their IDEs of choice.
If you've created a profile for an IDE that you think other students would
appreciate, we'd love to have you add the requisite profile files and
instructions to ide_profiles/. For example if you wanted to add a VS Code
profile, you'd add:

* /ide_profiles/vscode/.vscode
* /ide_profiles/vscode/README.md

The README should explain what the profile does, how to take advantage of it,
and how to install it.

Frankly, I've never been involved in a project with multiple IDE profiles
before. I believe the best way to handle this would be to keep them out of the
repo root to avoid clutter. My expectation is that most profiles will include
instructions to copy files to a new location to get picked up by the IDE, but
that's just a guess.

One last note here: regardless of the IDE used, every submitted project must
still be compilable with cmake and make./
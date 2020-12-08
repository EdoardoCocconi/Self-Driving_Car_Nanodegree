#include <uWS/uWS.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "Eigen-3.3/Eigen/Core"
#include "Eigen-3.3/Eigen/QR"
#include "helpers.h"
#include "json.hpp"
#include "spline.h"

// for convenience
using nlohmann::json;
using std::string;
using std::vector;

// start in lane 1
int target_lane = 1;
// have a reference velocity to target
//bool sport = true;
bool sport = false;
double ref_vel = 0.0; // mph
double MAX_SPEED = 49.5; // mph
double ACCELERATION = 3.0; // m/s^2
double DECELERATION = 4.0; // m/s^2
auto begin = std::chrono::high_resolution_clock::now();
double dt;
double lanechange_timer;
double BRAKE_REACTION_TIME = 1.0; //  s
string state = "GO";
string previous_state = "GO";
double car_x;
double car_y;
double prev_car_x;
double prev_car_y;

int target_vehicle;
double target_speed; // m/s



int main() {

    if (sport) {
        MAX_SPEED = 120.0; // mph
        ACCELERATION = 4.0; // m/s^2
        DECELERATION = 8.0; // m/s^2
        BRAKE_REACTION_TIME = 0.0; // s
    }

    uWS::Hub h;

    // Load up map values for waypoint's x,y,s and d normalized normal vectors
    vector<double> map_waypoints_x;
    vector<double> map_waypoints_y;
    vector<double> map_waypoints_s;
    vector<double> map_waypoints_dx;
    vector<double> map_waypoints_dy;

    // Waypoint map to read from
    string map_file_ = "../data/highway_map.csv";
    // The max s value before wrapping around the track back to 0
    double max_s = 6945.554;

    std::ifstream in_map_(map_file_.c_str(), std::ifstream::in);

    string line;
    while (getline(in_map_, line)) {
        std::istringstream iss(line);
        double x;
        double y;
        float s;
        float d_x;
        float d_y;
        iss >> x;
        iss >> y;
        iss >> s;
        iss >> d_x;
        iss >> d_y;
        map_waypoints_x.push_back(x);
        map_waypoints_y.push_back(y);
        map_waypoints_s.push_back(s);
        map_waypoints_dx.push_back(d_x);
        map_waypoints_dy.push_back(d_y);
    }

    h.onMessage([&map_waypoints_x, &map_waypoints_y, &map_waypoints_s,
                        &map_waypoints_dx, &map_waypoints_dy]
                        (uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length,
                         uWS::OpCode opCode) {
                    // "42" at the start of the message means there's a websocket message event.
                    // The 4 signifies a websocket message
                    // The 2 signifies a websocket event
                    if (length && length > 2 && data[0] == '4' && data[1] == '2') {

                        auto s = hasData(data);

                        if (s != "") {
                            auto j = json::parse(s);

                            string event = j[0].get<string>();

                            if (event == "telemetry") {
                                // j[1] is the data JSON object

                                // Main car's localization Data
                                prev_car_x = car_x;
                                car_x = j[1]["x"];
                                prev_car_y = car_y;
                                car_y = j[1]["y"];
                                double car_s = j[1]["s"];
                                double car_d = j[1]["d"];
                                int current_lane = (int) (car_d / 4.0);
                                double car_yaw = j[1]["yaw"];
                                double car_speed = j[1]["speed"];
                                double car_speed_meters = car_speed / 2.24;

                                // Previous path data given to the Planner
                                auto previous_path_x = j[1]["previous_path_x"];
                                auto previous_path_y = j[1]["previous_path_y"];
                                // Previous path's end s and d values
                                double end_path_s = j[1]["end_path_s"];
                                double end_path_d = j[1]["end_path_d"];

                                // Sensor Fusion Data, a list of all other cars on the same side
                                //   of the road.
                                auto sensor_fusion = j[1]["sensor_fusion"];

                                json msgJson;

                                auto end = std::chrono::high_resolution_clock::now();
                                auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
                                dt = std::chrono::duration<double>(elapsed).count();
                                if (ref_vel == 0.0) {
                                    dt = 0.02;
                                }
                                begin = std::chrono::high_resolution_clock::now();
                                lanechange_timer += dt;

                                int prev_size = previous_path_x.size();
                                double obstacle_distance = HUGE_VAL;
                                vector<bool> side_obstacles = {false, false, false};
                                vector<double> lanes_obstacles = {HUGE_VAL - 1, HUGE_VAL - 1, HUGE_VAL - 1};
                                lanes_obstacles[target_lane] += 1;

                                for (int i = 0; i < sensor_fusion.size(); ++i) {
                                    double vx = sensor_fusion[i][3]; // m/s
                                    double vy = sensor_fusion[i][4]; // m/s
                                    double check_speed = sqrt(vx * vx + vy * vy); // m/s
                                    double check_car_s = sensor_fusion[i][5];
                                    check_car_s += (end_path_s - car_s) / car_speed_meters * check_speed;
                                    double s_predicted = check_car_s + (check_car_s - end_path_s) / car_speed_meters * check_speed;
                                    float d = sensor_fusion[i][6];
                                    int check_lane = (int) (d / 4.0);

                                    double safety_distance = (check_speed / 3.5) * (check_speed / 3.5);
                                    double back_safety_distance = std::max(6.0, 6.0 + check_speed - car_speed_meters);

                                    if (check_car_s > end_path_s - back_safety_distance && check_car_s < car_s + safety_distance + car_speed_meters - check_speed) {
                                        if (check_lane == 0) {
                                            side_obstacles[0] = true;
                                        } else if (check_lane == 1) {
                                            side_obstacles[1] = true;
                                        } else if (check_lane == 2) {
                                            side_obstacles[2] = true;
                                        }
                                    }

                                    if (check_car_s > end_path_s - back_safety_distance) {
                                        if (check_lane == 0 && s_predicted < lanes_obstacles[0]) {
                                            lanes_obstacles[0] = check_car_s;
                                        } else if (check_lane == 1 && s_predicted < lanes_obstacles[1]) {
                                            lanes_obstacles[1] = check_car_s;
                                        } else if (check_lane == 2 && s_predicted < lanes_obstacles[2]) {
                                            lanes_obstacles[2] = check_car_s;
                                        }
                                    }

                                    // If the car is in my lane
                                    if ((d < (2 + 4 * target_lane + 2)) && (d > (2 + 4 * target_lane - 2))) {
                                        // Check s values greater than mine and s gap
                                        double time = (car_speed_meters - check_speed) / DECELERATION;
                                        double braking_distance = car_speed_meters * time - check_speed * time - 0.5 * DECELERATION * time * time + (end_path_s - car_s) + safety_distance;
                                        if ((check_car_s > car_s) && (check_car_s - car_s) < obstacle_distance) {
                                            obstacle_distance = check_car_s - car_s;
                                            target_speed = check_speed;
                                            target_vehicle = i;
                                            if ((check_car_s - car_s) < braking_distance && state == "GO") {
                                                previous_state = state;
                                                state = "BRAKE";
                                            }
                                        }
                                    }
                                }


                                int best_lane;
                                double max = 0;

                                for (int i = 0; i < lanes_obstacles.size(); ++i) {
                                    if (lanes_obstacles[i] > max) {
                                        best_lane = i;
                                        max = lanes_obstacles[i];
                                    }
                                }


                                if (state == "BRAKE") {

//                                    std::cout << "***** " << state << " *****" << "\n";
                                    if (ref_vel < target_speed * 2.24) {
                                        previous_state = state;
                                        state = "FOLLOW";
                                    }
                                    if (best_lane != target_lane && lanechange_timer > BRAKE_REACTION_TIME) {
                                        previous_state = state;
                                        state = "CHANGE LANE";
                                    }
                                    float d_target = sensor_fusion[target_vehicle][6];
                                    int target_vehicle_lane = (int) (d_target / 4.0);
                                    if (target_vehicle_lane != target_lane) {
                                        previous_state = state;
                                        state = "GO";
                                    }
                                    if (sensor_fusion[target_vehicle][5] < car_s) {
                                        previous_state = state;
                                        state = "GO";
                                    }

                                    ref_vel -= dt * 2.24 * DECELERATION;

                                } else if (state == "GO") {

//                                    std::cout << "***** " << state << " *****" << "\n";
                                    if (best_lane != target_lane && lanechange_timer > 4.0) {
                                        previous_state = state;
                                        state = "CHANGE LANE";
                                    }
                                    ref_vel += dt * 2.24 * ACCELERATION;
                                    if (ref_vel > MAX_SPEED) {
                                        ref_vel = MAX_SPEED;
                                    }

                                }

                                if (state == "FOLLOW") {

//                                    std::cout << "***** " << state << " vehicle " << target_vehicle << " *****" << "\n";
                                    double target_s = sensor_fusion[target_vehicle][5];
                                    if (dt * std::abs(ref_vel - target_speed * 2.24) <= dt * 2.24 * DECELERATION) {
                                        if (target_s - car_s  == (target_speed / 3.5) * (target_speed / 3.5)){
                                            ref_vel = target_speed * 2.24;
                                        } else if (target_s - car_s > (target_speed / 3.5) * (target_speed / 3.5)){
                                            ref_vel = target_speed * 2.24 + 0.1 * dt;
                                        } else {
                                            ref_vel = target_speed * 2.24 - 0.1 * dt;
                                        }
                                    } else {
                                        previous_state = state;
                                        state = "GO";
                                    }
                                    float d_target = sensor_fusion[target_vehicle][6];
                                    int target_vehicle_lane = (int) (d_target / 4.0);
                                    if (target_vehicle_lane != target_lane) {
                                        previous_state = state;
                                        state = "GO";
                                    }
                                    if (target_s < car_s) {
                                        previous_state = state;
                                        state = "GO";
                                    }
                                    if (best_lane != target_lane && lanechange_timer > 4.0) {
                                        previous_state = state;
                                        state = "CHANGE LANE";
                                    }

                                }

                                if (state == "CHANGE LANE") {

//                                    std::cout << "***** " << state << " *****" << "\n";
                                    lanechange_timer = 0.0;
                                    state = previous_state;
                                    if (target_lane < best_lane) {
                                        if (!side_obstacles[target_lane + 1]) {
                                            ++target_lane;
                                            state = "GO";
                                        }
                                    } else if (target_lane > best_lane) {
                                        if (!side_obstacles[target_lane - 1]) {
                                            --target_lane;
                                            state = "GO";
                                        }
                                    }

                                }

//                                std::cout << "========== CYCLE ==========" << "\n";


//                                REFERENCE: the following spline implementation has been done by following the Udacity Q&A at the link
//                                https://www.youtube.com/watch?v=7sI3VHFPP0w&feature=emb_logo&ab_channel=Udacity


                                // Create a list of widely spaced (x, y) waypoints, evenly spaced at 30m
                                // Later we will interpolate these waypoints with a spline and fill it in with more points that control the speed of the vehicle
                                vector<double> ptsx;
                                vector<double> ptsy;

                                // reference x, y, yaw states
                                // either we will reference the starting point as where the car is or at the previous path end point
                                double ref_x = car_x;
                                double ref_y = car_y;
                                double ref_yaw = deg2rad(car_yaw);

                                // if previous size is almost empty, use the car as starting reference
                                if (prev_size < 2) {
                                    // Use the two points that make the path tangent to the car
                                    ptsx.push_back(prev_car_x);
                                    ptsx.push_back(car_x);
                                    ptsy.push_back(prev_car_y);
                                    ptsy.push_back(car_y);

                                } else { // Use the previous path's end point as starting reference
                                    // Redefine reference state as previous path end point
                                    ref_x = previous_path_x[prev_size - 1];
                                    ref_y = previous_path_y[prev_size - 1];

                                    double ref_x_prev = previous_path_x[prev_size - 2];
                                    double ref_y_prev = previous_path_y[prev_size - 2];
                                    ref_yaw = atan2(ref_y - ref_y_prev, ref_x - ref_x_prev);

                                    // Use two points that make the path tangent to the previous path's end point
                                    ptsx.push_back(ref_x_prev);
                                    ptsx.push_back(ref_x);
                                    ptsy.push_back(ref_y_prev);
                                    ptsy.push_back(ref_y);

                                }

                                if (prev_size > 0) {
                                    car_s = end_path_s;
                                }
                                // In Frenet add evenly 30m spaced points ahead of the stating reference
                                vector<double> next_wp0 = getXY(car_s + 30, 2 + 4 * target_lane, map_waypoints_s, map_waypoints_x, map_waypoints_y);
                                vector<double> next_wp1 = getXY(car_s + 60, 2 + 4 * target_lane, map_waypoints_s, map_waypoints_x, map_waypoints_y);
                                vector<double> next_wp2 = getXY(car_s + 90, 2 + 4 * target_lane, map_waypoints_s, map_waypoints_x, map_waypoints_y);

                                ptsx.push_back(next_wp0[0]);
                                ptsx.push_back(next_wp1[0]);
                                ptsx.push_back(next_wp2[0]);

                                ptsy.push_back(next_wp0[1]);
                                ptsy.push_back(next_wp1[1]);
                                ptsy.push_back(next_wp2[1]);

                                for (int i = 0; i < ptsx.size(); ++i) {
                                    // shift car reference angle to 0 degres
                                    double shift_x = ptsx[i] - ref_x;
                                    double shift_y = ptsy[i] - ref_y;

                                    ptsx[i] = (shift_x * cos(0 - ref_yaw) - shift_y * sin(0 - ref_yaw));
                                    ptsy[i] = (shift_x * sin(0 - ref_yaw) + shift_y * cos(0 - ref_yaw));

                                }

                                // create a spline
                                tk::spline s;

                                // set (x, y) points to the spline
                                s.set_points(ptsx, ptsy);

                                // Define the actual (x,y) points we will use for the planner
                                vector<double> next_x_vals;
                                vector<double> next_y_vals;

                                // Start with all of the previous path points from last time
                                for (int i = 0; i < previous_path_x.size(); ++i) {
                                    next_x_vals.push_back(previous_path_x[i]);
                                    next_y_vals.push_back(previous_path_y[i]);
                                }

                                // Calculate how to break up spline points so that we travel at our desired reference velocity
                                double target_x = 30.0;
                                double target_y = s(target_x);
                                double target_dist = sqrt(target_x * target_x + target_y + target_y);

                                double x_add_on = 0;

                                // Fill up the rest of our path  planner after filling it with previous points, here  we will always output 50 points
                                for (int i = 0; i < 15 - previous_path_x.size(); ++i) {
                                    double N = (target_dist / (0.02 * ref_vel / 2.24));
                                    double x_point = x_add_on + target_x / N;
                                    double y_point = s(x_point);

                                    x_add_on = x_point;

                                    double x_ref = x_point;
                                    double y_ref = y_point;

                                    // rotate back to normal after rotating it earlier
                                    x_point = (x_ref * cos(ref_yaw) - y_ref * sin(ref_yaw));
                                    y_point = (x_ref * sin(ref_yaw) + y_ref * cos(ref_yaw));

                                    x_point += ref_x;
                                    y_point += ref_y;

                                    next_x_vals.push_back(x_point);
                                    next_y_vals.push_back(y_point);

                                }

                                msgJson["next_x"] = next_x_vals;
                                msgJson["next_y"] = next_y_vals;

                                auto msg = "42[\"control\"," + msgJson.dump() + "]";

                                ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
                            }  // end "telemetry" if
                        } else {
                            // Manual driving
                            std::string msg = "42[\"manual\",{}]";
                            ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
                        }
                    }  // end websocket if
                }

    ); // end h.onMessage

    h.onConnection([&h](
            uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest
    req) {
        std::cout << "Connected!!!" <<
                  std::endl;
    });

    h.onDisconnection([&h](
            uWS::WebSocket<uWS::SERVER> ws,
            int code,
            char *message, size_t
            length) {
        ws.

                close();

        std::cout << "Disconnected" <<
                  std::endl;
    });

    int port = 4567;
    if (h.
            listen(port)
            ) {
        std::cout << "Listening to port " << port <<
                  std::endl;
    } else {
        std::cerr << "Failed to listen to port" <<
                  std::endl;
        return -1;
    }

    h.

            run();

}
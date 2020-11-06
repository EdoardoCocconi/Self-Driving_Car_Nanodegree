#include <random>
#include <algorithm>
#include <iostream>
#include <iterator>

#include "particle_filter.h"

using namespace std;


// Module explained at Udacity Q&A: https://www.youtube.com/watch?v=-3HI3Iw3Z9g&feature=youtu.be&ab_channel=Udacity
void ParticleFilter::init(double gps_x, double gps_y, double gps_theta, double std[]) {
    /**
     * Set the number of particles. Initialize all particles to
     * first position (based on estimates of gps_x, gps_y, gps_theta and their uncertainties
     * from GPS) and all weights to 1.
     * Add random Gaussian noise to each particle.
     */
    num_particles = 80;  // Set the number of particles

    std::default_random_engine gen;

    std::normal_distribution<double> distribution_x(gps_x, std[0]);
    std::normal_distribution<double> distribution_y(gps_y, std[1]);
    std::normal_distribution<double> distribution_theta(gps_theta, std[2]);

    for (int i = 0; i < num_particles; i++) {
        Particle particle;
        particle.id = i;
        particle.x = distribution_x(gen);
        particle.y = distribution_y(gen);
        particle.theta = distribution_theta(gen);
        particle.weight = 1;
        particles.push_back(particle);
    }

    is_initialized = true;

}


// Module explained at Udacity Q&A: https://www.youtube.com/watch?v=-3HI3Iw3Z9g&feature=youtu.be&ab_channel=Udacity
void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {

    // Add measurements to each particle and add random Gaussian noise.

    std::default_random_engine gen;
    double new_x, new_y, new_theta;

    for (Particle &particle : particles) {
        if (yaw_rate == 0) {
            new_x = particle.x + velocity * delta_t * cos(particle.theta);
            new_y = particle.y + velocity * delta_t * sin(particle.theta);
            new_theta = particle.theta;
        } else {
            new_x = particle.x + velocity / yaw_rate * (sin(particle.theta + yaw_rate * delta_t) - sin(particle.theta));
            new_y = particle.y + velocity / yaw_rate * (cos(particle.theta) - cos(particle.theta + yaw_rate * delta_t));
            new_theta = particle.theta + yaw_rate * delta_t;
        }

        std::normal_distribution<double> distribution_x(new_x, std_pos[0]);
        std::normal_distribution<double> distribution_y(new_y, std_pos[1]);
        std::normal_distribution<double> distribution_theta(new_theta, std_pos[2]);

        particle.x = distribution_x(gen);
        particle.y = distribution_y(gen);
        particle.theta = distribution_theta(gen);
    }

}



void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predictions, std::vector<LandmarkObs> &observations) {

    /**
     * Find the predicted measurement that is closest to each
     * observed measurement and assign the observed measurement to this
     * particular landmark.
     */

    double distance;
    double distance_min;

    for (LandmarkObs &observation : observations) {
        distance_min = HUGE_VAL;
        for (LandmarkObs &prediction : predictions) {
            distance = dist(observation.x, observation.y, prediction.x, prediction.y);
            if (distance < distance_min) {
                observation.id = prediction.id;
                distance_min = distance;
            }
        }
    }

}



void ParticleFilter::updateWeights(double sensor_range, double std_landmark[],
                                   std::vector<LandmarkObs> observations, Map map_landmarks) {

    /**
     * Update the weights of each particle using a multi-variate Gaussian
     * distribution. You can read more about this distribution here:
     * https://en.wikipedia.org/wiki/Multivariate_normal_distribution
     * NOTE: The observations are given in the VEHICLE'S coordinate system.
     *   Particles are located according to the MAP'S coordinate system.
     *   Coordinates are therefore transformed between the two systems.
     *   The following is a good resource for the theory:
     *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
     *   and the following is a good resource for the actual equation to implement
     *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
     */

    weights.clear();

    for (Particle &particle : particles) {
        // What landmarks would be seen if the position of the particle was correct?
        // In other words: What landmarks are predicted to be seen?
        vector<LandmarkObs> predictions;

        for (Map::single_landmark_s &landmark : map_landmarks.landmark_list) {
            double distance = dist(particle.x, particle.y, landmark.x_f, landmark.y_f);
            if (distance < sensor_range) {
                predictions.push_back(LandmarkObs{landmark.id_i, landmark.x_f, landmark.y_f});
            }
        }

        // Converting observations (sensor readings) from vehicle coordinates to map coordinates
        vector<LandmarkObs> map_observations;
        LandmarkObs map_observation;

        for (LandmarkObs &observation : observations) {
            map_observation.x = observation.x * cos(particle.theta) - observation.y * sin(particle.theta) + particle.x;
            map_observation.y = observation.x * sin(particle.theta) + observation.y * cos(particle.theta) + particle.y;
            map_observations.push_back(map_observation);
        }

        // Pairing what is predicted with what is actually observed to assess the accuracy of the prediction
        dataAssociation(predictions, map_observations);

        std::vector<int> associations;
        std::vector<double> sense_x;
        std::vector<double> sense_y;

        for (LandmarkObs &map_observation : map_observations) {
            associations.push_back(map_observation.id);
            sense_x.push_back(map_observation.x);
            sense_y.push_back(map_observation.y);
        }

        SetAssociations(particle, associations, sense_x, sense_y);

        // Calculate the new weight of the particle based on how well its predictions match each sensor observation.
        particle.weight = 1.0; // set to 1 so at the first multiplication particle.weight becomes equal to w

        for (LandmarkObs &map_observation : map_observations) {
            Map::single_landmark_s landmark = map_landmarks.landmark_list[map_observation.id - 1]; // ids start from 1, not 0
            // The following formula is used to calculate the new weight of the particle based on how well its predictions match with the sensor readings
            // The formula is explained in the Self-Driving Car Nanodegree by Udacity
            double w = exp(-(pow(map_observation.x - landmark.x_f, 2) / (2 * pow(std_landmark[0], 2)) + pow(map_observation.y - landmark.y_f, 2) / (2 * pow(std_landmark[1], 2)))) / (2 * M_PI * std_landmark[0] * std_landmark[1]);
            particle.weight *= w;
        }

        weights.push_back(particle.weight);

    }

}



void ParticleFilter::resample() {

    /**
     * Resample particles with replacement with probability proportional
     * to their weight.
     * Discrete distribution has been implemented as shown at the following link:
     * http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
     */

    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<> distribution(weights.begin(), weights.end());

    vector<Particle> drawn_particles;

    for (int i = 0; i < num_particles; i++) {
        int random_index = distribution(gen);
        drawn_particles.push_back(particles[random_index]);
    }

    particles = drawn_particles;

}



void ParticleFilter::SetAssociations(Particle &particle, std::vector<int> &associations, std::vector<double> &sense_x,
                                         std::vector<double> &sense_y) {

    /* particle: the particle to assign association's (x,y) world coordinates mapping to
     * associations: The landmark id that goes along with each listed association
     * sense_x: the associations x mapping already converted each listed association, and  to world coordinates
     * sense_y: the associations y mapping already converted to world coordinates
     */

    particle.associations.clear();
    particle.sense_x.clear();
    particle.sense_y.clear();

    particle.associations = associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;

}

string ParticleFilter::getAssociations(Particle best) {
    vector<int> v = best.associations;
    stringstream ss;
    copy(v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length() - 1);  // get rid of the trailing space
    return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
    vector<double> v;

    if (coord == "X") {
        v = best.sense_x;
    } else {
        v = best.sense_y;
    }

    std::stringstream ss;
    copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length() - 1);  // get rid of the trailing space
    return s;
}

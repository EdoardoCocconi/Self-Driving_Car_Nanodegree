#include "kalman_filter.h"

#include <cmath>

using Eigen::MatrixXd;
using Eigen::VectorXd;

/* 
 * Please note that the Eigen library does not initialize 
 *   VectorXd or MatrixXd objects with zeros upon creation.
 */

KalmanFilter::KalmanFilter() {}

KalmanFilter::~KalmanFilter() {}

void KalmanFilter::Predict() {
    // predict the state
    x_ = F_ * x_;
    P_ = F_ * P_ * F_.transpose() + Q_;
}

void KalmanFilter::Update(const VectorXd &z) {
    // update the state by using Kalman Filter equations
    MatrixXd y = z - H_ * x_;
    MatrixXd Ht = H_.transpose();
    MatrixXd S = H_ * P_ * Ht + R_;
    MatrixXd K = P_ * Ht * S.inverse();

    // new state
    x_ = x_ + (K * y);
    long x_size = x_.size();
    MatrixXd I = MatrixXd::Identity(x_size, x_size);
    P_ = (I - K * H_) * P_;
}

void KalmanFilter::UpdateEKF(const VectorXd &z) {
    // update the state by using Extended Kalman Filter equations
    float xread = x_(0);
    float yread = x_(1);
    float vx = x_(2);
    float vy = x_(3);

    float rho = std::sqrt(xread * xread + yread * yread);
    float theta = std::atan2(yread, xread);
    float rho_dot = (xread * vx + yread * vy) / rho;

    VectorXd z_pred = VectorXd(3);
    z_pred << rho, theta, rho_dot;

    VectorXd y = z - z_pred;

    //
    while(y(1) > M_PI){
        y(1) -= 2 * M_PI;
    }

    while(y(1) < -M_PI){
        y(1) += 2 * M_PI;
    }

    MatrixXd Ht = H_.transpose();
    MatrixXd S = H_ * P_ * Ht + R_;
    MatrixXd K = P_ * Ht * S.inverse();

    // new state
    x_ = x_ + (K * y);
    long x_size = x_.size();
    MatrixXd I = MatrixXd::Identity(x_size, x_size);
    P_ = (I - K * H_) * P_;
}

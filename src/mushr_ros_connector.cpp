#include "mushr_ros_connector.h"
#include "body_ros_connector.h"
#include "mjglobal.h"
#include "mushr_mujoco_util.h"

namespace mushr_mujoco_ros {

MuSHRROSConnector::MuSHRROSConnector(ros::NodeHandle* nh, const YAML::Node& e)
  : BodyROSConnector{nh, e}
{
    // check if the car body exists
    mjModel* m = mjglobal::mjmodel();
    velocity_ = 0.0;
    steering_angle_ = 0.0;

    std::string control_topic = "control";
    if (e["control_topic"])
    {
        control_topic = e["control_topic"].as<std::string>();
    }
    control_sub_ = nh_->subscribe(
        pvt_name(control_topic), 1, &MuSHRROSConnector::control_cb, this);

    std::string tv = car_ref("throttle_velocity");
    std::string sp = car_ref("steering_pos");

    velocity_ctrl_idx_ = mushr_mujoco_util::mj_name2id_ordie(m, mjOBJ_ACTUATOR, tv);
    steering_angle_ctrl_idx_
        = mushr_mujoco_util::mj_name2id_ordie(m, mjOBJ_ACTUATOR, sp);

    init_sensors();
}

void MuSHRROSConnector::send_state()
{
    BodyROSConnector::send_state();
    send_sensor_state();
}

void MuSHRROSConnector::set_body_state(mushr_mujoco_ros::BodyState& bs)
{
    BodyROSConnector::set_body_state(bs);

    bs.ctrl_steering_angle = steering_angle_;
    bs.ctrl_velocity = velocity_;
    mjData* d = mjglobal::mjdata_lock();
    get_gyro(d, bs.imu);
    get_velocimeter(d, bs.velocity);
    mjglobal::mjdata_unlock();
}

void MuSHRROSConnector::mujoco_controller()
{
    mjData* d = mjglobal::mjdata_lock();

    d->ctrl[velocity_ctrl_idx_] = velocity_;
    d->ctrl[steering_angle_ctrl_idx_] = steering_angle_;

    mjglobal::mjdata_unlock();
}

void MuSHRROSConnector::control_cb(
    const ackermann_msgs::AckermannDriveStampedConstPtr& msg)
{
    steering_angle_ = msg->drive.steering_angle;
    velocity_ = msg->drive.speed;
}

std::string MuSHRROSConnector::car_ref(const std::string name)
{
    return body_name_ + "_" + name;
}

} // namespace mushr_mujoco_ros

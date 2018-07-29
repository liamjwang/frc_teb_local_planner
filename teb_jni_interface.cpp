// Liam Wang
#include <teb_local_planner/planner_interface.h>
#include <teb_local_planner/optimal_planner.h>
#include <fake_geometry.h>
#include "teb_jni_interface.h"
#include "include/teb_local_planner/pose_se2.h"
#include "teb-java-example/headers/TebInterface.h"

teb_local_planner::PoseSE2 poseSE2FromJobject(JNIEnv *env, const jobject &obj) {
    jclass PoseSE2Class = env->GetObjectClass(obj);
    teb_local_planner::PoseSE2 pose(
            env->GetDoubleField(obj, env->GetFieldID(PoseSE2Class, "x", "D")),
            env->GetDoubleField(obj, env->GetFieldID(PoseSE2Class, "y", "D")),
            env->GetDoubleField(obj, env->GetFieldID(PoseSE2Class, "theta", "D"))
    );
    return pose;
}

void saturateVelocity(double &vx, double &vy, double &omega, double max_vel_x, double max_vel_y, double max_vel_theta, double max_vel_x_backwards) {
    // Limit translational velocity for forward driving
    if (vx > max_vel_x)
        vx = max_vel_x;

    // limit strafing velocity
    if (vy > max_vel_y)
        vy = max_vel_y;
    else if (vy < -max_vel_y)
        vy = -max_vel_y;

    // Limit angular velocity
    if (omega > max_vel_theta)
        omega = max_vel_theta;
    else if (omega < -max_vel_theta)
        omega = -max_vel_theta;

    // Limit backwards velocity
    if (max_vel_x_backwards <= 0) {
        std::cout << "TebLocalPlannerROS(): Do not choose max_vel_x_backwards to be <=0. Disable backwards driving by increasing the optimization weight for penalyzing backwards driving." << std::endl;
    } else if (vx < -max_vel_x_backwards)
        vx = -max_vel_x_backwards;
}

teb_local_planner::PlannerInterfacePtr planner_;
teb_local_planner::TebConfig cfg_;
teb_local_planner::TebVisualizationPtr visual_;

JNIEXPORT void JNICALL Java_TebInterface_initialize(JNIEnv *, jclass, jobject) {
    teb_local_planner::ObstContainer obstacles_{};
    teb_local_planner::RobotFootprintModelPtr robot_model = boost::make_shared<teb_local_planner::PointRobotFootprint>();
    teb_local_planner::ViaPointContainer via_points_{};

    planner_ = teb_local_planner::PlannerInterfacePtr(new teb_local_planner::TebOptimalPlanner(cfg_, &obstacles_, robot_model, visual_, &via_points_));
}

JNIEXPORT jobject JNICALL
Java_TebInterface_plan(JNIEnv *env, jclass, jobject jstart, jobject jgoal, jobject jstart_vel, jboolean free_goal_vel) {
    auto start = poseSE2FromJobject(env, jstart);
    std::cout << "Start Pose: " << start << std::endl;
    auto goal = poseSE2FromJobject(env, jgoal);
    std::cout << "End Pose: " << goal << std::endl;
    auto start_vel_pose = poseSE2FromJobject(env, jstart_vel);
    std::cout << "Start Vel: " << start_vel_pose << std::endl;
    std::cout << "Free Goal Vel: " << (static_cast<bool>(free_goal_vel) ? "True" : "False") << std::endl;

    fake_geometry_msgs::Twist start_vel{};
    start_vel.linear.x = start_vel_pose.x();
    start_vel.linear.y = start_vel_pose.y();
    start_vel.angular.z = start_vel_pose.theta();

    bool success = planner_->plan(start, goal, &start_vel, free_goal_vel);
//    if (!success) {
//        planner_->clearPlanner(); // force reinitialization for next time
//        std::cout << "teb_local_planner was not able to obtain a local plan for the current setting." << std::endl;
//    }

    double x,y,theta;
    planner_->getVelocityCommand(x, y, theta);

//    saturateVelocity(x, y, theta,
//            cfg_.robot.max_vel_x,
//            cfg_.robot.max_vel_y,
//            cfg_.robot.max_vel_theta,
//            cfg_.robot.max_vel_x_backwards);

    jclass poseSE2Class = env->FindClass("LPoseSE2;");
    jmethodID poseSE2Constructor = env->GetMethodID(poseSE2Class, "<init>", "(DDD)V");
    jobject cmd_vel = env->NewObject(poseSE2Class, poseSE2Constructor, x, y, theta);
    return cmd_vel;
}

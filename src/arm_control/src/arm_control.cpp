#include <algorithm>
#include <map>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"

#include "sensor_msgs/msg/joy.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

#include "interbotix_xs_msgs/msg/joint_single_command.hpp"

struct InputMapping {
    std::string type;
    int index;
    double threshold{0.5};
};

struct JointMapping {
    InputMapping positive;
    InputMapping negative;
};

class ArmController : public rclcpp::Node {
    public:
        ArmController() : Node("arm_controller")
        {
            // Parameters
            declare_parameter("default_joint_step", 0.07);
            declare_parameter("step_increment", 0.01);
            declare_parameter("min_joint_step", 0.005);
            declare_parameter("max_joint_step", 0.10);

            // Waist
            declare_parameter("joints.waist.positive.type", "axis");
            declare_parameter("joints.waist.positive.index", 6);
            declare_parameter("joints.waist.positive.threshold", 0.5);

            declare_parameter("joints.waist.negative.type", "axis");
            declare_parameter("joints.waist.negative.index", 6);
            declare_parameter("joints.waist.negative.threshold", -0.5);

            // Shoulder
            declare_parameter("joints.shoulder.positive.type", "button");
            declare_parameter("joints.shoulder.positive.index", 4);

            declare_parameter("joints.shoulder.negative.type", "axis");
            declare_parameter("joints.shoulder.negative.index", 2);
            declare_parameter("joints.shoulder.negative.threshold", -0.5);

            // Elbow
            declare_parameter("joints.elbow.positive.type", "button");
            declare_parameter("joints.elbow.positive.index", 5);

            declare_parameter("joints.elbow.negative.type", "axis");
            declare_parameter("joints.elbow.negative.index", 5);
            declare_parameter("joints.elbow.negative.threshold", -0.5);

            // Wrist
            declare_parameter("joints.wrist_angle.positive.type", "button");
            declare_parameter("joints.wrist_angle.positive.index", 3);

            declare_parameter("joints.wrist_angle.negative.type", "button");
            declare_parameter("joints.wrist_angle.negative.index", 0);

            // Gripper
            declare_parameter("gripper.open.index", 2);
            declare_parameter("gripper.close.index", 1);

            // Step
            declare_parameter("step_control.increase.index", 7);
            declare_parameter("step_control.increase.threshold", 0.5);

            declare_parameter("step_control.decrease.index", 7);
            declare_parameter("step_control.decrease.threshold", -0.5);

            joint_step_ = get_parameter("default_joint_step").as_double();

            step_increment_ = get_parameter("step_increment").as_double();

            min_step_ = get_parameter("min_joint_step").as_double();

            max_step_ = get_parameter("max_joint_step").as_double();

            joint_mappings_["waist"] = {{get_parameter("joints.waist.positive.type").as_string(),
                                        static_cast<int>(
                                            get_parameter("joints.waist.positive.index").as_int()),
                                        get_parameter("joints.waist.positive.threshold").as_double()},
                                        {get_parameter("joints.waist.negative.type").as_string(),
                                        static_cast<int>(
                                            get_parameter("joints.waist.negative.index").as_int()),
                                        get_parameter("joints.waist.negative.threshold").as_double()}};

            joint_mappings_["shoulder"] = {
                {get_parameter("joints.shoulder.positive.type").as_string(),
                static_cast<int>(
                    get_parameter("joints.shoulder.positive.index").as_int())},
                {get_parameter("joints.shoulder.negative.type").as_string(),
                static_cast<int>(
                    get_parameter("joints.shoulder.negative.index").as_int()),
                get_parameter("joints.shoulder.negative.threshold").as_double()}};

            joint_mappings_["elbow"] = {
                {get_parameter("joints.elbow.positive.type").as_string(),
                static_cast<int>(
                    get_parameter("joints.elbow.positive.index").as_int())},
                {get_parameter("joints.elbow.negative.type").as_string(),
                static_cast<int>(
                    get_parameter("joints.elbow.negative.index").as_int()),
                get_parameter("joints.elbow.negative.threshold").as_double()}};

            joint_mappings_["wrist_angle"] = {
                {get_parameter("joints.wrist_angle.positive.type").as_string(),
                static_cast<int>(
                    get_parameter("joints.wrist_angle.positive.index").as_int())},
                {get_parameter("joints.wrist_angle.negative.type").as_string(),
                static_cast<int>(
                    get_parameter("joints.wrist_angle.negative.index").as_int())}};

            gripper_open_ = {
                "button",
                static_cast<int>(
                    get_parameter("gripper.open.index").as_int())};

            gripper_close_ = {
                "button",
                static_cast<int>(
                    get_parameter("gripper.close.index").as_int())};

            step_increase_ = {
                "axis",
                static_cast<int>(
                    get_parameter("step_control.increase.index").as_int()),
                get_parameter("step_control.increase.threshold").as_double()};

            step_decrease_ = {
                "axis",
                static_cast<int>(
                    get_parameter("step_control.decrease.index").as_int()),
                get_parameter("step_control.decrease.threshold").as_double()};

            // Publishers
            joint_pub_ = create_publisher<interbotix_xs_msgs::msg::JointSingleCommand>(
                "commands/joint_single", 10);

            // Subscribers
            joy_sub_ = create_subscription<sensor_msgs::msg::Joy>(
                "commands/joy_raw",
                10,
                std::bind(&ArmController::joy_callback,
                        this,
                        std::placeholders::_1));

            joint_state_sub_ = create_subscription<sensor_msgs::msg::JointState>(
                "joint_states",
                10,
                std::bind(&ArmController::joint_state_callback,
                        this,
                        std::placeholders::_1));

            // Timer
            jog_timer_ = create_wall_timer(
                std::chrono::milliseconds(50), // 20Hz
                std::bind(
                    &ArmController::jog_timer_callback,
                    this));
        }

        ~ArmController() {
            RCLCPP_INFO(get_logger(), "Arm controller shutting down");
            for (const auto &[joint_name, position] : joint_positions_) {
                if (joint_name == "gripper")
                    continue;

                interbotix_xs_msgs::msg::JointSingleCommand cmd;
                cmd.name = joint_name;
                cmd.cmd = position;

                joint_pub_->publish(cmd);
            }
        }

    private:
        // Joint states
        std::map<std::string, double> joint_positions_;

        // Step size
        double joint_step_;
        double step_increment_;
        double min_step_;
        double max_step_;

        // Saved joy state
        sensor_msgs::msg::Joy latest_joy_;

        bool joy_received_ = false;

        bool prev_dpad_up_ = false;
        bool prev_dpad_down_ = false;

        // Controller mappings
        std::map<std::string, JointMapping> joint_mappings_;

        InputMapping gripper_open_;
        InputMapping gripper_close_;
        InputMapping step_increase_;
        InputMapping step_decrease_;

        // ROS interfaces
        rclcpp::Publisher<interbotix_xs_msgs::msg::JointSingleCommand>::SharedPtr joint_pub_;

        rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;

        rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_sub_;

        rclcpp::TimerBase::SharedPtr jog_timer_;

        bool input_active(const sensor_msgs::msg::Joy &joy, const InputMapping &mapping) {
            if (mapping.type == "button") {
                return joy.buttons[mapping.index];
            }

            if (mapping.threshold > 0) {
                return joy.axes[mapping.index] >= mapping.threshold;
            }

            return joy.axes[mapping.index] <= mapping.threshold;
        }

        // Joint state callback
        void joint_state_callback(const sensor_msgs::msg::JointState::SharedPtr msg) {
            for (size_t i = 0; i < msg->name.size(); i++) {
                joint_positions_[msg->name[i]] = msg->position[i];
            }
        }

        // Joy callback
        void joy_callback(const sensor_msgs::msg::Joy::SharedPtr msg) {
            latest_joy_ = *msg;
            joy_received_ = true;

            // Increase step
            bool dpad_up = input_active(*msg, step_increase_);

            if (dpad_up && !prev_dpad_up_) {
                joint_step_ = std::min(joint_step_ + step_increment_, max_step_);

                RCLCPP_INFO(get_logger(), "Step size: %.3f rad", joint_step_);
            }

            prev_dpad_up_ = dpad_up;

            // Decrease step
            bool dpad_down = input_active(*msg, step_decrease_);

            if (dpad_down && !prev_dpad_down_) {
                joint_step_ = std::max(joint_step_ - step_increment_, min_step_);

                RCLCPP_INFO(get_logger(), "Step size: %.3f rad", joint_step_);
            }

            prev_dpad_down_ = dpad_down;
        }

        // Publish joint command
        void jog_joint(const std::string &joint_name, double delta) {
            auto it = joint_positions_.find(joint_name);

            if (it == joint_positions_.end())
                return;

            interbotix_xs_msgs::msg::JointSingleCommand cmd;

            cmd.name = joint_name;
            cmd.cmd = static_cast<float>(it->second + delta);

            joint_pub_->publish(cmd);
        }

        // Gripper command
        void command_gripper(float pwm) {
            interbotix_xs_msgs::msg::JointSingleCommand cmd;

            cmd.name = "gripper";
            cmd.cmd = pwm;

            joint_pub_->publish(cmd);
        }

        // Jog timer
        void jog_timer_callback() {
            if (!joy_received_)
                return;

            for (const auto &[joint_name, mapping] : joint_mappings_) {
                if (input_active(latest_joy_, mapping.positive)) {
                    jog_joint(joint_name, joint_step_);
                }

                if (input_active(latest_joy_, mapping.negative)) {
                    jog_joint(joint_name, -joint_step_);
                }
            }

            if (input_active(latest_joy_, gripper_open_)) {
                    command_gripper(300.0);
            }

            else if (input_active(latest_joy_, gripper_close_)) {
                command_gripper(-300.0);
            }

            else {
                command_gripper(0.0);
            }
        }
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<ArmController>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
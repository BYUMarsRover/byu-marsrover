#include <iostream>
#include <chrono>
#include <cmath>
#include <memory>
#include "cuda_utils.h"
#include "logging.h"
#include "utils.h"

#include "yolo.hpp"
// #include "sl_tools.h"

#include <sl/Camera.hpp>
#include <NvInfer.h>
#include <cv_bridge/cv_bridge.h>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/image_encodings.hpp>  // Do we need this?
#include <sensor_msgs/msg/image.hpp>        // Do we need this?

// Sensor data includes
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/magnetic_field.hpp>
#include <nav_msgs/msg/odometry.hpp>

// Object detction includes
#include <vision_msgs/msg/detection3_d_array.hpp>
#include <vision_msgs/msg/detection3_d.hpp>
#include <image_transport/image_transport.hpp>

// Point cloud includes
// #include <sensor_msgs/point_cloud2_iterator.hpp>
// #include <sensor_msgs/msg/point_cloud2.hpp>
// #include <point_cloud_transport/point_cloud_transport.hpp>

//Testing service to start detections
#include <std_srvs/srv/set_bool.hpp>


using namespace nvinfer1;
#define NMS_THRESH 0.4  //Not sure where this is used

#ifndef DEG2RAD
#define DEG2RAD 0.017453293
#define RAD2DEG 57.295777937
#endif

// Basic structure to compare timestamps of a sensor. Determines if a specific sensor data has been updated or not.
struct TimestampHandler {

    // Compare the new timestamp to the last valid one. If it is higher, save it as new reference.
    inline bool isNew(sl::Timestamp& ts_curr, sl::Timestamp& ts_ref) {
        bool new_ = ts_curr > ts_ref;
        if (new_) ts_ref = ts_curr;
        return new_;
    }
    // Specific function for IMUData.
    inline bool isNew(sl::SensorsData::IMUData& imu_data) {
        return isNew(imu_data.timestamp, ts_imu);
    }
    // Specific function for MagnetometerData.
    inline bool isNew(sl::SensorsData::MagnetometerData& mag_data) {
        return isNew(mag_data.timestamp, ts_mag);
    }
    // Specific function for BarometerData.
    inline bool isNew(sl::SensorsData::BarometerData& baro_data) {
        return isNew(baro_data.timestamp, ts_baro);
    }

    sl::Timestamp ts_imu = 0, ts_baro = 0, ts_mag = 0; // Initial values
};

// Function to display sensor parameters.
void printSensorConfiguration(sl::SensorParameters& sensor_parameters) {
    if (sensor_parameters.isAvailable) {
        std::cout << "*****************************" << std::endl;
        std::cout << "Sensor Type: " << sensor_parameters.type << std::endl;
        // std::cout << "Max Rate: "    << sensor_parameters.sampling_rate << SENSORS_UNIT::HERTZ << std::endl;
        std::cout << "Max Rate: "    << sensor_parameters.sampling_rate << std::endl;
        std::cout << "Range: ["      << sensor_parameters.range << "] " << sensor_parameters.sensor_unit << std::endl;
        std::cout << "Resolution: "  << sensor_parameters.resolution << " " << sensor_parameters.sensor_unit << std::endl;
        if (isfinite(sensor_parameters.noise_density)) std::cout << "Noise Density: " << sensor_parameters.noise_density <<" "<< sensor_parameters.sensor_unit<<"/√Hz"<<std::endl;
        if (isfinite(sensor_parameters.random_walk)) std::cout << "Random Walk: " << sensor_parameters.random_walk <<" "<< sensor_parameters.sensor_unit<<"/s/√Hz"<<std::endl;
    }
} 


class ObjectDetectionNode : public rclcpp::Node {
public:
    ObjectDetectionNode() : Node("object_detection") {
        // Publishers
        
        detection_annotation_ = image_transport::create_publisher(this, "object_detection/annotated");

        object_detection_pub_ = this->create_publisher<vision_msgs::msg::Detection3DArray>("object_detection", 10);
        
        // Declare the 'engine_name' parameter with an empty string as the default value
        this->declare_parameter<std::string>("engine_name", "");
        std::string engine_name;
        this->get_parameter("engine_name", engine_name);
        // Period in milliseconds for frame process 
        int process_period_ms;      
        this->declare_parameter<int>("process_period_ms", 2000);
        this->get_parameter("process_period_ms", process_period_ms);
        // Period in milliseconds for sensor data
        int sensor_process_period_ms;      
        this->declare_parameter<int>("sensor_process_period_ms", 20);
        this->get_parameter("sensor_process_period_ms", sensor_process_period_ms);

        this->declare_parameter<float>("confidence_thresh", 0.3);
        this->get_parameter("confidence_thresh", this->conf_thresh);

        // Initialize pose with identity
        cam_w_pose.pose_data.setIdentity();

        setup_node();

        setup_yolo(engine_name);  
        // Timer for the detection loop
        timer_ = this->create_wall_timer(std::chrono::milliseconds(process_period_ms), std::bind(&ObjectDetectionNode::processFrame, this));


        /* SETUP IMU, MAG, ODOM Publishers */
        imu_publisher_ = this->create_publisher<sensor_msgs::msg::Imu>("imu/data", 10);
        mag_publisher_ = this->create_publisher<sensor_msgs::msg::MagneticField>("imu/mag", 10);
        odom_publisher_ = this->create_publisher<nav_msgs::msg::Odometry>("odom", 10);

        //TODO: TEST this timer to run seperately from the zed obstacles
        timer_sensors_ = this->create_wall_timer(
            std::chrono::milliseconds(sensor_process_period_ms), std::bind(&ObjectDetectionNode::process_ZED_data, this));

        //Testing service call to start the object detection
        service_ = this->create_service<std_srvs::srv::SetBool>(
            "/toggle_object_detection",
            std::bind(&ObjectDetectionNode::handleToggleDetect, this, std::placeholders::_1, std::placeholders::_2)
        );
        
    }

private:

    // void setup_pc(){

    //     const int NEURAL_W = 896;
    //     const int NEURAL_H = 512;

    //     int pc_w = 0, pc_h = 0;
    //     // mPcResolution = PcRes::COMPACT;
    //     // // ----> Point Cloud resolution
    //     // switch (mPcResolution) {
    //     //     case PcRes::PUB: // Same as image and depth map
    //     //     pc_w = pub_w;
    //     //     pc_h = pub_h;
    //     //     break;
    //     //     case PcRes::FULL:
    //     //     pc_w = NEURAL_W;
    //     //     pc_h = NEURAL_H;
    //     //     break;
    //     //     case PcRes::COMPACT:
    //     //     pc_w = NEURAL_W / 2;
    //     //     pc_h = NEURAL_H / 2;
    //     //     break;
    //     //     case PcRes::REDUCED:
    //     //     pc_w = NEURAL_W / 4;
    //     //     pc_h = NEURAL_H / 4;
    //     //     break;
    //     // }
    //     // mPcResol = sl::Resolution(pc_w, pc_h);
        
    //     // Temp define of PC resolution
    //     pc_w = NEURAL_W / 2;
    //     pc_h = NEURAL_H / 2;
    //     mPcResol = sl::Resolution(pc_w, pc_h);
    // }

    // bool start3dMapping()
    // {

    //     const int NEURAL_W = 896;
    //     const int NEURAL_H = 512;

    //     int pc_w = 0, pc_h = 0;
    //     // mPcResolution = PcRes::COMPACT;
    //     // // ----> Point Cloud resolution
    //     // switch (mPcResolution) {
    //     //     case PcRes::PUB: // Same as image and depth map
    //     //     pc_w = pub_w;
    //     //     pc_h = pub_h;
    //     //     break;
    //     //     case PcRes::FULL:
    //     //     pc_w = NEURAL_W;
    //     //     pc_h = NEURAL_H;
    //     //     break;
    //     //     case PcRes::COMPACT:
    //     //     pc_w = NEURAL_W / 2;
    //     //     pc_h = NEURAL_H / 2;
    //     //     break;
    //     //     case PcRes::REDUCED:
    //     //     pc_w = NEURAL_W / 4;
    //     //     pc_h = NEURAL_H / 4;
    //     //     break;
    //     // }
    //     // mPcResol = sl::Resolution(pc_w, pc_h);
        
    //     // Temp define of PC resolution
    //     pc_w = NEURAL_W / 2;
    //     pc_h = NEURAL_H / 2;
    //     mPcResol = sl::Resolution(pc_w, pc_h);
        
    //     // DEBUG_MAP("start3dMapping");
    //     // if (mDepthDisabled) {
    //     //     RCLCPP_WARN(
    //     //     get_logger(),
    //     //     "Cannot start 3D Mapping if `depth.depth_mode` is set to `0` [NONE]");
    //     //     return false;
    //     // }

    //     // if (mSpatialMappingRunning) {
    //     //     RCLCPP_WARN(
    //     //     get_logger(),
    //     //     "Cannot start 3D Mapping. The module is already running!");
    //     //     return false;
    //     // }

    //     // bool required = mMappingEnabled;

    //     // if (!required) {
    //     //     return false;
    //     // }

    //     RCLCPP_INFO_STREAM(get_logger(), "*** Starting Spatial Mapping ***");

    //     sl::SpatialMappingParameters params;
    //     params.map_type =
    //         sl::SpatialMappingParameters::SPATIAL_MAP_TYPE::FUSED_POINT_CLOUD;
    //     params.use_chunk_only = true;

    //     sl::SpatialMappingParameters spMapPar;

    //     float lRes = spMapPar.allowed_resolution.first;
    //     float hRes = spMapPar.allowed_resolution.second;

    //     if (mMappingRes < lRes) {
    //         RCLCPP_WARN_STREAM(
    //         get_logger(),
    //         "'mapping.resolution' value ("
    //             << mMappingRes
    //             << " m) is lower than the allowed resolution "
    //             "values. Fixed automatically");
    //         mMappingRes = lRes;
    //     }
    //     if (mMappingRes > hRes) {
    //         RCLCPP_WARN_STREAM(
    //         get_logger(),
    //         "'mapping.resolution' value ("
    //             << mMappingRes
    //             << " m) is higher than the allowed resolution "
    //             "values. Fixed automatically");
    //         mMappingRes = hRes;
    //     }

    //     params.resolution_meter = mMappingRes;

    //     float lRng = spMapPar.allowed_range.first;
    //     float hRng = spMapPar.allowed_range.second;

    //     if (mMappingRangeMax < 0) {
    //         mMappingRangeMax =
    //         sl::SpatialMappingParameters::getRecommendedRange(mMappingRes, *mZed.get());
    //         RCLCPP_INFO_STREAM(
    //         get_logger(), "Mapping: max range set to "
    //             << mMappingRangeMax
    //             << " m for a resolution of "
    //             << mMappingRes << " m");
    //     } else if (mMappingRangeMax < lRng) {
    //         RCLCPP_WARN_STREAM(
    //         get_logger(), "'mapping.max_mapping_range_m' value ("
    //             << mMappingRangeMax
    //             << " m) is lower than the allowed "
    //             "resolution values. Fixed "
    //             "automatically");
    //         mMappingRangeMax = lRng;
    //     } else if (mMappingRangeMax > hRng) {
    //         RCLCPP_WARN_STREAM(
    //         get_logger(), "'mapping.max_mapping_range_m' value ("
    //             << mMappingRangeMax
    //             << " m) is higher than the allowed "
    //             "resolution values. Fixed "
    //             "automatically");
    //         mMappingRangeMax = hRng;
    //     }

    //     params.range_meter = mMappingRangeMax;

    //     sl::ERROR_CODE err = mZed->enableSpatialMapping(params);

    //     if (err == sl::ERROR_CODE::SUCCESS) {
    //         if (mPubFusedCloud == nullptr) {
    //     #ifndef FOUND_FOXY
    //         mPubFusedCloud = point_cloud_transport::create_publisher(
    //             this->shared_from_this(), mPointcloudFusedTopic,
    //             mQos.get_rmw_qos_profile(), mPubOpt);
    //         RCLCPP_INFO_STREAM(
    //             get_logger(), "Advertised on topic "
    //             << mPubFusedCloud.getTopic()
    //             << " @ " << mFusedPcPubRate
    //             << " Hz");
    //     #else
    //         mPubFusedCloud = create_publisher<sensor_msgs::msg::PointCloud2>(
    //             mPointcloudFusedTopic, mQos, mPubOpt);
    //         RCLCPP_INFO_STREAM(
    //             get_logger(), "Advertised on topic "
    //             << mPubFusedCloud->get_topic_name()
    //             << " @ " << mFusedPcPubRate
    //             << " Hz");
    //     #endif
    //         }

    //         mSpatialMappingRunning = true;

    //         startFusedPcTimer(mFusedPcPubRate);

    //         RCLCPP_INFO_STREAM(
    //         get_logger(),
    //         " * Resolution: " << params.resolution_meter << " m");
    //         RCLCPP_INFO_STREAM(
    //         get_logger(),
    //         " * Max Mapping Range: " << params.range_meter << " m");
    //         RCLCPP_INFO_STREAM(
    //         get_logger(), " * Map point cloud publishing rate: "
    //             << mFusedPcPubRate << " Hz");

    //         return true;
    //     } else {
    //         mSpatialMappingRunning = false;
    //         if (mFusedPcTimer) {
    //         mFusedPcTimer->cancel();
    //         }

    //         RCLCPP_WARN(
    //         get_logger(), "Mapping not activated: %s",
    //         sl::toString(err).c_str());

    //         return false;
    //     }
    // }
    
    void setup_yolo(std::string engine_name) {
        /* Custom YOLOv8 model initialization */
        if (!engine_name.empty()) {
            RCLCPP_INFO(this->get_logger(), "Using YOLOv8 model engine: %s", engine_name.c_str());
        } else {
            RCLCPP_ERROR(this->get_logger(), "No YOLOv8 model engine specified.");
            throw std::runtime_error("Engine not specified.");
            return;
        }
        if (detector_.init(engine_name)) {
            RCLCPP_ERROR(this->get_logger(), "Detector initialization failed!");
            throw std::runtime_error("Detector init failed");
            return;
        }
        RCLCPP_INFO(this->get_logger(), "Initialized object detection");
    }

    cv::Rect get_rect(BBox box) {
        return cv::Rect(round(box.x1), round(box.y1), round(box.x2 - box.x1), round(box.y2 - box.y1));
    }

    std::vector<sl::uint2> cvt(const BBox &bbox_in) {
        std::vector<sl::uint2> bbox_out(4);
        bbox_out[0] = sl::uint2(bbox_in.x1, bbox_in.y1);
        bbox_out[1] = sl::uint2(bbox_in.x2, bbox_in.y1);
        bbox_out[2] = sl::uint2(bbox_in.x2, bbox_in.y2);
        bbox_out[3] = sl::uint2(bbox_in.x1, bbox_in.y2);
        return bbox_out;
    }

       
    void setup_node(){
        //TODO: Check to make sure below is not used anymore
        // sensor_msgs::CameraInfoPtr left_camera_info_msg;
        // left_camera_info_msg.reset(new sensor_msgs::CameraInfo());
        // std::string left_camera_frame_id = "zed2i_left_camera_optical_frame";
        
        /* ZED camera initializaion */
        // Opening the ZED camera before the model deserialization to avoid cuda context issue
        
        sl::InitParameters init_parameters;
        init_parameters.sdk_verbose = true;
        init_parameters.input.setFromSerialNumber(20382332);
        init_parameters.depth_mode = sl::DEPTH_MODE::ULTRA;
        // TODO: Check if this works
        init_parameters.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Z_UP_X_FWD;
        //CHECK THAT METER WAS THE DEFAULT
        init_parameters.coordinate_units = sl::UNIT::METER;

        RCLCPP_INFO(this->get_logger(), "Camera serial number: %s", std::to_string(zed.getCameraInformation().serial_number).c_str());


        // Open the camera
        auto returned_state = zed.open(init_parameters);
        if (returned_state != sl::ERROR_CODE::SUCCESS) {
            RCLCPP_ERROR(this->get_logger(), "Camera Open failed with error code: %s. Exit program.", sl::toString(returned_state).c_str());
            throw std::runtime_error("Camera initialization failed.");
            return;
        }

        // Start Postional Tracking with parameters
        sl::PositionalTrackingParameters pose_tracking_params;
        //todo fix this
        // pose_tracking_params.mode = sl::POSITIONAL_TRACKING_MODE::GEN_2;
        pose_tracking_params.enable_area_memory = false;
        auto positional_init = zed.enablePositionalTracking(pose_tracking_params);
        if (positional_init != sl::ERROR_CODE::SUCCESS) {
            std::cerr << "[ZED][ERROR] Can't start tracking of camera" << std::endl;
            // Handle the error in your application.
        }

        // Possible use of a Region of interest for postional tracking
        // TODO
        /*
        std::string roi_file_path = ""; // Set the path of your ROI file
        sl::Mat mask_roi;
        auto err = mask_roi.read(roi_file_path.c_str());
        if (err == sl::ERROR_CODE::SUCCESS)
            zed.setRegionOfInterest(mask_roi, {sl::MODULE::ALL});
        else
            std::cout << "Error loading Region of Interest file: " << err << std::endl;
        */

        /* //TODO GPS FUSION
        zed.startPublishing();

        // Setup the Sensor Fusion Module for 
        
        sl::InitFusionParameters init_fusion_param;
        init_fusion_param.coordinate_system = init_parameters.coordinate_system;
        init_fusion_param.coordinate_units = init_parameters.coordinate_units;
        init_fusion_param.verbose = true;
        sl::FUSION_ERROR_CODE fusion_init_code = fusion_.init(init_fusion_param);
        if (fusion_init_code != sl::FUSION_ERROR_CODE::SUCCESS) {
            std::cerr << "[Fusion][ERROR] Failed to initialize fusion, error: " << fusion_init_code << std::endl;
            // Handle the error in your application.
        }

        sl::CameraIdentifier uuid(zed.getCameraInformation().serial_number);
        fusion_.subscribe(uuid);

        sl::GNSSCalibrationParameters gnss_calibration_parameter;
        gnss_calibration_parameter.enable_reinitialization = false;
        gnss_calibration_parameter.enable_translation_uncertainty_target = false;
        gnss_calibration_parameter.gnss_vio_reinit_threshold = 5;
        gnss_calibration_parameter.target_yaw_uncertainty = 1e-2;
        gnss_calibration_parameter.gnss_antenna_position = sl::float3(0,0,0); // Set your antenna position


        sl::PositionalTrackingFusionParameters positional_tracking_fusion_parameters;
        positional_tracking_fusion_parameters.enable_GNSS_fusion = true;
        positional_tracking_fusion_parameters.gnss_calibration_parameters = gnss_calibration_parameter;
        sl::FUSION_ERROR_CODE tracking_error_code = fusion_.enablePositionalTracking(positional_tracking_fusion_parameters);
        if (tracking_error_code != sl::FUSION_ERROR_CODE::SUCCESS) {
            std::cout << "[Fusion][ERROR] Could not start tracking, error: " << tracking_error_code << std::endl;
            // Handle the error in your application
        }

        



        */


        /* ZED object detection initialization */
        sl::ObjectDetectionParameters detection_parameters;
        detection_parameters.enable_tracking = true;
        //detection_parameters.enable_segmentation = false; // designed to give person pixel mask, ZED SDK 4 only
        detection_parameters.detection_model = sl::OBJECT_DETECTION_MODEL::CUSTOM_BOX_OBJECTS;
        returned_state = zed.enableObjectDetection(detection_parameters);
        if (returned_state != sl::ERROR_CODE::SUCCESS) {
            RCLCPP_ERROR(this->get_logger(), "enableObjectDetection failed with error code: %s. Exit program.", sl::toString(returned_state).c_str());
            zed.close();
            throw std::runtime_error("Object detection enable failed.");
            return;
        }


        auto camera_config = zed.getCameraInformation().camera_configuration;
        sl::Resolution pc_resolution(std::min((int) camera_config.resolution.width, 720), std::min((int) camera_config.resolution.height, 404));
        auto camera_info = zed.getCameraInformation(pc_resolution).camera_configuration;

        RCLCPP_INFO(this->get_logger(),"Initialized ZED camera");

        /* Object detection data initialization */
        display_resolution_ = zed.getCameraInformation().camera_configuration.resolution;
        
    }
    
    void handleToggleDetect(const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
                                std::shared_ptr<std_srvs::srv::SetBool::Response> response){
        run_detector_ = request->data;
        response->success = true;
        response->message = run_detector_ ? "Detection enabled" : "Detection disabled";
        RCLCPP_INFO(this->get_logger(), "Toggled Detection: %s", response->message.c_str());
    }

    void processFrame() {
        if (!run_detector_) {
            return;  // Do nothing if detections is disabled
        }
        // Grab image from ZED and process detections
        if (zed.grab() == sl::ERROR_CODE::SUCCESS) {
            /* Left image */
            // zed.retrieveImage(left_sl, sl::VIEW::LEFT);
            /* Object detections */
            publishDetections();

            // publish_sensor_data();
        }
    }

    void process_ZED_data(){
        if (zed.grab() == sl::ERROR_CODE::SUCCESS) {
            publish_sensor_data();
            publish_position_data();
        }
    }

    void publish_position_data(){
        // Publish odometry data
        auto current_timestamp = this->now();
        sl::Pose camera_pose;
        zed.getPosition(camera_pose, sl::REFERENCE_FRAME::WORLD);

        auto odom_msg = std::make_unique<nav_msgs::msg::Odometry>();
        odom_msg->header.stamp = current_timestamp;
        odom_msg->header.frame_id = "odom";
        odom_msg->child_frame_id = "zed_base_link";

        odom_msg->pose.pose.position.x = camera_pose.getTranslation().x;
        odom_msg->pose.pose.position.y = camera_pose.getTranslation().y;
        odom_msg->pose.pose.position.z = camera_pose.getTranslation().z;
        odom_msg->pose.pose.orientation.x = camera_pose.getOrientation().x;
        odom_msg->pose.pose.orientation.y = camera_pose.getOrientation().y;
        odom_msg->pose.pose.orientation.z = camera_pose.getOrientation().z;
        odom_msg->pose.pose.orientation.w = camera_pose.getOrientation().w;

        // odom_msg->twist.twist.linear.x = camera_pose.getVelocity().x;
        // odom_msg->twist.twist.linear.y = camera_pose.getVelocity().y;
        // odom_msg->twist.twist.linear.z = camera_pose.getVelocity().z;
        // odom_msg->twist.twist.angular.x = camera_pose.getEulerAngles().x;
        // odom_msg->twist.twist.angular.y = camera_pose.getEulerAngles().y;
        // odom_msg->twist.twist.angular.z = camera_pose.getEulerAngles().z;

        odom_publisher_->publish(std::move(odom_msg));
    }
    
    void publish_sensor_data(){
        sl::SensorsData sensors_data;

        zed.getSensorsData(sensors_data, sl::TIME_REFERENCE::CURRENT);

        if (ts.isNew(sensors_data.imu)) {
            std::cout << "IMU Orientation: {" << sensors_data.imu.pose.getOrientation() << "}" << std::endl;
            std::cout << "IMU Linear Acceleration: {" << sensors_data.imu.linear_acceleration << "} [m/sec^2]" << std::endl;
            std::cout << "IMU Angular Velocity: {" << sensors_data.imu.angular_velocity << "} [deg/sec]" << std::endl;
        }
        else{
            std::cout << "No new sensor data this update" << std::endl;
            return;
        }

        // Check if Magnetometer data has been updated
        // if (ts.isNew(sensors_data.magnetometer)) {
        std::cout << " Magnetometer Magnetic Field: {" << sensors_data.magnetometer.magnetic_field_calibrated << "} [uT]" << std::endl;
        // }

        auto current_timestamp = this->now();

        // Publish IMU data
        auto imu_msg = std::make_unique<sensor_msgs::msg::Imu>();
        imu_msg->header.stamp = current_timestamp;
        imu_msg->header.frame_id = "zed_imu_link";

        auto imu_data = sensors_data.imu;
        auto mag_data = sensors_data.magnetometer;
        imu_msg->orientation.x = imu_data.pose.getOrientation().x;
        imu_msg->orientation.y = imu_data.pose.getOrientation().y;
        imu_msg->orientation.z = imu_data.pose.getOrientation().z;
        imu_msg->orientation.w = imu_data.pose.getOrientation().w;
        imu_msg->angular_velocity.x = imu_data.angular_velocity.x;
        imu_msg->angular_velocity.y = imu_data.angular_velocity.y;
        imu_msg->angular_velocity.z = imu_data.angular_velocity.z;
        imu_msg->linear_acceleration.x = imu_data.linear_acceleration.x;
        imu_msg->linear_acceleration.y = imu_data.linear_acceleration.y;
        imu_msg->linear_acceleration.z = imu_data.linear_acceleration.z;

        imu_publisher_->publish(std::move(imu_msg));

        // Publish magnetometer data
        auto mag_msg = std::make_unique<sensor_msgs::msg::MagneticField>();
        mag_msg->header.stamp = current_timestamp;
        mag_msg->header.frame_id = "zed_imu_link";
        mag_msg->magnetic_field.x = mag_data.magnetic_field_calibrated.x;
        mag_msg->magnetic_field.y = mag_data.magnetic_field_calibrated.y;
        mag_msg->magnetic_field.z = mag_data.magnetic_field_calibrated.z;

        mag_publisher_->publish(std::move(mag_msg));
    }

    // void publishPointCloud()
    // {
    //     // sl_tools::StopWatch pcElabTimer(get_clock());

    //     pointcloudMsgPtr pcMsg = std::make_unique<sensor_msgs::msg::PointCloud2>();

    //     // Initialize Point Cloud message
    //     // https://github.com/ros/common_msgs/blob/jade-devel/sensor_msgs/include/sensor_msgs/point_cloud2_iterator.h

    //     int width = mPcResol.width;
    //     int height = mPcResol.height;

    //     int ptsCount = width * height;

    //     pcMsg->header.stamp = sl_tools::slTime2Ros(mMatCloud.timestamp);
    

    //     // ---> Check that `pcMsg->header.stamp` is not the same of the latest
    //     // published pointcloud Avoid to publish the same old data
    //     if (mLastTs_pc == pcMsg->header.stamp) {
    //         // Data not updated by a grab calling in the grab thread
    //         std::cout << "publishPointCloud: ignoring not update data" << std::endl;
    //         return;
    //     }
    //     mLastTs_pc = pcMsg->header.stamp;
    //     // <--- Check that `pcMsg->header.stamp` is not the same of the latest
    //     // published pointcloud

    //     if (pcMsg->width != width || pcMsg->height != height) {
    //         pcMsg->header.frame_id ="zed";      // Set the header values of the ROS message

    //         pcMsg->is_bigendian = false;
    //         pcMsg->is_dense = false;

    //         pcMsg->width = width;
    //         pcMsg->height = height;

    //         sensor_msgs::PointCloud2Modifier modifier(*(pcMsg.get()));
    //         modifier.setPointCloud2Fields(
    //         4, "x", 1, sensor_msgs::msg::PointField::FLOAT32, "y", 1,
    //         sensor_msgs::msg::PointField::FLOAT32, "z", 1,
    //         sensor_msgs::msg::PointField::FLOAT32, "rgb", 1,
    //         sensor_msgs::msg::PointField::FLOAT32);
    //     }

    //     sl::Vector4<float> * cpu_cloud = mMatCloud.getPtr<sl::float4>();

    //     // Data copy
    //     float * ptCloudPtr = reinterpret_cast<float *>(&pcMsg->data[0]);
    //     memcpy(
    //         ptCloudPtr, reinterpret_cast<float *>(cpu_cloud),
    //         ptsCount * 4 * sizeof(float));

    //     // Pointcloud publishing
    //     DEBUG_STREAM_PC("Publishing POINT CLOUD message");
    //     #ifndef FOUND_FOXY
    //     try {
    //         mPubCloud.publish(std::move(pcMsg));
    //     } catch (std::system_error & e) {
    //         DEBUG_STREAM_COMM("Message publishing ecception: " << e.what());
    //     } catch (...) {
    //         DEBUG_STREAM_COMM("Message publishing generic ecception: ");
    //     }
    //     #else
    //     try {
    //         mPubCloud->publish(std::move(pcMsg));
    //     } catch (std::system_error & e) {
    //         DEBUG_STREAM_COMM("Message publishing ecception: " << e.what());
    //     } catch (...) {
    //         DEBUG_STREAM_COMM("Message publishing generic ecception: ");
    //     }
    //     #endif

    //     // Publish freq calculation
    //     double mean = mPcPeriodMean_sec->addValue(mPcFreqTimer.toc());
    //     mPcFreqTimer.tic();

    //     // Point cloud elaboration time
    //     mPcProcMean_sec->addValue(pcElabTimer.toc());
    //     DEBUG_STREAM_PC("Point cloud freq: " << 1. / mean);
    // }

    // void callback_pubFusedPc()
    // {
    //     DEBUG_STREAM_ONCE_MAP("Mapping callback called");

    //     pointcloudMsgPtr pointcloudFusedMsg =
    //         std::make_unique<sensor_msgs::msg::PointCloud2>();

    //     uint32_t fusedCloudSubCount = 0;
    //     try {
    //     #ifndef FOUND_FOXY
    //         fusedCloudSubCount = mPubFusedCloud.getNumSubscribers();
    //     #else
    //         fusedCloudSubCount = count_subscribers(mPubFusedCloud->get_topic_name());
    //     #endif
    //     } catch (...) {
    //         rcutils_reset_error();
    //         DEBUG_STREAM_MAP("pubFusedPc: Exception while counting subscribers");
    //         return;
    //     }

    //     if (fusedCloudSubCount == 0) {
    //         return;
    //     }

    //     if (!mZed->isOpened()) {
    //         return;
    //     }

    //     mZed->requestSpatialMapAsync();

    //     while (mZed->getSpatialMapRequestStatusAsync() == sl::ERROR_CODE::FAILURE) {
    //         // Mesh is still generating
    //         rclcpp::sleep_for(1ms);
    //     }

    //     sl::ERROR_CODE res = mZed->retrieveSpatialMapAsync(mFusedPC);

    //     if (res != sl::ERROR_CODE::SUCCESS) {
    //         RCLCPP_WARN_STREAM(
    //         get_logger(), "Fused point cloud not extracted: "
    //             << sl::toString(res).c_str());
    //         return;
    //     }

    //     size_t ptsCount = mFusedPC.getNumberOfPoints();
    //     bool resized = false;

    //     if (pointcloudFusedMsg->width != ptsCount ||
    //         pointcloudFusedMsg->height != 1)
    //     {
    //         // Initialize Point Cloud message
    //         // https://github.com/ros/common_msgs/blob/jade-devel/sensor_msgs/include/sensor_msgs/point_cloud2_iterator.h
    //         pointcloudFusedMsg->header.frame_id =
    //         mMapFrameId;      // Set the header values of the ROS message
    //         pointcloudFusedMsg->is_bigendian = false;
    //         pointcloudFusedMsg->is_dense = false;
    //         pointcloudFusedMsg->width = ptsCount;
    //         pointcloudFusedMsg->height = 1;

    //         sensor_msgs::PointCloud2Modifier modifier(*pointcloudFusedMsg);
    //         modifier.setPointCloud2Fields(
    //         4, "x", 1, sensor_msgs::msg::PointField::FLOAT32, "y", 1,
    //         sensor_msgs::msg::PointField::FLOAT32, "z", 1,
    //         sensor_msgs::msg::PointField::FLOAT32, "rgb", 1,
    //         sensor_msgs::msg::PointField::FLOAT32);

    //         resized = true;
    // }

    void publishDetections() {
        // Running inference
        //CHECK TO MAKE SURE: THIS MIGHT NEED TO RUN ONLY WHEN WE WANT IT TO hence the subscriber count
        zed.retrieveImage(left_sl, sl::VIEW::LEFT);
   
        auto detections = detector_.run(left_sl, display_resolution_.height, display_resolution_.width, this->conf_thresh);
        
        // Convert the sl::Mat to cv::Mat
        left_cv_ = slMat2cvMat(left_sl);

        // Preparing for ZED SDK ingesting
        std::vector<sl::CustomBoxObjectData> objects_in;
        
        for (auto &it : detections) {
            sl::CustomBoxObjectData tmp;
            // Fill the detections into the correct format
            tmp.unique_object_id = sl::generate_unique_id();
            tmp.probability = it.prob;
            tmp.label = (int) it.label;
            tmp.bounding_box_2d = cvt(it.box);
            tmp.is_grounded = ((int) it.label == 0); // Only the first class (person) is grounded, that is moving on the floor plane
            // others are tracked in full 3D space
            objects_in.push_back(tmp);
        }
        // Send the custom detected boxes to the ZED
        zed.ingestCustomBoxObjects(objects_in);


        //DETECTION ANNOTATION PUBLISHER HERE


        // Publish detections if subscribers are present
        // if (object_detection_pub_->get_subscription_count() > 0) {
        zed.retrieveObjects(objects, object_tracker_params_rt_);
        std::cout << "Object Detection Check" << std::endl;
        
        if (!objects.object_list.empty()) {

            // Create and populate the Detection3DArray message
            vision_msgs::msg::Detection3DArray detection_array_msg;
            
            detection_array_msg.header.stamp = now();
            detection_array_msg.header.frame_id = "zed";

            for (const auto& object : objects.object_list) {
                vision_msgs::msg::Detection3D detection_msg;
                detection_msg.header = detection_array_msg.header;

                // Resize results to hold at least one result for each detection
                detection_msg.results.resize(1); // or adjust based on your needs

                detection_msg.results[0].id = object.raw_label; //check the difference between raw_label and id
                detection_msg.results[0].score = object.confidence;

                detection_msg.results[0].pose.pose.position.x = object.position.x;
                detection_msg.results[0].pose.pose.position.y = object.position.y;
                detection_msg.results[0].pose.pose.position.z = object.position.z;

                // In the future we can publish the 3D bbox around the object

                // detection.id = object.id; // DO WE NEED TO PUBLISH THE OBJECT ID?
                detection_array_msg.detections.push_back(detection_msg);
                std::cout << "Object Detected" << std::endl;
            }

            object_detection_pub_->publish(detection_array_msg);
        }
        // }

        // TODO: WE MIGHT NEED THE CVT FUNCTION IN HERE FOR ANNotations??
        // THE OLD CODE LOOKS A LITTLE DIFFERENT BELOW THIS
        // Annotate and publish image
        if (detection_annotation_.getNumSubscribers() > 0) {
            left_cv_ = slMat2cvMat(left_sl);
            for (const auto& detection : detections) {
                cv::Rect r = get_rect(detection.box);
                cv::rectangle(left_cv_, r, cv::Scalar(0x27, 0xC1, 0x36), 2);
                cv::putText(left_cv_, std::to_string(static_cast<int>(detection.label)), cv::Point(r.x, r.y - 1), cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(0xFF, 0xFF, 0xFF), 2);
            }
            
            sensor_msgs::msg::Image::SharedPtr msg = cv_bridge::CvImage(std_msgs::msg::Header(), sensor_msgs::image_encodings::TYPE_8UC4, left_cv_).toImageMsg();
            // std_msgs::msg::Header header;
            // msg.header.stamp = this->now(); // Set the current time
            detection_annotation_.publish(msg);
            std::cout << "Published image"  << std::endl;

            // header.frame_id = "object_detection"; // Set appropriate frame ID
            // cv_bridge::CvImage annotated_img(header, sensor_msgs::image_encodings::TYPE_8UC4, left_cv_);
            // detection_annotation_.publish(annotated_img.toImageMsg());
        }
    }

    // Member variables
    sl::Camera zed;
    Yolo detector_;
    cv::Mat left_cv_;
    sl::Objects objects;
    sl::ObjectDetectionRuntimeParameters object_tracker_params_rt_;
    sl::Resolution display_resolution_;
    // sl::Fusion fusion_;

    /* Resolution calcualations */
    sl::Resolution resolution;

    sl::Mat left_sl, point_cloud;
    sl::Pose cam_w_pose = sl::Pose();

    // std::string mag_frame_id = "zed2i_mag_link";
    // std::string imu_frame_id = "zed2i_imu_link";    

    rclcpp::Publisher<vision_msgs::msg::Detection3DArray>::SharedPtr object_detection_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::TimerBase::SharedPtr timer_sensors_;

    float conf_thresh;

    TimestampHandler ts;  //TODO: get the time stamp handler to only publish new data

    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_publisher_;
    rclcpp::Publisher<sensor_msgs::msg::MagneticField>::SharedPtr mag_publisher_;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_publisher_;
    //TODO: Test second timer to run timing of objects and data independently
    // rclcpp::TimerBase::SharedPtr timer_;


    //Spatial Mapping:
    // rclcpp::Time mLastTs_pc;
    // sl::Mat mMatCloud;
    // sl::FusedPointCloud mFusedPC;
    // point_cloud_transport::Publisher mPubCloud;
    // point_cloud_transport::Publisher mPubFusedCloud;
    // sl::Resolution mPcResol;


    //IMAGE TRANSPORT
    image_transport::Publisher detection_annotation_;
    
    rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr service_;
    bool run_detector_ = false; //Flag to run object detection
};


int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    try {
        rclcpp::spin(std::make_shared<ObjectDetectionNode>());
    } catch (const std::runtime_error &e) {
        RCLCPP_FATAL(rclcpp::get_logger("rclcpp"), "Node terminated due to error: %s", e.what());
    }
    rclcpp::shutdown();
    return 0;
}

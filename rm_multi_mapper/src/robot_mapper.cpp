#include <robot_mapper.h>

robot_mapper::robot_mapper(ros::NodeHandle & nh,
		const std::string & robot_prefix, const int robot_num) :
		robot_num(robot_num), prefix(
				"/" + robot_prefix
						+ boost::lexical_cast<std::string>(robot_num)), merged(
				false), move_base_action_client(prefix + "/move_base", true), map_idx(
				0) {

	std::string pref = robot_prefix
			+ boost::lexical_cast<std::string>(robot_num);
	octomap_server.reset(new RmOctomapServer(ros::NodeHandle(nh, pref), pref));
	octomap_server->openFile("free_space.bt");

	capture_client = nh.serviceClient<rm_capture_server::Capture>(
			prefix + "/capture");

	clear_unknown_space_client = nh.serviceClient<std_srvs::Empty>(
			prefix + "/move_base/clear_unknown_space");

	servo_pub = nh.advertise<std_msgs::Float32>(
			prefix + "/mobile_base/commands/servo_angle", 1);

	pub_keypoints = nh.advertise<pcl::PointCloud<pcl::PointXYZ> >(
			prefix + "/keypoints", 10);

	set_map_client = nh.serviceClient<rm_localization::SetMap>(
			prefix + "/set_map");

	set_intial_pose = nh.serviceClient<rm_localization::SetInitialPose>(
			prefix + "/set_initial_pose");

	visualization_offset = Eigen::Vector3f(0, robot_num * 20, 0);

	/*
	 initial_transformation.setIdentity();
	 initial_transformation.translate(
	 Eigen::Vector3f(0, (robot_num - 1) * 30, 0));

	 rm_localization::SetInitialPose data;
	 tf::Transform init_tf;
	 tf::transformEigenToTF(initial_transformation.cast<double>(), init_tf);
	 tf::transformTFToMsg(init_tf, data.request.pose);
	 if (!set_intial_pose.call(data)) {
	 ROS_ERROR("Coudld not set initial pose to robot");
	 }
	 */

}

void robot_mapper::save_image() {

	rm_capture_server::Capture srv;
	srv.request.num_frames = 1;

	bool res = capture_client.call(srv);

	if (res) {

		cv::Mat rgb = cv::imdecode(srv.response.rgb_png_data,
				CV_LOAD_IMAGE_UNCHANGED);

		cv::Mat depth = cv::imdecode(srv.response.depth_png_data,
				CV_LOAD_IMAGE_UNCHANGED);

		cv::imwrite("rgb/" + boost::lexical_cast<std::string>(map_idx) + ".png", rgb);
		cv::imwrite("depth/" + boost::lexical_cast<std::string>(map_idx) + ".png", depth);

		map_idx++;
	}

}

void robot_mapper::save_circle() {

	move_base_msgs::MoveBaseGoal goal;
	goal.target_pose.header.frame_id = "base_link";
	goal.target_pose.header.stamp = ros::Time::now();

	goal.target_pose.pose.position.x = 0;
	goal.target_pose.pose.position.y = 0;
	goal.target_pose.pose.position.z = 0;

	tf::Quaternion q;
	q.setRotation(tf::Vector3(0, 0, 1), M_PI / 9);
	tf::quaternionTFToMsg(q, goal.target_pose.pose.orientation);

	for (int i = 0; i < 36; i++) {


		save_image();

		move_base_action_client.sendGoal(goal);

		//wait for the action to return
		bool finished_before_timeout = move_base_action_client.waitForResult(
				ros::Duration(30.0));

		if (finished_before_timeout) {
			actionlib::SimpleClientGoalState state =
					move_base_action_client.getState();
			ROS_INFO("Action finished: %s", state.toString().c_str());
		} else
			ROS_INFO("Action did not finish before the time out.");

	}

	map->publish_keypoints(pub_keypoints, prefix);

}

void robot_mapper::capture() {
	rm_capture_server::Capture srv;
	srv.request.num_frames = 1;

	bool res = capture_client.call(srv);

	if (res) {

		cv::Mat rgb, rgb_dist = cv::imdecode(srv.response.rgb_png_data,
				CV_LOAD_IMAGE_UNCHANGED);

		cv::Mat depth, depth_dist = cv::imdecode(srv.response.depth_png_data,
				CV_LOAD_IMAGE_UNCHANGED);

		cv::imwrite(
				"rgb" + prefix + "_" + boost::lexical_cast<std::string>(map_idx)
						+ ".png", rgb_dist);
		cv::imwrite(
				"depth" + prefix + "_"
						+ boost::lexical_cast<std::string>(map_idx) + ".png",
				rgb_dist);

		Eigen::Affine3d transform_d;
		tf::transformMsgToEigen(srv.response.transform, transform_d);
		Eigen::Affine3f transform = transform_d.cast<float>();

		if (map.get()) {

			ROS_INFO("intrinsics %f %f %f %f", intrinsics[0], intrinsics[1],
					intrinsics[2], intrinsics[3]);

			//cv::undistort(rgb_dist, rgb, camera_matrix, dist_params);
			//cv::undistort(depth_dist, depth, camera_matrix, dist_params);
			rgb = rgb_dist;
			depth = depth_dist;

			keypoint_map map1(rgb, depth, transform, intrinsics);
			map1.save(
					"maps/cloudot_"
							+ boost::lexical_cast<std::string>(robot_num)
							+ "_map_"
							+ boost::lexical_cast<std::string>(map_idx));
			map->merge_keypoint_map(map1, 50, 300);

			//map->merge_images(rgb, depth, transform);

			map->publish_keypoints(pub_keypoints, prefix);

		} else {

			camera_matrix = cv::Mat(3, 3, CV_64F,
					srv.response.camera_info.K.data());
			dist_params = cv::Mat(srv.response.camera_info.D, true);

			intrinsics << srv.response.camera_info.K[0], srv.response.camera_info.K[4], srv.response.camera_info.K[2], srv.response.camera_info.K[5];

			ROS_INFO("intrinsics %f %f %f %f", intrinsics[0], intrinsics[1],
					intrinsics[2], intrinsics[3]);

			//cv::undistort(rgb_dist, rgb, camera_matrix, dist_params);
			//cv::undistort(depth_dist, depth, camera_matrix, dist_params);
			rgb = rgb_dist;
			depth = depth_dist;

			map.reset(new keypoint_map(rgb, depth, transform, intrinsics));
			//map->align_z_axis();
			map->save(
					"maps/cloudot_"
							+ boost::lexical_cast<std::string>(robot_num)
							+ "_map_"
							+ boost::lexical_cast<std::string>(map_idx));
		}
		map_idx++;

		std::cerr << map->keypoints3d.size() << " " << map->descriptors.rows
				<< " " << map->weights.size() << std::endl;

	} else {
		ROS_ERROR("Failed to call service /cloudbot%d/capture", robot_num);
		return;
	}
}

void robot_mapper::capture_sphere() {

	std_msgs::Float32 start_angle;
	start_angle.data = M_PI / 4;
	float stop_angle = -M_PI / 4;
	float delta = -M_PI / 18;

	for (int i = 0; i < 36; i++) {

		servo_pub.publish(start_angle);

		move_base_msgs::MoveBaseGoal goal;
		goal.target_pose.header.frame_id = "base_link";
		goal.target_pose.header.stamp = ros::Time::now();

		goal.target_pose.pose.position.x = 0;
		goal.target_pose.pose.position.y = 0;
		goal.target_pose.pose.position.z = 0;

		tf::Quaternion q;
		q.setRotation(tf::Vector3(0, 0, 1), M_PI / 9);
		tf::quaternionTFToMsg(q, goal.target_pose.pose.orientation);

		move_base_action_client.sendGoal(goal);

		//wait for the action to return
		bool finished_before_timeout = move_base_action_client.waitForResult(
				ros::Duration(30.0));

		if (finished_before_timeout) {
			actionlib::SimpleClientGoalState state =
					move_base_action_client.getState();
			ROS_INFO("Action finished: %s", state.toString().c_str());
		} else
			ROS_INFO("Action did not finish before the time out.");

		for (float angle = start_angle.data + delta; angle >= stop_angle;
				angle += delta) {

			std_msgs::Float32 angle_msg;
			angle_msg.data = angle;
			servo_pub.publish(angle_msg);
			capture();
		}
	}

	std_msgs::Float32 angle_msg;
	angle_msg.data = 0;
	servo_pub.publish(angle_msg);

	map->remove_bad_points(1);
	map->optimize();

	map->publish_keypoints(pub_keypoints, prefix);

}

void robot_mapper::capture_circle() {

	for (int i = 0; i < 36; i++) {

		sleep(1);
		capture();

		move_base_msgs::MoveBaseGoal goal;
		goal.target_pose.header.frame_id = "base_link";
		goal.target_pose.header.stamp = ros::Time::now();

		goal.target_pose.pose.position.x = 0;
		goal.target_pose.pose.position.y = 0;
		goal.target_pose.pose.position.z = 0;

		tf::Quaternion q;
		q.setRotation(tf::Vector3(0, 0, 1), M_PI / 9);
		tf::quaternionTFToMsg(q, goal.target_pose.pose.orientation);

		move_base_action_client.sendGoal(goal);

		//wait for the action to return
		bool finished_before_timeout = move_base_action_client.waitForResult(
				ros::Duration(30.0));

		if (finished_before_timeout) {
			actionlib::SimpleClientGoalState state =
					move_base_action_client.getState();
			ROS_INFO("Action finished: %s", state.toString().c_str());
		} else
			ROS_INFO("Action did not finish before the time out.");

	}

	map->remove_bad_points(1);
	map->optimize();

	map->publish_keypoints(pub_keypoints, prefix);

}

void robot_mapper::set_map() {
	rm_localization::SetMap data;
	pcl::toROSMsg(map->keypoints3d, data.request.keypoints3d);

	cv_bridge::CvImage desc;
	desc.image = map->descriptors;
	desc.encoding = sensor_msgs::image_encodings::TYPE_32FC1;
	data.request.descriptors = *(desc.toImageMsg());

	set_map_client.call(data);

	map->publish_pointclouds(octomap_server, prefix);
}

void robot_mapper::move_to_random_point() {

	//while (true) {

		std_srvs::Empty empty;
		clear_unknown_space_client.call(empty);

		move_base_msgs::MoveBaseGoal goal;
		goal.target_pose.header.frame_id = "base_link";
		goal.target_pose.header.stamp = ros::Time::now();

		goal.target_pose.pose.position.x = (((float) rand()) / RAND_MAX - 0.5);
		goal.target_pose.pose.position.y = (((float) rand()) / RAND_MAX - 0.5);
		goal.target_pose.pose.position.z = 0;

		tf::Quaternion q;
		q.setEuler(0, 0, 0);
		tf::quaternionTFToMsg(q, goal.target_pose.pose.orientation);

		move_base_action_client.sendGoal(goal);

		//wait for the action to return
		bool finished_before_timeout = move_base_action_client.waitForResult(
				ros::Duration(30.0));

		if (finished_before_timeout) {
			actionlib::SimpleClientGoalState state =
					move_base_action_client.getState();
			ROS_INFO("Action finished: %s", state.toString().c_str());
			//if (state == actionlib::SimpleClientGoalState::SUCCEEDED)
				//break;
		} else
			ROS_INFO("Action did not finish before the time out.");

	//}

}

void robot_mapper::merge(robot_mapper::Ptr & other) {

	if (map->merge_keypoint_map(*other->map, 50, 5000)) {
		ROS_INFO("Merged maps from robot %d and %d", robot_num,
				other->robot_num);
		map->save(
				"maps/merged_map" + boost::lexical_cast<std::string>(robot_num)
						+ "_"
						+ boost::lexical_cast<std::string>(other->robot_num));

		other->map = map;
		other->merged = true;
		other->visualization_offset = visualization_offset;

	} else {
		ROS_INFO("Could not merge maps from robot %d and %d", robot_num,
				other->robot_num);
	}

}

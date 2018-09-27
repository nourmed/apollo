// Copyright 2018 Baidu Inc. All Rights Reserved.
// @author: Duo Xu(xuduo03@baidu.com)
// @file: radar_obstacle_perception.cc
// @brief: radar perception
#include "modules/perception/radar/app/radar_obstacle_perception.h"

#include <string>
#include <vector>
#include "modules/perception/lib/utils/perf.h"
#include "modules/perception/lib/config_manager/config_manager.h"
#include "modules/perception/lib/registerer/registerer.h"

using apollo::perception::lib::ConfigManager;
using apollo::perception::lib::ModelConfig;

namespace apollo {
namespace perception {
namespace radar {

bool RadarObstaclePerception::Init(const std::string pipeline_name) {
  ConfigManager* config_manager = lib::Singleton<ConfigManager>::get_instance();
  CHECK_NOTNULL(config_manager);
  std::string model_name = pipeline_name;
  const ModelConfig* model_config = NULL;
  CHECK(config_manager->GetModelConfig(model_name, &model_config))
        << "not found model: " << model_name;

  std::string detector_name;
  CHECK(model_config->get_value("Detector", &detector_name))
        << "Detector not found";

  std::string roi_filter_name;
  CHECK(model_config->get_value("RoiFilter", &roi_filter_name))
        << "RoiFilter not found";

  std::string tracker_name;
  CHECK(model_config->get_value("Tracker", &tracker_name))
        << "Tracker not found";

  BaseDetector* detector;
  detector = BaseDetectorRegisterer::GetInstanceByName(detector_name);
  CHECK_NOTNULL(detector);
  detector_.reset(detector);

  BaseRoiFilter* roi_filter =
      BaseRoiFilterRegisterer::GetInstanceByName(roi_filter_name);
  CHECK_NOTNULL(roi_filter);
  roi_filter_.reset(roi_filter);

  BaseTracker* tracker = BaseTrackerRegisterer::GetInstanceByName(tracker_name);
  CHECK_NOTNULL(tracker);
  tracker_.reset(tracker);

  CHECK(detector_->Init()) << "radar detector init error";
  CHECK(roi_filter_->Init()) << "radar roi filter init error";
  CHECK(tracker_->Init()) << "radar tracker init error";

  return true;
}

bool RadarObstaclePerception::Perceive(const ContiRadar& corrected_obstacles,
                                       const RadarPerceptionOptions& options,
                                       std::vector<base::ObjectPtr>* objects) {
  PERCEPTION_PERF_FUNCTION();
  const std::string& sensor_name = options.sensor_name;
  PERCEPTION_PERF_BLOCK_START();
  base::FramePtr detect_frame_ptr(new base::Frame());
  CHECK(detector_->Detect(corrected_obstacles,
                          options.detector_options,
                          detect_frame_ptr)) << "radar detect error";
  AINFO << "Detected frame objects number: "
           << detect_frame_ptr->objects.size();
  PERCEPTION_PERF_BLOCK_END_WITH_INDICATOR(sensor_name, "detector");
  CHECK(roi_filter_->RoiFilter(options.roi_filter_options,
                               detect_frame_ptr)) << "radar roi filter error";
  AINFO << "RoiFiltered frame objects number: "
           << detect_frame_ptr->objects.size();
  PERCEPTION_PERF_BLOCK_END_WITH_INDICATOR(sensor_name, "roi_filter");

  base::FramePtr tracker_frame_ptr(new base::Frame());
  CHECK(tracker_->Track(*detect_frame_ptr,
                        options.track_options,
                        tracker_frame_ptr)) << "radar track error";
  AINFO << "tracked frame objects number: "
           << tracker_frame_ptr->objects.size();
  PERCEPTION_PERF_BLOCK_END_WITH_INDICATOR(sensor_name, "tracker");

  return true;
}

std::string RadarObstaclePerception::Name() const {
  return "RadarObstaclePerception";
}

PERCEPTION_REGISTER_RADAR_OBSTACLE_PERCEPTION(RadarObstaclePerception);

}  // namespace radar
}  // namespace perception
}  // namespace apollo

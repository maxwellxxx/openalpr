/*
 * Copyright (c) 2015 New Designs Unlimited, LLC
 * Opensource Automated License Plate Recognition [http://www.openalpr.com]
 * 
 * This file is part of OpenAlpr.
 * 
 * OpenAlpr is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License 
 * version 3 as published by the Free Software Foundation 
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

using namespace std;

namespace alpr
{

  Config::Config(const std::string country, const std::string config_file, const std::string runtime_dir)
  {

    string debug_message = "";

    this->loaded = false;


    string configFile;

    char* envConfigFile;
    envConfigFile = getenv (ENV_VARIABLE_CONFIG_FILE);
    if (config_file.compare("") != 0)
    {
        // User has supplied a config file.  Use that.
      configFile = config_file;
      debug_message = "Config file location provided via API";
    }
    else if (envConfigFile != NULL)
    {
      // Environment variable is non-empty.  Use that.
      configFile = envConfigFile;
      debug_message = "Config file location provided via environment variable: " + string(ENV_VARIABLE_CONFIG_FILE);
    }
    else if (DirectoryExists(getExeDir().c_str()) && fileExists((getExeDir() + CONFIG_FILE).c_str()))
    {
          configFile = getExeDir() + CONFIG_FILE;
      debug_message = "Config file location provided via exe location";
    }
    else
    {
      // Use the default
      configFile = DEFAULT_CONFIG_FILE;
      debug_message = "Config file location provided via default location";
    }

    //string configFile = (this->runtimeBaseDir + CONFIG_FILE);

    if (fileExists(configFile.c_str()) == false)
    {
      std::cerr << "--(!) Config file '" << configFile << "' does not exist!" << endl;
      std::cerr << "--(!)             You can specify the configuration file location via the command line " << endl;
      std::cerr << "--(!)             or by setting the environment variable '" << ENV_VARIABLE_CONFIG_FILE << "'" << endl;
      return;
    }
    else if (DirectoryExists(configFile.c_str()))
    {
      std::cerr << "--(!) Config file '" << configFile << "' was specified as a directory, rather than a file!" << endl;
      std::cerr << "--(!)             Please specify the full path to the 'openalpr.conf file'" << endl;
      std::cerr << "--(!)             e.g., /etc/openalpr/openalpr.conf" << endl;
      return;
    }


    this->country = country;


    loadCommonValues(configFile);

    if (runtime_dir.compare("") != 0)
    {
      // User provided a runtime directory directly into the library.  Use this.
      this->runtimeBaseDir = runtime_dir;
    }

    if ((DirectoryExists(this->runtimeBaseDir.c_str()) == false) &&
            (DirectoryExists((getExeDir() + RUNTIME_DIR).c_str())))
    {
            // Runtime dir in the config is invalid and there is a runtime dir in the same dir as the exe.
      this->runtimeBaseDir = getExeDir() + RUNTIME_DIR;

    }

    if (DirectoryExists(this->runtimeBaseDir.c_str()) == false)
    {
      std::cerr << "--(!) Runtime directory '" << this->runtimeBaseDir << "' does not exist!" << endl;
      std::cerr << "--(!)                   Please update the OpenALPR config file: '" << configFile << "'" << endl;
      std::cerr << "--(!)                   to point to the correct location of your runtime_dir" << endl;
      return;
    }

    std::string country_config_file = this->runtimeBaseDir + "/config/" + country + ".conf";
    if (fileExists(country_config_file.c_str()) == false)
    {
      std::cerr << "--(!) Country config file '" << country_config_file << "' does not exist.  Missing config for the country: '" << country<< "'!" << endl;
      return;
    }
    
    loadCountryValues(country_config_file, country);

    if (fileExists((this->runtimeBaseDir + "/ocr/tessdata/" + this->ocrLanguage + ".traineddata").c_str()) == false)
    {
      std::cerr << "--(!) Runtime directory '" << this->runtimeBaseDir << "' is invalid.  Missing OCR data for the country: '" << country<< "'!" << endl;
      return;
    }
    
    if (this->debugGeneral)
    {
      std::cout << debug_message << endl;
    }

    this->loaded = true;
  }
  Config::~Config()
  {
    
  }

  void Config::loadCommonValues(string configFile)
  {

    CSimpleIniA iniObj;
    iniObj.LoadFile(configFile.c_str());
    CSimpleIniA* ini = &iniObj;
    
    runtimeBaseDir = getString(ini, "", "runtime_dir", "/usr/share/openalpr/runtime_data");

    std::string detectorString = getString(ini, "", "detector", "lbpcpu");
    std::transform(detectorString.begin(), detectorString.end(), detectorString.begin(), ::tolower);

    if (detectorString.compare("lbpcpu") == 0)
      detector = DETECTOR_LBP_CPU;
    else if (detectorString.compare("lbpgpu") == 0)
      detector = DETECTOR_LBP_GPU;
    else if (detectorString.compare("morphcpu") == 0)
      detector = DETECTOR_MORPH_CPU;
    else
    {
      std::cerr << "Invalid detector specified: " << detectorString << ".  Using default" << std::endl;
      detector = DETECTOR_LBP_CPU;
    }
    
    detection_iteration_increase = getFloat(ini, "", "detection_iteration_increase", 1.1);
    detectionStrictness = getInt(ini, "", "detection_strictness", 3);
    maxPlateWidthPercent = getFloat(ini, "", "max_plate_width_percent", 100);
    maxPlateHeightPercent = getFloat(ini, "", "max_plate_height_percent", 100);
    maxDetectionInputWidth = getInt(ini, "", "max_detection_input_width", 1280);
    maxDetectionInputHeight = getInt(ini, "", "max_detection_input_height", 768);

    skipDetection = getBoolean(ini, "", "skip_detection", false);
    
    prewarp = getString(ini, "", "prewarp", "");
            
    maxPlateAngleDegrees = getInt(ini, "", "max_plate_angle_degrees", 15);


    ocrImagePercent = getFloat(ini, "", "ocr_img_size_percent", 100);
    stateIdImagePercent = getFloat(ini, "", "state_id_img_size_percent", 100);

    ocrMinFontSize = getInt(ini, "", "ocr_min_font_point", 100);

    postProcessMinConfidence = getFloat(ini, "", "postprocess_min_confidence", 100);
    postProcessConfidenceSkipLevel = getFloat(ini, "", "postprocess_confidence_skip_level", 100);
    postProcessMinCharacters = getInt(ini, "", "postprocess_min_characters", 100);
    postProcessMaxCharacters = getInt(ini, "", "postprocess_max_characters", 100);

    debugGeneral = 	getBoolean(ini, "", "debug_general",		false);
    debugTiming = 	getBoolean(ini, "", "debug_timing",		false);
    debugPrewarp = 	getBoolean(ini, "", "debug_prewarp",		false);
    debugDetector = 	getBoolean(ini, "", "debug_detector",		false);
    debugStateId = 	getBoolean(ini, "", "debug_state_id",		false);
    debugPlateLines = 	getBoolean(ini, "", "debug_plate_lines", 	false);
    debugPlateCorners = 	getBoolean(ini, "", "debug_plate_corners", 	false);
    debugCharSegmenter = 	getBoolean(ini, "", "debug_char_segment", 	false);
    debugCharAnalysis =	getBoolean(ini, "", "debug_char_analysis",	false);
    debugColorFiler = 	getBoolean(ini, "", "debug_color_filter", 	false);
    debugOcr = 		getBoolean(ini, "", "debug_ocr", 		false);
    debugPostProcess = 	getBoolean(ini, "", "debug_postprocess", 	false);
    debugShowImages = 	getBoolean(ini, "", "debug_show_images",	false);
    debugPauseOnFrame = 	getBoolean(ini, "", "debug_pause_on_frame",	false);

  }
  
  
  void Config::loadCountryValues(string configFile, string country)
  {
    CSimpleIniA iniObj;
    iniObj.LoadFile(configFile.c_str());
    CSimpleIniA* ini = &iniObj;
    
    minPlateSizeWidthPx = getInt(ini, "", "min_plate_size_width_px", 100);
    minPlateSizeHeightPx = getInt(ini, "", "min_plate_size_height_px", 100);

    multiline = 	getBoolean(ini, "", "multiline",		false);

    plateWidthMM = getFloat(ini, "", "plate_width_mm", 100);
    plateHeightMM = getFloat(ini, "", "plate_height_mm", 100);

    charHeightMM = getFloat(ini, "", "char_height_mm", 100);
    charWidthMM = getFloat(ini, "", "char_width_mm", 100);
    charWhitespaceTopMM = getFloat(ini, "", "char_whitespace_top_mm", 100);
    charWhitespaceBotMM = getFloat(ini, "", "char_whitespace_bot_mm", 100);

    templateWidthPx = getInt(ini, "", "template_max_width_px", 100);
    templateHeightPx = getInt(ini, "", "template_max_height_px", 100);

    charAnalysisMinPercent = getFloat(ini, "", "char_analysis_min_pct", 0);
    charAnalysisHeightRange = getFloat(ini, "", "char_analysis_height_range", 0);
    charAnalysisHeightStepSize = getFloat(ini, "", "char_analysis_height_step_size", 0);
    charAnalysisNumSteps = getInt(ini, "", "char_analysis_height_num_steps", 0);

    segmentationMinBoxWidthPx = getInt(ini, "", "segmentation_min_box_width_px", 0);
    segmentationMinCharHeightPercent = getFloat(ini, "", "segmentation_min_charheight_percent", 0);
    segmentationMaxCharWidthvsAverage = getFloat(ini, "", "segmentation_max_segment_width_percent_vs_average", 0);

    plateLinesSensitivityVertical = getFloat(ini, "", "plateline_sensitivity_vertical", 0);
    plateLinesSensitivityHorizontal = getFloat(ini, "", "plateline_sensitivity_horizontal", 0);

    ocrLanguage = getString(ini, "", "ocr_language", "none");
    
    ocrImageWidthPx = round(((float) templateWidthPx) * ocrImagePercent);
    ocrImageHeightPx = round(((float)templateHeightPx) * ocrImagePercent);
    stateIdImageWidthPx = round(((float)templateWidthPx) * stateIdImagePercent);
    stateIdimageHeightPx = round(((float)templateHeightPx) * stateIdImagePercent);
    
  }

  void Config::debugOff()
  {
    debugGeneral = 	false;
    debugTiming = 	false;
    debugStateId = 	false;
    debugPlateLines = 	false;
    debugPlateCorners = 	false;
    debugCharSegmenter = 	false;
    debugCharAnalysis =	false;
    debugColorFiler = 	false;
    debugOcr = 		false;
    debugPostProcess = 	false;
    debugPauseOnFrame = 	false;
  }


  string Config::getCascadeRuntimeDir()
  {
    return this->runtimeBaseDir + CASCADE_DIR;
  }
  string Config::getKeypointsRuntimeDir()
  {
    return this->runtimeBaseDir + KEYPOINTS_DIR;
  }
  string Config::getPostProcessRuntimeDir()
  {
    return this->runtimeBaseDir + POSTPROCESS_DIR;
  }
  string Config::getTessdataPrefix()
  {
    return this->runtimeBaseDir + "/ocr/";
  }




  float Config::getFloat(CSimpleIniA* ini, string section, string key, float defaultValue)
  {
    const char * pszValue = ini->GetValue(section.c_str(), key.c_str(), NULL /*default*/);
    if (pszValue == NULL)
    {
      if (this->debugGeneral)
        std::cout << "Warning: missing configuration entry for: " << section << "->" << key << endl;
      return defaultValue;
    }

    float val = atof(pszValue);
    return val;
  }
  int Config::getInt(CSimpleIniA* ini, string section, string key, int defaultValue)
  {
    const char * pszValue = ini->GetValue(section.c_str(), key.c_str(), NULL /*default*/);
    if (pszValue == NULL)
    {
      if (this->debugGeneral)
        std::cout << "Warning: missing configuration entry for: " << section << "->" << key << endl;
      return defaultValue;
    }

    int val = atoi(pszValue);
    return val;
  }
  bool Config::getBoolean(CSimpleIniA* ini, string section, string key, bool defaultValue)
  {
    const char * pszValue = ini->GetValue(section.c_str(), key.c_str(), NULL /*default*/);
    if (pszValue == NULL)
    {
      if (this->debugGeneral)
        std::cout << "Warning: missing configuration entry for: " << section << "->" << key << endl;
      return defaultValue;
    }

    int val = atoi(pszValue);
    return val != 0;
  }
  string Config::getString(CSimpleIniA* ini, string section, string key, string defaultValue)
  {
    const char * pszValue = ini->GetValue(section.c_str(), key.c_str(), NULL /*default*/);
    if (pszValue == NULL)
    {
      if (this->debugGeneral)
        std::cout << "Warning: missing configuration entry for: " << section << "->" << key << endl;
      return defaultValue;
    }

    string val = string(pszValue);
    return val;
  }
}
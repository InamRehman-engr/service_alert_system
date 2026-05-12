#include "cxx_include/esp_modem_gnss_parser.hpp"

constexpr auto const TAG = "GNSS Parser";

void printGpsStruct(const esp_modem::gps &gpsData) {
  std::cout << "Latitude: " << gpsData.latitude << " degrees" << std::endl;
  std::cout << "Longitude: " << gpsData.longitude << " degrees" << std::endl;
  std::cout << "Altitude: " << gpsData.altitude << " meters" << std::endl;
  std::cout << "Run Type: " << static_cast<int>(gpsData.run)
            << std::endl; // Assuming GpsRunType is an enum class
  std::cout << "Fix Type: " << static_cast<int>(gpsData.fix)
            << std::endl; // Assuming GpsFixType is an enum class
  std::cout << "Fix Mode: " << static_cast<int>(gpsData.fix_mode)
            << std::endl; // Assuming GpsFixMode is an enum class
  std::cout << "GLONASS Satellites in Use: "
            << static_cast<int>(gpsData.glonass_sats_in_use) << std::endl;
  std::cout << "GNSS Satellites in Use: "
            << static_cast<int>(gpsData.gnss_sats_in_use) << std::endl;
  std::cout << "GNSS Satellites in View: "
            << static_cast<int>(gpsData.gnss_sats_in_view) << std::endl;
  std::cout << "UTC Time: " << static_cast<int>(gpsData.tim.hour) << ":"
            << static_cast<int>(gpsData.tim.minute) << ":"
            << static_cast<int>(gpsData.tim.second) << "."
            << gpsData.tim.thousand << std::endl;
  std::cout << "Date: " << static_cast<int>(gpsData.date.day) << "/"
            << static_cast<int>(gpsData.date.month) << "/" << gpsData.date.year
            << std::endl;
  std::cout << "Horizontal DOP: " << gpsData.dop_h << std::endl;
  std::cout << "Position DOP: " << gpsData.dop_p << std::endl;
  std::cout << "Vertical DOP: " << gpsData.dop_v << std::endl;
  std::cout << "Speed: " << gpsData.speed << " knots" << std::endl;
  std::cout << "Course over Ground: " << gpsData.cog << " degrees" << std::endl;
  std::cout << "Atmospheric Pressure (Sea Level): " << gpsData.hpa << " hPa"
            << std::endl;
  std::cout << "Atmospheric Pressure (Current Location): " << gpsData.vpa
            << " hPa" << std::endl;
  std::cout << "Carrier to Noise Density Ratio (C/No) " << gpsData.carrier_noise
            << " dBHz" << std::endl;
  std::cout << "Direction " << gpsData.direction << std::endl;
}

esp_modem::command_result
get_gnss_information_sim70xx_lib(std::string_view out, esp_modem::gps &gps) {
  constexpr std::string_view pattern = "+CGNSINF: ";
  if (out.find(pattern) == std::string_view::npos) {
    return esp_modem::command_result::FAIL;
  }
  /**
   * Parsing +CGNSINF:
   * <GNSS run status>,
   * <Fix status>,
   * <UTC date &  Time>,
   * <Latitude>,
   * <Longitude>,
   * <MSL Altitude>,
   * <Speed Over Ground>,
   * <Course Over Ground>,
   * <Fix Mode>,
   * <Reserved1>,
   * <HDOP>,
   * <PDOP>,
   * <VDOP>,
   * <Reserved2>,
   * <GNSS Satellites in View>,
   * <Reserved3>,
   * <HPA>,
   * <VPA>
   */
  out = out.substr(pattern.size());
  int pos = 0;
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // GNSS run status
  int GNSS_run_status;
  if (std::from_chars(out.data(), out.data() + pos, GNSS_run_status).ec ==
      std::errc::invalid_argument) {
    return esp_modem::command_result::FAIL;
  }
  gps.run = (esp_modem::GpsRunType)GNSS_run_status;
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Fix status
  {
    std::string_view fix_status = out.substr(0, pos);
    if (fix_status.length() > 1) {
      int Fix_status;
      if (std::from_chars(out.data(), out.data() + pos, Fix_status).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      gps.fix = (esp_modem::GpsFixType)Fix_status;
    } else {
      gps.fix = (esp_modem::GpsFixType)GPS_FIX_INVALID;
    }
  } // clean up Fix status
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // UTC date &  Time
  {
    std::string_view UTC_date_and_Time = out.substr(0, pos);
    if (UTC_date_and_Time.length() > 1) {
      if (std::from_chars(out.data() + 0, out.data() + 4, gps.date.year).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 4, out.data() + 6, gps.date.month).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 6, out.data() + 8, gps.date.day).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 8, out.data() + 10, gps.tim.hour).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 10, out.data() + 12, gps.tim.minute)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 12, out.data() + 14, gps.tim.second)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 15, out.data() + 18, gps.tim.thousand)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
    } else {
      gps.date.year = 0;
      gps.date.month = 0;
      gps.date.day = 0;
      gps.tim.hour = 0;
      gps.tim.minute = 0;
      gps.tim.second = 0;
      gps.tim.thousand = 0;
    }

  } // clean up UTC date &  Time
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Latitude
  {
    std::string_view Latitude = out.substr(0, pos);
    if (Latitude.length() > 1) {
      gps.latitude = std::stof(std::string(out.substr(0, pos)));
    } else {
      gps.latitude = 0;
    }
  } // clean up Latitude
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Longitude
  {
    std::string_view Longitude = out.substr(0, pos);
    if (Longitude.length() > 1) {
      gps.longitude = std::stof(std::string(out.substr(0, pos)));
    } else {
      gps.longitude = 0;
    }
  } // clean up Longitude
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Altitude
  {
    std::string_view Altitude = out.substr(0, pos);
    if (Altitude.length() > 1) {
      gps.altitude = std::stof(std::string(out.substr(0, pos)));
    } else {
      gps.altitude = 0;
    }
  } // clean up Altitude
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Speed Over Ground Km/hour
  {
    std::string_view gps_speed = out.substr(0, pos);
    if (gps_speed.length() > 1) {
      gps.speed = std::stof(std::string(gps_speed));
    } else {
      gps.speed = 0;
    }
  } // clean up gps_speed
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Course Over Ground degrees
  {
    std::string_view gps_cog = out.substr(0, pos);
    if (gps_cog.length() > 1) {
      gps.cog = std::stof(std::string(gps_cog));
    } else {
      gps.cog = 0;
    }
  } // clean up gps_cog
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Fix Mode
  {
    std::string_view FixModesubstr = out.substr(0, pos);
    if (FixModesubstr.length() >= 1) {
      int Fix_Mode;
      if (std::from_chars(out.data(), out.data() + pos, Fix_Mode).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      gps.fix_mode = (esp_modem::GpsFixMode)Fix_Mode;
    } else {
      gps.fix_mode = (esp_modem::GpsFixMode)GPS_MODE_INVALID;
    }
  } // clean up Fix Mode
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // HDOP
  {
    std::string_view HDOP = out.substr(0, pos);
    if (HDOP.length() > 1) {
      gps.dop_h = std::stof(std::string(HDOP));
    } else {
      gps.dop_h = 0;
    }
  } // clean up HDOP
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // PDOP
  {
    std::string_view PDOP = out.substr(0, pos);
    if (PDOP.length() > 1) {
      gps.dop_p = std::stof(std::string(PDOP));
    } else {
      gps.dop_p = 0;
    }
  } // clean up PDOP
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // VDOP
  {
    std::string_view VDOP = out.substr(0, pos);
    if (VDOP.length() > 1) {
      gps.dop_v = std::stof(std::string(VDOP));
    } else {
      gps.dop_v = 0;
    }
  } // clean up VDOP
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // gnss_sats_in_view
  {
    std::string_view gnss_sats_in_view = out.substr(0, pos);
    if (gnss_sats_in_view.length() > 1) {
      if (std::from_chars(out.data(), out.data() + pos, gps.gnss_sats_in_view)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
    } else {
      gps.gnss_sats_in_view = 0;
    }
  } // clean up gnss_sats_in_view

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // HPA
  {
    std::string_view HPA = out.substr(0, pos);
    if (HPA.length() > 1) {
      gps.hpa = std::stof(std::string(HPA));
    } else {
      gps.hpa = 0;
    }
  } // clean up HPA
  out = out.substr(pos + 1);
  // VPA
  {
    std::string_view VPA = out.substr(0, pos);
    if (VPA.length() > 1) {
      gps.vpa = std::stof(std::string(VPA));
    } else {
      gps.vpa = 0;
    }
  } // clean up VPA
  // printGpsStruct(gps);
  return esp_modem::command_result::OK;
}

esp_modem::command_result get_gnss_information_sim868_lib(std::string_view out,
                                                          esp_modem::gps &gps) {
  constexpr std::string_view pattern = "+CGNSINF: ";
  if (out.find(pattern) == std::string_view::npos) {
    return esp_modem::command_result::FAIL;
  }
  /**
   * Parsing +CGNSINF:
   * <GNSS run status>,
   * <Fix status>,
   * <UTC date &  Time>,
   * <Latitude>,
   * <Longitude>,
   * <MSL Altitude>,
   * <Speed Over Ground>,
   * <Course Over Ground>,
   * <Fix Mode>,
   * <Reserved1>,
   * <HDOP>,
   * <PDOP>,
   * <VDOP>,
   * <Reserved2>,
   * <GNSS Satellites in View>,
   * <GNSS Satellites in Use>,
   * <Glonass Satellites in Use>,
   * <Reserved3>,
   * <C/N0max>,
   * <HPA>,
   * <VPA>
   */
  out = out.substr(pattern.size());
  int pos = 0;
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // GNSS run status
  int GNSS_run_status;
  if (std::from_chars(out.data(), out.data() + pos, GNSS_run_status).ec ==
      std::errc::invalid_argument) {
    return esp_modem::command_result::FAIL;
  }
  gps.run = (esp_modem::GpsRunType)GNSS_run_status;
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Fix status
  {
    std::string_view fix_status = out.substr(0, pos);
    if (fix_status.length() > 1) {
      int Fix_status;
      if (std::from_chars(out.data(), out.data() + pos, Fix_status).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      gps.fix = (esp_modem::GpsFixType)Fix_status;
    } else {
      gps.fix = (esp_modem::GpsFixType)GPS_FIX_INVALID;
    }
  } // clean up Fix status
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // UTC date &  Time
  {
    std::string_view UTC_date_and_Time = out.substr(0, pos);
    if (UTC_date_and_Time.length() > 1) {
      if (std::from_chars(out.data() + 0, out.data() + 4, gps.date.year).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 4, out.data() + 6, gps.date.month).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 6, out.data() + 8, gps.date.day).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 8, out.data() + 10, gps.tim.hour).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 10, out.data() + 12, gps.tim.minute)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 12, out.data() + 14, gps.tim.second)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 15, out.data() + 18, gps.tim.thousand)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
    } else {
      gps.date.year = 0;
      gps.date.month = 0;
      gps.date.day = 0;
      gps.tim.hour = 0;
      gps.tim.minute = 0;
      gps.tim.second = 0;
      gps.tim.thousand = 0;
    }

  } // clean up UTC date &  Time
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Latitude
  {
    std::string_view Latitude = out.substr(0, pos);
    if (Latitude.length() > 1) {
      gps.latitude = std::stof(std::string(out.substr(0, pos)));
    } else {
      gps.latitude = 0;
    }
  } // clean up Latitude
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Longitude
  {
    std::string_view Longitude = out.substr(0, pos);
    if (Longitude.length() > 1) {
      gps.longitude = std::stof(std::string(out.substr(0, pos)));
    } else {
      gps.longitude = 0;
    }
  } // clean up Longitude
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Altitude
  {
    std::string_view Altitude = out.substr(0, pos);
    if (Altitude.length() > 1) {
      gps.altitude = std::stof(std::string(out.substr(0, pos)));
    } else {
      gps.altitude = 0;
    }
  } // clean up Altitude
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Speed Over Ground Km/hour
  {
    std::string_view gps_speed = out.substr(0, pos);
    if (gps_speed.length() > 1) {
      gps.speed = std::stof(std::string(gps_speed));
    } else {
      gps.speed = 0;
    }
  } // clean up gps_speed
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Course Over Ground degrees
  {
    std::string_view gps_cog = out.substr(0, pos);
    if (gps_cog.length() > 1) {
      gps.cog = std::stof(std::string(gps_cog));
    } else {
      gps.cog = 0;
    }
  } // clean up gps_cog
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Fix Mode
  {
    std::string_view FixModesubstr = out.substr(0, pos);
    if (FixModesubstr.length() >= 1) {
      int Fix_Mode;
      if (std::from_chars(out.data(), out.data() + pos, Fix_Mode).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      gps.fix_mode = (esp_modem::GpsFixMode)Fix_Mode;
    } else {
      gps.fix_mode = (esp_modem::GpsFixMode)GPS_MODE_INVALID;
    }
  } // clean up Fix Mode
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // HDOP
  {
    std::string_view HDOP = out.substr(0, pos);
    if (HDOP.length() > 1) {
      gps.dop_h = std::stof(std::string(HDOP));
    } else {
      gps.dop_h = 0;
    }
  } // clean up HDOP
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // PDOP
  {
    std::string_view PDOP = out.substr(0, pos);
    if (PDOP.length() > 1) {
      gps.dop_p = std::stof(std::string(PDOP));
    } else {
      gps.dop_p = 0;
    }
  } // clean up PDOP
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // VDOP
  {
    std::string_view VDOP = out.substr(0, pos);
    if (VDOP.length() > 1) {
      gps.dop_v = std::stof(std::string(VDOP));
    } else {
      gps.dop_v = 0;
    }
  } // clean up VDOP
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // gnss_sats_in_view
  {
    std::string_view gnss_sats_in_view = out.substr(0, pos);
    if (gnss_sats_in_view.length() > 1) {
      if (std::from_chars(out.data(), out.data() + pos, gps.gnss_sats_in_view)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
    } else {
      gps.gnss_sats_in_view = 0;
    }
  } // clean up gnss_sats_in_view

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // gnss_sats_used
  {
    std::string_view gnss_sats_in_use = out.substr(0, pos);
    if (gnss_sats_in_use.length() > 1) {
      if (std::from_chars(out.data(), out.data() + pos, gps.gnss_sats_in_use)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
    } else {
      gps.gnss_sats_in_use = 0;
    }
  } // clean up gnss_sats_used

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }

  // glonass_sats_used
  {
    std::string_view glonass_sats_in_use = out.substr(0, pos);
    if (glonass_sats_in_use.length() > 1) {
      if (std::from_chars(out.data(), out.data() + pos, gps.glonass_sats_in_use)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
    } else {
      gps.glonass_sats_in_use = 0;
    }
  } // clean up glonass_sats_used

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }

  // carrier_noise
  {
    std::string_view carrier_noise = out.substr(0, pos);
    if (carrier_noise.length() > 1) {
      if (std::from_chars(out.data(), out.data() + pos, gps.carrier_noise).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
    } else {
      gps.carrier_noise = 0;
    }
  } // clean up carrier_noise

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }

  // HPA
  {
    std::string_view HPA = out.substr(0, pos);
    if (HPA.length() > 1) {
      gps.hpa = std::stof(std::string(HPA));
    } else {
      gps.hpa = 0;
    }
  } // clean up HPA
  out = out.substr(pos + 1);
  // VPA
  {
    std::string_view VPA = out.substr(0, pos);
    if (VPA.length() > 1) {
      gps.vpa = std::stof(std::string(VPA));
    } else {
      gps.vpa = 0;
    }
  } // clean up VPA
  // printGpsStruct(gps);
  return esp_modem::command_result::OK;
}

esp_modem::command_result
get_gnss_information_sim76xx_lib(std::string_view out, esp_modem::gps &gps) {
  constexpr std::string_view pattern = "+CGNSSINFO: ";
  if (out.find(pattern) == std::string_view::npos) {
    return esp_modem::command_result::FAIL;
  }

  /**
   * Parsing +CGNSSINFO:
   * <Fix Mode>
   * <Valid GPS Satellite>
   * <Valid Glonass Satellite>
   * <Valid BEIDOU Satellite>
   * <Latitude>,
   * <N/S>,
   * <Longitude>,
   * <E/W>,
   * <date ddmmyy>,
   * <UTC Time hhmmss.s>,
   * <MSL Altitude>,
   * <Speed Over Ground>,
   * <Course Over Ground>,
   * <PDOP>,
   * <HDOP>,
   * <VDOP>,
   */

  out = out.substr(pattern.size());
  int pos = 0;
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Fix Mode
  {
    std::string_view FixModesubstr = out.substr(0, pos);
    if (FixModesubstr.length() >= 1) {
      int Fix_Mode;
      if (std::from_chars(out.data(), out.data() + pos, Fix_Mode).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      gps.fix_mode = (esp_modem::GpsFixMode)Fix_Mode;
    } else {
      gps.fix_mode = (esp_modem::GpsFixMode)GPS_MODE_INVALID;
    }
  } // clean up Fix Mode

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Valid GPS Satellite in view
  {
    std::string_view ValidGPSsubstr = out.substr(0, pos);
    if (ValidGPSsubstr.length() >= 1) {
      gps.gnss_sats_in_view = std::stoi(std::string(out.substr(0, pos)));
    } else {
      gps.gnss_sats_in_view = 0;
    }
  }

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Valid Glonass Satellite in View
  {
    std::string_view ValidGlonasssubstr = out.substr(0, pos);
    if (ValidGlonasssubstr.length() >= 1) {
      gps.glonass_sats_in_view = std::stoi(std::string(out.substr(0, pos)));
    } else {
      gps.glonass_sats_in_view = 0;
    }
  }

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Valid BEIDOU in view Satellite
  {
    std::string_view validBeidouSats = out.substr(0, pos);
    if (validBeidouSats.length() >= 1) {
      gps.beidou_sats_in_view = std::stoi(std::string(out.substr(0, pos)));
    } else {
      gps.beidou_sats_in_view = 0;
    }
  }

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Latitude
  {
    std::string_view Latitude = out.substr(0, pos);
    if (Latitude.length() > 1) {
      double lat = std::stod(std::string(out.substr(0, pos)));
      int degrees = static_cast<int>(lat / 100);
      double minutes = lat - degrees * 100;
      gps.latitude = degrees + minutes / 60;
    } else {
      gps.latitude = 0;
    }
  } // clean up Latitude

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // North/South Direction
  int NW_ind = 0;
  {
    std::string_view north_south = out.substr(0, pos);
    if (north_south.length() > 1) {
      out[pos] == 'N' ? NW_ind = 1 : NW_ind = 0;
    } else {
      NW_ind = -1;
    }
  } // clean up North/South Direction

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Longitude
  {
    std::string_view Longitude = out.substr(0, pos);
    if (Longitude.length() > 1) {
      double lon = std::stod(std::string(out.substr(0, pos)));
      int degrees = static_cast<int>(lon / 100);
      double minutes = lon - degrees * 100;
      gps.longitude = degrees + minutes / 60;
    } else {
      gps.longitude = 0;
    }
  } // clean up Longitude

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // East/West Direction
  {
    std::string_view east_west = out.substr(0, pos);
    if (east_west.length() > 1) {
      if (NW_ind == 1) {
        out[pos] == 'E'
            ? gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_NE
            : gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_NW;
      } else if (NW_ind == 0) {
        out[pos] == 'E'
            ? gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_SE
            : gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_SW;
      } else {
        gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_NONE;
      }
      // gps.direction.east_west_indicator = out[pos];
    } else {
      // gps.direction.east_west_indicator = '\0';
      gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_NONE;
    }
  } // clean up East/West Direction

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // date
  {
    std::string_view UTC_date = out.substr(0, pos);
    if (UTC_date.length() > 1) {
      if (std::from_chars(out.data() + 0, out.data() + 2, gps.date.day).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 2, out.data() + 4, gps.date.month).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 4, out.data() + 6, gps.date.year).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      gps.date.year += 2000;
    } else {
      gps.date.year = 0;
      gps.date.month = 0;
      gps.date.day = 0;
    }
  } // clean up UTC date

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // UTC Time
  {
    std::string_view UTC_time = out.substr(0, pos);
    if (UTC_time.length() > 1) {
      if (std::from_chars(out.data() + 0, out.data() + 2, gps.tim.hour).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 2, out.data() + 4, gps.tim.minute).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 4, out.data() + 6, gps.tim.second).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 7, out.data() + pos, gps.tim.second)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
    } else {
      gps.tim.hour = 0;
      gps.tim.minute = 0;
      gps.tim.second = 0;
      gps.tim.thousand = 0;
    }
  } // clean up UTC time

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Altitude
  {
    std::string_view Altitude = out.substr(0, pos);
    if (Altitude.length() > 1) {
      gps.altitude = std::stof(std::string(out.substr(0, pos)));
    } else {
      gps.altitude = 0;
    }
  } // clean up Altitude

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Speed Over Ground Km/hour
  {
    std::string_view gps_speed = out.substr(0, pos);
    if (gps_speed.length() > 1) {
      gps.speed = std::stof(std::string(gps_speed));
    } else {
      gps.speed = 0;
    }
  } // clean up gps_speed

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Course Over Ground degrees
  {
    std::string_view gps_cog = out.substr(0, pos);
    if (gps_cog.length() > 1) {
      gps.cog = std::stof(std::string(gps_cog));
    } else {
      gps.cog = 0;
    }
  } // clean up gps_cog

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // PDOP
  {
    std::string_view PDOP = out.substr(0, pos);
    if (PDOP.length() > 1) {
      gps.dop_p = std::stof(std::string(PDOP));
    } else {
      gps.dop_p = 0;
    }
  } // clean up PDOP

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // HDOP
  {
    std::string_view HDOP = out.substr(0, pos);
    if (HDOP.length() > 1) {
      gps.dop_h = std::stof(std::string(HDOP));
    } else {
      gps.dop_h = 0;
    }
  } // clean up HDOP

  out = out.substr(pos + 1);
  // VDOP
  {
    std::string_view VDOP = out.substr(0, pos);
    if (VDOP.length() > 1) {
      gps.dop_v = std::stof(std::string(VDOP));
    } else {
      gps.dop_v = 0;
    }
  } // clean up VDOP

  // printGpsStruct(gps);
  return esp_modem::command_result::OK;
}

esp_modem::command_result get_gnss_information_A76XX_lib(std::string_view out,
                                                         esp_modem::gps &gps) {
  constexpr std::string_view pattern = "+CGPSINFO: ";
  if (out.find(pattern) == std::string_view::npos) {
    return esp_modem::command_result::FAIL;
  }

  /**
   * Parsing +CGPSINFO:
   * <Latitude>,
   * <N/S>,
   * <Longitude>,
   * <E/W>,
   * <date ddmmyy>,
   * <UTC Time hhmmss.s>,
   * <MSL Altitude>,
   * <Speed Over Ground>,
   * <Course Over Ground>,
   */
  out = out.substr(pattern.size());
  int pos = 0;
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Latitude
  {
    std::string_view Latitude = out.substr(0, pos);
    if (Latitude.length() > 1) {
      double lat = std::stod(std::string(out.substr(0, pos)));
      int degrees = static_cast<int>(lat / 100);
      double minutes = lat - degrees * 100;
      gps.latitude = degrees + minutes / 60;
    } else {
      gps.latitude = 0;
    }
  } // clean up Latitude

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // North/South Direction
  int NW_ind = 0;
  {
    std::string_view north_south = out.substr(0, pos);
    if (north_south.length() > 1) {
      out[pos] == 'N' ? NW_ind = 1 : NW_ind = 0;
    } else {
      NW_ind = -1;
    }
  } // clean up North/South Direction

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Longitude
  {
    std::string_view Longitude = out.substr(0, pos);
    if (Longitude.length() > 1) {
      double lon = std::stod(std::string(out.substr(0, pos)));
      int degrees = static_cast<int>(lon / 100);
      double minutes = lon - degrees * 100;
      gps.longitude = degrees + minutes / 60;
    } else {
      gps.longitude = 0;
    }
  } // clean up Longitude

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // East/West Direction
  {
    std::string_view east_west = out.substr(0, pos);
    if (east_west.length() > 1) {
      if (NW_ind == 1) {
        out[pos] == 'E'
            ? gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_NE
            : gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_NW;
      } else if (NW_ind == 0) {
        out[pos] == 'E'
            ? gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_SE
            : gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_SW;
      } else {
        gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_NONE;
      }
      // gps.direction.east_west_indicator = out[pos];
    } else {
      // gps.direction.east_west_indicator = '\0';
      gps.direction = (esp_modem::direction_indicator)GPS_DIRECTION_NONE;
    }
  } // clean up East/West Direction

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // date
  {
    std::string_view UTC_date = out.substr(0, pos);
    if (UTC_date.length() > 1) {
      if (std::from_chars(out.data() + 0, out.data() + 2, gps.date.day).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 2, out.data() + 4, gps.date.month).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 4, out.data() + 6, gps.date.year).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      gps.date.year += 2000;
    } else {
      gps.date.year = 0;
      gps.date.month = 0;
      gps.date.day = 0;
    }
  } // clean up UTC date

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // UTC Time
  {
    std::string_view UTC_time = out.substr(0, pos);
    if (UTC_time.length() > 1) {
      if (std::from_chars(out.data() + 0, out.data() + 2, gps.tim.hour).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 2, out.data() + 4, gps.tim.minute).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 4, out.data() + 6, gps.tim.second).ec ==
          std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
      if (std::from_chars(out.data() + 7, out.data() + pos, gps.tim.thousand)
              .ec == std::errc::invalid_argument) {
        return esp_modem::command_result::FAIL;
      }
    } else {
      gps.tim.hour = 0;
      gps.tim.minute = 0;
      gps.tim.second = 0;
      gps.tim.thousand = 0;
    }
  } // clean up UTC time

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Altitude
  {
    std::string_view Altitude = out.substr(0, pos);
    if (Altitude.length() > 1) {
      gps.altitude = std::stof(std::string(out.substr(0, pos)));
    } else {
      gps.altitude = 0;
    }
  } // clean up Altitude

  out = out.substr(pos + 1);
  if ((pos = out.find(',')) == std::string::npos) {
    return esp_modem::command_result::FAIL;
  }
  // Speed Over Ground Km/hour
  {
    std::string_view gps_speed = out.substr(0, pos);
    if (gps_speed.length() > 1) {
      gps.speed = std::stof(std::string(gps_speed));
    } else {
      gps.speed = 0;
    }
  } // clean up gps_speed

  out = out.substr(pos + 1);
  // Course Over Ground degrees
  {
    std::string_view gps_cog = out.substr(0, pos);
    if (gps_cog.length() > 1) {
      gps.cog = std::stof(std::string(gps_cog));
    } else {
      gps.cog = 0;
    }
  } // clean up gps_cog

  // printGpsStruct(gps);
  return esp_modem::command_result::OK;
}
#pragma once
/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
//
// Created on: 23.08.2022
// Author: franz

#include "cxx_include/esp_modem_api.hpp"
#include "cxx_include/esp_modem_command_library_utils.hpp"
#include "cxx_include/esp_modem_dce.hpp"
#include "cxx_include/esp_modem_dce_factory.hpp"
#include "cxx_include/esp_modem_dce_module.hpp"
#include "cxx_include/esp_modem_dte.hpp"
#include "esp_log.h"
#include "esp_modem_c_api_types.h"
#include "esp_modem_config.h"
#include "sdkconfig.h"
#include <charconv>
#include <iostream>
#include <list>
#include <string_view>

esp_modem::command_result get_gnss_information_sim70xx_lib(std::string_view out,
                                                           esp_modem::gps &gps);
esp_modem::command_result get_gnss_information_sim868_lib(std::string_view out,
                                                          esp_modem::gps &gps);
esp_modem::command_result get_gnss_information_sim76xx_lib(std::string_view out,
                                                           esp_modem::gps &gps);
esp_modem::command_result get_gnss_information_A76XX_lib(std::string_view out,
                                                         esp_modem::gps &gps);
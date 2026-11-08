# Custom OLED status screen (left/central half only): shows both halves' battery.
if(CONFIG_ZMK_DISPLAY AND CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM AND CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/klor_status_both.c)
endif()

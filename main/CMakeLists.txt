# Set usual component variables
set(app_sources "main.c"
"display/display.c"
"rfid/rfid.c"
"json_parser/json_parser.c"
"json_parser/timetable.c"
"json_parser/task.c"
"wifi/wifi_time.c"
"buzzer/buzzer.c"
"led/led.c"

)

idf_component_register(SRCS ${app_sources}
                       INCLUDE_DIRS "." "display" "rfid" "json_parser" "wifi" "buzzer" "led"
                       REQUIRES ulp u8g2 u8g2-hal-esp-idf json nvs_flash esp_wifi driver
                       WHOLE_ARCHIVE
                       )

#
# ULP support additions to component CMakeLists.txt.
#
# 1. The ULP app name must be unique (if multiple components use ULP).
set(ulp_app_name ulp_${COMPONENT_NAME})
#
# 2. Specify all C and Assembly source files.
#    Files should be placed into a separate directory (in this case, ulp/),
#    which should not be added to COMPONENT_SRCS.
set(ulp_sources "ulp/mainLP.c")

#
# 3. List all the component source files which include automatically
#    generated ULP export file, ${ulp_app_name}.h:
set(ulp_exp_dep_srcs ${app_sources})

#
# 4. Call function to build ULP binary and embed in project using the argument
#    values above.
ulp_embed_binary(${ulp_app_name} "${ulp_sources}" "${ulp_exp_dep_srcs}")

idf_component_register(SRCS "setup_portal.cpp" "factory_reset.cpp" "main.cpp" "web_admin.cpp" "ota.cpp" "preferences.cpp" "ui_bridge.cpp" "live.cpp" "settings.cpp" "system_info.cpp" "provision.cpp" "../vendor/waveshare/waveshare_rgb_lcd_port.c" "../../src/panel_model.cpp" "../../src/panel_font_14.c" "../../src/panel_font_20.c"
 INCLUDE_DIRS "compat" "../../include" "../vendor/waveshare"
 EMBED_TXTFILES "root_ca.pem" "ota-public.pem" "web_admin.html" "web_admin.js")

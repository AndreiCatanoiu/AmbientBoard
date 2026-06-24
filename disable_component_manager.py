import os

# Dezactiveaza managerul de componente IDF (nu il folosim: LVGL vine prin
# lib_deps PlatformIO, nu din registry-ul IDF). Evita un bug de configurare
# CMake din platform-espressif32 / idf_component_manager.
os.environ["IDF_COMPONENT_MANAGER"] = "0"

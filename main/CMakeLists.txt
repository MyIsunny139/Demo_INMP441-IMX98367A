idf_component_register(
    SRCS "demo_INMP441.c" "App_Init.c"      
    INCLUDE_DIRS "."
    REQUIRES INMP441 MAX98367A wifi_sta wss_client
    )

deps_config := \
	/Users/adrianreimer/esp/esp-idf/components/app_trace/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/aws_iot/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/bt/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/driver/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/esp32/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/esp_event/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/esp_http_client/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/esp_http_server/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/ethernet/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/fatfs/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/freemodbus/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/freertos/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/heap/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/libsodium/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/log/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/lwip/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/mbedtls/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/mdns/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/mqtt/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/nvs_flash/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/openssl/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/pthread/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/spi_flash/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/spiffs/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/vfs/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/wear_levelling/Kconfig \
	/Users/adrianreimer/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/adrianreimer/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/adrianreimer/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/adrianreimer/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;

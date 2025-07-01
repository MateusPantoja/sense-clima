AVAILABLE_TARGETS = qcx212_0h00
TOOLCHAIN         = GCC
PROJECT_NAME      = Template
BINNAME           = HTNB32L-XXX-Template

TOP := ../..

BUILD_AT = n
BUILD_AT_DEBUG = n
THIRDPARTY_MBEDTLS_ENABLE  = y
MQTT_EXAMPLE = y
BUILD_MQTT_STATIC = y
MQTT_LIBRARY = y
HT_USART_API_ENABLE := y
HT_SPI_API_ENABLE := n
HT_I2C_API_ENABLE := n
DRIVER_USART_ENABLE = y
HT_DEFAULT_LINKER_FILE = y

HT_LIBRARY_MQTT_ENABLE = y
HT_LIBRARY_CJSON_ENABLE = y
UART_UNILOG_ENABLE = y

AZURE_IOT_ENABLE = y

CFLAGS_INC        +=  -I Inc

obj-y             += Src/main.o \
                     Src/HT_BSP_Custom.o \
                     Src/HT_GPIO_Api.o \
                     Src/HT_MQTT_Api.o \
                     Src/HT_SenseClima.o

include $(TOP)/SDK/PLAT/tools/scripts/Makefile.rules


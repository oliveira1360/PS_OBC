<<<<<<< HEAD
# Nome do executável final
TARGET = main.exe
TEST_TARGET = test.exe

# Compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall -Wextra -g -I inc

# Ficheiros fonte
SRC = src/main.c \
      src/app/modeSelecter.c \
      src/app/stateCheck.c \
      src/app/sensors.c \
      src/app/mission.c \
      src/app/init.c \
      src/hal/hal_i2c.c \
      src/hal/hal_gpio.c \
      src/hal/hal_peripherals_init.c \
      src/hal/hal_qspi.c \
      src/hal/hal_spi.c \
      src/hal/hal_usart.c \
      src/hal/hal_system.c \
      src/drivers/i2c_driver.c \
      src/drivers/usart_driver.c \
      src/drivers/spi_driver.c \
      src/peripherals/gnss.c \
      src/peripherals/imu.c \
      src/peripherals/eps.c \
      src/peripherals/pressure.c \
      src/peripherals/propulsor.c \
      src/peripherals/temperature.c \
      src/peripherals/ttc.c

TEST_SRC = test/test_main.c \
           test/unit/drivers/test_i2c_driver.c \
           test/unit/drivers/test_spi_driver.c \
           test/unit/drivers/test_usart_driver.c \
           test/unit/test_i2c_queue.c \
           test/unit/sensors/test_gnss.c \
           test/unit/sensors/test_imu.c \
           test/unit/sensors/test_pressure.c \
           test/unit/sensors/test_temperature.c \
           test/unit/sensors/test_eps.c \
           test/unit/sensors/test_parse_boundary.c \
           test/unit/app/test_init.c \
           test/unit/app/test_modes.c \
           test/unit/app/test_state_machine.c \
           test/integration/test_sensor_pipeline.c \
           test/stress/test_ecss_robustness.c \
           src/hal/hal_i2c.c \
           src/hal/hal_spi.c \
           src/hal/hal_usart.c \
           src/hal/hal_gpio.c \
           src/hal/hal_peripherals_init.c \
           src/hal/hal_qspi.c \
           src/hal/hal_system.c \
           src/drivers/i2c_driver.c \
           src/drivers/spi_driver.c \
           src/drivers/usart_driver.c \
           src/peripherals/gnss.c \
           src/peripherals/imu.c \
           src/peripherals/eps.c \
           src/peripherals/pressure.c \
           src/peripherals/temperature.c \
           src/peripherals/propulsor.c \
           src/peripherals/ttc.c \
           src/app/init.c \
           src/app/modeSelecter.c \
           src/app/stateCheck.c \
           src/app/sensors.c \
           src/app/mission.c

OBJ = $(addprefix build/, $(notdir $(SRC:.c=.o)))

VPATH = src src/app src/hal src/drivers src/peripherals

.PHONY: all test clean create_dir

all: create_dir $(TARGET)

create_dir:
	@if not exist build mkdir build

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

build/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test:
	$(CC) $(CFLAGS) -I test/inc -o $(TEST_TARGET) $(TEST_SRC) -lm
	./$(TEST_TARGET)

clean:
	@if exist build rmdir /s /q build
=======
#
#  There exist several targets which are by default empty and which can be 
#  used for execution of your targets. These targets are usually executed 
#  before and after some main targets. They are: 
#
#     .build-pre:              called before 'build' target
#     .build-post:             called after 'build' target
#     .clean-pre:              called before 'clean' target
#     .clean-post:             called after 'clean' target
#     .clobber-pre:            called before 'clobber' target
#     .clobber-post:           called after 'clobber' target
#     .all-pre:                called before 'all' target
#     .all-post:               called after 'all' target
#     .help-pre:               called before 'help' target
#     .help-post:              called after 'help' target
#
#  Targets beginning with '.' are not intended to be called on their own.
#
#  Main targets can be executed directly, and they are:
#  
#     build                    build a specific configuration
#     clean                    remove built files from a configuration
#     clobber                  remove all built files
#     all                      build all configurations
#     help                     print help mesage
#  
#  Targets .build-impl, .clean-impl, .clobber-impl, .all-impl, and
#  .help-impl are implemented in nbproject/makefile-impl.mk.
#
#  Available make variables:
#
#     CND_BASEDIR                base directory for relative paths
#     CND_DISTDIR                default top distribution directory (build artifacts)
#     CND_BUILDDIR               default top build directory (object files, ...)
#     CONF                       name of current configuration
#     CND_ARTIFACT_DIR_${CONF}   directory of build artifact (current configuration)
#     CND_ARTIFACT_NAME_${CONF}  name of build artifact (current configuration)
#     CND_ARTIFACT_PATH_${CONF}  path to build artifact (current configuration)
#     CND_PACKAGE_DIR_${CONF}    directory of package (current configuration)
#     CND_PACKAGE_NAME_${CONF}   name of package (current configuration)
#     CND_PACKAGE_PATH_${CONF}   path to package (current configuration)
#
# NOCDDL


# Environment 
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib


# build
build: .build-post

.build-pre:
# Add your pre 'build' code here...

.build-post: .build-impl
# Add your post 'build' code here...


# clean
clean: .clean-post

.clean-pre:
# Add your pre 'clean' code here...
# WARNING: the IDE does not call this target since it takes a long time to
# simply run make. Instead, the IDE removes the configuration directories
# under build and dist directly without calling make.
# This target is left here so people can do a clean when running a clean
# outside the IDE.

.clean-post: .clean-impl
# Add your post 'clean' code here...


# clobber
clobber: .clobber-post

.clobber-pre:
# Add your pre 'clobber' code here...

.clobber-post: .clobber-impl
# Add your post 'clobber' code here...


# all
all: .all-post

.all-pre:
# Add your pre 'all' code here...

.all-post: .all-impl
# Add your post 'all' code here...


# help
help: .help-post

.help-pre:
# Add your pre 'help' code here...

.help-post: .help-impl
# Add your post 'help' code here...



# include project implementation makefile
include nbproject/Makefile-impl.mk

# include project make variables
include nbproject/Makefile-variables.mk
>>>>>>> origin/OBC_board

################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-1530502033: ../common.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/sysconfig_1.15.0/sysconfig_cli.bat" --script "C:/Users/caleb/workspace_v12/Irwin_EnhancedSmartThermostat/common.syscfg" -o "syscfg" -s "C:/ti/simplelink_cc32xx_sdk_7_10_00_13/.metadata/product.json" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/ti_drivers_config.c: build-1530502033 ../common.syscfg
syscfg/ti_drivers_config.h: build-1530502033
syscfg/ti_net_config.c: build-1530502033
syscfg/ti_utils_build_linker.cmd.genlibs: build-1530502033
syscfg/syscfg_c.rov.xs: build-1530502033
syscfg/ti_sysbios_config.h: build-1530502033
syscfg/ti_sysbios_config.c: build-1530502033
syscfg/ti_drivers_net_wifi_config.c: build-1530502033
syscfg: build-1530502033

syscfg/%.o: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-armllvm_3.2.2.LTS/bin/tiarmclang.exe" -c -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=none -mlittle-endian -mthumb -Oz -I"C:/Users/caleb/workspace_v12/Irwin_EnhancedSmartThermostat" -I"C:/Users/caleb/workspace_v12/Irwin_EnhancedSmartThermostat/MCU+Image" -I"C:/ti/simplelink_cc32xx_sdk_7_10_00_13/source" -I"C:/ti/simplelink_cc32xx_sdk_7_10_00_13/kernel/tirtos7/packages" -I"C:/ti/simplelink_cc32xx_sdk_7_10_00_13/source/ti/posix/ticlang" -DCC32XX -DDeviceFamily_CC3220 -gdwarf-3 -march=armv7e-m -MMD -MP -MF"syscfg/$(basename $(<F)).d_raw" -MT"$(@)" -I"C:/Users/caleb/workspace_v12/Irwin_EnhancedSmartThermostat/MCU+Image/syscfg"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-armllvm_3.2.2.LTS/bin/tiarmclang.exe" -c -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=none -mlittle-endian -mthumb -Oz -I"C:/Users/caleb/workspace_v12/Irwin_EnhancedSmartThermostat" -I"C:/Users/caleb/workspace_v12/Irwin_EnhancedSmartThermostat/MCU+Image" -I"C:/ti/simplelink_cc32xx_sdk_7_10_00_13/source" -I"C:/ti/simplelink_cc32xx_sdk_7_10_00_13/kernel/tirtos7/packages" -I"C:/ti/simplelink_cc32xx_sdk_7_10_00_13/source/ti/posix/ticlang" -DCC32XX -DDeviceFamily_CC3220 -gdwarf-3 -march=armv7e-m -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)" -I"C:/Users/caleb/workspace_v12/Irwin_EnhancedSmartThermostat/MCU+Image/syscfg"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-437402479: ../image.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/sysconfig_1.15.0/sysconfig_cli.bat" --script "C:/Users/caleb/workspace_v12/Irwin_EnhancedSmartThermostat/image.syscfg" -o "syscfg" -s "C:/ti/simplelink_cc32xx_sdk_7_10_00_13/.metadata/product.json" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/ti_drivers_net_wifi_config.json: build-437402479 ../image.syscfg
syscfg: build-437402479



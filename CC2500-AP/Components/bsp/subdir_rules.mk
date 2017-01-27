################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
Components/bsp/bsp.obj: ../Components/bsp/bsp.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/bin/cl430" --cmd_file="/Users/austin/MIT/2017_Spring/Cube_Bots_UROP/cubebotsurop/Applications/configuration/smpl_nwk_config.dat" --cmd_file="/Users/austin/MIT/2017_Spring/Cube_Bots_UROP/cubebotsurop/Applications/configuration/Access Point/smpl_config_AP.dat"  -vmsp --use_hw_mpy=none --include_path="/Applications/ti/ccsv7/ccs_base/msp430/include" --include_path="/Users/austin/MIT/2017_Spring/Cube_Bots_UROP/cubebotsurop/Components/bsp" --include_path="/Users/austin/MIT/2017_Spring/Cube_Bots_UROP/cubebotsurop/Components/bsp/boards/EZ430RF" --include_path="/Users/austin/MIT/2017_Spring/Cube_Bots_UROP/cubebotsurop/Components/bsp/drivers" --include_path="/Users/austin/MIT/2017_Spring/Cube_Bots_UROP/cubebotsurop/Components/mrfi" --include_path="/Users/austin/MIT/2017_Spring/Cube_Bots_UROP/cubebotsurop/Components/simpliciti/nwk" --include_path="/Users/austin/MIT/2017_Spring/Cube_Bots_UROP/cubebotsurop/Components/simpliciti/nwk_applications" --include_path="/Users/austin/MIT/2017_Spring/Cube_Bots_UROP/cubebotsurop/utils" --include_path="/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/include" --advice:power="all" --define=__MSP430F2274__ -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU19 --preproc_with_compile --preproc_dependency="Components/bsp/bsp.d" --obj_directory="Components/bsp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '



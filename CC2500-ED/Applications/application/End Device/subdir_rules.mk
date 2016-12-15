################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
Applications/application/End\ Device/main_manyEDs.obj: ../Applications/application/End\ Device/main_manyEDs.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"c:/ti/ccsv6/tools/compiler/msp430_4.3.1/bin/cl430" --cmd_file="C:\Users\RobM\Desktop\Documents\CCSworkspace\eZ430-AP_as_data_hub\Applications\configuration\smpl_nwk_config.dat" --cmd_file="C:\Users\RobM\Desktop\Documents\CCSworkspace\eZ430-AP_as_data_hub\Applications\configuration\End Device\smpl_config_ED.dat"  -vmsp -O3 --opt_for_speed=3 --include_path="c:/ti/ccsv6/ccs_base/msp430/include" --include_path="c:/ti/ccsv6/tools/compiler/msp430_4.3.1/include" --include_path="c:/ti/ccsv6/tools/compiler/msp430_4.3.1/include" --include_path="C:/Users/RobM/Desktop/Documents/CCSL_Stuff/Projects/Ribosomal Robotics/SimpliciTI-CCS-1.1.1/Components/bsp" --include_path="C:/Users/RobM/Desktop/Documents/CCSL_Stuff/Projects/Ribosomal Robotics/SimpliciTI-CCS-1.1.1/Components/bsp/boards/EZ430RF" --include_path="C:/Users/RobM/Desktop/Documents/CCSL_Stuff/Projects/Ribosomal Robotics/SimpliciTI-CCS-1.1.1/Components/bsp/drivers" --include_path="C:/Users/RobM/Desktop/Documents/CCSL_Stuff/Projects/Ribosomal Robotics/SimpliciTI-CCS-1.1.1/Components/mrfi" --include_path="C:/Users/RobM/Desktop/Documents/CCSL_Stuff/Projects/Ribosomal Robotics/SimpliciTI-CCS-1.1.1/Components/simpliciti/nwk" --include_path="C:/Users/RobM/Desktop/Documents/CCSL_Stuff/Projects/Ribosomal Robotics/SimpliciTI-CCS-1.1.1/Components/simpliciti/nwk_applications" -g --c99 --define=__MSP430F2274__ --define=MRFI_CC2500 --diag_warning=225 --printf_support=minimal --preproc_with_compile --preproc_dependency="Applications/application/End Device/main_manyEDs.pp" --obj_directory="Applications/application/End Device" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '



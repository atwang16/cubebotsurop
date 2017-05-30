################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Application/application/Access\ Point/main_AP_Async_TalkandListen_.obj: ../Application/application/Access\ Point/main_AP_Async_TalkandListen_.cpp $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/bin/cl430" --cmd_file="C:\Users\milesdai\Documents\tempData\MIT\UROP\cubebots\Application\configuration\smpl_nwk_config.dat" --cmd_file="C:\Users\milesdai\Documents\tempData\MIT\UROP\cubebots\Application\configuration\Access Point\smpl_config_AP.dat"  -vmsp --use_hw_mpy=none --include_path="C:/ti/ccsv7/ccs_base/msp430/include" --include_path="C:/Users/milesdai/Documents/tempData/MIT/UROP/cubebots/Components/bsp" --include_path="C:/Users/milesdai/Documents/tempData/MIT/UROP/cubebots/Components/bsp/boards/EZ430RF" --include_path="C:/Users/milesdai/Documents/tempData/MIT/UROP/cubebots/Components/bsp/drivers" --include_path="C:/Users/milesdai/Documents/tempData/MIT/UROP/cubebots/Components/mrfi" --include_path="C:/Users/milesdai/Documents/tempData/MIT/UROP/cubebots/Components/simpliciti/nwk" --include_path="C:/Users/milesdai/Documents/tempData/MIT/UROP/cubebots/Components/simpliciti/nwk_applications" --include_path="C:/Users/milesdai/Documents/tempData/MIT/UROP/cubebots/utils" --include_path="C:/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/include" --advice:power="all" --define=__MSP430F2274__ -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU19 --preproc_with_compile --preproc_dependency="Application/application/Access Point/main_AP_Async_TalkandListen_.d" --obj_directory="Application/application/Access Point" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '



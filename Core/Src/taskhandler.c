#include "main.h"
#include "string.h"
#include "FreeRTOS.h"
#include "stdio.h"
#include "queue.h"
#include "task.h"

extern QueueHandle_t DataQueue;
extern QueueHandle_t PrintQueue;

extern TaskHandle_t MenuTaskHandle;
extern TaskHandle_t CmdTaskHandle;
extern TaskHandle_t PrintTaskHandle;
extern TaskHandle_t LEDTaskHandle;
extern TaskHandle_t ADCTaskHandle;
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart2;



int extract_cmd(command_t* cmd);


const char* InvalidMessage = "Invalid Option!!!\n";
const char * msg =	"=============================\n"
					"        Main Menu            \n"
					"=============================\n"
					"Toggle LED ---------------  1\n"
					"ADC Value ----------------- 2\n"
					"Exit -----------------------3\n"
					"Enter your choice here: \n";



void MenuTask(void* param){

	command_t* Command;
	uint32_t Command_Address;

	while(1){
		xQueueSend(PrintQueue, &msg, portMAX_DELAY);
		xTaskNotifyWait(0, 0, &Command_Address, portMAX_DELAY);

		Command = (command_t*)Command_Address;

		if(Command->length == 1){
			uint8_t option = Command->command[0]- 48;
			switch(option){
			case 1:
				xTaskNotify(LEDTaskHandle, 0, eNoAction);
				break;
			case 2:

				xTaskNotify(ADCTaskHandle, 0, eNoAction);
				break;
			case 3:
				break;  //exit
			default:
				xQueueSend(PrintQueue, &InvalidMessage, portMAX_DELAY);
				continue;
			}
		}
		else{
			xQueueSend(PrintQueue, &InvalidMessage, portMAX_DELAY);
			continue;
		}

		xTaskNotifyWait(0, 0, NULL,portMAX_DELAY);
	}
}


void CmdHandlerTask(void* param){
	BaseType_t ret;
	command_t cmd;
	for(int i = 0; i < 10; i++){  // initialize command array
		cmd.command[i] = 0;
	}
	cmd.length = 0;  // initialize length
	while(1){
		ret = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		if(ret == pdTRUE){
			int returnValue= extract_cmd(&cmd);
			if(returnValue == -1) Error_Handler();
			xTaskNotify(MenuTaskHandle, (uint32_t)(&cmd), eSetValueWithOverwrite);
		}
	}
}

int extract_cmd(command_t* cmd){
	BaseType_t status;
	uint8_t data;
	uint8_t index = 0;
	//for(int i = 0; i < 10; i++){
	//	cmd->command[i]= 5;  // any number less than 10 and other than 0, 1, 2
//	}
	status = uxQueueMessagesWaiting(DataQueue);
	if(!status) return -1;
	for(int i = 0; i < 10; i++){
		status = xQueueReceive(DataQueue, &data, 0 );
		if(status == pdTRUE){
			if(data == '\n') break;
			cmd->command[i] = data;
			index = i;
		}
	}
	cmd->command[index+1]='\0';
	cmd->length = index + 1;

	return 0;

}

void PrintTask(void* param){
	uint32_t* dataBuffer;
	while(1){
		xQueueReceive(PrintQueue, &dataBuffer, portMAX_DELAY );
		HAL_UART_Transmit(&huart2, (uint8_t*)dataBuffer, strlen((char*)dataBuffer), HAL_MAX_DELAY );

	}
}

void LEDTask(void* param){
	while(1){

		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		xTaskNotify(MenuTaskHandle, 0, eNoAction);

	}
}



void ADCTask(void* param){
	uint16_t rawValue;
	float temp ;

	char msg1[50];

	while(1){

		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		HAL_ADC_Start(&hadc1);
		rawValue = HAL_ADC_GetValue(&hadc1);
		temp = ((float)rawValue) / 4095 * 3300;
		temp = ((temp - 760.0) / 2.5) + 25;

		sprintf(msg1, "ADC Value: %f\n", temp);
		const char* msg = msg1;

		xQueueSend(PrintQueue, (uint32_t*)&msg, portMAX_DELAY);
		HAL_ADC_Stop(&hadc1);
		xTaskNotify(MenuTaskHandle, 0, eNoAction);
	}
}



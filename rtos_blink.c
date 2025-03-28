#include <stdio.h>
#include "semphr.h"
#include "FreeRTOS.h"
#include "task.h"

#define BTN_A_PIN 5
#define LED_PIN 14

QueueHandle_t Queue_data;
SemaphoreHandle_t XMutex;
int button_state = 0;

void setup(){
    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN); 
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

void vButton_Task(void *pvParameters){
    for(;;){
        if(xSemaphoreTake(XMutex, portMAX_DELAY) == pdTRUE){
            button_state  = !gpio_get(BTN_A_PIN);
            xSemaphoreGive(XMutex);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void vButton_processing_task(void *pvParameters){
    int previous_button_state = 0;

    for(;;){
        if(xSemaphoreTake(XMutex, portMAX_DELAY) == pdTRUE){
            if(button_state != previous_button_state){
                previous_button_state = button_state;
                xQueueSend(Queue_data, &button_state, 0);
            }
            xSemaphoreGive(XMutex);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void vLed_blink_task(void *pvParameters){
    int received_button_state;
    
    for(;;){
        if(xQueueReceive(Queue_data, &received_button_state, portMAX_DELAY) == pdTRUE){
            gpio_put(LED_PIN, received_button_state);
        }
    }
}

int main()
{
    stdio_init_all();
    setup();
    
    XMutex = xSemaphoreCreateMutex();
    if (XMutex == NULL){
        printf("Erro ao criar mutex\n");
        return 1;
    }

    Queue_data = xQueueCreate(1, sizeof(int));
    if (Queue_data == NULL){
        printf("Erro ao criar fila\n");
        return 1;
    }

    xTaskCreate(vButton_Task, "Button Task", 128, NULL, 1, NULL);
    xTaskCreate(vButton_processing_task, "Button Processing", 128, NULL, 1, NULL);
    xTaskCreate(vLed_blink_task, "LED Task", 128, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1); // Caso o scheduler retorne (o que não deveria acontecer)
}
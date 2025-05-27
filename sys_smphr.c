#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "pico/bootrom.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"

// Definição dos botões 
#define btn_a 5
#define btn_b 6
#define btn_joy 22

// Definição dos LEDs
#define led_r 13
#define led_g 11
#define led_b 12

// Definição de variáveis do Display
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// Definição do buzzer
#define buzzer_pin 10

// Parametros Globais
#define MAX_USERS 8
ssd1306_t ssd;      

// Handles
SemaphoreHandle_t xSlotSemaphore;    // Semáforo de contagem para vagas
SemaphoreHandle_t xResetSemaphore;    // Semáforo binário para o reset
SemaphoreHandle_t xDisplayMutex;         // Mutex para proteger o acesso ao OLED

// Protóriopo de Tarefas e Funções
void vTaskEntrada(void *params);
void vTaskSaida(void *params);
void vTaskReset(void *params);
void gpio_setup();
void display_setup();
void atualizaLEDs();
void atualizarDisplay();
void gpio_irq_handler(uint gpio, uint32_t events);

// Função principal
int main(){
    gpio_setup();
    display_setup();

    gpio_set_irq_enabled_with_callback(btn_joy, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Criação dos semáforos e mutex
    xSlotSemaphore = xSemaphoreCreateCounting(MAX_USERS, 0);
    xResetSemaphore = xSemaphoreCreateBinary();
    xDisplayMutex = xSemaphoreCreateMutex();

    // Criação das Tasks
    xTaskCreate(vTaskEntrada, "Entrada", 1024, NULL, 1, NULL);
    xTaskCreate(vTaskSaida,   "Saida",   1024, NULL, 1, NULL);
    xTaskCreate(vTaskReset,   "Reset",   1024, NULL, 1, NULL);
    
    vTaskStartScheduler();
    panic_unsupported();
}

// Task de Entrada
void vTaskEntrada(void *params){
    atualizarDisplay();
    atualizaLEDs();

    while (1) {
        if (!gpio_get(btn_a)) {
            vTaskDelay(pdMS_TO_TICKS(200));  // Debounce

            if (uxSemaphoreGetCount(xSlotSemaphore) < MAX_USERS) {
                xSemaphoreGive(xSlotSemaphore);
                atualizaLEDs();
                atualizarDisplay();

            } else {
                pwm_set_gpio_level(buzzer_pin, 60);  
                vTaskDelay(pdMS_TO_TICKS(100));
                pwm_set_gpio_level(buzzer_pin, 0);  

                atualizarDisplay();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Task de Saida
void vTaskSaida(void *params){
    while (1) {
        if (!gpio_get(btn_b)) {
            vTaskDelay(pdMS_TO_TICKS(200));

            if (xSemaphoreTake(xSlotSemaphore, 0) == pdTRUE) {
                
                atualizaLEDs();
                atualizarDisplay();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Task de reset
void vTaskReset(void *params){
    while (1) {
        if (xSemaphoreTake(xResetSemaphore, portMAX_DELAY) == pdTRUE) {
            // Reinicia a contagem esvaziando o semáforo
            while (uxSemaphoreGetCount(xSlotSemaphore) > 0) {
                xSemaphoreTake(xSlotSemaphore, 0);
            }

            // Beep duplo ao resetar 
            pwm_set_gpio_level(buzzer_pin, 60);  
            vTaskDelay(pdMS_TO_TICKS(100));
            pwm_set_gpio_level(buzzer_pin, 0);  
            pwm_set_gpio_level(buzzer_pin, 60);  
            vTaskDelay(pdMS_TO_TICKS(100));
            pwm_set_gpio_level(buzzer_pin, 0);  

            atualizarDisplay();
            atualizaLEDs();
        }
    }
}

// Configuração dos GPIOs
void gpio_setup(){
    // Ativando as GPIOs do LED RGB
    gpio_init(led_r);
    gpio_set_dir(led_r, GPIO_OUT);
    gpio_init(led_g);
    gpio_set_dir(led_g, GPIO_OUT);
    gpio_init(led_b);
    gpio_set_dir(led_b, GPIO_OUT);

    // Ativando os botões
    gpio_init(btn_a);
    gpio_set_dir(btn_a, GPIO_IN);
    gpio_pull_up(btn_a);
    
    gpio_init(btn_b);
    gpio_set_dir(btn_b, GPIO_IN);
    gpio_pull_up(btn_b);

    gpio_init(btn_joy);
    gpio_set_dir(btn_joy, GPIO_IN);
    gpio_pull_up(btn_joy);

    // Configuração do PWM do Buzzer
    gpio_set_function(buzzer_pin, GPIO_FUNC_PWM);
    uint slice_buz = pwm_gpio_to_slice_num(buzzer_pin);
    pwm_set_clkdiv(slice_buz, 40);
    pwm_set_wrap(slice_buz, 12500);
    pwm_set_enabled(slice_buz, true);  
}

// Configuração do display
void display_setup(){
    // Configuração do display
    i2c_init(I2C_PORT, 400 * 1000);                               // I2C Initialisation. Using it at 400Khz.
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display
    
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

// Função para atualizar os LEDs
void atualizaLEDs(){
    if(uxSemaphoreGetCount(xSlotSemaphore) == 0){
        // Liga LED azul
        gpio_put(led_b, true); 
        gpio_put(led_r, false);
        gpio_put(led_g, false);
    } else if (uxSemaphoreGetCount(xSlotSemaphore) < MAX_USERS-1){
        // Liga LED verde
        gpio_put(led_b, false);
        gpio_put(led_g, true);
        gpio_put(led_r, false);
    } else if (uxSemaphoreGetCount(xSlotSemaphore) == MAX_USERS-1){
        // Liga LED amarelo
        gpio_put(led_b, false);
        gpio_put(led_g, true);
        gpio_put(led_r, true);

    } else {
        // Liga LED vermelho
        gpio_put(led_b, false);
        gpio_put(led_g, false);
        gpio_put(led_r, true);
    }
}

// Função para atualizar o display
void atualizarDisplay() {
    if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY)) {
        ssd1306_fill(&ssd, false);

        char buffer[20];
        int usuarios = uxSemaphoreGetCount(xSlotSemaphore);
        sprintf(buffer, "Num. users: %d", usuarios); 

        if (usuarios == MAX_USERS){
            ssd1306_draw_string(&ssd, "Cap. maxima", 1, 35);
        }
        ssd1306_draw_string(&ssd, "Painel de Ctrl", 1, 5);
        ssd1306_draw_string(&ssd, buffer, 1, 20);
        ssd1306_send_data(&ssd);
        vTaskDelay(pdMS_TO_TICKS(100));

        xSemaphoreGive(xDisplayMutex);
    }
}

// Tratamento de interrupções
void gpio_irq_handler(uint gpio, uint32_t events){
    if (gpio == btn_joy) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xResetSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}


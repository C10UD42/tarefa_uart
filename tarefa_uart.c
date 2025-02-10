//Declaração das bibliotecas necessárias
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "numeros.h"
#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define UART_ID uart0

const uint led_pin_green = 11;
const uint led_pin_blue  = 12;
const uint led_pin_red   = 13;
const uint button_A = 5; // Botão A = 5
const uint button_B = 6; //Botão B = 6

uint8_t led_r = 15; // Intensidade do vermelho
uint8_t led_g = 0; // Intensidade do verde
uint8_t led_b = 15; // Intensidade do azul

uint8_t current_number = 0;

bool* numeros[10] = {numeroZero, numeroUm, numeroDois, numeroTres, numeroQuatro, numeroCinco, numeroSeis, numeroSete, numeroOito, numeroNove};

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void set_one_led(uint8_t number, uint8_t r, uint8_t g, uint8_t b)
{
    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(r, g, b);

    // Define todos os LEDs com a cor especificada
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        if (numeros[number][i])
        {
            put_pixel(color); // Liga o LED com um no buffer
        }
        else
        {
            put_pixel(0);  // Desliga os LEDs com zero no buffer
        }
    }
}

bool debouncing(){
    //implementar o debouncing
    static uint32_t last_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time < 200) {
        return false;
    }
    last_time = current_time;
    return true;

}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    if (!debouncing()) 
        return;

    if(gpio == button_A) {
        // Alterna o estado do LED verde (pino 11)
        gpio_put(led_pin_green, !gpio_get(led_pin_green));
        printf("Alterado led verde\n");
    }
    
    if(gpio == button_B) {
        // Alterna o estado do LED azul (pino 12)
        gpio_put(led_pin_blue, !gpio_get(led_pin_blue));
        printf("Alterado led azul\n");
    }
}

int main()
{
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_t ssd; // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    bool cor = true;
    
    stdio_init_all();
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &tarefa_uart_program);

    tarefa_uart_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    gpio_init(led_pin_green);
    gpio_set_dir(led_pin_green, GPIO_OUT);
    gpio_init(led_pin_blue);
    gpio_set_dir(led_pin_blue, GPIO_OUT);
    gpio_init(led_pin_red);
    gpio_set_dir(led_pin_red, GPIO_OUT);
    
    gpio_init(button_A);
    gpio_set_dir(button_A, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(button_A);

    gpio_init(button_B);
    gpio_set_dir(button_B, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(button_B);

    gpio_set_irq_enabled_with_callback(button_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true)
    {
        cor = !cor;
        // Atualiza o conteúdo do display com animações
        ssd1306_fill(&ssd, !cor); // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
        
        if (stdio_usb_connected())
        { 
            char entrada;
            entrada = getchar();
            ssd1306_draw_char(&ssd, entrada, 47, 48);
            if (entrada >= '0' && entrada <= '9'){
                current_number = (uint8_t)(entrada - '0');  // Converte o char para uint8_t
            }
            printf("Número atual: %d\n", current_number);
            set_one_led(current_number, led_r, led_g, led_b);
        }
        if (gpio_get(led_pin_green)){
            ssd1306_draw_string(&ssd, "led verde        aceso", 8, 10); // Desenha uma string
        }
        if (gpio_get(led_pin_blue)){
            ssd1306_draw_string(&ssd, "led azul aceso", 8, 30); // Desenha uma string
        }  
        
        ssd1306_send_data(&ssd); // Atualiza o display

        set_one_led(current_number, led_r, led_g, led_b);
        sleep_ms(100);
    }

    return 0;
}
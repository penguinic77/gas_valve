#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/mcpwm.h"
#include "motor_control.h"

static bool mcpwm_inited = false;

void motor_control(int direction, int speed) {
    if(!mcpwm_inited) {
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_NUM_25); 
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL<<26),
            .mode = GPIO_MODE_OUTPUT,
        };
        gpio_config(&io_conf);

        mcpwm_config_t pwm_config = {
            .frequency = 1000, 
            .cmpr_a = 0,       
            .cmpr_b = 0,       
            .duty_mode = MCPWM_DUTY_MODE_0,
            .counter_mode = MCPWM_UP_COUNTER
        };
        mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
        mcpwm_inited = true;
    }

    if (direction == 1) {
        gpio_set_level(GPIO_NUM_26, 1); 
    } else if (direction == -1) {
        gpio_set_level(GPIO_NUM_26, 0); 
    }

    float duty = speed;
    if(duty>100) duty=100; 
    if(duty<0) duty=0;
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}

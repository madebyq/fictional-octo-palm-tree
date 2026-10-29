#include <stdio.h>

#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"

#include "example_apps.h"
#include "round_protocol.h"

static const char *TAG = "round";
static QueueHandle_t events;
static lv_obj_t *title_label;
static lv_obj_t *body_label;
static lv_obj_t *footer_label;

static void next_app(lv_event_t *event)
{
    (void)event;
    const round_event_t next = {.type = ROUND_EVENT_NEXT};
    if (xQueueSend(events, &next, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Event queue full; tap ignored");
    }
}

static lv_obj_t *make_label(lv_obj_t *screen, int y)
{
    lv_obj_t *label = lv_label_create(screen);
    lv_obj_set_width(label, 300);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, y);
    return label;
}

static void create_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_color(screen, lv_color_white(), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    title_label = make_label(screen, -115);
    body_label = make_label(screen, -20);
    footer_label = make_label(screen, 70);
    lv_obj_t *button = lv_button_create(screen);
    lv_obj_set_size(button, 110, 48);
    lv_obj_align(button, LV_ALIGN_CENTER, 0, 140);
    lv_obj_add_event_cb(button, next_app, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, "Next");
    lv_obj_center(label);
}

static void render(const round_runtime_t *runtime)
{
    round_view_t view;
    if (round_render(runtime, &view)) {
        ESP_ERROR_CHECK(bsp_display_lock(UINT32_MAX));
        lv_label_set_text(title_label, view.title);
        lv_label_set_text(body_label, view.body);
        lv_label_set_text(footer_label, view.footer);
        bsp_display_unlock();
    }
}

static void serial_input(void *argument)
{
    (void)argument;
    round_parser_t parser = {0};
    for (;;) {
        int byte = getchar();
        if (byte == EOF) {
            clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        round_event_t event;
        if (round_parser_feed(&parser, (uint8_t)byte,
                              (uint32_t)(esp_timer_get_time() / 1000), &event)) {
            if (xQueueSend(events, &event, pdMS_TO_TICKS(100)) != pdTRUE) {
                ESP_LOGW(TAG, "Event queue full; command ignored");
            }
        }
    }
}

void app_main(void)
{
    static example_state_t state;
    static round_app_t apps[EXAMPLE_APP_COUNT];
    round_runtime_t runtime;
    example_apps_init(&state, apps);
    ESP_ERROR_CHECK(round_init(&runtime, apps, EXAMPLE_APP_COUNT) ? ESP_OK : ESP_FAIL);
    events = xQueueCreate(16, sizeof(round_event_t));
    ESP_ERROR_CHECK(events ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(bsp_display_start() ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(bsp_display_brightness_set(30));
    ESP_ERROR_CHECK(bsp_display_lock(UINT32_MAX));
    create_ui();
    bsp_display_unlock();
    render(&runtime);
    ESP_ERROR_CHECK(xTaskCreate(serial_input, "serial_input", 4096, NULL, 4, NULL) == pdPASS
                        ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_LOGI(TAG, "Ready: !working, !idle, /app next, @solar <watts> <battery-percent>");
    for (;;) {
        round_event_t event;
        if (xQueueReceive(events, &event, portMAX_DELAY) == pdTRUE &&
            round_dispatch(&runtime, &event)) {
            render(&runtime);
        }
    }
}

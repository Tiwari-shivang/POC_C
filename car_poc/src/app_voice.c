#include "app_voice.h"
#include "hal.h"
#include "platform.h"
#include <string.h>
#include <stdio.h>

#define VOICE_BUFFER_SIZE (64U)
#define WAKE_PHRASE "hey car"
#define MAX_INTENTS (5U)

typedef struct {
    const char* intent;
    const char* response;
} voice_intent_t;

static const voice_intent_t intent_table[MAX_INTENTS] = {
    {"open sunroof", "Opening sunroof"},
    {"close sunroof", "Closing sunroof"},
    {"set temp", "Setting temperature"},
    {"turn on radio", "Turning on radio"},
    {"navigate home", "Navigating to home"}
};

typedef struct {
    char last_command[VOICE_BUFFER_SIZE];
    char last_response[VOICE_BUFFER_SIZE];
} voice_state_t;

static voice_state_t state = {{0}, {0}};

void app_voice_init(void) {
    state.last_command[0] = '\0';
    state.last_response[0] = '\0';
}

static bool starts_with_wake_phrase(const char* input) {
    if (input == NULL) {
        return false;
    }
    
    return (strncmp(input, WAKE_PHRASE, strlen(WAKE_PHRASE)) == 0);
}

static bool find_intent_match(const char* command, char* response, uint16_t response_len) {
    uint8_t i = 0U;
    
    if ((command == NULL) || (response == NULL) || (response_len == 0U)) {
        return false;
    }
    
    for (i = 0U; i < MAX_INTENTS; i++) {
        if (strstr(command, intent_table[i].intent) != NULL) {
            strncpy(response, intent_table[i].response, response_len - 1U);
            response[response_len - 1U] = '\0';
            return true;
        }
    }
    
    strncpy(response, "Command not recognized", response_len - 1U);
    response[response_len - 1U] = '\0';
    return false;
}

static void process_voice_command(const char* command) {
    char response[VOICE_BUFFER_SIZE];
    bool intent_found = false;
    
    if (command == NULL) {
        return;
    }
    
    strncpy(state.last_command, command, VOICE_BUFFER_SIZE - 1U);
    state.last_command[VOICE_BUFFER_SIZE - 1U] = '\0';
    
    if (!starts_with_wake_phrase(command)) {
        strncpy(state.last_response, "Wake phrase not detected", VOICE_BUFFER_SIZE - 1U);
        state.last_response[VOICE_BUFFER_SIZE - 1U] = '\0';
        return;
    }
    
    intent_found = find_intent_match(command + strlen(WAKE_PHRASE), response, VOICE_BUFFER_SIZE);
    
    strncpy(state.last_response, response, VOICE_BUFFER_SIZE - 1U);
    state.last_response[VOICE_BUFFER_SIZE - 1U] = '\0';
    
    printf("Voice: %s -> %s\n", command, response);
}

void app_voice_step(void) {
    char voice_buffer[VOICE_BUFFER_SIZE];
    bool voice_available = false;
    
    voice_available = hal_read_voice_line(voice_buffer, VOICE_BUFFER_SIZE);
    
    if (voice_available) {
        process_voice_command(voice_buffer);
    }
}
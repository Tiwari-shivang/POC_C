#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int Unity_NumberOfTests = 0;
static int Unity_TestsFailed = 0;
static int Unity_TestsIgnored = 0;
static int Unity_CurrentTestFailed = 0;
static int Unity_CurrentTestIgnored = 0;

void UnityBegin(const char* filename) {
    Unity_NumberOfTests = 0;
    Unity_TestsFailed = 0;
    Unity_TestsIgnored = 0;
    Unity_CurrentTestFailed = 0;
    Unity_CurrentTestIgnored = 0;
    
    printf("%s:", filename);
}

int UnityEnd(void) {
    printf("\n-----------------------\n");
    printf("%d Tests %d Failures %d Ignored\n", 
           Unity_NumberOfTests, Unity_TestsFailed, Unity_TestsIgnored);
    
    if (Unity_TestsFailed == 0) {
        printf("OK\n");
    } else {
        printf("FAIL\n");
    }
    
    return (Unity_TestsFailed > 0) ? 1 : 0;
}

void UnityPrint(const char* string) {
    printf("%s", string);
}

void UnityPrintNumberUnsigned(const uint32_t number) {
    printf("%u", number);
}

void UnityTestResultsBegin(const char* file, const uint32_t line) {
    printf("\n%s:%u:", file, line);
}

void UnityTestResultsFailBegin(const uint32_t line) {
    printf("\n  line %u: ", line);
}

void UnityConcludeTest(void) {
    Unity_NumberOfTests++;
    if (Unity_CurrentTestIgnored) {
        Unity_TestsIgnored++;
        printf("IGNORE");
    } else if (Unity_CurrentTestFailed) {
        Unity_TestsFailed++;
        printf("FAIL");
    } else {
        printf("PASS");
    }
    Unity_CurrentTestFailed = 0;
    Unity_CurrentTestIgnored = 0;
}

void UnityAssertEqualNumber(const int32_t expected, const int32_t actual, const char* msg, const uint32_t lineNumber, const char style) {
    if (expected != actual) {
        Unity_CurrentTestFailed = 1;
        UnityTestResultsFailBegin(lineNumber);
        printf("Expected %d Was %d", expected, actual);
        if (msg) {
            printf(".%s", msg);
        }
    }
}

void UnityAssertEqualIntArray(const int32_t* expected, const int32_t* actual, const uint32_t num_elements, const char* msg, const uint32_t lineNumber, const char style) {
    uint32_t i;
    for (i = 0; i < num_elements; i++) {
        if (expected[i] != actual[i]) {
            Unity_CurrentTestFailed = 1;
            UnityTestResultsFailBegin(lineNumber);
            printf("Element %u Expected %d Was %d", i, expected[i], actual[i]);
            if (msg) {
                printf(".%s", msg);
            }
            break;
        }
    }
}

void UnityAssertBits(const int32_t mask, const int32_t expected, const int32_t actual, const char* msg, const uint32_t lineNumber) {
    if ((expected & mask) != (actual & mask)) {
        Unity_CurrentTestFailed = 1;
        UnityTestResultsFailBegin(lineNumber);
        printf("Expected 0x%08X Was 0x%08X", (expected & mask), (actual & mask));
        if (msg) {
            printf(".%s", msg);
        }
    }
}

void UnityAssertEqualString(const char* expected, const char* actual, const char* msg, const uint32_t lineNumber) {
    if (strcmp(expected, actual) != 0) {
        Unity_CurrentTestFailed = 1;
        UnityTestResultsFailBegin(lineNumber);
        printf("Expected '%s' Was '%s'", expected, actual);
        if (msg) {
            printf(".%s", msg);
        }
    }
}

void UnityAssertEqualStringLen(const char* expected, const char* actual, const uint32_t length, const char* msg, const uint32_t lineNumber) {
    if (strncmp(expected, actual, length) != 0) {
        Unity_CurrentTestFailed = 1;
        UnityTestResultsFailBegin(lineNumber);
        printf("Expected '%.*s' Was '%.*s'", (int)length, expected, (int)length, actual);
        if (msg) {
            printf(".%s", msg);
        }
    }
}

void UnityAssertEqualMemory(const void* expected, const void* actual, const uint32_t length, const char* msg, const uint32_t lineNumber) {
    if (memcmp(expected, actual, length) != 0) {
        Unity_CurrentTestFailed = 1;
        UnityTestResultsFailBegin(lineNumber);
        printf("Memory Mismatch");
        if (msg) {
            printf(".%s", msg);
        }
    }
}

void UnityAssertNumbersWithin(const int32_t delta, const int32_t expected, const int32_t actual, const char* msg, const uint32_t lineNumber) {
    int32_t diff = actual - expected;
    if (diff < 0) diff = -diff;
    
    if (diff > delta) {
        Unity_CurrentTestFailed = 1;
        UnityTestResultsFailBegin(lineNumber);
        printf("Expected %d +/- %d Was %d", expected, delta, actual);
        if (msg) {
            printf(".%s", msg);
        }
    }
}

void UnityFail(const char* msg, const uint32_t line) {
    Unity_CurrentTestFailed = 1;
    UnityTestResultsFailBegin(line);
    printf("Fail");
    if (msg) {
        printf(".%s", msg);
    }
}

void UnityIgnore(const char* msg, const uint32_t line) {
    Unity_CurrentTestIgnored = 1;
    UnityTestResultsFailBegin(line);
    printf("Ignore");
    if (msg) {
        printf(".%s", msg);
    }
}
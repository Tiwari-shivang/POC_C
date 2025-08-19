#ifndef UNITY_H
#define UNITY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void UnityBegin(const char* filename);
int UnityEnd(void);
void UnityPrint(const char* string);
void UnityPrintNumberUnsigned(const uint32_t number);
void UnityTestResultsBegin(const char* file, const uint32_t line);
void UnityTestResultsFailBegin(const uint32_t line);
void UnityConcludeTest(void);

void UnityAssertEqualNumber(const int32_t expected, const int32_t actual, const char* msg, const uint32_t lineNumber, const char style);
void UnityAssertEqualIntArray(const int32_t* expected, const int32_t* actual, const uint32_t num_elements, const char* msg, const uint32_t lineNumber, const char style);
void UnityAssertBits(const int32_t mask, const int32_t expected, const int32_t actual, const char* msg, const uint32_t lineNumber);
void UnityAssertEqualString(const char* expected, const char* actual, const char* msg, const uint32_t lineNumber);
void UnityAssertEqualStringLen(const char* expected, const char* actual, const uint32_t length, const char* msg, const uint32_t lineNumber);
void UnityAssertEqualMemory(const void* expected, const void* actual, const uint32_t length, const char* msg, const uint32_t lineNumber);
void UnityAssertNumbersWithin(const int32_t delta, const int32_t expected, const int32_t actual, const char* msg, const uint32_t lineNumber);
void UnityFail(const char* msg, const uint32_t line);
void UnityIgnore(const char* msg, const uint32_t line);

#define UNITY_BEGIN() UnityBegin(__FILE__)
#define UNITY_END() UnityEnd()
#define RUN_TEST(func) func(); UnityConcludeTest()
#define TEST_ASSERT_EQUAL_INT(expected, actual) UnityAssertEqualNumber((int32_t)(expected), (int32_t)(actual), NULL, __LINE__, 'I')
#define TEST_ASSERT_TRUE(condition) UnityAssertEqualNumber((int32_t)(condition), 1, " Expected TRUE Was FALSE", __LINE__, 'B')
#define TEST_ASSERT_FALSE(condition) UnityAssertEqualNumber((int32_t)(condition), 0, " Expected FALSE Was TRUE", __LINE__, 'B')
#define TEST_ASSERT_EQUAL_UINT8(expected, actual) UnityAssertEqualNumber((int32_t)(expected), (int32_t)(actual), NULL, __LINE__, 'C')
#define TEST_ASSERT_EQUAL_UINT16(expected, actual) UnityAssertEqualNumber((int32_t)(expected), (int32_t)(actual), NULL, __LINE__, 'S')
#define TEST_ASSERT_EQUAL_UINT32(expected, actual) UnityAssertEqualNumber((int32_t)(expected), (int32_t)(actual), NULL, __LINE__, 'X')

#ifdef __cplusplus
}
#endif

#endif /* UNITY_H */
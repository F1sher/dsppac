#include "dsp.h"
#include "./unity/unity.h"


void test_NoDeviceConnected_should_RaiseError(void)
{
    int res = 0;
	cyusb_handle *usb_h = NULL;
	
	res = init_controller(&usb_h);

	TEST_ASSERT_EQUAL_INT(-1, res);
}

void test_SizeOfSignal_should_Equal256(void)
{
	TEST_ASSERT_EQUAL_INT(256, SIZEOF_SIGNAL);
}

void test_area_signal_ForConstSignal_should_RetZero(void)
{
	int i = 0;
	int *a = (int *)calloc(SIZEOF_SIGNAL, sizeof(double));
	if (a == NULL) {
		TEST_FAIL_MESSAGE("calloc error");
		
		return ;
	}

	for (i = 0; i < SIZEOF_SIGNAL; i++) {
		a[i] = 1000;
	}

	double area = area_signal(a);
	free(a); a = NULL;

	TEST_ASSERT_EQUAL_DOUBLE(0.0, area);	   
}

void test_min_bubble_ForMin_should_RetMinusOne(void)
{
	int a[128] = {0};
	a[10] = -1;

	int min = 0;

	min_bubble(a, 128, &min, NULL);
	TEST_ASSERT_EQUAL_INT(-1, min);
}

void test_min_bubble_ForMinNum_should_RetTen(void)
{
	int a[128] = {0};
	a[10] = -1;

	int min_num = 0;

	min_bubble(a, 128, NULL, &min_num);
	TEST_ASSERT_EQUAL_INT(10, min_num);
}

void test_time_signal_ForConstSignal_should_RetMinusOne(void)
{
	int *a = (int *)calloc(SIZEOF_SIGNAL, sizeof(int));
	if (a == NULL) {
		TEST_FAIL_MESSAGE("calloc error");

		return ;
	}

	CFT_fraction = 0.5;
	double x0 = time_signal(a);
   
	free(a);

	TEST_ASSERT_EQUAL_DOUBLE(-1.0, x0);
}

void test_time_signal_ForStepSignal_should_Ret19point5(void)
{
	int i = 0;
	int *a = (int *)calloc(SIZEOF_SIGNAL, sizeof(int));
	if (a == NULL) {
		TEST_FAIL_MESSAGE("calloc error");

		return ;
	}

	for (i = 20; i < SIZEOF_SIGNAL; i++) {
		a[i] = -100;
	}

	CFT_fraction = 0.5;
	double x0 = time_signal(a);
   
	free(a);

	TEST_ASSERT_EQUAL_DOUBLE(19.5, x0);
}


int main() 
{
    UNITY_BEGIN();

	RUN_TEST(test_NoDeviceConnected_should_RaiseError);
	RUN_TEST(test_SizeOfSignal_should_Equal256);

	RUN_TEST(test_area_signal_ForConstSignal_should_RetZero);
	
	RUN_TEST(test_min_bubble_ForMin_should_RetMinusOne);
	RUN_TEST(test_min_bubble_ForMinNum_should_RetTen);

	RUN_TEST(test_time_signal_ForConstSignal_should_RetMinusOne);
	RUN_TEST(test_time_signal_ForStepSignal_should_Ret19point5);

	return UNITY_END();
}

#include <stdio.h>

void func_1(void) {
	printf("func_1\n");
}

// implicit conversion
void func_2(void (f)(void)) {
	printf("func_2:"); 
	f();
}

// implicit argument conversion
void (*func_3(void (f)(void)))(void) {
	printf("func_3:"); 
	f();
	return f;
}

void (*func_4(void (*f)(void)))(void) {
	printf("func_4:"); 
	f();
	return f;
}

typedef void (*func_to_func)(void);

func_to_func func_5(func_to_func f) {
	printf("func_5:"); 
	f();
	return f;
}

int main(int argc, char *argv[]) {
	func_2(func_1);

	void (*f) (void) = func_3(func_1);
	f();

	f = func_4(func_1);
	f();

	f = func_5(func_1);
	f();
}